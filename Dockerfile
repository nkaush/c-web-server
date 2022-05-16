FROM alpine:latest

ENV TZ=US/Pacific

RUN apk --update add --no-cache gcc clang make git gdb valgrind libc-dev

WORKDIR /mount

EXPOSE 49500

ENTRYPOINT [ "/bin/sh" ]
