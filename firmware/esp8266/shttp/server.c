#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include <time.h>

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#include "debug.h"
#include "simplehttp/http.h"

#include "parser.h"
#include "router.h"

static int listeningSocket;
static xQueueHandle connectionQueue;
static xTaskHandle dataTask;

volatile shttpConfig *shttpServerConfig;

static bool bind_and_listen(const char *port) {
    struct addrinfo hints;

    // settings
    memset( &hints, 0, sizeof( hints ) );
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // fetch hints and addr_list
    struct addrinfo *addrList;
    if (getaddrinfo("0.0.0.0", port, &hints, &addrList ) != 0) {
        LOG(ERROR, "shttp: getaddrinfo failed!")
        return false;
    }

    // Try the sockaddrs until a binding succeeds
    for (struct addrinfo *cur = addrList; cur != NULL; cur = cur->ai_next) {
        listeningSocket = socket(
            cur->ai_family,
            cur->ai_socktype,
            cur->ai_protocol
        );
        if (listeningSocket < 0){
            continue;
        }

		// SO_REUSEADDR option is disabled by default in lwip
#if SO_REUSE
        const char n = 1;
        if (setsockopt(listeningSocket, SOL_SOCKET, SO_REUSEADDR, &n, sizeof(n)) != 0) {
            close(listeningSocket);
            LOG(ERROR, "shttp: Setting SO_REUSEADDR option failed!")
            continue;
        }
#endif

        // now bind to any interface (there's only one)
		struct sockaddr_in *serv_addr = NULL;
		serv_addr = (struct sockaddr_in *)cur->ai_addr;		
		serv_addr->sin_addr.s_addr = htonl(INADDR_ANY);
        if(bind(listeningSocket, (struct sockaddr *)serv_addr, cur->ai_addrlen) != 0 ) {
            close(listeningSocket);
            LOG(ERROR, "shttp: Could not bind to interface!");
            continue;
        }

        // start listening
        if (listen(listeningSocket, SHTTP_MAX_QUEUED_CONNECTIONS) != 0){
            close(listeningSocket);
            LOG(ERROR, "shttp: Listening failed!");
            continue;
        }

        // we are listening, stop iterating
        freeaddrinfo(addrList);
        return true;
    }

    // if we get here we exhausted the address list
    freeaddrinfo(addrList);
    return false;
}

void readTask(void *userData) {
    int socket;
    char *recv_buffer;
    int result;
    shttpParserState *parser;

    while(1) {
        // fetch a connection from the queue
        xQueueReceive(connectionQueue, &socket, portMAX_DELAY);

        // allocate receive buffer
        recv_buffer = malloc(SHTTP_MAX_RECV_BUFFER);
        if (recv_buffer == NULL) {
            LOG(ERROR, "shttp: Out of memory, terminating connection");
            close(socket);
            continue;
        }

        // create a parser
        parser = shttp_parser_init_state();

        // receive data
        while(1) {
            result = recv(socket, recv_buffer, SHTTP_MAX_RECV_BUFFER, 0);
            if (result <= 0) {
                if ((errno == EPIPE) || (errno == ECONNRESET) || (result == 0)) {
                    // client disconnected
                    LOG(DEBUG, "shttp: client disconnected");
                    close(socket);
                    break;
                }
                
                if (errno == EINTR) {
                    // interrupted, try again
                    continue;
                }
            } else {
                // received some bytes, run parser on it
                if (!shttp_parse(parser, recv_buffer, result, socket)) {
                    // parser thinks we should close the connection
                    LOG(DEBUG, "shttp: parse called for quit");
                    break;
                }
            }
        }

        // clean up
        free(recv_buffer);
        shttp_destroy_parser(parser);
        close(socket);
        LOG(DEBUG, "shttp: connection closed");
    }
}

void shttp_listen(shttpConfig *config) {
    struct sockaddr_in clientAddr;
    socklen_t addrLen;
    int incomingSocket;

    // bind and listen
    bool result = bind_and_listen(config->port);
    if (result == false){
        LOG(ERROR, "shttp: Giving up");
        return;
    }

    // Create data processing queue
    connectionQueue = xQueueCreate(SHTTP_MAX_QUEUED_CONNECTIONS, sizeof(int));
    if (connectionQueue == NULL) {
        LOG(ERROR, "shttp: Could not create connection queue, terminating");
        close(listeningSocket);
        return;
    }

    // start data processing task
    if (xTaskCreate(readTask, "shttp.read", SHTTP_STACK_SIZE, NULL, SHTTP_PRIO, &dataTask) != pdPASS) {
        LOG(ERROR, "shttp: Could not create data processing task, terminating");
        vQueueDelete(connectionQueue);
        close(listeningSocket);
        return;
    }

    shttpServerConfig = config;
    LOG(DEBUG, "shttp: server ready to accept connections");

    // accept connections
    while(1) {
        addrLen = sizeof(clientAddr);

        // this blocks until a client connects
        incomingSocket = accept(listeningSocket, (struct sockaddr *) &clientAddr, &addrLen);
        if (incomingSocket < 0) {
            if (errno == EINTR) {
                continue;
            }
            LOG(ERROR, "shttp: Could not accept connection, terminating");
            vTaskDelete(dataTask);
            vQueueDelete(connectionQueue);
            close(listeningSocket);
            return;
        }

        LOG(TRACE, "shttp: Client connected, signaling communications thread");
        xQueueSendToBack(connectionQueue, &incomingSocket, portMAX_DELAY);
    }
}
