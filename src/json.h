#include <stdbool.h>
#include <stdint.h>

struct json_stream;
typedef struct json_stream json_stream;

typedef void (*json_flush_callback)(uint8_t *buffer, size_t size, void *context);

json_stream *json_new(size_t buffer_size, json_flush_callback on_flush, void *context);
void json_free(json_stream *json);

void json_flush(json_stream *json);

void json_object_start(json_stream *json);
void json_object_end(json_stream *json);

void json_array_start(json_stream *json);
void json_array_end(json_stream *json);

void json_integer(json_stream *json, long long x);
void json_float(json_stream *json, float x);
void json_string(json_stream *json, const char *x);
void json_boolean(json_stream *json, bool x);
void json_null(json_stream *json);

