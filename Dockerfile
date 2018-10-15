FROM alpine:latest as builder
MAINTAINER Jack Murdock <jack_murdock@comcast.com>

# build the binary
WORKDIR /parodus
RUN apk add --update --repository https://dl-3.alpinelinux.org/alpine/edge/testing/ \
cmake autoconf make musl-dev gcc g++ openssl openssl-dev git cunit cunit-dev automake libtool util-linux-dev

COPY . /parodus/

RUN mkdir build
RUN cd build && cmake .. && make && make install


FROM alpine:latest
RUN apk --no-cache add ca-certificates openssl

RUN mkdir /tests/

COPY --from=builder /usr/local/bin/parodus /usr/local/bin/parodus
COPY --from=builder /parodus/build/src/parodus /usr/bin/
COPY --from=builder /parodus/build/_install/lib/*.so /usr/lib/
COPY --from=builder /parodus/build/_install/lib64/*.so /usr/lib/
COPY --from=builder /parodus/build/tests/* /tests/

RUN ln -s /usr/lib/libmsgpackc.so /usr/lib/libmsgpackc.so.2 && \
ln -s /usr/lib/libtrower-base64.so /usr/lib/libtrower-base64.so.1.0.0 && \
ln -s /usr/lib/libnopoll.so /usr/lib/libnopoll.so.0 && \
ln -s /usr/lib/libcimplog.so /usr/lib/libcimplog.so.1.0.0 && \
ln -s /usr/lib/libnanomsg.so /usr/lib/libnanomsg.so.5 && \
ln -s /usr/lib/libnanomsg.so /usr/lib/libnanomsg.so.5.1.0 && \
ln -s /usr/lib/libcjson.so /usr/lib/libcjson.so.1

ENTRYPOINT ["parodus"]
