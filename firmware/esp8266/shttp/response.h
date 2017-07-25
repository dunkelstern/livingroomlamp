#ifndef shttp_response_h_included
#define shttp_response_h_included

#include "simplehttp/http.h"

void shttp_write_response(shttpResponse *response, int socket);

#endif /* shttp_response_h_included */