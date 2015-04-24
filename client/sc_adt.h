#ifndef __SCX_ADT_LIST_H__
#define __SCX_ADT_LIST_H__

struct adt_list {
	struct adt_list* next;
	struct adt_list* prev;
};

#define adt_iterate_list(__iterator, __list)    \
    for (__iterator = (__list)->next;   \
         __iterator != __list;      \
         __iterator = (__iterator)->next)

#define adt_reverse_iterate_list(__iterator, __list)    \
    for (__iterator = __list;	\
         (__iterator)->next != __list;      \
         __iterator = (__iterator)->next);	\
    for ( ;	\
         __iterator != __list;      \
         __iterator = (__iterator)->prev)

#define ADT_INIT_LIST(name) { &(name), &(name) }

static inline void adt_init_list(struct adt_list* list)
{
    list->next = list->prev = list;
}

static inline int adt_empty_list(struct adt_list* list)
{
    return (list == list->next) && (list == list->prev);
}

static inline void __adt_list_add(struct adt_list* _new,
                                  struct adt_list* prev,
                                  struct adt_list* next)
{
    next->prev = _new;
    _new->next = next;
    _new->prev = prev;
    prev->next = _new;
}

static inline void adt_link_list(struct adt_list* head, struct adt_list* list)
{
    __adt_list_add(list, head, head->next);
}

static inline void adt_unlink_list(struct adt_list* list)
{
    struct adt_list* next, *prev;

    next = list->next;
    prev = list->prev;
    next->prev = prev;
    prev->next = next;
}

static inline void adt_sort_list(struct adt_list* head,
		int (*compare_func)(struct adt_list *, struct adt_list *))
{
	struct adt_list *it, *jt, *kt;

	if (adt_empty_list(head))
		return;

	for (it = head->next->next; it != head; it = it->next) {
		for (jt = head->next; jt != it; jt = jt->next) {
			if (compare_func(it, jt) < 0) {
				kt = it;
				it = it->prev;
				adt_unlink_list(kt);
				adt_link_list(jt->prev, kt);
				break;
			}
		}
	}
}

static inline struct adt_list *adt_find_list(struct adt_list* head,
		int (*equal_func)(struct adt_list *, void *), void *value)
{
	struct adt_list *it;
	adt_iterate_list(it, head) {
		if (equal_func(it, value))
			return it;
	}
	return NULL;
}

#endif /*!__ADT_LIST_H__*/
