

/* singly-linked lists */
#define GSK_SLIST_FIND(start_elt, next_member, condition, rv)            \
  for (rv = start_elt; rv; rv = rv->next_member)                         \
    if (condition (rv))                                                  \
      break

void gsk_slist_sort  (gpointer *first_inout,
                      gsize     next_offset,
                      GCompareDataFunc comparator,
                      gpointer  comparator_data);
void gsk_slist_reverse  (gpointer *first_inout,
                         gsize     next_offset);

/* singly-linked queues */
#define GSK_SQUEUE_PREPEND(first_elt, last_elt, next_member, to_add)     \
  G_STMT_START{                                                          \
    to_add->next_member = first_elt;                                     \
    to_add->prev_member = NULL;                                          \
    if (first_elt == NULL)                                               \
      last_elt = to_add;                                                 \
    first_elt = to_add;                                                  \
  }G_STMT_END
#define GSK_SQUEUE_APPEND(first_elt, last_elt, next_member, to_add)      \
  G_STMT_START{                                                          \
    if (last_elt != NULL)                                                \
      last_elt->next_member = to_add;                                    \
    else                                                                 \
      first_elt = to_add;                                                \
    last_elt = to_add;                                                   \
    to_add->next_member = NULL;                                          \
  }G_STMT_END
#define GSK_SQUEUE_POP_FIRST(first_elt, last_elt, next_member, rv)       \
  G_STMT_START{                                                          \
    rv = first_elt;                                                      \
    if (rv)                                                              \
      {                                                                  \
        first_elt = first_elt->next_member;                              \
        if (first_elt == NULL)                                           \
          last_elt = NULL;                                               \
        rv->next_member = NULL;                                          \
      }                                                                  \
  }G_STMT_END

void gsk_squeue_sort (gpointer *first_inout,
                      gpointer *last_inout,
                      gsize     next_offset,
                      GCompareDataFunc comparator,
                      gpointer  comparator_data);
void gsk_squeue_reverse (gpointer *first_inout,
                         gpointer *last_inout,
                         gsize     next_offset);

/* doubly-linked queues */
#define GSK_DQUEUE_REMOVE(first_elt, last_elt, prev_member, next_member, \
                          to_remove)                                     \
  G_STMT_START{                                                          \
    if (to_remove->prev_member)                                          \
      to_remove->prev_member->next_member = to_remove->next_member;      \
    else                                                                 \
      first_elt = to_remove->next_member;                                \
    if (to_remove->next_member)                                          \
      to_remove->next_member->prev_member = to_remove->prev_member;      \
    else                                                                 \
      last_elt = to_remove->prev_member;                                 \
  }G_STMT_END
#define GSK_DQUEUE_PREPEND(first_elt, last_elt, prev_member, next_member,\
                           to_add)                                       \
  G_STMT_START{                                                          \
    to_add->next_member = first_elt;                                     \
    to_add->prev_member = NULL;                                          \
    if (first_elt)                                                       \
      first_elt->prev_member = to_add;                                   \
    else                                                                 \
      last_elt = to_add;                                                 \
    first_elt = to_add;                                                  \
  }G_STMT_END
#define GSK_DQUEUE_APPEND(first_elt, last_elt, prev_member, next_member, \
                          to_add)                                        \
  GSK_DQUEUE_PREPEND(last_elt, first_elt, next_member, prev_member, to_add)
#define GSK_DQUEUE_FIND_FORWARD(first_elt, last_elt, prev_member, next_member, \
                                condition, rv)                           \
  GSK_SLIST_FIND(first_elt, next_member, condition, rv)
#define GSK_DQUEUE_FIND_BACKWARD(first_elt, last_elt, prev_member, next_member, \
                                 condition, rv)                          \
  GSK_SLIST_FIND(last_elt, prev_member, condition, rv)
#define GSK_DQUEUE_POP_FIRST(first_elt, last_elt, prev_member, next_member, \
                             rv)                                         \
  G_STMT_START{                                                          \
    rv = first_elt;                                                      \
    if (rv)                                                              \
      {                                                                  \
        first_elt = first_elt->next_member;                              \
        if (first_elt)                                                   \
          first_elt->prev_member = NULL;                                 \
        else                                                             \
          last_elt = NULL;                                               \
        rv->next_member = NULL;                                          \
      }                                                                  \
  }G_STMT_END
#define GSK_DQUEUE_POP_LAST(first_elt, last_elt, prev_member, next_member,\
                            rv)                                          \
  GSK_DQUEUE_POP_FIRST(last_elt,first_elt,next_member,prev_member,rv)

#define GSK_DQUEUE_SORT(first_elt, last_elt, prev_member, next_member,   \
                        comparator, comparator_data)                     \
  gsk_dqueue_sort((gpointer*)(&first_elt),(gpointer*)(&last_elt),        \
                  (char*)&(first_elt->prev_member) - (char*)first_elt,   \
                  (char*)&(first_elt->next_member) - (char*)first_elt,   \
                  comparator, comparator_data)
#define GSK_DQUEUE_SORT_BACKWARD(first_elt, last_elt, prev_member, next_member,\
                        comparator, comparator_data)                     \
  gsk_dqueue_sort((gpointer*)(&last_elt),(gpointer*)(&first_elt),        \
                  (char*)&(first_elt->next_member) - (char*)first_elt,   \
                  (char*)&(first_elt->prev_member) - (char*)first_elt,   \
                  comparator, comparator_data)
#define GSK_DQUEUE_REVERSE(first_elt, last_elt, prev_member, next_member,\
                        comparator, comparator_data)                     \
  gsk_dqueue_reverse((gpointer*)(&first_elt),(gpointer*)(&last_elt),     \
                  (char*)&(first_elt->prev_member) - (char*)first_elt,   \
                  (char*)&(first_elt->next_member) - (char*)first_elt)

void gsk_dqueue_sort (gpointer *first_inout,
                      gpointer *last_inout,
                      gsize     prev_offset,
                      gsize     next_offset,
                      GCompareDataFunc comparator,
                      gpointer  comparator_data);
void gsk_dqueue_reverse (gpointer *first_inout,
                         gpointer *last_inout,
                         gsize     prev_offset,
                         gsize     next_offset);
