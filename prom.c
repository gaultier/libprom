#include "prom.h"
#include <errno.h>   // errno on strtoll, strtod
#include <stddef.h>  // size_t
#include <stdlib.h>  // strtoll, strtod
#include <string.h>  // memcmp

// From https://github.com/cyb70289/utf8/blob/master/naive.c
/* Return 0 - success,  >0 - index(1 based) of first error char */
static int utf8_naive(const unsigned char* data, int len) {
    int err_pos = 1;

    while (len) {
        int bytes;
        const unsigned char byte1 = data[0];

        /* 00..7F */
        if (byte1 <= 0x7F) {
            bytes = 1;
            /* C2..DF, 80..BF */
        } else if (len >= 2 && byte1 >= 0xC2 && byte1 <= 0xDF &&
                   (signed char)data[1] <= (signed char)0xBF) {
            bytes = 2;
        } else if (len >= 3) {
            const unsigned char byte2 = data[1];

            /* Is byte2, byte3 between 0x80 ~ 0xBF */
            const int byte2_ok = (signed char)byte2 <= (signed char)0xBF;
            const int byte3_ok = (signed char)data[2] <= (signed char)0xBF;

            if (byte2_ok && byte3_ok &&
                /* E0, A0..BF, 80..BF */
                ((byte1 == 0xE0 && byte2 >= 0xA0) ||
                 /* E1..EC, 80..BF, 80..BF */
                 (byte1 >= 0xE1 && byte1 <= 0xEC) ||
                 /* ED, 80..9F, 80..BF */
                 (byte1 == 0xED && byte2 <= 0x9F) ||
                 /* EE..EF, 80..BF, 80..BF */
                 (byte1 >= 0xEE && byte1 <= 0xEF))) {
                bytes = 3;
            } else if (len >= 4) {
                /* Is byte4 between 0x80 ~ 0xBF */
                const int byte4_ok = (signed char)data[3] <= (signed char)0xBF;

                if (byte2_ok && byte3_ok && byte4_ok &&
                    /* F0, 90..BF, 80..BF, 80..BF */
                    ((byte1 == 0xF0 && byte2 >= 0x90) ||
                     /* F1..F3, 80..BF, 80..BF, 80..BF */
                     (byte1 >= 0xF1 && byte1 <= 0xF3) ||
                     /* F4, 80..8F, 80..BF, 80..BF */
                     (byte1 == 0xF4 && byte2 <= 0x8F))) {
                    bytes = 4;
                } else {
                    return err_pos;
                }
            } else {
                return err_pos;
            }
        } else {
            return err_pos;
        }

        len -= bytes;
        err_pos += bytes;
        data += bytes;
    }

    return 0;
}

enum PROM_LEX {
    PROM_LEX_CURLY_BRACE_LEFT = 1,
    PROM_LEX_CURLY_BRACE_RIGHT = 2,
    PROM_LEX_COMMA = 4,
    PROM_LEX_EQUAL = 5,
    PROM_LEX_METRIC_NAME = 7,
    PROM_LEX_NEWLINE = 8,
    PROM_LEX_LABEL_NAME = 10,
    PROM_LEX_END = 11,
    PROM_LEX_LABEL_VALUE = 12,
    PROM_LEX_METRIC_VALUE = 13,
    PROM_LEX_TIMESTAMP = 14,
    PROM_LEX_WHITESPACE = 15,
    PROM_LEX_HELP = 16,
    PROM_LEX_TEXT = 17,
    PROM_LEX_TYPE = 18,
    PROM_LEX_COMMENT = 19,
};

enum lex_state {
    LEX_STATE_INIT = 0,
    LEX_STATE_TIMESTAMP = 1,
    LEX_STATE_STRING = 2,
    LEX_STATE_LABEL_NAME = 3,
    LEX_STATE_METRIC_NAME = 4,
    LEX_STATE_METRIC_VALUE = 5,
    LEX_STATE_LABEL_VALUE = 6,
    LEX_STATE_SPACE = 8,
    LEX_STATE_LABELS = 9,
    LEX_STATE_COMMENT = 10,
    LEX_STATE_META_1 = 11,
    LEX_STATE_META_2 = 12,
};

struct lex {
    const unsigned char* s;
    size_t s_size;
    enum lex_state state;
    size_t* i;
    size_t start;
};

static int c_is_num(unsigned int c) { return c >= '0' && c <= '9'; }

static int c_is_alpha_or_underscore(unsigned int c) {
    return ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_');
}

static int c_is_alpha_or_underscore_or_colon(unsigned int c) {
    return c_is_alpha_or_underscore(c) || c == ':';
}

static unsigned char buf_at(const unsigned char* s, size_t s_size, size_t i) {
    return i < s_size ? s[i] : '\0';
}

static int lex(struct lex* l) {
    if (buf_at(l->s, l->s_size, *l->i) == '\0') return PROM_LEX_END;

    if (buf_at(l->s, l->s_size, *l->i) == '\n') {
        l->state = LEX_STATE_INIT;
        l->start = *l->i;
        *l->i += 1;
        return PROM_LEX_NEWLINE;
    }

    if (buf_at(l->s, l->s_size, *l->i) == ' ' ||
        buf_at(l->s, l->s_size, *l->i) == '\t') {
        l->start = *l->i;

        while (buf_at(l->s, l->s_size, *l->i) == ' ' ||
               buf_at(l->s, l->s_size, *l->i) == '\t')
            *l->i += 1;

        return PROM_LEX_WHITESPACE;
    }

    if (buf_at(l->s, l->s_size, *l->i) == '#' &&
        (buf_at(l->s, l->s_size, *l->i + 1) == ' ' ||
         buf_at(l->s, l->s_size, *l->i + 1) == '\t')) {
        l->start = *l->i;
        *l->i += 1;

        while (buf_at(l->s, l->s_size, *l->i) == ' ' ||
               buf_at(l->s, l->s_size, *l->i) == '\t')
            l->start = *l->i, *l->i += 1;

        l->state = LEX_STATE_COMMENT;
    } else if (buf_at(l->s, l->s_size, *l->i) == '#') {
        l->start = *l->i;
        // Normal comment, free text
        while (*l->i < l->s_size && buf_at(l->s, l->s_size, *l->i) != '\n')
            *l->i += 1;
        return PROM_LEX_COMMENT;
    }

    if (l->state == LEX_STATE_COMMENT &&
        buf_at(l->s, l->s_size, *l->i) == 'H' &&
        buf_at(l->s, l->s_size, *l->i + 1) == 'E' &&
        buf_at(l->s, l->s_size, *l->i + 2) == 'L' &&
        buf_at(l->s, l->s_size, *l->i + 3) == 'P' &&
        (buf_at(l->s, l->s_size, *l->i + 4) == ' ' ||
         buf_at(l->s, l->s_size, *l->i + 4) == '\t')) {
        l->start = *l->i;
        *l->i += 5;

        while (buf_at(l->s, l->s_size, *l->i) == ' ' ||
               buf_at(l->s, l->s_size, *l->i) == '\t')
            *l->i += 1;

        l->state = LEX_STATE_META_1;
        return PROM_LEX_HELP;
    }

    if (l->state == LEX_STATE_COMMENT &&
        buf_at(l->s, l->s_size, *l->i) == 'T' &&
        buf_at(l->s, l->s_size, *l->i + 1) == 'Y' &&
        buf_at(l->s, l->s_size, *l->i + 2) == 'P' &&
        buf_at(l->s, l->s_size, *l->i + 3) == 'E' &&
        (buf_at(l->s, l->s_size, *l->i + 4) == ' ' ||
         buf_at(l->s, l->s_size, *l->i + 4) == '\t')) {
        l->start = *l->i;
        *l->i += 5;

        while (buf_at(l->s, l->s_size, *l->i) == ' ' ||
               buf_at(l->s, l->s_size, *l->i) == '\t')
            *l->i += 1;

        l->state = LEX_STATE_META_1;
        return PROM_LEX_TYPE;
    }

    if (l->state == LEX_STATE_COMMENT) {
        // Normal comment, free text
        while (*l->i < l->s_size && buf_at(l->s, l->s_size, *l->i) != '\n')
            *l->i += 1;
        return PROM_LEX_COMMENT;
    }

    if (l->state == LEX_STATE_META_1 &&
        c_is_alpha_or_underscore_or_colon(buf_at(l->s, l->s_size, *l->i))) {
        l->start = *l->i;

        while (
            c_is_alpha_or_underscore_or_colon(buf_at(l->s, l->s_size, *l->i)) ||
            c_is_num(buf_at(l->s, l->s_size, *l->i)))
            *l->i += 1;

        l->state = LEX_STATE_META_2;
        return PROM_LEX_METRIC_NAME;
    }

    if (l->state == LEX_STATE_META_2 &&
        buf_at(l->s, l->s_size, *l->i) != '\n') {
        l->start = *l->i;

        while (*l->i < l->s_size && buf_at(l->s, l->s_size, *l->i) != '\n')
            *l->i += 1;
        l->state = LEX_STATE_INIT;
        return PROM_LEX_TEXT;
    }

    if (l->state == LEX_STATE_INIT &&
        c_is_alpha_or_underscore_or_colon(buf_at(l->s, l->s_size, *l->i))) {
        l->start = *l->i;

        while (
            c_is_alpha_or_underscore_or_colon(buf_at(l->s, l->s_size, *l->i)) ||
            c_is_num(buf_at(l->s, l->s_size, *l->i)))
            *l->i += 1;
        l->state = LEX_STATE_METRIC_VALUE;
        return PROM_LEX_METRIC_NAME;
    }

    if (l->state == LEX_STATE_METRIC_VALUE &&
        buf_at(l->s, l->s_size, *l->i) == '{') {
        l->state = LEX_STATE_LABELS;
        l->start = *l->i;
        *l->i += 1;
        return PROM_LEX_CURLY_BRACE_LEFT;
    }

    if (l->state == LEX_STATE_LABELS &&
        c_is_alpha_or_underscore(buf_at(l->s, l->s_size, *l->i))) {
        l->start = *l->i;
        while (c_is_alpha_or_underscore(buf_at(l->s, l->s_size, *l->i)) ||
               c_is_num(buf_at(l->s, l->s_size, *l->i)))
            *l->i += 1;
        return PROM_LEX_LABEL_NAME;
    }

    if (l->state == LEX_STATE_LABELS && buf_at(l->s, l->s_size, *l->i) == '}') {
        l->state = LEX_STATE_METRIC_VALUE;
        l->start = *l->i;
        *l->i += 1;
        return PROM_LEX_CURLY_BRACE_RIGHT;
    }

    if (l->state == LEX_STATE_LABELS && buf_at(l->s, l->s_size, *l->i) == '=') {
        l->state = LEX_STATE_LABEL_VALUE;
        l->start = *l->i;
        *l->i += 1;
        return PROM_LEX_EQUAL;
    }

    if (l->state == LEX_STATE_LABELS && buf_at(l->s, l->s_size, *l->i) == ',') {
        l->start = *l->i;
        *l->i += 1;
        return PROM_LEX_COMMA;
    }

    if (l->state == LEX_STATE_LABEL_VALUE &&
        buf_at(l->s, l->s_size, *l->i) == '"') {
        *l->i += 1;
        l->start = *l->i;
        while (*l->i < l->s_size) {
            if (l->s[*l->i] == '"' && l->s[*l->i - 1] != '\\') break;
            *l->i += 1;
        }
        if (l->s[*l->i] != '"') {
            return PROM_PARSE_UNTERMINATED_STRING;
        }

        *l->i += 1;
        l->state = LEX_STATE_LABELS;
        return PROM_LEX_LABEL_VALUE;
    }

    if (l->state == LEX_STATE_METRIC_VALUE &&
        buf_at(l->s, l->s_size, *l->i) != ' ' &&
        buf_at(l->s, l->s_size, *l->i) != '\t' &&
        buf_at(l->s, l->s_size, *l->i) != '\n' &&
        buf_at(l->s, l->s_size, *l->i) != '{') {
        l->start = *l->i;

        while (*l->i < l->s_size && buf_at(l->s, l->s_size, *l->i) != ' ' &&
               buf_at(l->s, l->s_size, *l->i) != '\t' &&
               buf_at(l->s, l->s_size, *l->i) != '\n' &&
               buf_at(l->s, l->s_size, *l->i) != '{')
            *l->i += 1;

        l->state = LEX_STATE_TIMESTAMP;
        return PROM_LEX_METRIC_VALUE;
    }

    if (l->state == LEX_STATE_TIMESTAMP &&
        c_is_num(buf_at(l->s, l->s_size, *l->i))) {
        l->start = *l->i;

        while (c_is_num(buf_at(l->s, l->s_size, *l->i))) *l->i += 1;
        return PROM_LEX_TIMESTAMP;
    }

    if (l->state == LEX_STATE_TIMESTAMP &&
        buf_at(l->s, l->s_size, *l->i) == '\n') {
        l->state = LEX_STATE_INIT;
        l->start = *l->i;
        *l->i += 1;
        return PROM_LEX_NEWLINE;
    }

    return PROM_PARSE_INVALID_TOKEN;
}

struct prom_parser {
    struct lex l;
    long long int ms_now;
    struct metric* m;
    void* (*realloc_fn)(void* ptr, size_t size);
};

static int lex_next(struct lex* l) {
    int ret = 0;

    while ((ret = lex(l)) == PROM_LEX_WHITESPACE) {
    }
    return ret;
}

static int parse_labels(struct prom_parser* p, int ret) {
    if (ret != PROM_LEX_CURLY_BRACE_LEFT) return ret;

    while (1) {
        if ((ret = lex_next(&p->l)) < 0) {
            return ret;
        }
        if (ret == PROM_LEX_CURLY_BRACE_RIGHT) {
            break;
        }
        if (ret == PROM_LEX_LABEL_NAME) {
            size_t label_name_size = *p->l.i - p->l.start;
            p->m->labels_size += 1;
            if (p->m->labels_size > p->m->labels_capacity) {
                p->m->labels_capacity = 10 + p->m->labels_capacity * 2;
                p->m->labels = p->realloc_fn(
                    p->m->labels, sizeof(struct label) * p->m->labels_capacity);
                if (p->m->labels == NULL) return PROM_PARSE_ENOMEM;
            }
            struct label* label = &p->m->labels[p->m->labels_size - 1];
            label->label_name = p->l.s + p->l.start;
            label->label_name_size = label_name_size;

            if ((ret = lex_next(&p->l)) < 0) {
                return ret;
            }
            if (ret != PROM_LEX_EQUAL) {
                return PROM_PARSE_UNREACHABLE;
            }

            if ((ret = lex_next(&p->l)) < 0) {
                return ret;
            }
            if (ret != PROM_LEX_LABEL_VALUE) return PROM_PARSE_UNREACHABLE;

            size_t label_value_size = *p->l.i - p->l.start;
            label->label_value = p->l.s + p->l.start;
            label->label_value_size = label_value_size - 1;
            if (utf8_naive(label->label_value, label->label_value_size) != 0)
                return PROM_PARSE_INVALID_UTF8;

            if ((ret = lex_next(&p->l)) < 0) {
                return ret;
            }

            if (ret != PROM_LEX_COMMA) break;
        } else {
            return PROM_PARSE_UNREACHABLE;
        }
    }

    if (ret != PROM_LEX_CURLY_BRACE_RIGHT) {
        return PROM_PARSE_MISSING_CLOSING_CURLY_BRACKET;
    }

    return lex_next(&p->l);
}

static int parse_type(struct prom_parser* p, int ret,
                      size_t* type_metric_name_start,
                      size_t* type_metric_name_size) {
    if (ret != PROM_LEX_TYPE) return ret;
    if ((ret = lex_next(&p->l)) < 0) {
        return ret;
    }
    if (ret != PROM_LEX_METRIC_NAME) return PROM_PARSE_MISSING_HELP_METRIC_NAME;

    *type_metric_name_start = p->l.start;
    *type_metric_name_size = *p->l.i - p->l.start;

    if ((ret = lex_next(&p->l)) < 0) {
        return ret;
    }
    if (ret != PROM_LEX_TEXT) return PROM_PARSE_MISSING_TYPE_TEXT;

    const char gauge[] = {'g', 'a', 'u', 'g', 'e'};
    const char histogram[] = {'h', 'i', 's', 't', 'o', 'g', 'r', 'a', 'm'};
    const char counter[] = {'c', 'o', 'u', 'n', 't', 'e', 'r'};
    const char summary[] = {'s', 'u', 'm', 'm', 'a', 'r', 'y'};
    const char untyped[] = {'u', 'n', 't', 'y', 'p', 'e', 'd'};
    const size_t type_size = *p->l.i - p->l.start;
    if (type_size == sizeof(gauge) &&
        memcmp(p->l.s + p->l.start, gauge, type_size) == 0)
        p->m->type = PROM_METRIC_TYPE_GAUGE;
    else if (type_size == sizeof(histogram) &&
             memcmp(p->l.s + p->l.start, histogram, type_size) == 0)
        p->m->type = PROM_METRIC_TYPE_HISTOGRAM;
    else if (type_size == sizeof(counter) &&
             memcmp(p->l.s + p->l.start, counter, type_size) == 0)
        p->m->type = PROM_METRIC_TYPE_COUNTER;
    else if (type_size == sizeof(summary) &&
             memcmp(p->l.s + p->l.start, summary, type_size) == 0)
        p->m->type = PROM_METRIC_TYPE_SUMMARY;
    else if (type_size == sizeof(untyped) &&
             memcmp(p->l.s + p->l.start, untyped, type_size) == 0)
        p->m->type = PROM_METRIC_TYPE_UNTYPED;
    else
        return PROM_PARSE_UNKNOWN_TYPE_TEXT;

    if ((ret = lex_next(&p->l)) < 0) {
        return ret;
    }
    if (ret != PROM_LEX_NEWLINE) return PROM_PARSE_MISSING_NEWLINE;
    return 0;
}

static int parse_help(struct prom_parser* p, int ret,
                      size_t* help_metric_name_start,
                      size_t* help_metric_name_size) {
    if (ret != PROM_LEX_HELP) return ret;
    if ((ret = lex_next(&p->l)) < 0) {
        return ret;
    }
    if (ret != PROM_LEX_METRIC_NAME) return PROM_PARSE_MISSING_HELP_METRIC_NAME;

    *help_metric_name_start = p->l.start;
    *help_metric_name_size = *p->l.i - p->l.start;

    if ((ret = lex_next(&p->l)) < 0) {
        return ret;
    }
    if (ret != PROM_LEX_TEXT) return PROM_PARSE_MISSING_HELP_TEXT;
    if (utf8_naive(p->l.s + p->l.start, *p->l.i - p->l.start) != 0)
        return PROM_PARSE_INVALID_UTF8;

    p->m->help = p->l.s + p->l.start;
    p->m->help_size = *p->l.i - p->l.start;

    if ((ret = lex_next(&p->l)) < 0) {
        return ret;
    }
    if (ret != PROM_LEX_NEWLINE) return PROM_PARSE_MISSING_NEWLINE;
    return 0;
}

int prom_parse_2(const unsigned char* s, size_t s_size, long long int ms_now,
                 size_t* i, struct metric* m,
                 void* (*realloc_fn)(void* ptr, size_t size)) {
    struct prom_parser p = {.realloc_fn = realloc_fn,
                            .l = {.s = s, .s_size = s_size, .i = i},
                            .ms_now = ms_now,
                            .m = m};
    int ret = 0;
    size_t help_metric_name_start = 0;
    size_t help_metric_name_size = 0;
    size_t type_metric_name_start = 0;
    size_t type_metric_name_size = 0;
    // NOTE: some allocated memory, not freed, could be leaked here
    *(p.m) = (struct metric){0};

    while (1) {
        if ((ret = lex_next(&p.l)) < 0) {
            return ret;
        }
        if (ret == PROM_LEX_END) return PROM_PARSE_END;
        if (ret == PROM_LEX_NEWLINE) continue;
        if (ret == PROM_LEX_COMMENT) {
            if ((ret = lex_next(&p.l)) < 0) {
                return ret;
            }
            if (ret != PROM_LEX_NEWLINE) return PROM_PARSE_MISSING_NEWLINE;

            continue;
        }
        if (ret == PROM_LEX_HELP) {
            if ((ret = parse_help(&p, ret, &help_metric_name_start,
                                  &help_metric_name_size)) < 0)
                return ret;
            continue;
        }
        if (ret == PROM_LEX_TYPE) {
            if ((ret = parse_type(&p, ret, &type_metric_name_start,
                                  &type_metric_name_size)) < 0)
                return ret;
            continue;
        }
        if (ret == PROM_LEX_METRIC_NAME) break;
    }

    if (ret == PROM_LEX_METRIC_NAME) {
        p.m->metric_name = p.l.s + p.l.start;
        p.m->metric_name_size = *p.l.i - p.l.start;

        if ((ret = lex_next(&p.l)) < 0) {
            return ret;
        }
        if ((ret = parse_labels(&p, ret)) < 0) return ret;

        if (ret != PROM_LEX_METRIC_VALUE) {
            return PROM_PARSE_MISSING_METRIC_VALUE;
        }

        char* e = NULL;
        // TODO: handle null termination
        p.m->value = strtod((const char*)(p.l.s + p.l.start), &e);
        const char* normal_end = (char*)(&p.l.s[*p.l.i]);
        if (e && e != normal_end) {
            return PROM_PARSE_INVALID_FLOAT;
        }

        if ((ret = lex_next(&p.l)) < 0) {
            return ret;
        }
        if (ret == PROM_LEX_TIMESTAMP) {
            p.m->timestamp = strtoll((const char*)(p.l.s + p.l.start), &e, 10);
            if (errno == ERANGE) return PROM_PARSE_TIMESTAMP_OVERFLOW;

            if ((ret = lex_next(&p.l)) < 0) {
                return ret;
            }
            if (ret != PROM_LEX_NEWLINE) {
                return PROM_PARSE_MISSING_NEWLINE;
            }

        } else if (ret == PROM_LEX_NEWLINE) {
            p.m->timestamp = p.ms_now;
        } else {
            return PROM_PARSE_MISSING_NEWLINE;
        }
    }

    if ((ret = lex_next(&p.l)) < 0) {
        return ret;
    }
    if (ret == PROM_LEX_END) return PROM_PARSE_END;
    if (ret == PROM_LEX_NEWLINE) return PROM_PARSE_METRIC_OK;
    return PROM_PARSE_UNREACHABLE;
}
