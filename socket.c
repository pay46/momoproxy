#include "socket.h"
#include "common.h"

/**
 *读取固定字节数
 *@fd 文件描述符
 *@buf 接收缓冲区
 *@count 要读到的字节数
 *@return 成功：返回count 失败：返回-1 读取到EOF 返回 < count
 */
ssize_t readn(int fd, void *buf, size_t count) {
    char* bufp = (char*) buf;
    size_t nread;
    size_t nleft = count;
    while (nleft > 0) {
        if ((nread = read(fd, bufp, nleft)) < 0) {
            //出错的情况有两种，一种是read错误，另一种是被信号打断
            //如果被信号打断继续读取
            if (errno == EINTR) {
                continue;
            }
            return -1;
        } else if (nread == 0) {
            return count - nleft;
        }
        bufp += nread;
        nleft -= nread;
    }
    return count;
}

/**
 *发送固定字节数
 *@fd 文件描述符
 *@buf 发送缓冲区
 *@count 要发送的字节数
 *@return 成功：返回count 失败：返回-1
 */
ssize_t writen(int fd, const void *buf, size_t count) {
    char* bufp = (char*) buf;
    size_t nwrite;
    size_t nleft = count;
    while (nleft > 0) {
        if ((nwrite = write(fd, bufp, nleft)) < 0) {
            //如果被信号打断继续读取
            if (errno == EINTR) {
                continue;
            }
            return -1;
        } else if (nwrite == 0) {
            return count - nleft;
        }
        bufp += nwrite;
        nleft -= nwrite;
    }
    return count;
}

/**
 *偷窥套接字缓冲区里头的数据，不移除
 *@socket 套接字
 *@buf 接收缓存区
 *@len 偷窥长度
 *@return 成功 返回 >= 0, 失败返回：0
 */
ssize_t recv_peek(int sockfd, void* buf, size_t len) {
    while (1) {
        //MSG_PEEK接收但是不除套接字缓存区的数据
        int ret = recv(sockfd, buf, len, MSG_PEEK);
        //处理被信号中断的情况
        if (ret == -1 && errno == EINTR) {
            continue;
        }
        return ret;
    }
}

/**
 *按行读取数据
 *@sockfd 套接字
 *@buf 接收缓冲区
 *@maxlen 每行的最大长度
 *@return 成功返回 >= 0 失败返回 -1
 */
ssize_t readline(int sockfd, void *buf, size_t maxLen) {
    int nread;
    int nleft = maxLen;
    int ret;
    char buffer[1024];
    char* bufp;

    bufp = buffer;
    while (1) {
        nread = recv_peek(sockfd, bufp, nleft);
        //没有数据时候返回
        if (nread <= 0) {
            return nread;
        }

        int i;
        for (i = 0; i < nread; i++) {
            //如果读到了换行符合，则全部数据读出来并返回
            if (bufp[i] == '\n') {
                ret = readn(sockfd, buf, i + 1);
                if (ret != i + 1) {
                    return -1;
                }
                return nread;
            }
        }
        //如果没有读到换行，也把数据读出来存在buf里头
        ret = readn(sockfd, bufp, nread);
        if (ret != nread) {
            return -1;
        }
        nleft -= nread;
        bufp += nread;
    }
    return -1;
}

/**
 * 本函数使用方法
 *int ret；
 *ret = read_timeout(fd, 5);
 *if(ret == 0){
 *	read(fd......);没有超时正常读取
 *} else if (ret == -1) {
 *	if(errno == ETIMEDOUT) {
 *		timeout.....超时处理
 *	} else {
 *		die("read_timeout");出错了
 *	}
 *}
 *
 *检查socket套接字固定时间内是否可读
 *sockfd 套接字
 *wait_second 要等待的秒数
 *未超时 返回 0 超时 失败 返回 -1 超时 返回 -1 且 errno = ETIMEDOUT 
 */
int read_timeout(int sockfd, unsigned int wait_second) {
    int ret = 0;
    if (wait_second > 0) {
        fd_set read_fdset;
        struct timeval timeout;

        FD_ZERO(&read_fdset);
        FD_SET(sockfd, &read_fdset);

        timeout.tv_sec = wait_second;
        timeout.tv_usec = 0;

        do {
            ret = select(sockfd + 1, &read_fdset, NULL, NULL, &timeout);
        } while (ret < -1 && errno == EINTR);

        if (ret == 0) {
            ret = -1;
            errno = ETIMEDOUT;
        }

        if (ret == 1) {
            ret = 0;
        }
    }
    return ret;
}

/**
 * 读取套接口数据 带超时
 * sockfd 套接口
 * wait_second 要等待的秒数
 * 成功返回 1 ， 失败返回 -1，  超时返回 -2， 对方关闭连接返回 0
 */
int readline_timeout(int sockfd, char* recvbuf, int len, int timeout) {
    int ret;
    ret = read_timeout(sockfd, timeout);

    if (ret == 0) {
        ret = readline(sockfd, recvbuf, len);
    } else if (ret == -1) {
        if (errno == ETIMEDOUT) {
            ret = -2; //超时
        } else {
            ret = -1; //read_timeout 出错
        }
    }

    if (ret < 0) {
        ret = -1; //readline出错
    }
    if (ret == 0) {
        ret = 0; //对方关闭了连接
    }
    return ret;
}

/**
 * 检测套接口 固定时间内是否可写
 * sockfd 套接口
 * wait_second 要等待的秒数
 * 成功返回 0 ， 失败返回 -1，超时 返回 -1 且 errno = ETIMEDOUT
 */
int write_timeout(int sockfd, unsigned int wait_second) {
    int ret = 0;
    if (wait_second > 0) {
        fd_set write_fdset;
        struct timeval timeout;

        FD_ZERO(&write_fdset);
        FD_SET(sockfd, &write_fdset);

        timeout.tv_sec = wait_second;
        timeout.tv_usec = 0;

        do {
            ret = select(sockfd + 1, NULL, &write_fdset, NULL, &timeout);
        } while (ret == -1 && errno == ETIMEDOUT);

        if (ret == 0) {
            ret = -1;
            errno = ETIMEDOUT;
        }

        if (ret == 1) {
            ret = 0;
        }
    }
    return ret;
}

/**
 * 往套接口写数据 带超时
 * sockfd 套接口
 * buf 要发送的缓存区
 * count 要发送的字节数
 * wait_second 要等待的秒数
 * 成功返回 1 ， 失败返回 -1，  超时返回 -2
 */
int writen_timeout(int sockfd, const char* buf, size_t count, int timeout) {
    int ret;
    ret = write_timeout(sockfd, timeout);

    if (ret == 0) {
        ret = writen(sockfd, buf, count);
    } else if (ret == -1) {
        if (errno == ETIMEDOUT) {
            ret = -2; //超时
        } else {
            ret = -1; //read_timeout 出错
        }
    }

    if (ret < 0) {
        ret = -1; //writen出错
    }

    return ret;
}

/**
 * 带超时的accept
 * sockfd 监听套接字
 * addr 接受对方地址信息的解构
 * wait_second 等待的秒数
 * 成功返回 连接套接字 ，超时 返回 -1 且 errno = ETIMEDOUT
 */
int accept_timeout(int sockfd, struct sockaddr_in* addr, unsigned int wait_second) {
    int ret;
    socklen_t addrlen = sizeof (struct sockaddr_in);

    if (wait_second > 0) {
        fd_set accept_fdset;
        struct timeval timeout;

        FD_ZERO(&accept_fdset);
        FD_SET(sockfd, &accept_fdset);

        timeout.tv_sec = wait_second;
        timeout.tv_usec = 0;

        do {
            ret = select(sockfd + 1, &accept_fdset, NULL, NULL, &timeout);
        } while (ret == -1 && errno == EINTR);

        if (ret == 0) {
            ret = -1;
            errno = ETIMEDOUT;
        }
        if (ret == -1) {
            return -1;
        }
    }

    if (addr == NULL) {
        ret = accept(sockfd, NULL, NULL);
    } else {
        ret = accept(sockfd, (struct sockaddr*) addr, &addrlen);
    }
    if (ret == -1) {
        die("accept2");
    }

    return ret;
}

/**
 * 把传入套接字设置成 非阻塞模式
 * sockfd 套接字
 */
void setfd_nonblock(int sockfd) {
    int ret;
    int flags;
    flags = fcntl(sockfd, F_GETFL);

    if (flags == -1) {
        die("fcntl");
    }

    flags |= O_NONBLOCK;

    ret = fcntl(sockfd, F_SETFL, flags);
    if (ret == -1) {
        die("fcntl");
    }
}

/**
 * 包套接字设置回成阻塞模式
 * sockfd 套接字
 */
void setfd_block(int sockfd) {
    int ret;
    int flags;

    flags = fcntl(sockfd, F_GETFL);

    if (flags == -1) {
        die("fcntl");
    }

    flags &= ~O_NONBLOCK;

    ret = fcntl(sockfd, F_SETFL, flags);
    if (ret == -1) {
        die("fcntl");
    }
}

int connect_timeout(int sockfd, struct sockaddr_in* addr, unsigned int wait_second) {
    int ret;
    socklen_t addrlen = sizeof (struct sockaddr_in);

    if (wait_second > 0) {
        setfd_nonblock(sockfd);
    }

    ret = connect(sockfd, (struct sockaddr*) addr, addrlen);
    if (ret < 0 && errno == EINPROGRESS) {
        fd_set write_fdset;
        struct timeval timeout;

        FD_ZERO(&write_fdset);
        FD_SET(sockfd, &write_fdset);
        timeout.tv_sec = wait_second;
        timeout.tv_usec = 0;

        do {
            ret = select(sockfd + 1, NULL, &write_fdset, NULL, &timeout);
        } while (ret < 0 && errno == EINTR);

        if (ret == -1) {
            return -1;
        }

        if (ret == 0) {
            ret = -1;
            errno = ETIMEDOUT;
        }

        //返回1的情况有2种 ： 连接成功  or  套接字错误
        //这个时候错误信息是不会保存在errno中的
        //所以要用getsockopt来获取
        if (ret == 1) {
            int err;
            socklen_t socklen = sizeof (err);
            int sockoptret = getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &err, &socklen);

            if (sockoptret == -1) {
                return -1;
            }
            if (sockoptret == 0) {
                ret = 0;
            } else {
                errno = err;
                ret = -1;
            }
        }
    }

    if (wait_second > 0) {
        setfd_block(sockfd);
    }
    return ret;
}

/**
 * 启动一个tcp服务器
 * @host 服务器的ip地址，服务器的主机名
 * @prot 服务端口
 * @return 监听套接字 
 */
int tcp_server(const char *host, unsigned short port) {
    int listenfd;

    //创建一个地址家族为PF_INET，的流式套接字
    if ((listenfd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        die("socket");
    }

    //创建一个服务器地址
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof (servaddr));
    servaddr.sin_family = AF_INET;
    if (host != NULL) {
        //inet_aton 是把点分十进制ip地址转换成网络ip地址，成功返回非0，失败返回0
        if (inet_aton(host, &servaddr.sin_addr) == 0) {//如果这里等于0就证明了不是地址
            struct hostent *hp;
            hp = gethostbyname(host);
            if (hp == NULL)
                die("gethostbyname");
            servaddr.sin_addr = *((struct in_addr*) hp->h_addr);
        }
    } else {
        servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    }
    //printf("%d\n", port);
    servaddr.sin_port = htons(port);

    //set address reuse
    int on = 1;
    if ((setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const char*) &on, sizeof (on))) < 0)
        die("setsockopt");

    if ((bind(listenfd, (struct sockaddr*) & servaddr, sizeof (servaddr))) < 0)
        die("bind");

    if (listen(listenfd, SOMAXCONN) < 0)
        die("listen");
    return listenfd;
}

/**
 * 创建一个tcp客户端连接
 * @char* ip
 * @int port
 * @Returns 正常情况返回连接fd 连接失败返回 -1
 */
int open_clientfd(char *ip, int port) {
    int sockfd;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        die("socket");
    }

    struct sockaddr_in to;
    to.sin_family = AF_INET;
    to.sin_addr.s_addr = inet_addr(ip);
    to.sin_port = htons(port);

    struct timeval timeo = {0, 0};
    socklen_t len = sizeof (timeo);
    timeo.tv_sec = 3;
    setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeo, len);

    //printf("connecting\n");
    if (connect(sockfd, (struct sockaddr *) &to, sizeof (struct sockaddr)) == 0) {
        //printf("connected\n");
        return sockfd;
    }
    close(sockfd);
    return -1;
}

int prwrite(int fd, const char *fmt, ...) {
    int n;
    int size = 1024 * 8;
    char* buf;
    char* tmpbuf;
    va_list ap;

    buf = (char*) malloc(size);
    if (buf == NULL)
        return -1;

    while (1) {
        va_start(ap, fmt);
        n = vsnprintf(buf, size, fmt, ap);
        va_end(ap);
        if (n > -1 && n < size) {
            break; //如果返回的不是负数，而且返回的n 没有超过 size，则跳出while，去发数据
        }

        //否则加大buf，再试一试
        if (n > -1) {
            size = n + 1;
        } else {
            size *= 2;
        }

        tmpbuf = (char*) realloc(buf, size);
        if (tmpbuf == NULL) {
            free(buf);
            return -1;
        } else {
            buf = tmpbuf;
        }
    }

    if (writen(fd, buf, n) < n) {
        free(buf);
        return -1;
    }
    
    //printf("writed: %s\n",buf);

    free(buf);
    return 0;
}
