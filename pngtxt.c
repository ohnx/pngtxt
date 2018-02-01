/* fread(), fprintf(), etc. */
#include <stdio.h>
/* malloc(), realloc(), free() */
#include <stdlib.h>
/* uint32_t */
#include <stdint.h>
/* htonl(), ntohl() */
#include <arpa/inet.h>

/* crc32() is external code - see crc32.c */
uint32_t crc32(const void *data, size_t len);

/* print message and exit */
void fatal(const char *msg) {
    fprintf(stderr, "Error: %s\n", msg);
    exit(1);
}

/* check validity of PNG header */
unsigned char buf_ref[8] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n'};
int png_valid(FILE *f) {
    static unsigned char buf[8];
    int i;

    /* read in the header */
    if (fread(buf, 1, 8, f) != 8) fatal("I/O error");

    /* check the header */
    for (i = 0; i < 8; i++) {
        if (buf[i] != buf_ref[i]) return 0;
    }

    /* All bytes match */
    return 1;
}

FILE *png;
FILE *txt;
long png_len;

#define CHUNK_MAX_SIZE 2147483647
void png_puttxt() {
    char key = ' ';
    unsigned char end_buf[12];
    unsigned char *chunk_buf, *tmp;
    uint32_t chunk_len, bread;
    uint32_t alloc_size = 32;

    /* seek to end of file minus 12 bytes (size of ending) */
    fseek(png, -12, SEEK_END);

    /* read the last 12 bytes */
    if (fread(end_buf, 1, 12, png) != 12) fatal("I/O error");

iter:
    /* increment the key */
    key++;

    /* seek to end of file minus 12 bytes (size of ending) */
    fseek(png, -12, SEEK_END);

    /* prepare the buffer */
    /* always allocate 14 more bytes to fit 10 bytes at start + 4 bytes at end */
    chunk_buf = malloc(sizeof(unsigned char) * (alloc_size + 14));
    chunk_buf[4] = 't';
    chunk_buf[5] = 'E';
    chunk_buf[6] = 'X';
    chunk_buf[7] = 't';
    chunk_buf[8] = key;
    chunk_buf[9] = 0;

    /* consume up to 2GiB of data */
    chunk_len = 0;
    while (chunk_len < CHUNK_MAX_SIZE) {
        /* keep appending to current buffer */
        bread = fread(chunk_buf + 10 + chunk_len, 1, alloc_size - chunk_len, txt);

        /* update total bytes in chunk buffer */
        chunk_len += bread;

        /* check number of bytes read */
        if (bread == 0 || feof(txt)) break;

        /* resize buffer if needed */
        if (alloc_size == chunk_len) {
            /* increase size */
            bread = alloc_size * 2;

            /* catch underflow */
            if (bread < alloc_size) alloc_size = CHUNK_MAX_SIZE;
            else alloc_size = bread;

            /* reallocate memory */
            tmp = realloc(chunk_buf, alloc_size + 14);

            /* check OOM errors */
            if (tmp == NULL) {
                free(chunk_buf);fclose(png);fclose(txt);
                fatal("Out of Memory");
            }

            /* realloc() succeeded */
            chunk_buf = tmp;
        }
    }

    /* PNG uses big-endian (aka network byte order) */

    /* Update length */
    *((uint32_t *)chunk_buf) = htonl(chunk_len + 2);

    /* calculate crc32 of data */
    *((uint32_t *)(chunk_buf + 10 + chunk_len)) = htonl(crc32(chunk_buf + 4, chunk_len + 6));

    /* write out data */
    bread = fwrite(chunk_buf, 1, chunk_len + 14, png);
    if (bread != chunk_len + 14) {
        free(chunk_buf);fclose(png);fclose(txt);
        fatal("Write error - data");
    }

    /* free memory */
    free(chunk_buf);

    /* write out ending */
    bread = fwrite(end_buf, 1, 12, png);
    if (bread != 12) {
        fclose(png);fclose(txt);
        fatal("Write error - ending");
    }

    /* check if there are more chunks to write */
    if (chunk_len == CHUNK_MAX_SIZE) goto iter;

    /* we're done! */
    return;
}

void png_readtxt() {
    
}

void print_usage(const char *exec) {
    fprintf(stderr, "Usage: %s <read or write> <input+output image> [input+output text file]\n", exec);
    fprintf(stderr, "\tIf text file is not specified, i/o will be done from stdin/stdout.\n");
    fatal("Invalid arguments");
}

/* main function */
int main(int argc, char **argv) {
    void (*op)();

    /* no way for args to be valid */
    if (argc < 3 || argc > 4) {
        print_usage(argv[0]);
    }

    /* read operation */
    if (*argv[1] == 'r') {
        op = &png_readtxt;
    } else if (*argv[1] == 'w') {
        op = &png_puttxt;
    } else {
        print_usage(argv[0]);
    }

    /* parse arguments */
    if (argc == 3) {
        /* open PNG file */
        png = fopen(argv[2], "r+b");
        if (!png) fatal("Failed to open i/o image");

        /* text is from stdin if write op, stdout if not */
        txt = *argv[1] == 'w' ? stdin : stdout;
    } else if (argc == 4) {
        /* input image and text given */
        png = fopen(argv[2], "r+b");
        if (!png) fatal("Failed to open i/o image");

        /* text is reading if write op, writing if not */
        txt = fopen(argv[3], *argv[1] == 'w' ? "r" : "w");
        if (!txt) fatal("Failed to open input text");
    }

    /* Check for PNG header */
    if (!png_valid(png)) fatal("Invalid input PNG");

    /* Get length of input png */
    fseek(png, 0, SEEK_END);
    png_len = ftell(png);
    rewind(png);

    /* Ensure the input PNG is > 20 bytes */
    if (png_len < 20) fatal("Invalid input PNG");

    /* call the appropriate function */
    op();

    /* Clean up */
    fclose(png);
    fclose(txt);
    return 0;
}
