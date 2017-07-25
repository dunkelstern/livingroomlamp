#ifndef shttp_urlcoder_h_included
#define shttp_urlcoder_h_included

#include <stdint.h>

char *shttp_url_decode_buffer(char *buffer, uint8_t len);
char *shttp_url_encode_buffer(char *buffer, uint8_t len);

#endif /* shttp_urlcoder_h_included */