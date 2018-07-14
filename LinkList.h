#ifndef LINKLIST_H
#define LINKLIST_H

typedef struct _tag_item TItem;

typedef struct _tag_list {
    TItem* next;
    int length;
} TList;

struct _tag_item {
    char name[256];
    char string_v[256];
    int int_v;
    TItem* next;
};

TList* TList_create();
int TList_getlen(TList* list);
int TList_insert(TList* list, TItem* item, int pos);
TItem* TList_get(TList* list, int pos);
TItem* TList_delete(TList* list, int pos);
void TList_destory(TList* list);
void TList_clean(TList* list);

int TList_is_set(TList* list, const char* key);
int TList_set(TList* list, const char* key, const char* val);
int TList_get_int(TList* list, const char* key);
char* TList_get_str(TList* list, const char* key);
void TList_print_all(TList* list);
#endif /* LINKLIST_H */

