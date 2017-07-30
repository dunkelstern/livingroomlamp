#include <esp_common.h>

#include <strings.h>
#include <stdlib.h>
#include <unistd.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <simplehttp/http.h>
#include <mdns/mdns.h>
#include <cJSON.h>

#include <files.h>

#define HOSTNAME "wohnzimmerlampe"

void startup(void *userData);

mdnsHandle *mdns;

typedef enum _mode {
    modeWhite = 0,
    modeCinema = 1,
    modeMoodlight = 2
} Mode;

static float hue = 0;
static float saturation = 1.0;
static float brightness = 1.0;
static float lowPowerRing = 1.0;
static float highPowerRing = 0.0;
static Mode mode = modeWhite;

typedef struct _getFileData {
    const char *data;
    const uint32_t size;
    char *mimeType;
} getFileData;

/******************************************************************************
 * FunctionName : user_rf_cal_sector_set
 * Description  : SDK just reversed 4 sectors, used for rf init data and paramters.
 *                We add this function to force users to set rf cal sector, since
 *                we don't know which sector is free in user's application.
 *                sector map for last several sectors : ABCCC
 *                A : rf cal
 *                B : rf init data
 *                C : sdk parameters
 * Parameters   : none
 * Returns      : rf cal sector
*******************************************************************************/
uint32 user_rf_cal_sector_set(void)
{
    flash_size_map size_map = system_get_flash_size_map();
    uint32 rf_cal_sec = 0;

    switch (size_map) {
        case FLASH_SIZE_4M_MAP_256_256:
            rf_cal_sec = 128 - 5;
            break;

        case FLASH_SIZE_8M_MAP_512_512:
            rf_cal_sec = 256 - 5;
            break;

        case FLASH_SIZE_16M_MAP_512_512:
        case FLASH_SIZE_16M_MAP_1024_1024:
            rf_cal_sec = 512 - 5;
            break;

        case FLASH_SIZE_32M_MAP_512_512:
        case FLASH_SIZE_32M_MAP_1024_1024:
            rf_cal_sec = 1024 - 5;
            break;
        case FLASH_SIZE_64M_MAP_1024_1024:
            rf_cal_sec = 2048 - 5;
            break;
        case FLASH_SIZE_128M_MAP_1024_1024:
            rf_cal_sec = 4096 - 5;
            break;
        default:
            rf_cal_sec = 0;
            break;
    }

    return rf_cal_sec;
}

void ICACHE_FLASH_ATTR wifi_event_handler_cb(System_Event_t *event) {
    static int running = 0;

    if (event->event_id == EVENT_STAMODE_GOT_IP) {
        if (running) {
    		mdns_update_ip(mdns, event->event_info.got_ip.ip);    
            return;
        }
        running = 1;
        
        if (xTaskCreate(startup, "server", 250, NULL, 4, NULL) != pdPASS) {
            printf("HTTP Startup failed!\n");
        } else {
            mdns = mdns_create(HOSTNAME);
            mdns_update_ip(mdns, event->event_info.got_ip.ip);    
            mdnsService *service = mdns_create_service("_http", mdnsProtocolTCP, 80);
            // mdns_service_add_txt(service, "bla", "blubb");
            mdns_add_service(mdns, service);
            mdns_start(mdns);
        }
    }
}

static void sendValuesToArduino(void) {
    printf("mode=%d\n", mode);
    vTaskDelay(10 / portTICK_RATE_MS);
    printf("hue=%d\n", (int)(hue * 255.0));
    vTaskDelay(10 / portTICK_RATE_MS);
    printf("saturation=%d\n", (int)(saturation * 255.0));
    vTaskDelay(10 / portTICK_RATE_MS);
    printf("brightness=%d\n", (int)(brightness * 255.0));
    vTaskDelay(10 / portTICK_RATE_MS);
    printf("lowpowerring=%d\n", (int)(lowPowerRing * 255.0));
    vTaskDelay(10 / portTICK_RATE_MS);
    printf("highpowerring=%d\n", (int)(highPowerRing * 255.0));
    vTaskDelay(10 / portTICK_RATE_MS);
}

static cJSON *buildResponse(void) {
    cJSON *root = cJSON_CreateObject();
    cJSON *jsHue = cJSON_CreateNumber(hue);
    cJSON *jsSaturation = cJSON_CreateNumber(saturation);
    cJSON *jsBrightness = cJSON_CreateNumber(brightness);
    cJSON *jsLowPower = cJSON_CreateNumber(lowPowerRing);
    cJSON *jsHighPower = cJSON_CreateNumber(highPowerRing);
    cJSON *jsMode = NULL;
    switch(mode) {
        case modeWhite:
            jsMode = cJSON_CreateString("white");
            break;
        case modeCinema:
            jsMode = cJSON_CreateString("cinema");
            break;
        case modeMoodlight:
            jsMode = cJSON_CreateString("moodlight");
            break;
    }
    cJSON_AddItemToObject(root, "hue", jsHue);
    cJSON_AddItemToObject(root, "saturation", jsSaturation);
    cJSON_AddItemToObject(root, "brightness", jsBrightness);
    cJSON_AddItemToObject(root, "lowPower", jsLowPower);
    cJSON_AddItemToObject(root, "highPower", jsHighPower);
    cJSON_AddItemToObject(root, "mode", jsMode);
    
    return root;
}

static shttpResponse *getParameters(shttpRequest *request, void *userData) {
    cJSON *root = buildResponse();
    return shttp_json_response(shttpStatusOK, root);
}

static shttpResponse *setParameters(shttpRequest *request, void *userData) {
    cJSON *item;
    cJSON *root = cJSON_Parse(request->bodyData);
    if (!root) {
        printf("Body len: %d", request->bodyLen);
        printf("%s", request->bodyData);
        return shttp_text_response(shttpStatusBadRequest, "Could not parse JSON");
    }

    item = cJSON_GetObjectItem(root, "hue");
    if (item) {
        hue = item->valuedouble;
    }

    item = cJSON_GetObjectItem(root, "saturation");
    if (item) {
        saturation = item->valuedouble;
    }

    item = cJSON_GetObjectItem(root, "brightness");
    if (item) {
        brightness = item->valuedouble;
    }
    
    item = cJSON_GetObjectItem(root, "lowPower");
    if (item) {
        lowPowerRing = item->valuedouble;
    }

    item = cJSON_GetObjectItem(root, "highPower");
    if (item) {
        highPowerRing = item->valuedouble;
    }

    item = cJSON_GetObjectItem(root, "mode");
    if (item) {
        char *m = item->valuestring;
        if (strcasecmp(m, "white") == 0) {
            mode = modeWhite;
        } else if (strcasecmp(m, "cinema") == 0) {
            mode = modeCinema;
        } else if (strcasecmp(m, "moodlight") == 0) {
            mode = modeMoodlight;
        }
    }
    cJSON_Delete(root);

    sendValuesToArduino();

    root = buildResponse();
    return shttp_json_response(shttpStatusOK, root);
}

static shttpResponse *getFile(shttpRequest *request, void *userData) {
    getFileData *fileData = (getFileData *)userData;

    shttpResponse *response = shttp_empty_response(shttpStatusOK);
    shttp_response_add_headers(response, "Content-Type", fileData->mimeType, NULL);
    response->body = (char *)fileData->data;
    response->bodyLen = fileData->size;

    return response;
}

/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void user_init(void) {
    printf("SDK version:%s\n", system_get_sdk_version());
    wifi_set_event_handler_cb(wifi_event_handler_cb);

    // wifi_set_opmode(STATION_MODE); 
    // struct station_config *sconfig = (struct station_config *)zalloc(sizeof(struct station_config));
    // sprintf(sconfig->ssid, "Kampftoast");
    // sprintf(sconfig->password, "dunkelstern738");
    // wifi_station_set_config(sconfig);
    // free(sconfig);
    // wifi_station_connect();
}

void startup(void *userData) {
    shttpConfig config;

    // set a hostname, if a request with a different host header
    // arrives the server will automatically return 404
    config.hostName = HOSTNAME ".local";

    // the port to use, default should be 80
    config.port = "80";

    // we don't care if the url ends with a slash
    config.appendSlashes = 1;

    config.routes = (shttpRoute *[]){
        GET( "/parameters",  getParameters, NULL),
        POST("/parameters", setParameters, NULL),
        GET( "",                getFile, &((getFileData){ index_html,     index_html_len,     "text/html" })),
        GET( "/main.css",       getFile, &((getFileData){ main_css,       main_css_len,       "text/css" })),
        GET( "/main.js",        getFile, &((getFileData){ main_js,        main_js_len,        "text/javascript" })),
        GET( "/favicon.ico",    getFile, &((getFileData){ favicon_ico,    favicon_ico_len,    "image/x-icon" })),
        GET( "/hexagon.png",    getFile, &((getFileData){ hexagon_png,    hexagon_png_len,    "image/png" })),
        GET( "/colorwheel.jpg", getFile, &((getFileData){ colorwheel_jpg, colorwheel_jpg_len, "image/jpeg" })),
        NULL
    };

    // start the server, this never returns
    shttp_listen(&config);
}

