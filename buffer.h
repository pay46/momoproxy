#ifndef BUFFER_H
#define BUFFER_H
struct buffer_s;

struct buffer_s* create_buffer(void);
void destory_buffer(struct buffer_s* buff);
size_t buffer_size(struct buffer_s* buff);
int buffer_append(struct buffer_s* buff, char* data, size_t length);
struct bufline_s* buffer_shift(struct buffer_s* buff);
ssize_t read_buffer(int fd, struct buffer_s* buff);
ssize_t write_buffer(int fd, struct buffer_s* buff);

#endif /* BUFFER_H */

