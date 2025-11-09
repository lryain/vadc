#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include "libvadc_api.h"

static const char *progname = "test_vadc";

static void print_timestamp(FILE *f)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    struct tm tm;
    localtime_r(&ts.tv_sec, &tm);
    char buf[64];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
    fprintf(f, "%s.%03ld", buf, ts.tv_nsec / 1000000);
}

static void signal_handler(int sig)
{
    fprintf(stderr, "%s: caught signal %d\n", progname, sig);
    print_timestamp(stderr);
    fprintf(stderr, " - pid=%d, errno=%d\n", getpid(), errno);
    fflush(stderr);
    /* restore default and re-raise to generate core if enabled */
    signal(sig, SIG_DFL);
    raise(sig);
}

int main(int argc, char** argv)
{
    progname = argc > 0 ? argv[0] : "test_vadc";

    /* register simple signal handlers to log crashes */
    signal(SIGSEGV, signal_handler);
    signal(SIGABRT, signal_handler);
    signal(SIGFPE,  signal_handler);
    signal(SIGILL,  signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGINT,  signal_handler);

    fprintf(stderr, "==== %s starting ===\n", progname);
    print_timestamp(stderr);
    fprintf(stderr, " - pid=%d\n", getpid());
    fprintf(stderr, " - argv (%d):\n", argc);
    for (int i = 0; i < argc; ++i) {
        fprintf(stderr, "    [%d] %s\n", i, argv[i]);
    }

    size_t arena_bytes = 32 * 1024 * 1024; // 32MB for example
    fprintf(stderr, "Allocating arena: %zu bytes\n", arena_bytes);
    MemoryArena* arena = vadc_create_arena(arena_bytes);
    if (!arena) {
        fprintf(stderr, "%s: Failed to allocate arena (requested=%zu) errno=%d\n", progname, arena_bytes, errno);
        return 1;
    }
    fprintf(stderr, "Arena allocated: %p\n", (void*)arena);

    const char* model = ""; // use default model in vadc if empty
    const char* input_file = NULL; // stdin

    /* Detailed parameter log */
    fprintf(stderr, "Calling vadc_run with parameters:\n");
    fprintf(stderr, "  model='%s'\n", model && model[0] ? model : "(default)");
    fprintf(stderr, "  input_file=%s\n", input_file ? input_file : "(stdin)");
    fprintf(stderr, "  min_silence=%.1f ms, min_speech=%.1f ms\n", 200.0f, 250.0f);
    fprintf(stderr, "  threshold=%.3f, neg_threshold=%.3f, speech_pad=%.1f ms\n", 0.5f, 0.35f, 30.0f);
    fprintf(stderr, "  desired_sequence_count=%.1f, raw_probabilities=%d\n", 1536.0f, 0);
    fprintf(stderr, "  stats_enabled=%d, preferred_batch=%d, audio_source=%d\n", 1, 96, 0);
    fprintf(stderr, "  verbose_logging=1 (internal vadc verbose enabled)\n");

    struct timespec tstart, tend;
    clock_gettime(CLOCK_MONOTONIC, &tstart);
    print_timestamp(stderr);
    fprintf(stderr, " - invoking vadc_run() now...\n");
    fflush(stderr);

    int ret = vadc_run(model, arena,
                       200.0f, 250.0f, // min silence, min speech
                       0.5f, 0.35f, 30.0f, // threshold, neg_threshold, speech_pad
                       1536.0f, // desired_sequence_count
                       0, // raw_probabilities
                       0, // output format (seconds)
                       input_file,
                       1, // stats enabled
                       96, // preferred batch
                       0, // audio source
                       0.0f, // start seconds
                       NULL, NULL, NULL, NULL, // output/log files
                       1 /* verbose */
                       );

    clock_gettime(CLOCK_MONOTONIC, &tend);
    long ms = (tend.tv_sec - tstart.tv_sec) * 1000 + (tend.tv_nsec - tstart.tv_nsec) / 1000000;

    print_timestamp(stderr);
    fprintf(stderr, "vadc_run returned: %d (elapsed %ld ms) errno=%d\n", ret, ms, errno);
    if (ret != 0) {
        fprintf(stderr, "%s: vadc_run returned non-zero. Check logs above for details.\n", progname);
    }

    fprintf(stderr, "Destroying arena %p\n", (void*)arena);
    vadc_destroy_arena(arena);
    fprintf(stderr, "%s exiting with code %d\n", progname, ret);

    return ret;
}
