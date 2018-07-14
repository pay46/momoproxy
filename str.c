#include "common.h"
#include "str.h"

/**
 * 字符串键值对分割函数
 * @param str 要分割的字符串
 * @param left key的赋值变量首地址
 * @param right value的赋值变量首地址
 * @param c 分割字符
 */
void str_split(const char *str, char *left, char *right, char c) {
    char *p = strchr(str, c);

    if (p == NULL) {
        strcpy(left, str);
    } else {
        strncpy(left, str, p - str);
        strcpy(right, p + 1);
    }
}

// 将strRes中的t替换为s，替换成功返回1，否则返回0。

int str_replace(char strRes[], char from[], char to[]) {
    int i, flag = 0;
    char *p, *q, *ts;
    for (i = 0; strRes[i]; ++i) {
        if (strRes[i] == from[0]) {
            p = strRes + i;
            q = from;
            while (*q && (*p++ == *q++));
            if (*q == '\0') {
                ts = (char *) malloc(strlen(strRes) + 1);
                strcpy(ts, p);
                strRes[i] = '\0';
                strcat(strRes, to);
                strcat(strRes, ts);
                free(ts);
                flag = 1;
            }
        }
    }
    return flag;
}

/**
 * explode -  separate string by separator
 *
 * @param string from - need separator 
 * @param char delim - separator
 * @param pointarray to - save return separate result
 * @param int item_num - return sub string total
 * 
 * @include stdlib.h 
 * @include string.h
 *
 * @example
 * char *s, **r;
 * int num;
 * explode(s, '\n', &r, &num);
 * for(i=0; i<num; i++){
 *     printf("%s\n", r[i]);
 * }
 * 
 * for(i=0; i<num; i++){
 *     free(r[i]);
 * }
 * free(r);
 */
void explode(const char *from, char delim, char ***to, int *item_num) {
    int i, j, k, n, temp_len;
    int max_len = strlen(from) + 1;
    char buf[max_len], **ret;
    for (i = 0, n = 1; from[i] != '\0'; i++) {
        if (from[i] == delim) n++;
    }
    ret = (char **) malloc(n * sizeof (char *));
    for (i = 0, k = 0; k < n; k++) {
        memset(buf, 0, max_len);
        for (j = 0; from[i] != '\0' && from[i] != delim; i++, j++) buf[j] = from[i];
        i++;
        temp_len = strlen(buf) + 1;
        ret[k] = malloc(temp_len);
        memcpy(ret[k], buf, temp_len);
    }
    *to = ret;
    *item_num = n;
}

void trim(char *str) {
    char* p = str;
    while (*p == '\r' || *p == '\n' || *p == ' ') {
        p++;
    }
    strcpy(str, p);
    
    p = str + strlen(str) - 1;
    while (*p == '\r' || *p == '\n' || *p == ' ')
        *p-- = '\0';
}