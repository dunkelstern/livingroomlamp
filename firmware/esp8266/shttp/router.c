#include "simplehttp/http.h"
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "debug.h"
#include "response.h"

extern shttpConfig *shttpServerConfig;

static shttpRoute *shttp_find_route(char *path, shttpMethod method, shttpRequest *request) {
    uint8_t pathLen = strlen(path);

    LOG(TRACE, "shttp: finding route for '%s' (%d chars)", path, pathLen);

    if (shttpServerConfig->appendSlashes) {
        if (path[pathLen - 1] == '/') {
            path[pathLen - 1] = '\0';
            pathLen--;
        }
    }

    uint8_t currentRoute = 0;
    shttpRoute *route = shttpServerConfig->routes[currentRoute];
    while(route != NULL) {
        uint8_t routeLen = strlen(route->path);
        LOG(TRACE, "shttp: trying route '%s' (%d chars)", route->path, routeLen);

        bool found = true;
        uint8_t pathIndex = 0, routeIndex = 0;
        for (; pathIndex < pathLen; pathIndex++) {
            if (routeIndex >= routeLen) {
                LOG(TRACE, "shttp: route length reached at %d, pathIndex: %d", routeIndex, pathIndex);
                found = false;
                break;
            }

            if (route->path[routeIndex] == '?') {
                LOG(TRACE, "shttp: parameter in route at %d (%d chars left)", routeIndex, pathLen - pathIndex);

                // found parameter, skip path to next slash or end
                for(uint8_t i = 0; i < pathLen - pathIndex; i++) {
                    if ((path[pathIndex + i] == '/') || (path[pathIndex + i] == ' ') || (i == pathLen - pathIndex - 1)) {
                        LOG(TRACE, "shttp: parameter length in path: %d", i + 1);
                        pathIndex += i;
                        break;
                    }
                }

                routeIndex++;
                continue;
            } else if (route->path[routeIndex] == '*') {
                LOG(TRACE, "shttp: wildcard in route at %d", routeIndex);
                found = true;
                routeIndex++;
                break;
            } else if (route->path[routeIndex] != path[pathIndex]) {
                LOG(TRACE, "shttp: mismatch at %d", routeIndex);
                found = false;
                break;
            }
            routeIndex++;
        }

        if (routeIndex < routeLen) {
            found = false;   
        }

        if (found) {
            // check if the method matches
            if ((route->allowedMethods & method) != 0) {
                break;
            }
        }

        currentRoute++;
        route = shttpServerConfig->routes[currentRoute];
    }

    return route;
}

static void shttp_parse_url_parameters(char *path, shttpRoute *route, shttpRequest *request) {    
    uint8_t pathLen = strlen(path);
    uint8_t routeLen = strlen(route->path);
    for (uint8_t pathIndex = 0, routeIndex = 0; pathIndex < pathLen; pathIndex++) {
        if (routeIndex >= routeLen) {
            break;
        }

        if (route->path[routeIndex] == '?') {
            LOG(TRACE, "shttp: parser -> URL path parameter in route at %d", routeIndex);

            // found parameter, find next slash
            for(uint8_t i = 0; i < pathLen - pathIndex; i++) {
                if ((path[pathIndex + i] == '/') || (path[pathIndex + i] == ' ') || (i == pathLen - pathIndex - 1)) {
                    // copy parameter value
                    char *param = malloc(i + 2);
                    memcpy(param, path + pathIndex, i + 1);
                    param[i + 1] = '\0';

                    LOG(TRACE, "shttp: URL path parameter '%s'", param);
                    // realloc parameter array and append param
                    request->pathParameters = realloc(request->pathParameters, (request->numPathParameters + 1) * sizeof(char *));
                    request->pathParameters[request->numPathParameters] = param;
                    request->numPathParameters++;

                    // skip over the parameter and continue parsing
                    pathIndex += i;
                    break;
                }
            }

            routeIndex++;
            continue;            
        } else if (route->path[routeIndex] == '*') {
            break;
        }

        routeIndex++;
    }

}

void shttp_exec_route(char *path, shttpMethod method, shttpRequest *request, int socket) {
    // find a route
    shttpRoute *route = shttp_find_route(path, method, request);
    LOG(TRACE, "shttp: Route %x", route);

    // no route found return 404
    if (!route) {
        LOG(TRACE, "shttp: no route, returning 404");
        shttp_write_response(shttp_empty_response(shttpStatusNotFound), socket);
    }

    // parse url parameters
    shttp_parse_url_parameters(path, route, request);
    LOG(TRACE, "shttp: %d URL path parameters", request->numPathParameters);

    // call callback and return response
    shttp_write_response(route->callback(request, route->userData), socket);
}

//
// API
//

shttpRoute *shttp_route(shttpMethod method, char *path, shttpRouteCallback *callback, void *userData) {
    shttpRoute *route = malloc(sizeof(shttpRoute));
    route->allowedMethods = method;
    route->path = path;
    route->callback = callback;
    route->userData = userData;

    return route;
}