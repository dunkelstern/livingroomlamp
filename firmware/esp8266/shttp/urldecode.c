#include <stdlib.h>
#include <string.h>
#include <stdint.h>

char *shttp_url_decode_buffer(char *buffer, uint8_t len) {
    // allocate output buffer and exit if not enough memory
    char *output = malloc(len + 1);
    if (output == NULL) {
        return NULL;
    }

    // copy bytes over to output buffer
    uint8_t j = 0;
    for (uint8_t i = 0; i < len; i++) {
        if (buffer[i] == '%') {
            // decode %xx where xx is a hex number
            output[j++] = (char)strtol(buffer + i, NULL, 16);
            i += 2;
        } else if (buffer[i] == '+') {
            // plus will get decoded to space
            output[j++] = ' ';
        } else {
            // all other characters will stay as is (yes that's possibly
            // naive and too simple)
            output[j++] = buffer[i];
        }
    }
    // zero terminate buffer
    output[j++] = '\0';

    // shrink buffer to conserve memory
    return realloc(output, j);
}

char *shttp_url_decode(char *value) {
    // slow but size efficient method reuse
    return shttp_url_decode_buffer(value, strlen(value));
}