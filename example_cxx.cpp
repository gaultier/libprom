#include <prom.h>
#include <iostream>
#include <string>
#include <vector>

int main() {
    std::string input = "# HELP foo some help\nfoo 5 6\nbar 1 2\n";

    int err = 0;

    size_t i = 0;
    struct metric m;
    while ((err = prom_parse((const unsigned char*)input.data(), input.size(),
                             42, &i, &m, realloc)) == PROM_PARSE_METRIC_OK) {
        std::cout << std::string{(const char*)m.metric_name, m.metric_name_size}
                  << std::endl;
        free(m.labels);
    }
    if (err < 0) return err;
}
