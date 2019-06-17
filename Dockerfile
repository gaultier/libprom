FROM alpine:3.9 as BUILDER
RUN apk update && apk add make gcc libc-dev
ARG LIB_TYPE

WORKDIR /prom
COPY . .
# Default flags contain '-Weverything' which is clang only
RUN make --stop LIB_TYPE=${LIB_TYPE} CFLAGS_COMMON="-std=c99 -Wall -Wextra -Wpedantic" build check install

FROM alpine:3.9 as runner 
COPY --from=BUILDER /usr/local/lib/libprom.* /usr/local/lib/
