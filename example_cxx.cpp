#include <prom.h>
#include <array>
#include <iostream>
#include <string>
#include <vector>

std::array<struct prom_label, 5> _labels;

void* custom_alloc(void* ptr, size_t size) {
    (void)ptr;
    if (size <= sizeof(struct prom_label) * _labels.size())
        return _labels.data();
    throw std::bad_alloc{};
}

int main() {
    std::string input = "# HELP foo some help\nfoo 5 6\nbar{a=\"0\"} 1 2\n";

    int err = 0;

    size_t i = 0;
    struct metric m;
    while ((err = prom_parse((const unsigned char*)input.data(), input.size(),
                             42, &i, &m, custom_alloc)) ==
           PROM_PARSE_METRIC_OK) {
        std::cout << std::string{(const char*)m.metric_name, m.metric_name_size}
                  << std::endl;

        for (size_t i = 0; i < m.labels_size; i++) {
            struct prom_label l = m.labels[i];
            std::cout << std::string{(const char*)l.label_name,
                                     l.label_name_size}
                      << std::endl;
            std::cout << std::string{(const char*)l.label_value,
                                     l.label_value_size}
                      << std::endl;
        }
    }
    if (err < 0) return err;
}
