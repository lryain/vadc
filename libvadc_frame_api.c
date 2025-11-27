#include "libvadc_frame_api.h"
#include "libvadc_api.h" /* 使用公共 API 的 vadc_create_arena()/vadc_destroy_arena() */
#include "vadc.h"
#include "memory.h"
#include <stdlib.h>
#include <string.h>

/**
 * VadcWrapper 结构体：用于 Frame API 的封装
 * 注意：我们保留一个外部分配的 arena 指针（external_arena）以便
 * 使用 vadc_create_arena()/vadc_destroy_arena() 的资源管理方法，
 * 确保与 vadc_run 使用的 arena 初始化一致，从而减少运行时故障。
 */
struct VadcWrapper {
    MemoryArena arena;       // 内部用到的 MemoryArena 结构（拷贝/视图）
    MemoryArena *external_arena; // 外部创建的 arena（若使用 vadc_create_arena）
    u8* arena_base;          // arena 的 base 指针（仅供兼容，若不为 NULL 则由本模块释放）
    VADC_Context context;
    Silero_Config config;
    void* backend; // 来自 backend_init 的后端句柄
};

VadcWrapper* vadc_wrapper_create(size_t arena_bytes, const char* model_path) {
    // 创建 VADC wrapper 并分配 arena
    // 注意: 对于 Silero VAD, arena 大小需要足够大，参考 test_vadc 示例使用 32MB
    VadcWrapper* w = (VadcWrapper*)malloc(sizeof(VadcWrapper));
    if (!w) return NULL;
    memset(w, 0, sizeof(*w));
    // 优先通过 vadc_create_arena 在库中创建 arena，这与 CLI/vadc_run 使用相同的分配
    // 和对齐约定，能避免 arena 初始化差异导致的不可预知行为。
    MemoryArena* created_arena = vadc_create_arena(arena_bytes);
    if (created_arena) {
        w->external_arena = created_arena;
        w->arena_base = created_arena->base;
        // 将创建的 arena 映射到内部 arena 视图中
        initializeMemoryArena(&w->arena, w->arena_base, created_arena->size);
    } else {
        // 如果无法使用 vadc_create_arena，则降低到手动 malloc（兼容旧逻辑）
        w->external_arena = NULL;
        w->arena_base = (u8*)malloc(arena_bytes);
        if (!w->arena_base) { free(w); return NULL; }
        initializeMemoryArena(&w->arena, w->arena_base, arena_bytes);
    }


    memset(&w->config, 0, sizeof(w->config));
    // 默认为 V5，以便在未加载模型前保持一个合理的初始值；后续 backend_init 会覆盖
    w->config.is_silero_v5 = true;
    w->config.batch_size = 1;
    w->config.batch_size_restriction = 1;

    memset(&w->context, 0, sizeof(w->context));

    // build model path String8
    String8 model_arg = {0};
    if (model_path && model_path[0]) {
        model_arg.begin = (u8*)model_path;
        model_arg.size = (int)strlen(model_path);
    }

    // 先初始化后端（CreateSession），以便 ort_init 能填写 config 的动态字段
    w->backend = backend_init(&w->arena, model_arg, &w->config);
    if (!w->backend) {
        if (w->external_arena) vadc_destroy_arena(w->external_arena);
        else free(w->arena_base);
        free(w);
        return NULL;
    }
    // 与 CLI 的 run_inference 一致：在有了 config（onnx 会填充一些字段）后，再分配 buffers
    {
        // Ensure sensible defaults populated from ort_init
        if (w->config.batch_size_restriction == -1) {
            w->config.batch_size = 1; // frame API 无并行批量
        } else {
            w->config.batch_size = w->config.batch_size_restriction;
        }

        // compute prob shape similar to run_inference
        if (w->config.output_dims == 3) {
            w->config.prob_shape_count = 3;
            w->config.prob_shape[0] = w->config.batch_size;
            w->config.prob_shape[1] = 2;
            w->config.prob_shape[2] = 1;
        } else {
            w->config.prob_shape_count = 2;
            w->config.prob_shape[0] = w->config.batch_size;
            w->config.prob_shape[1] = 1;
        }
        size_t prob_tensor_element_count = 1;
        for (size_t i = 0; i < w->config.prob_shape_count; ++i) {
            prob_tensor_element_count *= (size_t)w->config.prob_shape[i];
        }
        w->config.prob_tensor_element_count = prob_tensor_element_count;

        // input_count: set to minimum sequence count if not set
        if (w->config.input_size_min > 0) {
            w->config.input_count = w->config.input_size_min;
        } else {
            w->config.input_count = 512;
        }

        // Set up buffers sizes
        w->context.buffers.window_size_samples = (int)w->config.input_count;
        if (w->config.is_silero_v5) {
            w->context.buffers.input_samples = (float*)pushSize(&w->arena, (size_t)(w->context.buffers.window_size_samples + w->config.context_size) * sizeof(float) * (size_t)w->config.batch_size, 16);
        } else {
            w->context.buffers.input_samples = (float*)pushSize(&w->arena, (size_t)w->context.buffers.window_size_samples * sizeof(float) * (size_t)w->config.batch_size, 16);
        }
        w->context.buffers.output = (float*)pushSize(&w->arena, w->config.prob_tensor_element_count * sizeof(float), 16);

    // lstm buffers
    // Bytes to allocate: for v5 model, lstm_count equals lstm_hidden_size
    // For older (v3/v4) models, memory layout uses 2 as the first state dimension, so multiply by 2
    int lstm_hidden = w->config.lstm_hidden_size > 0 ? w->config.lstm_hidden_size : 128;
    w->context.buffers.lstm_count = lstm_hidden * (w->config.is_silero_v5 ? 1 : 2);
        w->context.buffers.lstm_h = (float*)pushSize(&w->arena, (size_t)w->context.buffers.lstm_count * sizeof(float), 16);
        w->context.buffers.lstm_c = (float*)pushSize(&w->arena, (size_t)w->context.buffers.lstm_count * sizeof(float), 16);
        w->context.buffers.lstm_h_out = (float*)pushSize(&w->arena, (size_t)w->context.buffers.lstm_count * sizeof(float), 16);
        w->context.buffers.lstm_c_out = (float*)pushSize(&w->arena, (size_t)w->context.buffers.lstm_count * sizeof(float), 16);

        if (!w->context.buffers.input_samples || !w->context.buffers.output) {
            fprintf(stderr, "[VADC] vadc_wrapper_create failed: arena too small (requested %zu bytes).\n", arena_bytes);
            fprintf(stderr, "[VADC] 建议至少分配 32MB（与 examples/test_vadc 一致）以保证模型能正常载入。\n");
            if (w->external_arena) vadc_destroy_arena(w->external_arena);
            else free(w->arena_base);
            free(w);
            return NULL;
        }
    }
    // 必须在 backend_create_tensors 之前设置 context.backend，因为 backend_run 会使用它
    w->context.backend = w->backend;
    backend_create_tensors(w->config, w->backend, w->context.buffers);
    return w;
}

void vadc_wrapper_destroy(VadcWrapper* w) {
    if (!w) return;
    // Note: upstream may provide a backend_free; if not, ignore
    // 如果外部 arena（由 vadc_create_arena 创建）存在则使用 vadc_destroy_arena 释放
    if (w->external_arena) {
        vadc_destroy_arena(w->external_arena);
    } else if (w->arena_base) {
        free(w->arena_base);
    }
    free(w);
}

int vadc_wrapper_process_frame(VadcWrapper* w, const int16_t* pcm_data, size_t samples, float* out_probability) {
    if (!w || !pcm_data || samples == 0 || !out_probability) return -1;
    size_t tocopy = samples < (size_t)512 ? samples : 512;
    for (size_t i = 0; i < tocopy; ++i) {
        w->context.buffers.input_samples[i] = (float)pcm_data[i] / 32768.0f;
    }
    // copy LSTM state
    size_t lstm_bytes = (size_t)w->context.buffers.lstm_count * sizeof(float);
    memcpy(w->context.buffers.lstm_h, w->context.buffers.lstm_h_out, lstm_bytes);
    memcpy(w->context.buffers.lstm_c, w->context.buffers.lstm_c_out, lstm_bytes);

    backend_run(&w->arena, &w->context, w->config);

    float p = *w->context.buffers.output;
    if (p < 0.0f) p = 0.0f;
    if (p > 1.0f) p = 1.0f;
    *out_probability = p;
    return 0;
}

int vadc_wrapper_frame_samples(void) { return 512; }
int vadc_wrapper_sample_rate(void) { return 16000; }

void vadc_wrapper_reset(VadcWrapper* w) {
    if (!w) return;
    if (w->context.buffers.lstm_h) memset(w->context.buffers.lstm_h, 0, w->context.buffers.lstm_count * sizeof(float));
    if (w->context.buffers.lstm_c) memset(w->context.buffers.lstm_c, 0, w->context.buffers.lstm_count * sizeof(float));
    if (w->context.buffers.lstm_h_out) memset(w->context.buffers.lstm_h_out, 0, w->context.buffers.lstm_count * sizeof(float));
    if (w->context.buffers.lstm_c_out) memset(w->context.buffers.lstm_c_out, 0, w->context.buffers.lstm_count * sizeof(float));
}
