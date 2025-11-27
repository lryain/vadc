/*
 * test_vadc_frame_api.c
 * 简单测试 libvadc frame API 能否用不同大小的 arena 初始化并成功加载模型。
 * 用途：验证在 audio_frontend 中使用 frame API 时是否因 arena 太小导致加载失败。
 */
#include <stdio.h>
#include <stdlib.h>
#include "libvadc_frame_api.h"

int main(int argc, char** argv) {
    const char *model_path = NULL;
    if (argc > 1) {
        model_path = argv[1];
    }

    size_t arena_small = 4 * 1024 * 1024; // 4MB
    size_t arena_big = 64 * 1024 * 1024;  // 64MB (更大以避免内存不足)

    printf("Testing model: %s\n", model_path ? model_path : "(auto-search)");

    // 先尝试大 arena，防止小 arena 导致程序集崩溃阻止进一步测试
    printf("Trying big arena: %zu bytes...\n", arena_big);
    VadcWrapper* w_big = vadc_wrapper_create(arena_big, model_path);
    if (w_big) {
        printf("SUCCESS: frame wrapper created with big arena\n");
        vadc_wrapper_destroy(w_big);
    } else {
        printf("FAIL: cannot create frame wrapper with big arena\n");
    }

    printf("Trying small arena: %zu bytes...\n", arena_small);
    VadcWrapper* w_small = vadc_wrapper_create(arena_small, model_path);
    if (w_small) {
        printf("SUCCESS: frame wrapper created with small arena\n");
        vadc_wrapper_destroy(w_small);
    } else {
        printf("FAIL: cannot create frame wrapper with small arena\n");
    }

    return 0;
}
