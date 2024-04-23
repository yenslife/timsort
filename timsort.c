#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "list.h"
#include "sort_impl.h"

static inline size_t run_size(struct list_head *head)
{
    if (!head)
        return 0;
    if (!head->next)
        return 1;
    return (size_t) (head->next->prev);
}

/* 表示 run 的頭以及下一個 run 的頭 */
struct pair { 
    struct list_head *head, *next;
};

static inline size_t merge_compute_minrun(size_t n)
{
    size_t r = 0;
    while (n >= 64) {
        r |= n & 1;
        n >>= 1;
    }
    return n + r;
}

/* 
  return head of the next run
 */
static struct pair *binary_insertion_sort(void *priv,
                                               list_cmp_func_t cmp,
                                               struct list_head *head,
                                               struct list_head *next,
                                               size_t minrun,
                                               size_t *len /* return length of run */)
{    
    /* 將 next 插入到 head 之後 */
    minrun = 10;
    struct list_head *prev = head, *list = next, *list_next;
    while (prev->next) {
        prev = prev->next; /* prev 指向 head 這個 run 的尾部 */
    }
    // printf("prev: %d, list: %d\n", list_entry(prev, element_t, list)->val, list_entry(list, element_t, list)->val);
    if (list) 
        list_next = list->next;
    prev->next = list; /* 將 next 插入到 head 之後，這樣他就不是 NULL，需要重新找到 next */
    list = prev;
    list_next = list->next;
    // (*len) = 1;
    while (minrun - (*len)) {
        printf("minrun: %d, minrun - (*len) %d, i: %d\n", minrun, minrun - (*len));
        if (!list) {
            printf("list 為空\n");
            break;
        }
        if (!list_next) {
            printf("到底了, list: %d\n", list_entry(list, element_t, list)->val);
            break;
        }
        printf("list: %d, list_next: %d\n", list_entry(list, element_t, list)->val, list_entry(list_next, element_t, list)->val);
        if (cmp(priv, list, list_next) <= 0) {
            printf("pass通過 不需要插入\n");
            list = list_next;
            list_next = list_next->next;
        } else {
            /* 出現遞減的情況，將 next 往前插入到合適的位置 */
            printf("出現遞增，準備插入\n");
            struct list_head *prev = head, *cur;
            if (prev)
                cur = prev->next;
            // printf("cmp(priv, prev, list_next) %d, cmp(priv, list_next, cur) %d\n", cmp(priv, prev, list_next), cmp(priv, list_next, cur));
            /* 如果比第一個小 */
            if (cmp(priv, prev, list_next) > 0) {
                printf("比第一個小\n");
                list->next = list_next->next;
                list_next->next = head;
                head = list_next;
                list_next = list->next;
            } else {
                while (cur && list_next && prev && !(cmp(priv, prev, list_next) <= 0 && cmp(priv, list_next, cur) <= 0)) {
                    // printf("cmp(priv, prev, list_next) %d, cmp(priv, list_next, cur) %d\n", cmp(priv, prev, list_next), cmp(priv, list_next, cur));
                    // printf("prev: %d, cur: %d, list_next: %d\n", list_entry(prev, element_t, list)->val, list_entry(cur, element_t, list)->val, list_entry(list_next, element_t, list)->val);
                    prev = cur;
                    cur = cur->next; /* TODO: 解決這邊的無窮迴圈 */
                }
                // printf("找到 list_next:%d 的插入點 prev:%d, cur:%d\n", list_entry(list_next, element_t, list)->val, list_entry(prev, element_t, list)->val, list_entry(cur, element_t, list)->val);
                struct list_head *tmp = list_next->next;
                list->next = tmp;
                prev->next = list_next;
                list_next->next = cur;
                list_next = tmp;    
            }

            printf("插入完畢\n");
            /* 印出插入結果 */
            int i = (*len);
            printf("%d *len\n", *len);
            for (struct list_head *pos = head; pos != NULL && i >= 0; pos = pos->next) {
                if (pos->next == NULL)
                    printf("%d\n", list_entry(pos, element_t, list)->val);
                else
                    printf("%d ", list_entry(pos, element_t, list)->val);
                i--;
            }
        }
        (*len)++;
        printf("len: %d\n", *len);
    }
    /* 將最後的節點指向 NULL */
    next = list_next;
    printf("head: %d\n", list_entry(head, element_t, list)->val);
    if (next)
        printf("next: %d\n", list_entry(next, element_t, list)->val);
    prev->next = NULL;

    struct pair *result = malloc(sizeof(struct pair));
    result->head = head;
    result->next = next;
    // free(result);

    return result;
}



static size_t stk_size;

static struct list_head *merge(void *priv,
                               list_cmp_func_t cmp,
                               struct list_head *a,
                               struct list_head *b)
{
    struct list_head *head;
    struct list_head **tail = &head;

    while (a && b) {
        /* if equal, take 'a' -- important for sort stability */
        if (cmp(priv, a, b) <= 0) {
            *tail = a;
            tail = &a->next;
            a = a->next;
        } else {
            *tail = b;
            tail = &b->next;
            b = b->next;
        }
    }

    /* Append the remaining elements */
    *tail = a ? a : b;

    return head;
}

static void build_prev_link(struct list_head *head,
                            struct list_head *tail,
                            struct list_head *list)
{
    tail->next = list;
    do {
        list->prev = tail;
        tail = list;
        list = list->next;
    } while (list);

    /* The final links to make a circular doubly-linked list */
    tail->next = head;
    head->prev = tail;
}

static void merge_final(void *priv,
                        list_cmp_func_t cmp,
                        struct list_head *head,
                        struct list_head *a,
                        struct list_head *b)
{
    struct list_head *tail = head;

    for (;;) {
        /* if equal, take 'a' -- important for sort stability */
        if (cmp(priv, a, b) <= 0) {
            tail->next = a;
            a->prev = tail;
            tail = a;
            a = a->next;
            if (!a)
                break;
        } else {
            tail->next = b;
            b->prev = tail;
            tail = b;
            b = b->next;
            if (!b) {
                b = a;
                break;
            }
        }
    }
    printf("end merge in final merge\n");

    /* Finish linking remainder of list b on to tail */
    build_prev_link(head, tail, b);
}

/* 
   find run 會回傳一個 run 頭以及下一個 run 的頭
   並且將 run 開頭的下一個元素的 prev 指標設為 run 的長度
   這樣在 merge_at() 中就可以直接取得 run 的長度
 */
static struct pair find_run(void *priv,
                            struct list_head *list,
                            list_cmp_func_t cmp,
                            size_t minrun)
{
    size_t len = 1;
    size_t *len_ptr = &len;
    struct list_head *next = list->next, *head = list;
    struct pair result;

    if (!next) {
        result.head = head, result.next = next;
        return result;
    }

    if (cmp(priv, list, next) > 0) {
        /* 如果是遞減的話，就將 run 反轉 */
        struct list_head *prev = NULL;
        do {
            len++;
            list->next = prev;
            prev = list;
            list = next;
            next = list->next;
            head = list;
        } while (next && cmp(priv, list, next) > 0);
        list->next = prev;
    } else {
        do {
            len++;
            list = next;
            next = list->next;
        } while (next && cmp(priv, list, next) <= 0);
        list->next = NULL; /* 將 run 的尾部指向 NULL */
    }
    /* 把 run 的內容印出來 */
    for (struct list_head *pos = head; pos != NULL; pos = pos->next) {
        if (pos->next == NULL)
            printf("%d\n", list_entry(pos, element_t, list)->val);
        else
            printf("%d ", list_entry(pos, element_t, list)->val);
    }
    struct pair *result_tmp;
    if (len < minrun) {
        /* 如果 run 的長度小於 minrun，就將 run 進行排序 */
        result_tmp = binary_insertion_sort(priv, cmp, head, next, minrun, len_ptr);
    }
    result_tmp->head->prev = NULL;
    result_tmp->next->prev = (struct list_head *) len; /* prev 的指標被用來存儲長度 */
    result.head = &result_tmp->head, result.next = &result_tmp->next; /* next is the start of next run */
    // head->prev = NULL;
    // head->next->prev = (struct list_head *) len; /* prev 的指標被用來存儲長度 */
    // result.head = head, result.next = next; /* next is the start of next run */
    return result;
}

static struct list_head *merge_at(void *priv,
                                  list_cmp_func_t cmp,
                                  struct list_head *at)
{
    size_t len = run_size(at) + run_size(at->prev);
    struct list_head *prev = at->prev->prev;
    struct list_head *list = merge(priv, cmp, at->prev, at);
    list->prev = prev;
    list->next->prev = (struct list_head *) len;
    --stk_size;
    return list;
}

static struct list_head *merge_force_collapse(void *priv,
                                              list_cmp_func_t cmp,
                                              struct list_head *tp)
{
    while (stk_size >= 3) {
        if (run_size(tp->prev->prev) < run_size(tp)) {
            tp->prev = merge_at(priv, cmp, tp->prev);
        } else {
            tp = merge_at(priv, cmp, tp);
        }
    }
    return tp;
}

/* 這個函數會將 tp 之後的 run 合併 
   先判斷 tp 之後的 run 是否比 tp 長，如果是的話就合併
   否則就合併 tp 之後的 run
*/
static struct list_head *merge_collapse(void *priv,
                                        list_cmp_func_t cmp,
                                        struct list_head *tp)
{
    int n;
    while ((n = stk_size) >= 2) {
        if ((n >= 3 &&
             run_size(tp->prev->prev) <= run_size(tp->prev) + run_size(tp)) ||
            (n >= 4 && run_size(tp->prev->prev->prev) <=
                           run_size(tp->prev->prev) + run_size(tp->prev))) {
            if (run_size(tp->prev->prev) < run_size(tp)) {
                tp->prev = merge_at(priv, cmp, tp->prev);
            } else {
                tp = merge_at(priv, cmp, tp);
            }
        } else if (run_size(tp->prev) <= run_size(tp)) {
            tp = merge_at(priv, cmp, tp);
        } else {
            break;
        }
    }

    return tp;
}

void timsort(void *priv, struct list_head *head, list_cmp_func_t cmp)
{
    stk_size = 0;

    struct list_head *list = head->next, *tp = NULL; /* tp 是堆疊的頂端，紀錄每個 run 的頭 */
    if (head == head->prev)
        return;

    /* Find the length of the list */
    size_t len = 0;
    struct list_head *pos;
    list_for_each (pos, head)
        len++;

    /* Compute the minimum run length */
    size_t minrun = merge_compute_minrun(len);
    printf("minrun: %lu\n", minrun);

    /* Convert to a null-terminated singly-linked list. */
    head->prev->next = NULL;

    do {
        /* Find next run */
        printf("找下一個 run\n");
        struct pair result = find_run(priv, list, cmp, minrun);
        printf("結束 find\n");
        /* print current run content */
        struct list_head *pos = &result.head;
        for (pos = pos->next; pos; pos = pos->next) {
            printf("%d ", list_entry(pos, element_t, list)->val);
        }
        result.head->prev = tp; /* 堆疊的下一個元素 */
        tp = result.head;
        list = result.next;
        stk_size++;
        tp = merge_collapse(priv, cmp, tp);
        printf("結束當前 RUN\n");
    } while (list);
    printf("加完 run 之後的 stk_size: %d\n", stk_size);

    /* End of input; merge together all the runs. */
    tp = merge_force_collapse(priv, cmp, tp);

    /* The final merge; rebuild prev links */
    struct list_head *stk0 = tp, *stk1 = stk0->prev;
    while (stk1 && stk1->prev)
        stk0 = stk0->prev, stk1 = stk1->prev;
    if (stk_size <= 1) {
        build_prev_link(head, head, stk0); // 需要重新建立 prev 指標，因為 merge_at() 會修改 prev 指標
        return;
    }
    printf("final_merge\n");
    merge_final(priv, cmp, head, stk1, stk0);
}
