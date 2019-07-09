FROM alpine:3.9 as BUILDER
RUN apk update && apk add make gcc libc-dev

WORKDIR /prom
COPY . .
RUN make --stop example check install

FROM alpine:3.9 as runner 
COPY --from=BUILDER /usr/local/include/prom.h /usr/local/include/prom.h
