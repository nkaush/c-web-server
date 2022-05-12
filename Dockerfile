FROM alpine:latest

ENV TZ=US/Pacific

RUN apk --update add --no-cache gcc clang make git gdb valgrind libc-dev

WORKDIR /mount

EXPOSE 49500

ENTRYPOINT [ "/bin/sh" ]

# docker build -t neilk3/cs241-vm .

# docker run -it --rm -v `pwd`:/mount --hostname sp22-cs241-206.cs.illinois.edu neilk3/cs241-vm
