#ifndef _COMMON_H_
#define _COMMON_H_

#include <syslog.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pwd.h>
#include <signal.h>
#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <pwd.h>
#include <shadow.h>
#include <crypt.h>
#include <dirent.h>
#include <time.h>
#include <sys/stat.h>
#include <linux/capability.h>
#include <sys/syscall.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <sys/mman.h>
#include <malloc.h>
#include <sys/select.h>
#include <sys/time.h>
#include <stdarg.h>

#define die(m) do{ perror(m); exit(EXIT_FAILURE);}while(0);

#define max(a,b) ((a) > (b) ? (a) : (b))
#define min(a,b) ((a) > (b) ? (b) : (a))
#define MAX_IDLE_TIME   (60 * 10)       /* 10 minutes of no activity */
#define MAXBUFFSIZE     ((size_t)(1024 * 96))   /* Max size of buffer */
#define MAX_HEADER_LEN 102400
#define MAX_BODY_LEN 102400
#define VERSION "Momoproxy 1.3.0"
#define MOMO_CONF "/etc/momo.conf"

#define IS_DEBUG TList_get_int(conftab, "debug") == 1


#endif
