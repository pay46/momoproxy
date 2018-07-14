#include "session.h"
#include "common.h"
#include "http.h"
#include "socket.h"
#include "LinkList.h"
#include "str.h"
#include "buffer.h"

extern TList* conftab;
char host[255];
int port;
int content_len;
int write_len;
char header_buf[MAX_HEADER_LEN];

static void http_relay(session_t* sess) {
    int ret;
    char tmp[1024] = {0};

    //获取请求body
    char body[MAX_BODY_LEN];
    if (content_len > 0) {
        readn(sess->ctrl_fd, body, content_len);
        if (IS_DEBUG) {
            printf("body:\n%s\n", body);
        }
    }

    //发送请求头
    //str_replace(header_buf, "Connection: keep-alive\r\n", "Connection: close\r\n");
    write_len = strlen(header_buf);
    //printf("sending header:\n%s\n", header_buf);
    ret = writen(sess->serv_fd, header_buf, write_len);
    if (ret != write_len) {
        close(sess->ctrl_fd);
        close(sess->serv_fd);
        die("writen");
    }
    //发送body
    write_len = strlen(body);
    ret = writen(sess->serv_fd, body, write_len);
    if (ret != write_len) {
        close(sess->ctrl_fd);
        close(sess->serv_fd);
        die("writen");
    }

    while ((ret = read(sess->serv_fd, tmp, 1)) == 1) {
        ret = writen(sess->ctrl_fd, tmp, 1);
        if (ret != 1) {
            close(sess->ctrl_fd);
            close(sess->serv_fd);
            die("writen");
        }
    }
}

static void relay(session_t* sess) {
    int ret;
    ssize_t bytes;
    int maxfd = max(sess->ctrl_fd, sess->serv_fd);

    setfd_nonblock(sess->ctrl_fd);
    setfd_nonblock(sess->serv_fd);

    double tdiff;
    time_t last_access;
    last_access = time(NULL);

    fd_set rset, wset;

    struct timeval tv;


    for (;;) {
        FD_ZERO(&rset);
        FD_ZERO(&wset);

        tv.tv_sec = MAX_IDLE_TIME - difftime(time(NULL), last_access);
        tv.tv_usec = 0;

        if (buffer_size(sess->sbuffer) > 0) {
            FD_SET(sess->ctrl_fd, &wset);
        }

        if (buffer_size(sess->cbuffer) > 0) {
            FD_SET(sess->serv_fd, & wset);
        }

        if (buffer_size(sess->sbuffer) < MAXBUFFSIZE) {
            FD_SET(sess->serv_fd, &rset);
        }

        if (buffer_size(sess->cbuffer) < MAXBUFFSIZE) {
            FD_SET(sess->ctrl_fd, &rset);
        }

        ret = select(maxfd + 1, &rset, &wset, NULL, &tv);

        if (ret == 0) {
            tdiff = difftime(time(NULL), last_access);
            if (tdiff > MAX_IDLE_TIME) {
                return;
            } else {
                continue;
            }
        } else if (ret < 0) {
            return;
        } else {
            last_access = time(NULL);
        }

        if (FD_ISSET(sess->serv_fd, &rset)) {
            bytes = read_buffer(sess->serv_fd, sess->sbuffer);
            if (content_len > 0) {
                content_len -= bytes;
                if (content_len == 0)
                    break;
            }
            if (bytes < 0)
                break;
        }

        if (FD_ISSET(sess->ctrl_fd, &rset)) {
            bytes = read_buffer(sess->ctrl_fd, sess->cbuffer);
            if (bytes < 0)
                break;

        }

        if (FD_ISSET(sess->serv_fd, &wset)) {
            bytes = write_buffer(sess->serv_fd, sess->cbuffer);
            if (bytes < 0)
                break;
        }

        if (FD_ISSET(sess->ctrl_fd, &wset)) {
            bytes = write_buffer(sess->ctrl_fd, sess->sbuffer);
            if (bytes < 0)
                break;
        }
    }

    //把剩余的buffer发送给client
    setfd_block(sess->ctrl_fd);
    while (buffer_size(sess->sbuffer) > 0) {
        bytes = write_buffer(sess->ctrl_fd, sess->sbuffer);
        if (bytes < 0)
            break;
    }
    //关闭client写端
    shutdown(sess->ctrl_fd, SHUT_WR);

    //把剩余的buffer 发送给server
    setfd_block(sess->serv_fd);
    while (buffer_size(sess->cbuffer) > 0) {
        bytes = write_buffer(sess->serv_fd, sess->cbuffer);
        if (bytes < 0)
            break;
    }

    return;
}

void begin_session(session_t* sess) {
    int ret;

    //判断是否允许
    char* allowIps = TList_get_str(conftab, "allow_address");

    //白明白过滤
    char **r;
    int num, i;
    int allow = -1;
    explode(allowIps, ',', &r, &num);

    for (i = 0; i < num; i++) {
        if (strcmp(sess->source_ip, r[i]) == 0) {
            allow = 1;
            break;
        }
    }

    if (allow == -1) {
        //获取请求头
        memset(header_buf, '\0', MAX_HEADER_LEN);

        read_header(sess->ctrl_fd, header_buf);

        if (IS_DEBUG) {
            printf("Hack header:\n%s\n", header_buf);
        }

        write(sess->ctrl_fd, "Not Allow\n", 10);

    } else {

        //获取请求头
        memset(header_buf, '\0', sizeof (header_buf));

        read_header(sess->ctrl_fd, header_buf);

        /*if (IS_DEBUG) {
            printf("request header:\n%s\n", header_buf);
        }*/

        //解释http头
        if (parse_header(header_buf, host, &port, &content_len) < 0) {
            close(sess->ctrl_fd);
            die("Not http request！");
        }
        /*取得主机IP地址*/
        struct hostent *hp;
        if ((hp = gethostbyname(host)) == NULL) {
            close(sess->ctrl_fd);
            die("gethostbyname");
        }

        char ip[32] = {0};
        sprintf(ip, "%s", inet_ntoa(*(struct in_addr*) hp->h_addr_list[0]));

        //链接真实服务器
        sess->serv_fd = open_clientfd(ip, port);


        if (sess->serv_fd < 0) {
            close(sess->ctrl_fd);
            close(sess->serv_fd);
            die("open_clientfd");
        }

        /*给真实服务器发送数据*/
        //设置数据接收超时时间
        struct timeval time;
        time.tv_sec = 3;
        time.tv_usec = 0;
        socklen_t len = sizeof (time);
        if (setsockopt(sess->serv_fd, SOL_SOCKET, SO_RCVTIMEO, &time, len) < 0) {
            close(sess->serv_fd);
            close(sess->ctrl_fd);
            die("setsockopt");
        }

        if (port == 443) {
            //响应隧道
            char* ssl_respone = "HTTP/1.1 200 Connection Established\r\n\r\n";
            write_len = strlen(ssl_respone);
            ret = writen(sess->ctrl_fd, ssl_respone, write_len);
            if (ret != write_len) {
                close(sess->ctrl_fd);
                close(sess->serv_fd);
                die("writen");
            }
            relay(sess);
        } else {
            int ret;

            //获取请求body
            char body[MAX_BODY_LEN];
            if (content_len > 0) {
                readn(sess->ctrl_fd, body, content_len);
                if (IS_DEBUG) {
                    printf("body:\n%s\n", body);
                }
            }

            //发送请求头
            str_replace(header_buf, "Connection: keep-alive\r\n", "Connection: close\r\n");
            if (IS_DEBUG) {
                printf("send header:\n%s\n", header_buf);
            }
            write_len = strlen(header_buf);
            ret = writen(sess->serv_fd, header_buf, write_len);
            if (ret != write_len) {
                close(sess->ctrl_fd);
                close(sess->serv_fd);
                die("writen");
            }


            //发送body
            write_len = strlen(body);
            ret = writen(sess->serv_fd, body, write_len);
            if (ret != write_len) {
                close(sess->ctrl_fd);
                close(sess->serv_fd);
                die("writen");
            }

            //获取请求头
            memset(header_buf, '\0', MAX_HEADER_LEN);
            read_header(sess->serv_fd, header_buf);
            str_replace(header_buf, "Connection: keep-alive\r\n", "Connection: close\r\n");
            if (IS_DEBUG) {
                printf("return header:\n%s\n", header_buf);
            }
            write_len = strlen(header_buf);
            ret = writen(sess->ctrl_fd, header_buf, write_len);
            if (ret != write_len) {
                close(sess->ctrl_fd);
                close(sess->serv_fd);
                die("writen");
            }
            relay(sess);
            //http_relay(sess);
        }
        close(sess->serv_fd);
    }
    close(sess->ctrl_fd);

}

void begin_session2(session_t* sess) {


    //判断是否允许
    char* allowIps = TList_get_str(conftab, "allow_address");

    //白明白过滤
    char **r;
    int num, i, ret;
    int allow = -1;
    explode(allowIps, ',', &r, &num);

    for (i = 0; i < num; i++) {
        if (strcmp(sess->source_ip, r[i]) == 0) {
            allow = 1;
            break;
        }
    }

    if (allow == -1) {
        write(sess->ctrl_fd, "Not Allow\n", 10);
    } else {
        //读取http首行，接受请求到sess
        parse_request_header(sess);

        //把client头信息以键值形式读入到header_map
        get_headers(sess->ctrl_fd, &sess->header_map); TList_print_all(&sess->header_map);

        /*取得主机IP地址*/
        struct hostent *hp;
        if ((hp = gethostbyname(sess->host)) == NULL) {
            close(sess->ctrl_fd);
            die("gethostbyname");
        }

        char ip[32] = {0};
        sprintf(ip, "%s", inet_ntoa(*(struct in_addr*) hp->h_addr_list[0]));


        //链接真实服务器
        sess->serv_fd = open_clientfd(ip, sess->port);


        if (sess->serv_fd < 0) {
            close(sess->ctrl_fd);
            close(sess->serv_fd);
            die("open_clientfd");
        }

        //设置数据接收超时时间
        struct timeval time;
        time.tv_sec = 3;
        time.tv_usec = 0;
        socklen_t len = sizeof (time);
        if (setsockopt(sess->serv_fd, SOL_SOCKET, SO_RCVTIMEO, &time, len) < 0) {
            close(sess->serv_fd);
            close(sess->ctrl_fd);
            die("setsockopt");
        }

        if (strcmp(sess->method, "CONNECT") != 0) {
            ret = http_connect(sess->serv_fd, sess->method, sess->path, sess->host, sess->port);
            if (ret != 0)
                die("http_connect");
        }

        //发送client头到server
        TItem* item = NULL;
        for (i = 0; i < TList_getlen(&sess->header_map); i++) {
            item = TList_get(&sess->header_map, i);
            if (strcmp(item->name, "content-length") == 0) {
                sess->content_len = item->int_v;
            }

            if (strcasecmp(item->name, "host") == 0 || strcasecmp(item->name, "keep-alive") == 0 || strcasecmp(item->name, "proxy-connection") == 0 ||
                    strcasecmp(item->name, "te") == 0 || strcasecmp(item->name, "trailers") == 0 || strcasecmp(item->name, "upgrade") == 0) {
                continue;
            }

            ret = prwrite(sess->serv_fd, "%s: %s\r\n", item->name, item->string_v);
            if (ret != 0) {
                die("pwrite");
            }
        }

        ret = prwrite(sess->serv_fd, "\r\n");
        if (ret != 0)
            die("pwrite");

        //如果有post数据，则转发
        if (sess->content_len > 0) {
            tran_data(sess->ctrl_fd, sess->serv_fd, sess->content_len);
        }


        char buf[1024];

        memset(buf, '\0', 1024);
        readline(sess->serv_fd, buf, 1024);
        trim(buf);

        printf("response: %s\n", buf);


        //把client头信息以键值形式读入到header_map
        //get_headers(sess-, &sess->header_map); //TList_print_all(&sess->header_map);

    }
    close(sess->ctrl_fd);
}