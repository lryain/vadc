// Public API header for libvadc: do NOT include the large internal headers
// here to avoid leaking constants (which may cause duplicate-definition
// link errors when using a static library). Forward-declare only the
// types needed by callers.

#pragma once
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations for opaque types from the vadc internals. Callers
   should treat these as opaque and only pass pointers between functions. */
typedef struct MemoryArena MemoryArena;
typedef struct String8 String8;

/* Simple C API wrappers for building vadc as a library and running the
   existing run_inference pipeline from code. */

/* Allocate and initialize a MemoryArena of the requested size. Caller must
   call vadc_destroy_arena to free. Returns NULL on failure. */
MemoryArena* vadc_create_arena(size_t arena_bytes);

/* Free arena previously created by vadc_create_arena */
void vadc_destroy_arena(MemoryArena* arena);

/* Run the same inference pipeline as the CLI's main() by forwarding
   parameters into run_inference(). Parameters match the CLI defaults in
   vadc.c. model_path may be NULL or empty string to use default.
   filename may be NULL to indicate stdin. Returns the same int result as
   run_inference(). */
int vadc_run(const char* model_path,
             MemoryArena* arena,
             float min_silence_duration_ms,
             float min_speech_duration_ms,
             float threshold,
             float neg_threshold,
             float speech_pad_ms,
             float desired_sequence_count,
             int raw_probabilities,
             int output_format_centi_seconds,
             const char* filename,
             int stats_output_enabled,
             int preferred_batch_size,
             int audio_source,
             float start_seconds,
             const char* audio_output_file,
             const char* log_output_file,
             const char* speech_audio_file,
             const char* noise_audio_file,
             int verbose_logging);

#ifdef __cplusplus
}
#endif
