FROM alpine:3.9 as BUILDER
RUN apk update && apk add make gcc g++ libc-dev

WORKDIR /prom
COPY . .
RUN CPATH=$PWD make example check install

FROM alpine:3.9 as runner 
COPY --from=BUILDER /usr/local/include/prom.h /usr/local/include/prom.h
