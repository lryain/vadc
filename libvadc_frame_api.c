#include "libvadc_frame_api.h"
#include "vadc.h"
#include "memory.h"
#include <stdlib.h>
#include <string.h>

struct VadcWrapper {
    MemoryArena arena;
    u8* arena_base;
    VADC_Context context;
    Silero_Config config;
    void* backend; // backend handle from backend_init
};

VadcWrapper* vadc_wrapper_create(size_t arena_bytes, const char* model_path) {
    VadcWrapper* w = (VadcWrapper*)malloc(sizeof(VadcWrapper));
    if (!w) return NULL;
    memset(w, 0, sizeof(*w));
    w->arena_base = (u8*)malloc(arena_bytes);
    if (!w->arena_base) { free(w); return NULL; }
    initializeMemoryArena(&w->arena, w->arena_base, arena_bytes);

    memset(&w->config, 0, sizeof(w->config));
    w->config.is_silero_v5 = true;
    w->config.batch_size = 1;
    w->config.batch_size_restriction = 1;

    memset(&w->context, 0, sizeof(w->context));
    w->context.buffers.window_size_samples = 512;
    w->context.buffers.lstm_count = 128;

    // allocate buffers
    w->context.buffers.input_samples = (float*)pushSize(&w->arena, 512 * sizeof(float), 16);
    w->context.buffers.output = (float*)pushSize(&w->arena, sizeof(float), 16);
    w->context.buffers.lstm_h = (float*)pushSize(&w->arena, 128 * sizeof(float), 16);
    w->context.buffers.lstm_c = (float*)pushSize(&w->arena, 128 * sizeof(float), 16);
    w->context.buffers.lstm_h_out = (float*)pushSize(&w->arena, 128 * sizeof(float), 16);
    w->context.buffers.lstm_c_out = (float*)pushSize(&w->arena, 128 * sizeof(float), 16);

    if (!w->context.buffers.input_samples || !w->context.buffers.output) {
        free(w->arena_base); free(w); return NULL;
    }

    // build model path String8
    String8 model_arg = {0};
    if (model_path && model_path[0]) {
        model_arg.begin = (u8*)model_path;
        model_arg.size = (int)strlen(model_path);
    }

    w->backend = backend_init(&w->arena, model_arg, &w->config);
    if (!w->backend) {
        free(w->arena_base); free(w); return NULL;
    }
    backend_create_tensors(w->config, w->backend, w->context.buffers);
    return w;
}

void vadc_wrapper_destroy(VadcWrapper* w) {
    if (!w) return;
    // Note: upstream may provide a backend_free; if not, ignore
    if (w->arena_base) free(w->arena_base);
    free(w);
}

int vadc_wrapper_process_frame(VadcWrapper* w, const int16_t* pcm_data, size_t samples, float* out_probability) {
    if (!w || !pcm_data || samples == 0 || !out_probability) return -1;
    size_t tocopy = samples < (size_t)512 ? samples : 512;
    for (size_t i = 0; i < tocopy; ++i) {
        w->context.buffers.input_samples[i] = (float)pcm_data[i] / 32768.0f;
    }
    // copy LSTM state
    memcpy(w->context.buffers.lstm_h, w->context.buffers.lstm_h_out, 128 * sizeof(float));
    memcpy(w->context.buffers.lstm_c, w->context.buffers.lstm_c_out, 128 * sizeof(float));

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
    if (w->context.buffers.lstm_h) memset(w->context.buffers.lstm_h, 0, 128 * sizeof(float));
    if (w->context.buffers.lstm_c) memset(w->context.buffers.lstm_c, 0, 128 * sizeof(float));
    if (w->context.buffers.lstm_h_out) memset(w->context.buffers.lstm_h_out, 0, 128 * sizeof(float));
    if (w->context.buffers.lstm_c_out) memset(w->context.buffers.lstm_c_out, 0, 128 * sizeof(float));
}
