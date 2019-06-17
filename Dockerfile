ARG LIB_TYPE
FROM alpine:3.9 as BUILDER
RUN apk update && apk add make gcc libc-dev

WORKDIR /prom
COPY . .
# Default flags contain '-Weverything' which is clang only
RUN make CFLAGS_COMMON="-std=c99 -Wall -Wextra -Wpedantic" LIB_TYPE=${LIB_TYPE} build check install

FROM alpine:3.9 as runner 
COPY --from=BUILDER /usr/local/lib/libprom.a /usr/local/lib/libprom.a
