# Libprom

A minimalistic, header-only C99 library to parse Prometheus metrics in their text format. It only depends on libc and does not use the system allocator (you can pass it your custom allocator). See `example.c` and `example_cxx.cpp` for usage.

# What is a Prometheus metric in textual form?

```
# TYPE allocated_bytes gauge
# HELP allocated_bytes This metric tracks allocated bytes
# Some comment
allocated_bytes{tier="backend", environment="production"} 1000 1563167927
foo 5
```

Prometheus is a monitoring system using a textual representation for metrics.
It is harder to parse as you might think. At first glance one might think this is line based, but comments make it not so. On top of it, akin to csv, double quotes can be escaped within a pair of double quotes, which render some UNIX tools like awk inadequate. Additionally, some parts (e.g timestamp and labels) are optional.

Hence this library.

Here we have 2 metrics, the first one having a type comment, a help comment, a normal comments, and 2 labels, the second one having only the required parts: name and value.

When running the C example program on this: 
`./example_c /tmp/metric.readme 1563167929`, we get a YAML representation:

```yaml
- Name: "allocated_bytes"
  Value: 1000.000000
  Timestamp: 1563167927
  Help: "This metric tracks allocated bytes"
  Type: gauge
  Labels:
    tier: "backend"
    environment: "production"
- Name: "foo"
  Value: 5.000000
  Timestamp: 1563167929
  Type: untyped
```

Note that comments are ignored per the Prometheus spec. One could extend the library to parse them as well if need be.
Also note that metrics without a timestamp are given the command line timestamp, it posssibly being the current timestamp (e.g `date +%s`).


And that's the essence of it! The library just parses the textual representation of metrics, you decide what to do whith them.

## Use it

- Run `make && make install`
- Include `prom.h` in your C or C++ code
- Use the only public function: `prom_parse`

Build the Docker image containing `prom.h`: `make dbuild`. This will also run the tests.

## Ownership system

All metric  fields are string views, i.e they only hold a pointer to an underlying string and a size instead of owning a copy of the string.
It means there are no allocations for those and that the underlying string needs to outlive the metric.
The only allocation needed is for the array of labels but you can pass you custom allocator, possibily using the stack. 
Do not forget to free the labels if you passed a heap allocator, because the library will not free those for you.

Also, all strings must contain UTF-8 (and this is checked), and are **not** null-terminated, i.e in C lingua, those are char arrays, not strings per se (which would be null terminated).

## Differences with the official Prometheus implemention

- Libprom does not sort labels within a metric, it preserves the original ordering
- Prometheus post-processes the metrics after parsing them to check if the metric name in a `TYPE` or `HELP` comment matches the metric name in the following metric line, or the group of metrics in case of a histogram or summary. Libprom does not do any post-processing.

## Error handling

Errors are just an int defined in `prom.h` (`PROM_PARSE_x` defines). They should be reasonably detailed. In case of an error, the offset `i` is not changed (i.e it points to the beginning of the erroneous line). The metric `m` can be considered to contain garbage. The parsing is halted. A potential manual error recovery mechanism could be to skip the current line and try again to parse.

## Develop

Build: `make example`.

Test: `make check`.

Build the docker image: `make dbuild`
