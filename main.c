/*
 * Momoproxy 是一个自由软件
 * 作者： 默缤
 * QQ：380255922@qqcom
 * Web：www.blobt.com
 */
#include "common.h"
#include "str.h"
#include "LinkList.h"
#include "socket.h"
#include "session.h"
#include "http.h"
#include "buffer.h"

TList* conftab;
session_t sess;

static void init();
static void pharse_args(int argc, char** argv);
static void load_conf(const char* file);

int main(int argc, char** argv) {

    /*初始化*/
    init();

    /*加载配置*/
    load_conf(MOMO_CONF);

    /*解析参数*/
    pharse_args(argc, argv);

    int port = TList_get_int(conftab, "listen_port");
    int listenfd = tcp_server(NULL, port); //创建一个tcp套接字

    int conn;
    struct sockaddr_in peer_addr; //peer_addr 是用来存放接受到socket的信息的
    pid_t pid;
    while (1) {
        //阻塞等待
        conn = accept_timeout(listenfd, (struct sockaddr_in*) &peer_addr, 0);
        if (conn == -1) {
            die("accept_timeout");
        }
        if (IS_DEBUG) {
            printf("ip=%s  port=%d conn=%d\n", inet_ntoa(peer_addr.sin_addr), ntohs(peer_addr.sin_port), conn);
        }


        pid = fork();

        if (pid == 0) {
            close(listenfd);
            sess.ctrl_fd = conn;
            strcpy(sess.source_ip, inet_ntoa(peer_addr.sin_addr));
            sess.cbuffer = create_buffer();
            sess.sbuffer = create_buffer();
            sess.content_len = 0;

            
            //开始处理请求
            begin_session2(&sess);
            close(conn);
            exit(EXIT_SUCCESS);
        } else {
            close(conn);
        }
    }
    return 0;
}

static void init() {
    //创建配置表,并赋初值
    conftab = TList_create();
    TList_set(conftab, "debug", "1"); //默认监听端口
    TList_set(conftab, "listen_port", "88"); //默认监听端口
    TList_set(conftab, "allow_address", "*"); //允许地址
    TList_set(conftab, "access_timeout", "30"); //接受连接最大时长
    TList_set(conftab, "connect_timeout", "30"); //建立连接最大时长

    //session初始化
    sess.ctrl_fd = -1;
    sess.serv_fd = -1;
    strcpy(sess.header, "");
    strcpy(sess.body, "");
    strcpy(sess.source_ip, "");

    //忽略SIGCHLD信号，防止僵死进程
    signal(SIGCHLD, SIG_IGN);
}

/**
 * 解析参数程序运行时指定的参数
 * @param argc
 * @param argv
 */
static void pharse_args(int argc, char** argv) {
    int i;
    for (i = 0; i < argc; i++) {
        if (argv[i][0] == '-') {
            if (strcmp(argv[i], "-V") == 0) {
                printf("%s\n", VERSION);
            }
        }
    }
}

/**
 * 读取配置文件
 * @param file 配置文件的路径全名
 */
static void load_conf(const char* file) {
    FILE *fp = fopen(file, "r");
    if (fp == NULL) {
        die("Open config file fail!\n");
    }

    char key[256] = {0};
    char value[256] = {0};
    char line[1024] = {0};
    while (fgets(line, sizeof (line), fp) != NULL) {
        if (strlen(line) < 2 || line[0] == '#') {
            continue;
        }
        line[strlen(line) - 1] = '\0'; //去除末尾\n
        str_split(line, key, value, '=');
        TList_set(conftab, key, value);
        memset(line, '\0', sizeof (line));
        memset(key, '\0', sizeof (key));
        memset(value, '\0', sizeof (value));
    }
}