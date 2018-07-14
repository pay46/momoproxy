#include "LinkList.h"
#include "common.h"

/**
 * 创建链表
 * @return TList 
 */
TList* TList_create() {
    TList* ret = NULL;
    ret = (TList*) malloc(sizeof (TList));
    if (ret != NULL) {
        ret->length = 0;
        ret->next = NULL;
    }
    return ret;
}

/**
 * 获取链表长度
 * @param list
 * @return 
 */
int TList_getlen(TList* list) {
    int ret = -1;
    if (list != NULL) {
        ret = list->length;
    }
    return ret;
}

/**
 * 往链表插入节点
 * @param list 链表
 * @param item 要插入的节点
 * @param pos 插入的位置
 * @return 
 */
int TList_insert(TList* list, TItem* item, int pos) {
    int i;
    int ret = -1;
    ret = (list != NULL && item != NULL && pos >= 0);
    if (ret) {
        if (pos > TList_getlen(list)) {
            pos = TList_getlen(list);
        }
        TItem* p = NULL;
        p = (TItem*) list;
        for (i = 0; i < pos; i++) {
            p = p->next;
        }
        item->next = p->next;
        p->next = item;
        list->length++;
    }
    return ret;
}

/**
 * 链表中获取特定位置节点
 * @param list
 * @param pos
 * @return 
 */
TItem* TList_get(TList* list, int pos) {
    int i;
    TItem* ret = NULL;
    if (list != NULL && pos >= 0) {
        if (pos > TList_getlen(list)) {
            pos = TList_getlen(list);
        }
        TItem* p = NULL;
        p = (TItem*) list;
        for (i = 0; i < pos; i++) {
            p = p->next;
        }
        ret = p->next;
    }
    return ret;
}

/**
 * 删除链表特定位置节点
 * @param list
 * @param pos
 * @return 
 */
TItem* TList_delete(TList* list, int pos) {
    int i;
    TItem* ret = NULL;
    if (list != NULL && pos >= 0) {
        if (pos > TList_getlen(list)) {
            pos = TList_getlen(list);
        }
        TItem* p = NULL;
        p = (TItem*) list;
        for (i = 0; i < pos; i++) {
            p = p->next;
        }
        ret = p->next;
        p->next = ret->next;
    }
    return ret;
}

/**
 * 清空一个链表
 * @param list
 */
void TList_clean(TList* list) {
    list->length = 0;

    TItem* p = NULL;
    while (list->next != NULL) {
        p = list->next;
        list->next = p->next;
        free(p);
    }
}

/**
 * 销毁一个链表
 * @param list
 */
void TList_destory(TList* list) {
    TList_clean(list);
    free(list);
}

/**
 * 查看链表中是否存在key值的节点
 * @param list
 * @param key
 * @return 存在返回 位置 ； 不存在返回 -1
 */
int TList_is_set(TList* list, const char* key) {
    int i, len;
    int ret = -1;
    if (list != NULL && key != NULL) {
        len = TList_getlen(list);
        TItem* p = NULL;
        p = (TItem*) list;
        for (i = 0; i < len; i++) {
            p = p->next;
            if (strcmp(p->name, key) == 0) {
                ret = i;
            }
        }
    }
    return ret;
}

/**
 * 往链表中插入一个字符串节点
 * @param list
 * @param key 键值
 * @param val 
 * @return 
 */
int TList_set(TList* list, const char* key, const char* val) {
    int ret = -1;
    TItem* item = NULL;
    //判断key是否存在
    ret = TList_is_set(list, key);
    if (ret >= 0) {
        item = TList_get(list, ret);
        strcpy(item->name, key);
        strcpy(item->string_v, val);
        item->int_v = atoi(val);
    } else {
        item = (TItem*) malloc(sizeof (TItem));
        if (item == NULL) {
            return -1;
        }
        strcpy(item->name, key);
        strcpy(item->string_v, val);
        item->int_v = atoi(val);
        ret = TList_insert(list, item, 0);
    }
    return ret;
}

/**
 * 获取链表中一个数值节点值
 * @param list
 * @param key 键值
 * @param val 
 * @return  
 */
int TList_get_int(TList* list, const char* key) {
    int ret = -1;
    TItem* item = NULL;

    //判断key是否存在
    ret = TList_is_set(list, key);

    if (ret >= 0) {
        item = TList_get(list, ret);
        ret = item->int_v;
    }
    return ret;
}

/**
 * 获取链表中一个字符串节点值
 * @param list
 * @param key 键值
 * @param val 
 * @return  
 */
char* TList_get_str(TList* list, const char* key) {
    char* ret = NULL;
    TItem* item = NULL;

    //判断key是否存在
    int pos = TList_is_set(list, key);
    if (pos >= 0) {
        item = TList_get(list, pos);
        ret = item->string_v;
    }
    return ret;
}

void TList_print_all(TList* list) {
    int i;
    int len = TList_getlen(list);
    TItem* item = NULL;
    for (i = 0; i < len; i++) {
        item = TList_get(list, i);
        printf("array[%s]=%s\n", item->name, item->string_v);
    }
}