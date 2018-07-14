#ifndef STR_H
#define STR_H
void str_split(const char *str, char *left, char *right, char c);
int str_replace(char strRes[], char from[], char to[]);
void explode(const char *from, char delim, char ***to, int *item_num);
void trim(char *str);
#endif /* STR_H */

