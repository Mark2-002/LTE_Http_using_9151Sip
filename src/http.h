#ifndef HTTP_CLIENT_APP_H_
#define HTTP_CLIENT_APP_H_

#include <stdint.h>
#define RECV_BUF_SIZE 2048
#define CLIENT_ID_SIZE 36
// #define CONFIG_HTTP_SAMPLE_HOSTNAME1 "webhook.site"
/* Resolve server hostname and fill socket address */
int server_resolve(void);

/* Create TCP socket and connect to server */
int server_connect(void);

/* HTTP client APIs */
int client_http_post_json(int16_t data[]);
int client_http_post_text(char text_data[]);
int client_http_put(void);
int client_http_get(void);
int client_get_new_id(void);

#endif /* HTTP_CLIENT_APP_H_ */
