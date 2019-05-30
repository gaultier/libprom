# Libprom

A minimalistic library to parse Prometheus metrics in their text format. It only depends on libc and does not allocate itself (you can pass it your custom allocator). See `example.c` for usage.

## Build

Release: `make`

Build the docker image containing `libprom_release.a`: `make dbuild`

Run the tests (release mode): `make check`

## Ownership system

All metric  fields are string views, i.e they only hold a pointer to an underlying string and a size instead of owning a copy of the string.
It means there are no allocations for those and that the underlying string needs to outlive the metric.
The only allocation needed is for the array of labels but you can pass you custom allocator.

Also, all strings must contain UTF-8 (and this is checked), and are **not** null-terminated.

## Differences with the official Prometheus implemention

- Libprom does not sort labels within a metric
- Prometheus post-processes the metrics after parsing them to check if the metric name in a `TYPE` or `HELP` comment matches the metric name in the following metric line, or the group of metrics in case of a histogram or summary.


## Error handling

Errors are just an int defined in `prom.h` (`PROM_PARSE_x` defines). They should be reasonably detailed. In case of an error, the offset `i` is not changed (i.e it points to the beginning of the erroneous line). The metric `m` can be considered to contain garbage. The parsing is halted. A potential manual error recovery mechanism could be to skip the current line and try again to parse.


## Develop

Build: `make libprom_debug.a` for the library and `make prom_example_debug` for the example.

Test: `make check_debug`.

