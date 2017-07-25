#ifndef shttp_http_h_included
#define shttp_http_h_included

//
// You may override the following parameters to customize memory usage
//

// Data processing task stack size
#ifndef SHTTP_STACK_SIZE
#define SHTTP_STACK_SIZE 1000
#endif

// Data processing task priority
#ifndef SHTTP_PRIO
#define SHTTP_PRIO 3
#endif

// Max HTTP recv buffer
#ifndef SHTTP_MAX_RECV_BUFFER
#define SHTTP_MAX_RECV_BUFFER 1500 /* default max MTU */
#endif

// Max HTTP body size
#ifndef SHTTP_MAX_BODY_SIZE
#define SHTTP_MAX_BODY_SIZE 4096
#endif

// simplehttp can accept multiple connections at once but only
// processes them in incoming order, one at a time. This defined
// how many connections may be queued before just dropping the
// connection
#ifndef SHTTP_MAX_QUEUED_CONNECTIONS
#define SHTTP_MAX_QUEUED_CONNECTIONS 10
#endif

// enable CJSON support
#ifndef SHTTP_CJSON
#define SHTTP_CJSON 1
#endif

//
// Includes
//

#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

#if SHTTP_CJSON
#include <stdlib.h>
#include <cJSON.h>
#endif

//
// Public API
//

// simple key/value struct, re-used for headers and parameters
typedef struct _shttpKeyValue {
    char *name;
    char *value;
} shttpKeyValue;

// a HTTP header
typedef shttpKeyValue shttpHeader;

// a HTTP URL parameter
typedef shttpKeyValue shttpParameter;

// HTTP request data
typedef struct _shttpRequest {
    // headers on the HTTP request
    shttpHeader *headers;
    // number of headers
    uint8_t numHeaders;

    // URL parameters (those after a ?)
    shttpParameter *parameters;
    // number of parameters
    uint8_t numParameters;

    // URL path parameters (those in an URL path)
    char **pathParameters;
    // number of parameters
    uint8_t numPathParameters;

    // request body
    char *bodyData;
    uint16_t bodyLen;
} shttpRequest;

// HTTP status code to make code more readable
typedef enum _shttpStatusCode {
    shttpStatusOK = 200,
    shttpStatusCreated = 201,
    shttpStatusAccepted = 202,
    shttpStatusNoContent = 204,
    
    shttpStatusMovedPermanently = 301,
    shttpStatusFound = 302,
    shttpStatusNotModified = 304,

    shttpStatusBadRequest = 400,
    shttpStatusUnauthorized = 401,
    shttpStatusForbidden = 403,
    shttpStatusNotFound = 404,
    shttpStatusNotAcceptable = 406,
    shttpStatusConflict = 409,
    shttpStatusRequestURITooLong = 414,

    shttpStatusInternalError = 500,
    shttpStatusNotImplemented = 501,
    shttpStatusBadGateway = 502,
    shttpStatusServiceUnavailable = 503
} shttpStatusCode;

// data generator callback
// parameters are:
// - the number of bytes already sent
// - output parameter (length of the chunk returned)
// - user data pointer from above
// returns char pointer with new data or NULL to finish the request (closes the connection)
typedef char *(shttpBodyCallback)(uint32_t sentBytes, uint32_t *len, void *userData);

// cleanup callback, called to clean up user data pointer
typedef void *(shttpCleanupCallback)(void *userData);

// HTTP response, returned from route callback
typedef struct _shttpResponse {
    // response code
    shttpStatusCode responseCode;

    // Headers to set, close up with NULL sentinel
    // Content-Length is calculated automatically
    shttpHeader *headers;
    // number of headers in array
    uint8_t headerCount;

    // the body to return, set to NULL to define callback or no data
    // Attention: you give this memory away, make sure it is on the
    // heap! Will be freed automatically after sending to the client.
    char *body;
    // body length,
    // - set to zero to use zero terminated string in body
    // - if set to zero using the callback, the connection is
    //   terminated when the response finishes
    uint32_t bodyLen;

    // user data pointer given to body callback
    void *callbackUserData;

    // body callback
    shttpBodyCallback *bodyCallback;

    // cleanup callback
    // is called whenever the response is finished (either erroring out
    // or finishing successfully) to clean up the user data pointer
    shttpCleanupCallback *cleanupCallback;
} shttpResponse;


// HTTP method flags
typedef enum _shttpMethod {
    shttpMethodGET     = (1 << 0),
    shttpMethodPOST    = (1 << 1),
    shttpMethodPUT     = (1 << 2),
    shttpMethodPATCH   = (1 << 3),
    shttpMethodDELETE  = (1 << 4),
    shttpMethodOPTIONS = (1 << 5),
    shttpMethodHEAD    = (1 << 6),
} shttpMethod;

typedef shttpResponse *(shttpRouteCallback)(shttpRequest *request);

typedef struct _shttpRoute {
    // allowed methods for this route, add them together to allow
    // multiple methods (flags)
    shttpMethod allowedMethods;

    // path to match
    // - use ? for parameter, parameters may not include slashes
    // - use * for wildcard. Wildcards are only allowed at the end
    char *path;

    // callback to call when route found
    shttpRouteCallback *callback;

    // if you define multiple routes with the same path and different
    // allowedMethods then the list is processed until a matching
    // entry is found.
} shttpRoute;

typedef struct _shttpConfig {
    // Hostname of the device,
    // set to NULL to listen to everything
    char *hostName; 

    // Port to listen to. You can probably use stuff like 'http' here too
    char *port;

    // set to 1 if a slash at the end of the url should be ignored
    // If true it essentially means the following is equivalent:
    // - `GET /hello`
    // - `GET /hello/`
    bool appendSlashes;

    // defined routes (for callbacks), close with a NULL sentinel
    // be aware that comparing the list is done sequentially, if no
    // match could be found the next item is tried until we reach the
    // end of the list and the server returns a 404
    shttpRoute **routes;
} shttpConfig;

// Start the shttp server, this function does not return
// use it in a thread or RTOS task.
void shttp_listen(shttpConfig *config);

// URL encode value, caller has to free the result
char *shttp_url_encode(char *value);

// URL decode value, caller has to free the result
char *shttp_url_decode(char *value);

//
// convenience functions
//

shttpRoute *shttp_route(shttpMethod method, char *path, shttpRouteCallback *callback);

#define GET(_path, _callback) shttp_route(shttpMethodGET, (_path), (_callback))
#define POST(_path, _callback) shttp_route(shttpMethodPOST, (_path), (_callback))
#define PUT(_path, _callback) shttp_route(shttpMethodPUT, (_path), (_callback))
#define PATCH(_path, _callback) shttp_route(shttpMethodPATCH, (_path), (_callback))
#define DELETE(_path, _callback) shttp_route(shttpMethodDELETE, (_path), (_callback))
#define OPTIONS(_path, _callback) shttp_route(shttpMethodOPTIONS, (_path), (_callback))
#define HEAD(_path, _callback) shttp_route(shttpMethodHEAD, (_path), (_callback) })

shttpResponse *shttp_empty_response(shttpStatusCode status);

#define BAD_REQUEST shttp_empty_response(shttpStatusBadRequest)
#define NOT_FOUND shttp_empty_response(shttpStatusNotFound)
#define NOT_IMPLEMENTED shttp_empty_response(shttpStatusNotImplemented)
#define NOT_ALLOWED shttp_empty_response(shttpStatusNotAllowed)
#define UNAUTHORIZED shttp_empty_response(shttpStatusUnauthorized)

// return a html response with correct headers set, NULL terminated
shttpResponse *shttp_html_response(shttpStatusCode status, char *html);

// return a text response with correct headers set, NULL terminated
shttpResponse *shttp_text_response(shttpStatusCode status, char *text);

// return a download with correct headers set and a specific length
// set len to 0 to indicate a NULL terminated string and calculate automatically
shttpResponse *shttp_download_response(shttpStatusCode status, char *buffer, uint32_t len, char *filename);

// return a download with the callback interface to conserve memory
// if len is set to 0 the connection will be terminated after finishing
shttpResponse *shttp_download_callback_response(shttpStatusCode status, uint32_t len, char *filename, shttpBodyCallback *callback, void *userData, shttpCleanupCallback *cleanup);

#if SHTTP_CJSON
// return a json response with correct headers set
shttpResponse *shttp_json_response(shttpStatusCode status, cJSON *json);
#endif

// add headers to `response`, allocates any memory needed, copies the input
// - add as many headers you like
// - order is name, value
// - end the list with NULL
void shttp_response_add_headers(shttpResponse *response, ...);

#endif /* shttp_http_h_included */