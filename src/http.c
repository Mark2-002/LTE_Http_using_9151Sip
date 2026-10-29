#include <zephyr/net/socket.h>
#include <zephyr/net/http/client.h>
#include <zephyr/logging/log.h>
#include "http.h"
#define EVENT_MASK (NET_EVENT_L4_CONNECTED | NET_EVENT_L4_DISCONNECTED)
LOG_MODULE_REGISTER(http_c, LOG_LEVEL_INF);


static char recv_buf[RECV_BUF_SIZE];
static char client_id_buf[CLIENT_ID_SIZE + 2];
static int counter = 0;
static int sock;
static struct sockaddr_storage server;
static struct sockaddr_in6 server_addr;
int server_resolve(void)
{
    int err;
    struct addrinfo *result;
    struct addrinfo hints = {
        .ai_family = AF_INET6, 
        .ai_socktype = SOCK_STREAM,
    };
    err = getaddrinfo(CONFIG_HTTP_SAMPLE_HOSTNAME,
                      CONFIG_HTTP_SAMPLE_PORT,
                      &hints,
                      &result);

    if (err != 0 || result == NULL)
    {
        LOG_ERR("getaddrinfo failed, err: %d, %s", err, gai_strerror(err));
        return -EIO;
    }

    memcpy(&server_addr, result->ai_addr, sizeof(struct sockaddr_in6));
    char ipv6_addr[NET_IPV6_ADDR_LEN];
    inet_ntop(AF_INET6, &server_addr.sin6_addr, ipv6_addr, sizeof(ipv6_addr));
    LOG_INF("IPv6 address resolved: %s", ipv6_addr);
    freeaddrinfo(result);
    return 0;
}

int server_connect(void)
{
    int err;
    sock = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);

    if (sock < 0)
    {
        LOG_ERR("Failed to create socket: %d", errno);
        return -errno;
    }

    LOG_INF("Connecting to server...");
    err = connect(sock,
                  (struct sockaddr *)&server_addr,
                  sizeof(struct sockaddr_in6));

    if (err < 0)
    {
        LOG_ERR("Connect failed, err: %d, %s", errno, strerror(errno));
        close(sock);
        return -errno;
    }

    LOG_INF("Connected!");
    return 0;
}
static int response_cb(struct http_response *rsp, enum http_final_call final_data, void *user_data)
{
    LOG_INF("Response status: %s", rsp->http_status);
    if (rsp->body_frag_len > 0)
    {
        char body_buf[rsp->body_frag_len];
        strncpy(body_buf, rsp->body_frag_start, rsp->body_frag_len);
        body_buf[rsp->body_frag_len] = '\0';
        LOG_INF("Received: %s", body_buf);
    }
    LOG_INF("Closing socket: %d", sock);
    close(sock);
    return 0;
}

static int client_id_cb(struct http_response *rsp, enum http_final_call final_data,
                        void *user_data)
{
    LOG_INF("Response status: %s", rsp->http_status);
    char client_id_buf_tmp[CLIENT_ID_SIZE + 1];
    strncpy(client_id_buf_tmp, rsp->body_frag_start, CLIENT_ID_SIZE);
    client_id_buf_tmp[CLIENT_ID_SIZE] = '\0';
    client_id_buf[0] = '/';
    strcat(client_id_buf, client_id_buf_tmp);

    LOG_INF("Successfully acquired client ID: %s", client_id_buf);

    LOG_INF("Closing socket: %d", sock);
    close(sock);
    return 0;
}

int client_http_post_json(int16_t data[])
{
    int err = 0;
    int bytes_written;
    struct http_request req;
    memset(&req, 0, sizeof(req));
    static char buffer[512];
    static char content_length[32];
    static const char *headers[] = {
        "Content-Type: application/json\r\n",
        content_length,
        "Connection: close\r\n",
        NULL};
    bytes_written = snprintf(buffer, sizeof(buffer), "{\n\"x\": %u,\n\"y\":%u,\n\"z\":%u,\n}", data[0], data[1], data[2]);
    if (bytes_written < 0)
    {
        LOG_INF("Unable to write to buffer, err: %d", bytes_written);
        return bytes_written;
    }
    snprintf(content_length, sizeof(content_length), "Content-Length: %d\r\n", bytes_written);

    req.header_fields = headers;
    req.method = HTTP_POST;
    req.url = CONFIG_HTTP_URL; 
    req.host = CONFIG_HTTP_SAMPLE_HOSTNAME;
    req.protocol = "HTTP/1.1";
    req.payload = buffer;
    req.payload_len = bytes_written;
    req.response = response_cb;
    req.recv_buf = recv_buf;
    req.recv_buf_len = sizeof(recv_buf);
    LOG_INF("HTTP POST request: %s", buffer);
    err = http_client_req(sock, &req, 5000, NULL);
    if (err < 0)
    {
        LOG_ERR("Failed to send HTTP POST request %s, err: %d", buffer, err);
    }
    return err;
}

int client_http_post_text(char text_data[])
{
    int err = 0;
    int bytes_written;
    struct http_request req;
    memset(&req, 0, sizeof(req));
    static char buffer[1024];
    static char content_length[32];

    const char *headers[] = {
        "Content-Type: text/plain\r\n", 
        content_length,                
        "Connection: close\r\n",
        NULL};
    strcpy(buffer, text_data);
    bytes_written = strlen(buffer);
    if (bytes_written < 0)
    {
        LOG_INF("Unable to write to buffer, err: %d", bytes_written);
        return bytes_written;
    }
    req.header_fields = headers;
    req.method = HTTP_POST;
    req.url = CONFIG_HTTP_URL;
    req.host = CONFIG_HTTP_SAMPLE_HOSTNAME;
    req.protocol = "HTTP/1.1";
    req.payload = buffer;
    req.payload_len = bytes_written;
    req.response = response_cb;
    req.recv_buf = recv_buf;
    req.recv_buf_len = sizeof(recv_buf);
    LOG_INF("HTTP POST request: %s", buffer);
    err = http_client_req(sock, &req, 5000, NULL);
    if (err < 0)
    {
        LOG_ERR("Failed to send HTTP POST request %s, err: %d", buffer, err);
    }

    return err;
}

int client_http_put(void)
{
    int err = 0;
    int bytes_written;
    struct http_request req;
    memset(&req, 0, sizeof(req));
    static char content_length[32];
    static char buffer[512];
    static const char *headers[] = {
        "Content-Type: application/json\r\n",
        content_length,
        "Connection: close\r\n",
        NULL};
    bytes_written = snprintf(buffer, sizeof(buffer), "{\"counter\": %d}", counter);
    if (bytes_written < 0)
    {
        LOG_INF("Unable to write to buffer, err: %d", bytes_written);
        return bytes_written;
    }
    snprintf(content_length, sizeof(content_length), "Content-Length: %d\r\n", bytes_written);
    req.header_fields = headers;
    req.method = HTTP_PUT;
    req.url = CONFIG_HTTP_URL;
    req.host = CONFIG_HTTP_SAMPLE_HOSTNAME;
    req.protocol = "HTTP/1.1";
    req.payload = buffer;
    req.payload_len = bytes_written;
    req.response = response_cb;
    req.recv_buf = recv_buf;
    req.recv_buf_len = sizeof(recv_buf);

    LOG_INF("HTTP PUT request: %s", buffer);
    err = http_client_req(sock, &req, 5000, NULL);
    if (err < 0)
    {
        LOG_ERR("Failed to send HTTP PUT request %s, err: %d", buffer, err);
    }
    counter++;
    return err;
}

int client_http_get(void)
{
    int err = 0;
    struct http_request req;
    memset(&req, 0, sizeof(req));
    const char *headers[] = {"Connection: close\r\n", NULL};
    req.header_fields = headers;
    req.method = HTTP_GET;
    req.url = CONFIG_HTTP_URL;
    req.host = CONFIG_HTTP_SAMPLE_HOSTNAME;
    req.protocol = "HTTP/1.1";
    req.response = response_cb;
    req.recv_buf = recv_buf;
    req.recv_buf_len = sizeof(recv_buf);
    /* Send the GET request to the server */
    LOG_INF("HTTP GET request");
    err = http_client_req(sock, &req, 5000, NULL);
    if (err < 0)
    {
        LOG_ERR("Failed to send HTTP GET request, err: %d", err);
    }
    return err;
}

int client_get_new_id(void)
{
    int err = 0;
    struct http_request req;
    memset(&req, 0, sizeof(req));
    const char *headers[] = {"Connection: close\r\n", NULL};
    req.header_fields = headers;
    req.method = HTTP_POST;
    req.url = CONFIG_HTTP_URL;
    req.host = CONFIG_HTTP_SAMPLE_HOSTNAME;
    req.protocol = "HTTP/1.1";
    req.response = client_id_cb;
    req.recv_buf = recv_buf;
    req.recv_buf_len = sizeof(recv_buf);
    LOG_INF("HTTP POST request");
    err = http_client_req(sock, &req, 5000, NULL);
    if (err < 0)
    {
        LOG_ERR("Failed to send HTTP POST request, err: %d", err);
    }
    return err;
}