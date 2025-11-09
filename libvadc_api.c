#include "libvadc_api.h"
#include "vadc.h"
#include <stdlib.h>
#include <string.h>

/* Implementation note: the full internal headers are included only here so the
   public header stays minimal and avoids exporting constants that could
   collide at link time when using a static library. */

MemoryArena* vadc_create_arena(size_t arena_bytes)
{
    MemoryArena* arena = (MemoryArena*)malloc(sizeof(MemoryArena));
    if (!arena) return NULL;
    u8* base = (u8*)malloc(arena_bytes);
    if (!base) { free(arena); return NULL; }
    initializeMemoryArena(arena, base, arena_bytes);
    return arena;
}

void vadc_destroy_arena(MemoryArena* arena)
{
    if (!arena) return;
    /* initializeMemoryArena stores the base pointer in arena->base */
    if (arena->base) free(arena->base);
    free(arena);
}

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
             int verbose_logging)
{
    String8 model_arg = {0};
    if (model_path && model_path[0]) {
        model_arg.begin = (u8*)model_path;
        model_arg.size = (int)strlen(model_path);
    }

    String8 filename_arg = {0};
    if (filename && filename[0]) {
        filename_arg.begin = (u8*)filename;
        filename_arg.size = (int)strlen(filename);
    }

    return run_inference(model_arg,
                         arena,
                         min_silence_duration_ms,
                         min_speech_duration_ms,
                         threshold,
                         neg_threshold,
                         speech_pad_ms,
                         desired_sequence_count,
                         raw_probabilities ? 1 : 0,
                         output_format_centi_seconds ? Segment_Output_Format_CentiSeconds : Segment_Output_Format_Seconds,
                         filename_arg,
                         stats_output_enabled ? 1 : 0,
                         (s32)preferred_batch_size,
                         audio_source,
                         start_seconds,
                         audio_output_file,
                         log_output_file,
                         speech_audio_file,
                         noise_audio_file,
                         verbose_logging ? 1 : 0);
}
