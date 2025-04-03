#ifndef QIOF_QUEUE_H_
#define QIOF_QUEUE_H_
/*
 * Tail queue definitions.
 */
#define Q_TAILQ_HEAD(name, type, qual)                                     \
        struct name                                                        \
        {                                                                  \
                qual type *tqh_first;      /* first element */             \
                qual type *qual *tqh_last; /* addr of last next element */ \
        }
#define QTAILQ_HEADQ(name, type) Q_TAILQ_HEAD(name, struct type, )

#define QTAILQ_HEAD_INITIALIZER_Q(head) \
        {NULL, &(head).tqh_first}

#define Q_TAILQ_ENTRY(type, qual)                                                 \
        struct                                                                    \
        {                                                                         \
                qual type *tqe_next;       /* next element */                     \
                qual type *qual *tqe_prev; /* address of previous next element */ \
        }
#define QTAILQ_ENTRY_Q(type) Q_TAILQ_ENTRY(struct type, )

/*
 * Tail queue functions.
 */
#define QTAILQ_INIT_Q(head)                            \
        do                                             \
        {                                              \
                (head)->tqh_first = NULL;              \
                (head)->tqh_last = &(head)->tqh_first; \
        } while (/*CONSTCOND*/ 0)

#define QTAILQ_INSERT_HEAD_Q(head, elm, field)                           \
        do                                                               \
        {                                                                \
                if (((elm)->field.tqe_next = (head)->tqh_first) != NULL) \
                        (head)->tqh_first->field.tqe_prev =              \
                            &(elm)->field.tqe_next;                      \
                else                                                     \
                        (head)->tqh_last = &(elm)->field.tqe_next;       \
                (head)->tqh_first = (elm);                               \
                (elm)->field.tqe_prev = &(head)->tqh_first;              \
        } while (/*CONSTCOND*/ 0)

#define QTAILQ_INSERT_TAIL_Q(head, elm, field)             \
        do                                                 \
        {                                                  \
                (elm)->field.tqe_next = NULL;              \
                (elm)->field.tqe_prev = (head)->tqh_last;  \
                *(head)->tqh_last = (elm);                 \
                (head)->tqh_last = &(elm)->field.tqe_next; \
        } while (/*CONSTCOND*/ 0)

#define QTAILQ_INSERT_AFTER_Q(head, listelm, elm, field)                         \
        do                                                                       \
        {                                                                        \
                if (((elm)->field.tqe_next = (listelm)->field.tqe_next) != NULL) \
                        (elm)->field.tqe_next->field.tqe_prev =                  \
                            &(elm)->field.tqe_next;                              \
                else                                                             \
                        (head)->tqh_last = &(elm)->field.tqe_next;               \
                (listelm)->field.tqe_next = (elm);                               \
                (elm)->field.tqe_prev = &(listelm)->field.tqe_next;              \
        } while (/*CONSTCOND*/ 0)

#define QTAILQ_INSERT_BEFORE_Q(listelm, elm, field)                 \
        do                                                          \
        {                                                           \
                (elm)->field.tqe_prev = (listelm)->field.tqe_prev;  \
                (elm)->field.tqe_next = (listelm);                  \
                *(listelm)->field.tqe_prev = (elm);                 \
                (listelm)->field.tqe_prev = &(elm)->field.tqe_next; \
        } while (/*CONSTCOND*/ 0)

#define QTAILQ_REMOVE_Q(head, elm, field)                         \
        do                                                        \
        {                                                         \
                if (((elm)->field.tqe_next) != NULL)              \
                        (elm)->field.tqe_next->field.tqe_prev =   \
                            (elm)->field.tqe_prev;                \
                else                                              \
                        (head)->tqh_last = (elm)->field.tqe_prev; \
                *(elm)->field.tqe_prev = (elm)->field.tqe_next;   \
        } while (/*CONSTCOND*/ 0)

#define QTAILQ_FOREACH(var, head, field)  \
        for ((var) = ((head)->tqh_first); \
             (var);                       \
             (var) = ((var)->field.tqe_next))

#define QTAILQ_FOREACH_SAFE(var, head, field, next_var)          \
        for ((var) = ((head)->tqh_first);                        \
             (var) && ((next_var) = ((var)->field.tqe_next), 1); \
             (var) = (next_var))

#define QTAILQ_FOREACH_REVERSE_Q(var, head, headname, field)                 \
        for ((var) = (*(((struct headname *)((head)->tqh_last))->tqh_last)); \
             (var);                                                          \
             (var) = (*(((struct headname *)((var)->field.tqe_prev))->tqh_last)))

/*
 * Tail queue access methods.
 */
#define QTAILQ_EMPTY(head) ((head)->tqh_first == NULL)
#define QTAILQ_FIRST(head) ((head)->tqh_first)
#define QTAILQ_NEXT(elm, field) ((elm)->field.tqe_next)

#define QTAILQ_LAST_Q(head, headname) \
        (*(((struct headname *)((head)->tqh_last))->tqh_last))
#define QTAILQ_PREV_Q(elm, headname, field) \
        (*(((struct headname *)((elm)->field.tqe_prev))->tqh_last))
#endif