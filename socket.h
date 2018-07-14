#ifndef SOCKET_H
#define SOCKET_H
#include "common.h"
ssize_t readn(int fd, void *buf, size_t count);
ssize_t writen(int fd, const void *buf, size_t count);
ssize_t recv_peek(int sockfd, void* buf, size_t len);
ssize_t readline(int sockfd, void *buf, size_t maxLen);
int read_timeout(int sockfd, unsigned int wait_second);
int readline_timeout(int sockfd, char* recvbuf, int len, int timeout);
int write_timeout(int sockfd, unsigned int wait_second);
int writen_timeout(int sockfd, const char* buf, size_t count, int timeout);
int accept_timeout(int sockfd, struct sockaddr_in* addr, unsigned int wait_second);
void setfd_nonblock(int sockfd);
void setfd_block(int sockfd);
int connect_timeout(int sockfd, struct sockaddr_in* addr, unsigned int wait_second);
int tcp_server(const char *host, unsigned short port);
int open_clientfd(char *ip, int port);
int prwrite(int fd, const char *fmt, ...);
#endif /* SOCKET_H */

