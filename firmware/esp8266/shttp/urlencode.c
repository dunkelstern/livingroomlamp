#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

char *shttp_url_encode_buffer(char *buffer, uint8_t len) {
    // allocate output buffer and exit if not enough memory
    uint8_t allocatedLength = len + 12; // reserve enough space for 4 encoded chars
    char *output = malloc(allocatedLength + 1); 
    if (output == NULL) {
        return NULL;
    }

    char *reserved = " !\"#$%&'()*+,-./:;<=>?@[\\]{|}";
    uint8_t j = 0;
    for (uint8_t i = 0; i < len; i++) {
        bool found = false;
        for (uint8_t k = 0; k < 29; k++) {
            if (buffer[i] == reserved[k]) {
                // check for realloc

                // length has to be greater than
                // - the remaining length (len - i)
                // - plus the 3 bytes of the escape sequence (3)
                // - plus the combined length of all escapes until now (j - i)
                if (allocatedLength < len - i + 3 + (j - i)) {
                    allocatedLength += 3;
                    output = realloc(output, allocatedLength);
                    if (output == NULL) {
                        return NULL;
                    }
                }
                output[j++] = '%';
                output[j++] = (reserved[k] >> 4) < 10 ? (reserved[k] >> 4) + 48 : (reserved[k] >> 4) + 86;
                output[j++] = (reserved[k] & 0x0f) < 10 ? (reserved[k] & 0x0f) + 48 : (reserved[k] & 0x0f) + 86;
                found = true;
                break;
            }
        }
        if (!found) {
            output[j++] = buffer[i];
        }
    }
    // zero terminate buffer
    output[j++] = '\0';

    // probably wasting 12 bytes here
    return output;
}

char *shttp_url_encode(char *value) {
    // slow but size efficient method reuse
    return shttp_url_encode_buffer(value, strlen(value));
}
