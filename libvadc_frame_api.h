// Thin frame-level C API for embedding vadc frame-by-frame
#pragma once
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct VadcWrapper VadcWrapper;

/* Create a VADC wrapper which allocates an internal MemoryArena of
   arena_bytes and initializes the ONNX backend using model_path (may be
   NULL to use defaults). Returns NULL on failure. */
VadcWrapper* vadc_wrapper_create(size_t arena_bytes, const char* model_path);

/* Destroy wrapper and free associated memory. */
void vadc_wrapper_destroy(VadcWrapper* w);

/* Process a single frame of int16 samples (mono). Samples count should
   be <= the model frame size (typically 512). On success writes the
   probability into out_probability (0.0-1.0) and returns 0. */
int vadc_wrapper_process_frame(VadcWrapper* w, const int16_t* pcm_data, size_t samples, float* out_probability);

/* Reset internal LSTM states to zeros. */
void vadc_wrapper_reset(VadcWrapper* w);

/* Query constants used by the wrapper */
int vadc_wrapper_frame_samples(void);
int vadc_wrapper_sample_rate(void);

#ifdef __cplusplus
}
#endif
