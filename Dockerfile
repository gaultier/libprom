FROM alpine as BUILDER
RUN apk update && apk add make gcc libc-dev

WORKDIR /prom
COPY . .
RUN make CFLAGS_COMMON="-std=c99 -Wall" build check install

FROM alpine as runner 
COPY --from=BUILDER /usr/local/lib/libprom.a /usr/local/lib/libprom.a
