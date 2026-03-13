#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <size_in_mb>\n", argv[0]);
        return 1;
    }

    long mb = atol(argv[1]);
    long bytes = mb * 1024 * 1024;
    char buffer[1024];

    for (long i = 0; i < bytes; i += sizeof(buffer)) {
        long remaining = bytes - i;
        long to_write = (remaining < sizeof(buffer)) ? remaining : sizeof(buffer);
        fwrite(buffer, 1, to_write, stdout);
    }

    return 0;
}