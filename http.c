#include "http.h"
#include "common.h"
#include "session.h"
#include "socket.h"
#include "str.h"

/**
 * 获取http头
 * @param buf
 * @param sockfd
 * @return int
 */
int read_header(int sockfd, char* buf) {
    char *bufp;
    char buffer[1024];
    int i, nbytes, cnt;

    bufp = buf;
    memset(buffer, 0, sizeof (buffer));
    //读取头部
    i = 0;
    cnt = 0;
    while ((nbytes = read(sockfd, buffer, 1)) == 1) {
        //获取连续4个换行前的信息
        if (i < 4) {
            if (buffer[0] == '\r' || buffer[0] == '\n') {
                i++;
            } else i = 0;
            bufp[cnt++] = buffer[0];
            if (i == 4) {
                break;
            }
        }
    }
    return 0;
}

/**
 * 把http头解析
 * @param headerbuf
 * @param host
 * @param port
 * @param content_len
 * @return int
 */
int parse_header(char* headerbuf, char* host, int* port, int* content_len) {

    //获取端口和host
    char * p = strstr(headerbuf, "Host:");
    if (!p) {
        return -1;
    }

    char * p1 = strchr(p, '\n');
    if (!p1) {
        return -1;
    }

    char * p2 = strchr(p + 5, ':'); /* 5是指'Host:'的长度 */

    if (p2 && p2 < p1) {//判断是否带端口
        int p_len = (int) (p1 - p2 - 1);
        char s_port[p_len];
        strncpy(s_port, p2 + 1, p_len);
        s_port[p_len] = '\0';

        *port = atoi(s_port);

        int h_len = (int) (p2 - p - 5 - 1);
        strncpy(host, p + 5 + 1, h_len); //Host:
        //assert h_len < 128;
        host[h_len] = '\0';
    } else {
        int h_len = (int) (p1 - p - 5 - 1 - 1);
        strncpy(host, p + 5 + 1, h_len);
        //assert h_len < 128;
        host[h_len] = '\0';
        *port = 80;
    }

    //获取content length
    p = NULL;
    p = strstr(headerbuf, "Content-Length:");

    if (p) {
        char * p1 = strchr(p, '\n');
        if (!p1) {
            return -1;
        }

        int c_len = (int) (p1 - p - 15 - 1 - 1);

        char tmp[32];
        strncpy(tmp, p + 15 + 1, c_len);
        tmp[c_len] = '\0';

        *content_len = atoi(tmp);
    } else {
        *content_len = 0;
    }

    return 0;
}

static int strip_return_port(char* host) {
    char *ptr;
    int port;

    ptr = strrchr(host, ':');
    if (ptr == NULL)
        return 0;
    //*ptr = '\0';
    ptr++;
    if (sscanf(ptr, "%d", &port) != 1)
        return 0;

    return port;

}

/**
 * 获取http头
 * @param buf
 * @param sockfd
 * @return int
 */
int parse_request_header(session_t* sess) {
    int ret;
    char buf[1024];

    memset(buf, '\0', 1024);
    ret = readline(sess->ctrl_fd, buf, 1024);
    trim(buf);

    memset(sess->host, '\0', sizeof (sess->host));
    memset(sess->protocol, '\0', sizeof (sess->protocol));
    memset(sess->method, '\0', sizeof (sess->method));
    memset(sess->path, '\0', sizeof (sess->path));
    ret = sscanf(buf, "%[^ ] %[^ ] %[^ ]", sess->method, sess->url, sess->protocol);

    char* url;
    if (strncasecmp(sess->url, "http://", 7) == 0) {
        url = strstr(sess->url, "//") + 2;
    } else {
        url = sess->url;
    }

    char* p = strchr(url, '/');

    if (p != NULL) {
        int len;
        len = p - url;
        memcpy(sess->host, url, len);
        sess->host[len] = '\0';
        strcpy(sess->path, p);
    } else {
        strcpy(sess->host, url);
        strcpy(sess->path, "/");
    }


    sess->port = strip_return_port(sess->host);

    if (sess->port == 0 || strcmp(sess->method, "CONNECT") != 0) {
        sess->port = 80;
    }

    /*printf("protocol: %s\n", sess->protocol);
    printf("url: %s\n", sess->url);
    printf("method: %s\n", sess->method);
    printf("path: %s\n", sess->path);
    printf("port: %d\n", sess->port);
     printf("host: %s\n", sess->host);*/

    return ret;
}

int get_headers(int fd, TList* header_map) {
    int ret;
    char buf[1024];
    char key[256];
    char value[10240];

    while (1) {
        memset(buf, '\0', sizeof (buf));
        memset(key, '\0', sizeof (key));
        memset(value, '\0', sizeof (value));

        ret = readline(fd, buf, sizeof (buf));

        if (buf[0] == '\r' && buf[1] == '\n')
            break;

        trim(buf);
        str_split(buf, key, value, ':');

        trim(value);
        TList_set(header_map, key, value);
    }

    return ret;
}

/**创建一个http的connection*/
int http_connect(int fd, const char* method, const char* path, const char* host, int port) {
    char portbuff[7];
    if (port != 80 && port != 443) {
        snprintf(portbuff, sizeof (portbuff), ":%d", port);
    } else {
        portbuff[0] = '\0';
    }
    return prwrite(fd,
            "%s %s HTTP/1.0\r\n"
            "Host: %s%s\r\n"
            "Connection: close\r\n",
            method, path, host, portbuff);
}

//把client的数据读出来

int tran_data(int from_fd, int to_fd, long int length) {
    char* buffer;
    ssize_t len;

    buffer = (char*) malloc(min(MAXBUFFSIZE, (unsigned long int) length));
    if (!buffer)
        return -1;

    do {
        len = readn(from_fd, buffer, min(MAXBUFFSIZE, (unsigned long int) length));
        if (len <= 0)
            goto ERROR_EXIT;
        //如果没有错误，则转发给服务器
        if (writen(to_fd, buffer, len) < 0)
            goto ERROR_EXIT;

        length -= len;
    } while (length > 0);

    //浏览器会留下2个bytes在post数据的末尾，这里我们来处理一下
    setfd_nonblock(from_fd);


    len = recv(from_fd, buffer, 2, MSG_PEEK);

    setfd_block(from_fd);


    if (len < 0 && errno != EAGAIN)
        goto ERROR_EXIT;

    //把那2个bytes的出来
    if ((len == 2) && buffer[0] == '\r' && buffer[1] == '\n') {
        ssize_t bytes_read;
        bytes_read = read(from_fd, buffer, 2);
        if (bytes_read == -1) {
            //TODO
        }
    }
    free(buffer);
    return 0;

ERROR_EXIT:
    free(buffer);
    return -1;
}