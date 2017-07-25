#ifndef shttp_parser_h_included
#define shttp_parser_h_included

#include "simplehttp/http.h"

typedef struct _shttpParserState shttpParserState;

shttpParserState *shttp_parser_init_state(void);
bool shttp_parse(shttpParserState *state, char *buffer, uint16_t len, int socket);
void shttp_destroy_parser(shttpParserState *state);

#endif /* shttp_parser_h_included */