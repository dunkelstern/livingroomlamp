#include "parser.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>

#include "debug.h"
#include "router.h"
#include "urlcoder.h"
#include "response.h"

#ifndef MIN
#define MIN(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })
#endif

extern shttpConfig *shttpServerConfig;

typedef struct _shttpParserState {
    bool introductionFinished;
    bool headerFinished;
    uint32_t expectedBodySize;

    shttpRequest request;
    shttpMethod method;
    char *path;

    uint8_t allocatedHeaders;
    uint8_t allocatedParameters;
} shttpParserState;

static __attribute__((noinline)) bool shttp_parse_introduction(shttpParserState *state) {
    char *data = state->request.bodyData;
    uint16_t len = state->request.bodyLen;
    uint16_t i; // parser index

    // find method
    if (strncmp("GET ", data, MIN(4, len)) == 0) {
        state->method = shttpMethodGET;
        i = 4;
    } else
    if (strncmp("POST ", data, MIN(5, len)) == 0) {
        state->method = shttpMethodPOST;
        i = 5;
    } else
    if (strncmp("PUT ", data, MIN(4, len)) == 0) {
        state->method = shttpMethodPUT;
        i = 4;
    } else
    if (strncmp("PATCH ", data, MIN(6, len)) == 0) {
        state->method = shttpMethodPATCH;
        i = 6;
    } else
    if (strncmp("DELETE ", data, MIN(7, len)) == 0) {
        state->method = shttpMethodDELETE;
        i = 7;
    } else
    if (strncmp("OPTIONS ", data, MIN(8, len)) == 0) {
        state->method = shttpMethodOPTIONS;
        i = 8;
    } else
    if (strncmp("HEAD ", data, MIN(5, len)) == 0) {
        state->method = shttpMethodHEAD;
        i = 5;
    } else {
        return false;
    }

    LOG(TRACE, "shttp: parser -> method: %d", state->method);

    // get path until the ? (if there is one)
    bool parameters = false;

    state->path = malloc(len - i);
    char *buf = state->path;
    for(; i < len; i++) {
        if ((data[i] != '?') && (data[i] != ' ')) {
            *buf++ = data[i];
        } else {
            if (data[i] == '?') {
                parameters = true;
            }
            break;
        }
    }
    *buf++ = '\0';

    LOG(TRACE, "shttp: parser -> path: '%s'", state->path);

    // read parameters
    if (parameters) {
        uint16_t keyStart = i;
        uint16_t valueStart = 0;
        for(; i < len; i++) {
            if (data[i] == '=') {
                // key ended, value started
                valueStart = i + 1;
            }
            if ((data[i] == '&') || (data[i] == ' ')) {
                // if we find an ampersand or a space the value ended
                // key = keyStart -> valueStart - 2
                // value = valueStart -> i - 1

                // at first check if we have to re-alloc the parameter list
                if (state->allocatedParameters < state->request.numParameters + 1) {
                    state->request.parameters = realloc(state->request.parameters, sizeof(shttpParameter) * (state->allocatedParameters + 1));
                    if (state->request.parameters == NULL) {
                        state->request.numParameters = 0;
                        state->allocatedParameters = 0;
                        LOG(ERROR, "shttp: Out of memory while parsing intro");
                        return false;
                    }

                    state->allocatedParameters++;
                }
                
                uint8_t tmpLen;
                
                // copy key
                tmpLen = (valueStart - 2) - keyStart;
                char *name = shttp_url_decode_buffer(data + keyStart, tmpLen);

                // copy value
                tmpLen = (i - 1) - valueStart;
                char *value = shttp_url_decode_buffer(data + valueStart, tmpLen);

                LOG(TRACE, "shttp: parser parameter -> '%s': '%s'", name, value);

                // now append the new parameter
                state->request.parameters[state->request.numParameters] = (shttpParameter){ name, value };

                // increment the param count
                state->request.numParameters++;

                // set next key start to next char
                keyStart = i + 1;
                valueStart = 0;
            }
            if (data[i] == ' ') {
                // this means the end of the parameter list
                break;
            }
        }
    }

    // find line break, move data in buffer and shrink buffer
    for(; i < len - 1; i++) {
        if ((data[i] == '\r') && (data[i + 1] == '\n')) {
            uint16_t newLen = state->request.bodyLen - i - 2;
            LOG(TRACE, "shttp: http request intro consumed %d bytes", state->request.bodyLen - newLen);
            memmove(state->request.bodyData, state->request.bodyData + i + 2, newLen);
            state->request.bodyData = realloc(state->request.bodyData, newLen);
            state->request.bodyLen = newLen;
            state->introductionFinished = true;
            break;
        }
    }

    return true;
}

static __attribute__((noinline)) bool shttp_parse_headers(shttpParserState *state) {
    char *data = state->request.bodyData;
    uint16_t len = state->request.bodyLen;
    uint16_t keyStart = UINT16_MAX;
    uint16_t valueStart = UINT16_MAX;

    if (len < 5) {
        return true;
    }

    // split out headers
    for(uint16_t i = 0; i < len - 1; i++) {
        // if we find a ':' and have not already started the value
        // we start the value at the first non-whitespace char after
        // the ':'
        if ((data[i] == ':') && (valueStart == UINT16_MAX)) {
            // skip over whitespace
            for (uint16_t j = i + 1; j < len; j++) {
                if ((data[j] != ' ') && (data[j] != '\t')) {
                    valueStart = j;
                    break;
                }
            }
            continue;
        } else if ((keyStart == UINT16_MAX) && (data[i] != '\r') && (data[i] != '\n')) {
            keyStart = i;
        }

        // line break so header line is finished
        if ((data[i] == '\r') && (data[i + 1] == '\n')) {
            // no k/v pair in this line -> end of header block, shrink buffer
            if ((keyStart == UINT16_MAX) && (valueStart == UINT16_MAX)) {
                uint16_t newLen = state->request.bodyLen - i - 2;
                memmove(state->request.bodyData, state->request.bodyData + i + 2, newLen);
                state->request.bodyData = realloc(state->request.bodyData, newLen + 1);
                state->request.bodyData[newLen] = '\0'; // zero terminate to be sure
                state->request.bodyLen = newLen;
                state->headerFinished = true;
                return true;
            }
            if (valueStart == UINT16_MAX) {
                // Only a key without a value? Parse error!
                return false;
            }

            // at first check if we have to re-alloc the header list
            if (state->allocatedHeaders < state->request.numHeaders + 1) {
                state->request.headers = realloc(state->request.headers, sizeof(shttpHeader) * (state->allocatedHeaders + 1));
                if (state->request.headers == NULL) {
                    state->request.numHeaders = 0;
                    state->allocatedHeaders = 0;
                    LOG(ERROR, "shttp: Out of memory while parsing headers");
                    return false;
                }

                state->allocatedHeaders++;
            }
            
            uint8_t tmpLen;
            
            // copy key
            tmpLen = (valueStart - 1) - keyStart;
            char *name = malloc(tmpLen + 1);
            memcpy(name, data + keyStart, tmpLen);
            for (uint8_t j = 0; j < tmpLen; j++) {
                name[j] = tolower(name[j]);
            }
            name[tmpLen - 1] = '\0';

            // copy value
            tmpLen = (i + 1) - valueStart;
            char *value = malloc(tmpLen + 1);
            memcpy(value, data + valueStart, tmpLen);
            value[tmpLen - 1] = '\0';
        
            LOG(TRACE, "shttp: parser -> header: '%s: %s'", name, value);

            // now append the new parameter
            state->request.headers[state->request.numHeaders] = (shttpHeader){ name, value };

            // increment the param count
            state->request.numHeaders++;

            // handle special headers directly
            if ((strlen(name) == 14) && (strcmp("content-length", name) == 0)) {
                state->expectedBodySize = atoi(value);
            }
            if ((shttpServerConfig->hostName != NULL) && (strlen(name) == 4) && (strcmp("host", name) == 0)) {
                if (strcmp(shttpServerConfig->hostName, value) != 0) {
                    // FIXME: wrong host return a response immediately                        
                }
            }

            // reset starting positions
            keyStart = UINT16_MAX;
            valueStart = UINT16_MAX;
        }
    }

    return true; // await more data
}


//
// API
//

shttpParserState *shttp_parser_init_state(void) {
    shttpParserState *result = malloc(sizeof(shttpParserState));
    result->introductionFinished = false;
    result->headerFinished = false;
    result->expectedBodySize = 0;
    
    result->request.numHeaders = 0;
    result->allocatedHeaders = 5;
    result->request.headers = malloc(5 * sizeof(shttpHeader));
    
    result->request.numParameters = 0;
    result->allocatedParameters = 0;
    result->request.parameters = NULL;

    result->request.numPathParameters = 0;
    result->request.pathParameters = NULL;

    result->request.bodyData = NULL;
    result->request.bodyLen = 0;

    return result;
}

bool shttp_parse(shttpParserState *state, char *buffer, uint16_t len, int socket) {
    bool result = true;

    if (state->request.bodyData) {
        // realloc internalized buffer to contain buffer
        if (state->request.bodyLen + len > SHTTP_MAX_BODY_SIZE) {
            LOG(ERROR, "shttp: HTTP request too long");
            shttp_write_response(shttp_empty_response(shttpStatusBadRequest), socket);
            return false;
        }
        state->request.bodyData = realloc(state->request.bodyData, state->request.bodyLen + len);
    } else {
        // create internalized buffer
        state->request.bodyData = malloc(len);
    }
    if (!state->request.bodyData) {
        LOG(ERROR, "shttp: Out of memory while building buffer");
        return false;
    }

    // copy buffer to internal buffer
    LOG(TRACE, "shttp: received %d bytes, appending to %d in buffer", len, state->request.bodyLen);
    memcpy(state->request.bodyData + state->request.bodyLen, buffer, len);
    state->request.bodyLen += len;

    while(1) {
        LOG(TRACE, "shttp: parser loop entered, %d bytes left (intro: %d, headers: %d)", state->request.bodyLen, state->introductionFinished, state->headerFinished);

        // check if we are in header or body mode
        if ((!state->introductionFinished) || (!state->headerFinished)) {
            // check if we have a \r\n in the buffer
            char *data = state->request.bodyData;
            for (uint16_t i = 0; i < state->request.bodyLen - 1; i++) {
                if ((data[i] == '\r') && (data[i + 1] == '\n')) {

                    // line break found, try to parse the stuff
                    if (!state->introductionFinished) {
                        result = shttp_parse_introduction(state);
                        break;
                    }
                    if (!state->headerFinished) {
                        result = shttp_parse_headers(state);
                        break;
                    }
                }
            }
            if (!result) {
                // parse error
                return false;
            }
            continue;
        }

        // if headers have finished, check if body size reached
        if (state->headerFinished) {
            if (state->request.bodyLen >= state->expectedBodySize) {
                // yeah we have everything, execute the route
                LOG(TRACE, "shttp: parser -> expected body size reached: %d/%d", state->request.bodyLen, state->expectedBodySize);

                // run the callback
                shttp_exec_route(state->path, state->method, &state->request, socket);

                // we operate in 'Connection: close' mode always
                return false;
            }
            LOG(TRACE, "shttp: parser -> waiting for more data");
            break;
        } else {
            if (state->request.bodyLen < 5) {
                LOG(TRACE, "shttp: parser -> waiting for more data, headers not finished");
                break;
            }
        }
    }

    // await more data
    return true;
}

void shttp_destroy_parser(shttpParserState *state) {
    LOG(TRACE, "shttp: parser -> destroy");

    // free headers
    if (state->request.headers != NULL) {
        for(uint8_t i = 0; i < state->request.numHeaders; i++) {
            free(state->request.headers[i].name);
            free(state->request.headers[i].value);
        }
        free(state->request.headers);
    }

    // free parameters
    if (state->request.parameters != NULL) {
        for(uint8_t i = 0; i < state->request.numParameters; i++) {
            free(state->request.parameters[i].name);
            free(state->request.parameters[i].value);
        }
        free(state->request.parameters);
    }

    // free path parameters
    if (state->request.pathParameters != NULL) {
        for(uint8_t i = 0; i < state->request.numPathParameters; i++) {
            free(state->request.pathParameters[i]);
        }
        free(state->request.pathParameters);
    }

    // free internal buffer
    if (state->request.bodyData) {
        free(state->request.bodyData);
    }

    // free path
    if (state->path) {
        free(state->path);
    }

    // free state object
    free(state);
}