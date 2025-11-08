#include "string8.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <wchar.h>
#include <locale.h>

/* Global storage for command line arguments (Linux) */
static int g_argc = 0;
static char **g_argv = NULL;

/* Set command line arguments from main (call this from main() on Linux) */
void set_command_line_args(int argc, char **argv)
{
    g_argc = argc;
    g_argv = argv;
}

String8 String8FromPointerSize(const s8 *pointer, strSize size)
{
    String8 result = {0};
    result.begin = pointer;
    result.size = size;

    return result;
}

String8 String8FromRange(const s8 *first, const s8 *one_past_last)
{
    String8 result = {0};
    result.begin = first;
    result.size = one_past_last - first;

    return result;
}

String8 String8FromCString(const char *cstring)
{
    return String8FromPointerSize(cstring, (strSize)strlen(cstring));
}

String8 String8ToCString(MemoryArena *arena, String8 source_string)
{
    s8 *cstring = pushSizeZeroed(arena, source_string.size + 1, TEMP_DEFAULT_ALIGNMENT);
    memmove(cstring, source_string.begin, source_string.size);

    return String8FromPointerSize(cstring, source_string.size);
}

String8 String8_pushfv(MemoryArena *arena, const char *format, va_list args)
{
    va_list args_copy;
    va_copy(args_copy, args);
    int buffer_size = 1024;
    char *buffer = pushArray(arena, buffer_size, char);
    int actual_size = vsnprintf(buffer, buffer_size, format, args);
    int actual_size_with_null = actual_size + 1;

    String8 result = {0};
    if (actual_size < buffer_size)
    {
        char *smaller_buffer = (char *)resizeAllocationInArena(arena, buffer, buffer_size, actual_size_with_null, 1);
        result = String8FromPointerSize((const s8 *)smaller_buffer, actual_size);
    }
    else
    {
        char *larger_buffer = (char *)resizeAllocationInArena(arena, buffer, buffer_size, actual_size_with_null, 1);
        int final_size = vsnprintf(larger_buffer, actual_size_with_null, format, args_copy);
        result = String8FromPointerSize((const s8 *)larger_buffer, final_size);
    }
    va_end(args_copy);

    return result;
}

String8 String8_pushf(MemoryArena *arena, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    String8 result = String8_pushfv(arena, format, args);
    va_end(args);

    return result;
}

int String8_ToWidechar(MemoryArena *arena, wchar_t **dest, String8 source)
{
    /* Not available on Linux - kept for compatibility */
    (void)arena; (void)dest; (void)source;
    return 0;
}

String8 Widechar_ToString8(MemoryArena *arena, const wchar_t *str, size_t str_length)
{
    /* Not available on Linux - kept for compatibility */
    (void)arena; (void)str; (void)str_length;
    String8 result = {0};
    return result;
}


String8 escape_json_string(MemoryArena *arena, String8 input)
{
    String8 empty_result = {0};

    // Allocate buffer (worst case: every char is escaped, plus null terminator)
    size_t input_length = (size_t)input.size;

    char *output = pushArray(arena, input_length * 2 + 1, char);
    if (!output) return empty_result;

    const char *in_ptr = (const char *)input.begin;
    const char *in_ptr_end = (const char *)input.begin + input_length;
    char *out_ptr = output;
    while (in_ptr != in_ptr_end) {
        switch (*in_ptr) {
            case '\"': *out_ptr++ = '\\'; *out_ptr++ = '\"'; break;
            case '\\': *out_ptr++ = '\\'; *out_ptr++ = '\\'; break;
            case '\b': *out_ptr++ = '\\'; *out_ptr++ = 'b'; break;
            case '\f': *out_ptr++ = '\\'; *out_ptr++ = 'f'; break;
            case '\n': *out_ptr++ = '\\'; *out_ptr++ = 'n'; break;
            case '\r': *out_ptr++ = '\\'; *out_ptr++ = 'r'; break;
            case '\t': *out_ptr++ = '\\'; *out_ptr++ = 't'; break;
            default: *out_ptr++ = *in_ptr; break;
        }
        in_ptr++;
    }
    *out_ptr = '\0';

    return String8FromRange((const s8 *)output, (const s8 *)out_ptr);
}

b32 String8_Equal(String8 a, String8 b)
{
    if (a.size != b.size)
    {
        return 0;
    }
    else
    {
        return memcmp(a.begin, b.begin, a.size) == 0;
    }
}

String8 *get_command_line_as_utf8(MemoryArena *arena, int *out_argcount)
{
    /* Linux version: use globally stored argc/argv */
    if (g_argc == 0 || g_argv == NULL) 
    {
        *out_argcount = 0;
        return NULL;
    }
    
    /* Allocate array of String8 for all arguments */
    String8 *result = pushArray(arena, g_argc, String8);
    if (!result)
    {
        *out_argcount = 0;
        return NULL;
    }
    
    /* Convert each char* argument to String8 */
    for (int i = 0; i < g_argc; ++i)
    {
        result[i] = String8FromCString(g_argv[i]);
    }
    
    *out_argcount = g_argc;
    return result;
}

