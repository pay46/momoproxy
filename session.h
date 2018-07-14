#ifndef SESSION_H
#define SESSION_H
#include "common.h"
#include "buffer.h"
#include "LinkList.h"

typedef struct session {
    int ctrl_fd; //链接套接字
    int serv_fd;
    char header[MAX_HEADER_LEN]; //http头部
    char body[MAX_BODY_LEN]; //http body
    char source_ip[32]; //源ip
    char method[32];
    char protocol[32];
    char host[1024];
    int port;
    char url[1024];
    char path[1024];
    TList header_map;
    struct buffer_s* cbuffer;
    struct buffer_s* sbuffer;
    int content_len;
} session_t;

void begin_session(session_t* sess);
void begin_session2(session_t* sess);
#endif /* SESSION_H */

