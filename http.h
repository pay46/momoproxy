#ifndef HTTP_H
#define HTTP_H
#include "LinkList.h"
#include "session.h"

int read_header(int sockfd, char* buf);
int parse_request_header(session_t* sess);
int parse_header(char* headerbuf, char* host, int* port, int* content_len);
int get_headers(int fd, TList* header_map);
int http_connect(int fd, const char* method, const char* path, const char* host, int port);
int tran_data(int from_fd, int to_fd, long int length);
#endif /* HTTP_H */

