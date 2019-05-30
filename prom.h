#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>  // size_t

#define PROM_PARSE_METRIC_OK 1
#define PROM_PARSE_END 0
#define PROM_PARSE_INVALID_FLOAT -1
#define PROM_PARSE_MISSING_NEWLINE -2
#define PROM_PARSE_TIMESTAMP_OVERFLOW -3
#define PROM_PARSE_MISSING_METRIC_VALUE -4
#define PROM_PARSE_UNTERMINATED_STRING -5
#define PROM_PARSE_INVALID_TOKEN -6
#define PROM_PARSE_MISSING_CLOSING_CURLY_BRACKET -7
#define PROM_PARSE_MISSING_HELP_METRIC_NAME -8
#define PROM_PARSE_MISSING_HELP_TEXT -9
#define PROM_PARSE_MISSING_TYPE_TEXT -10
#define PROM_PARSE_UNKNOWN_TYPE_TEXT -11
#define PROM_PARSE_INVALID_UTF8 -12
#define PROM_PARSE_ENOMEM -13
#define PROM_PARSE_UNREACHABLE -100

struct label {
    const unsigned char* label_name;
    size_t label_name_size;
    const unsigned char* label_value;
    size_t label_value_size;
};

enum PROM_METRIC_TYPE {
    PROM_METRIC_TYPE_UNTYPED,
    PROM_METRIC_TYPE_COUNTER,
    PROM_METRIC_TYPE_GAUGE,
    PROM_METRIC_TYPE_HISTOGRAM,
    PROM_METRIC_TYPE_SUMMARY,
};

struct metric {
    const unsigned char* metric_name;
    size_t metric_name_size;
    double value;
    long long int timestamp;
    const unsigned char* help;
    size_t help_size;
    enum PROM_METRIC_TYPE type;
    size_t type_size;
    // Labels
    struct label* labels;
    size_t labels_size;
    size_t labels_capacity;
};

int prom_parse(const unsigned char* s, size_t s_size, long long int ms_now,
               size_t* i, struct metric* m,
               void* (*realloc_fn)(void* ptr, size_t size));

#ifdef __cplusplus
}
#endif
