#include "server.h"
#include <err.h>

int main(int argc, char** argv) {
    if (argc != 2)
        errx(EXIT_FAILURE, "./server <port>");

    server_init(argv[1]);
    server_launch();

    exit(0);
}
