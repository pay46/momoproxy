#include "common.h"
#include "buffer.h"

struct bufline_s {
    char *string;
    struct bufline_s *next;
    size_t length;
    size_t pos;
};

struct buffer_s {
    struct bufline_s *head;
    struct bufline_s *tail;
    size_t size;
};

/**
 * 创建能添加到buffer的line结构
 * @param data
 * @param length
 * @return struct bufline_s*
 */
static struct bufline_s* make_new_line(char* data, size_t length) {
    if (data == NULL || length <= 0)
        return NULL;
    struct bufline_s *line;

    line = (struct bufline_s*) malloc(sizeof (struct bufline_s));
    if (!line)
        return NULL;

    line->string = (char*) malloc(length);
    if (!line->string) {
        free(line);
        return NULL;
    }

    memcpy(line->string, data, length);
    line->next = NULL;
    line->length = length;
    line->pos = 0;

    return line;
}

/**
 * 释放line结构
 * @param line
 */
static void free_line(struct bufline_s* line) {
    if (!line)
        return;
    free(line->string);
    free(line);
}

/**
 * 
 * @return 
 */
struct buffer_s* create_buffer(void) {
    struct buffer_s* bufptr;
    bufptr = (struct buffer_s*) malloc(sizeof (struct buffer_s));
    if (!bufptr)
        return NULL;
    bufptr->head = bufptr->tail = NULL;
    bufptr->size = 0;
    return bufptr;
}

void destory_buffer(struct buffer_s* buff) {
    if (!buff)
        return;
    struct bufline_s* next;

    while (buff->head) {
        next = buff->head->next;
        free_line(buff->head);
        buff->head = next;
    }

    free(buff);
}

size_t buffer_size(struct buffer_s* buff) {
    return buff->size;
}

int buffer_append(struct buffer_s* buff, char* data, size_t length) {

    if (buff == NULL || data == NULL || length <= 0)
        return -1;

    struct bufline_s* line = make_new_line(data, length);
    if (!line)
        return -1;

    if (buff->size == 0) {
        buff->head = buff->tail = line;
    } else {
        buff->tail->next = line;
        buff->tail = line;
    }

    buff->size += length;

    return 0;
}

struct bufline_s* buffer_shift(struct buffer_s* buff) {

    if (buff == NULL)
        return NULL;

    struct bufline_s* line;
    line = buff->head;
    buff->head = line->next;
    buff->size -= line->length;

    return line;
}

#define MAX_BUFFER_SIZE 1024

ssize_t read_buffer(int fd, struct buffer_s* buff) {

    if (fd <= 0 || buff == NULL)
        return -1;

    ssize_t bytes;
    char* buffer;

    if (buff->size >= MAX_BUFFER_SIZE)
        return 0;

    buffer = (char*) malloc(MAX_BUFFER_SIZE);
    if (!buffer)
        return -1;

    bytes = read(fd, buffer, MAX_BUFFER_SIZE);

    if (bytes > 0) {
        if (buffer_append(buff, buffer, bytes) < 0) {
            bytes = -1;
        }
    } else if (bytes == 0) {//对等方关闭链接
        bytes = -1;
    } else {
        switch (errno) {
            case EINTR:
                bytes = 0;
                break;
            default:
                bytes = -1;
                break;
        }
    }

    free(buffer);
    return bytes;
}

ssize_t write_buffer(int fd, struct buffer_s* buff) {
    if (fd <= 0 || buff == NULL)
        return -1;

    if (buff->size == 0)
        return 0;

    ssize_t bytes;
    struct bufline_s* line;

    line = buff->head;
    bytes = send(fd, line->string + line->pos, line->length - line->pos, MSG_NOSIGNAL);

    if (bytes >= 0) {
        line->pos += bytes;
        if (line->pos == line->length) {
            free(buffer_shift(buff));
        }

        return bytes;
    } else {
        switch (errno) {
#ifdef EWOULDBLOCK
            case EWOULDBLOCK:
#else
#ifdef EAGAIN
            case EAGAIN:
#endif
#endif
            case EINTR:
                return 0;
            case ENOBUFS:
            case ENOMEM:
                return 0;
            default:
                return -1;
        }
    }
}