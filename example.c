#include <errno.h>
#include <fcntl.h>
#include <stddef.h>  // size_t
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include "prom.h"

static void metric_to_yml(const struct metric* m) {
    printf("- Name: \"");
    fwrite(m->metric_name, m->metric_name_size, 1, stdout);

    printf("\"\n  Value: %f\n", m->value);
    printf("  Timestamp: %lld\n", m->timestamp);

    if (m->help) {
        printf("  Help: \"");
        fwrite(m->help, m->help_size, 1, stdout);
        printf("\"\n");
    }

    printf("  Type: ");
    switch (m->type) {
        case PROM_METRIC_TYPE_COUNTER:
            printf("counter");
            break;
        case PROM_METRIC_TYPE_GAUGE:
            printf("gauge");
            break;
        case PROM_METRIC_TYPE_HISTOGRAM:
            printf("histogram");
            break;
        case PROM_METRIC_TYPE_SUMMARY:
            printf("summary");
            break;
        default:
            printf("untyped");
            break;
    }
    printf("\n");

    if (m->labels) {
        printf("  Labels:\n");
        for (size_t l = 0; l < m->labels_size; l++) {
            printf("    ");
            fwrite(m->labels[l].label_name, m->labels[l].label_name_size, 1,
                   stdout);
            printf(": \"");
            fwrite(m->labels[l].label_value, m->labels[l].label_value_size, 1,
                   stdout);
            printf("\"\n");
        }
    }
}

static int file_read(int fd, unsigned char** out, size_t* out_size) {
    if (fd < 0) return ENOENT;

    struct stat stat = {0};
    const int res = fstat(fd, &stat);
    if (res == -1) {
        close(fd);
        return errno;
    }
    size_t size = stat.st_size;

    *out = calloc(1, 1 + size);

    size_t nread = read(fd, *out, size);
    if (nread < size) return errno;
    *out_size = size;

    return 0;
}

int main(int argc, char* argv[]) {
    if (argc != 3) return 1;
    int err = 0;
    long long int ms_now = strtoll(argv[2], NULL, 10);

    int input_fd = open(argv[1], O_RDONLY);
    unsigned char* input = NULL;
    size_t input_size = 0;
    if ((err = file_read(input_fd, &input, &input_size)) != 0) {
        printf("%d\n", err);
        return err;
    }

    size_t i = 0;
    struct metric m;
    while ((err = prom_parse_2(input, input_size, ms_now, &i, &m, realloc)) <
           0) {
        metric_to_yml(&m);
        free(m.labels);
    }
    if (err < 0) {
        printf("%d\n", err);
        return err;
    }
}
