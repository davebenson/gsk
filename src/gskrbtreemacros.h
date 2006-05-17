#ifndef __GSK_RBTREE_MACROS_H_
#define __GSK_RBTREE_MACROS_H_

/* A tree is a tuple:
 *    top
 *    type*
 *    is_red
 *    set_is_red
 *    parent
 *    left_child
 *    right_child
 *    comparator
 *
 * The methods are:
 *   INSERT(tree, node, collision_node) 
 *         Try to insert 'node' into tree.
 *         If an equivalent node exists,
 *         return it in collision_node.
 *         Otherwise, node is added to the tree
 *         (and collision_node is set to NULL).
 *   REMOVE(tree, node)
 *         Remove a node from the tree.
 *   LOOKUP(tree, node, out)
 *         Find a node in the tree.
 *         Finds the node equivalent with 
 *         'node', and returns it in 'out',
 *         or sets out to NULL if no node exists.
 *   LOOKUP_COMPARATOR(tree, key, key_comparator, out)
 *         Find a node in the tree, based on a key
 *         which may not be in the same format as the node.
 *   PREV(tree, cur, out)
 *         Set 'out' to the previous node in the tree before cur.
 *   NEXT(tree, cur, out)
 *         Set 'out' to the next node in the tree after cur.
 */
#define GSK_RBTREE_INSERT(tree, node, collision_node)                \
  GSK_RBTREE_INSERT_(tree, node, collision_node)
#define GSK_RBTREE_REMOVE(tree, node)                                \
  GSK_RBTREE_REMOVE_(tree, node)
#define GSK_RBTREE_LOOKUP(tree, key, out)                            \
  GSK_RBTREE_LOOKUP_(tree, key, out)
#define GSK_RBTREE_LOOKUP_COMPARATOR(tree, key, key_comparator, out) \
  GSK_RBTREE_LOOKUP_COMPARATOR_(tree, key, key_comparator, out)
#define GSK_RBTREE_NEXT(tree, in, out)                               \
  GSK_RBTREE_NEXT_(tree, in, out)
#define GSK_RBTREE_PREV(tree, in, out)                               \
  GSK_RBTREE_PREV_(tree, in, out)

#define GSK_RBTREE_INSERT_(top,type,is_red,set_is_red,parent,left_child,right_child,comparator, node,collision_node) \
  G_STMT_START{                                                                \
    type _gsk_last = NULL;                                                     \
    type _gsk_at = (top);                                                      \
    gboolean _gsk_last_was_left = FALSE;                                       \
    collision_node = NULL;                                                     \
    while (_gsk_at != NULL)                                                    \
      {                                                                        \
        int _gsk_compare_rv;                                                   \
        _gsk_last = _gsk_at;                                                   \
        comparator(_gsk_at, node, _gsk_compare_rv);                            \
        if (_gsk_compare_rv > 0)                                               \
          {                                                                    \
            _gsk_last_was_left = TRUE;                                         \
            _gsk_at = _gsk_at->left_child;                                     \
          }                                                                    \
        else if (_gsk_compare_rv < 0)                                          \
          {                                                                    \
            _gsk_last_was_left = FALSE;                                        \
            _gsk_at = _gsk_at->right_child;                                    \
          }                                                                    \
        else                                                                   \
          break;                                                               \
     }                                                                         \
    if (_gsk_at != NULL)                                                       \
      {                                                                        \
        /* collision */                                                        \
        collision_node = _gsk_at;                                              \
      }                                                                        \
    else if (_gsk_last == NULL)                                                \
      {                                                                        \
        /* only node in tree */                                                \
        top = node;                                                            \
        set_is_red (node, 0);                                                  \
      }                                                                        \
    else                                                                       \
      {                                                                        \
        node->parent = _gsk_last;                                              \
        if (_gsk_last_was_left)                                                \
          _gsk_last->left_child = node;                                        \
        else                                                                   \
          _gsk_last->right_child = node;                                       \
                                                                               \
        /* fixup */                                                            \
        _gsk_at = node;                                                        \
        set_is_red (_gsk_at, 1);                                               \
        while (top != _gsk_at && is_red(_gsk_at->parent))                      \
          {                                                                    \
            if (_gsk_at->parent == _gsk_at->parent->parent->left_child)        \
              {                                                                \
                type _gsk_y = _gsk_at->parent->parent->right_child;            \
                if (_gsk_y != NULL && is_red (_gsk_y))                         \
                  {                                                            \
                    set_is_red (_gsk_at->parent, 0);                           \
                    set_is_red (_gsk_y, 0);                                    \
                    set_is_red (_gsk_at->parent->parent, 1);                   \
                    _gsk_at = _gsk_at->parent->parent;                         \
                  }                                                            \
                else                                                           \
                  {                                                            \
                    if (_gsk_at == _gsk_at->parent->right_child)               \
                      {                                                        \
                        _gsk_at = _gsk_at->parent;                             \
                        GSK_RBTREE_ROTATE_LEFT (top,type,parent,left_child,right_child, _gsk_at);\
                      }                                                        \
                    set_is_red(_gsk_at->parent, 0);                            \
                    set_is_red(_gsk_at->parent->parent, 1);                    \
                    GSK_RBTREE_ROTATE_RIGHT (top,type,parent,left_child,right_child, \
                                           _gsk_at->parent->parent)            \
                  }                                                            \
              }                                                                \
            else                                                               \
              {                                                                \
                type _gsk_y = _gsk_at->parent->parent->left_child;             \
                if (_gsk_y != NULL && is_red (_gsk_y))                         \
                  {                                                            \
                    set_is_red (_gsk_at->parent, 0);                           \
                    set_is_red (_gsk_y, 0);                                    \
                    set_is_red (_gsk_at->parent->parent, 1);                   \
                    _gsk_at = _gsk_at->parent->parent;                         \
                  }                                                            \
                else                                                           \
                  {                                                            \
                    if (_gsk_at == _gsk_at->parent->left_child)                \
                      {                                                        \
                        _gsk_at = _gsk_at->parent;                             \
                        GSK_RBTREE_ROTATE_RIGHT (top,type,parent,left_child,right_child,\
                                               _gsk_at);                       \
                      }                                                        \
                    set_is_red(_gsk_at->parent, 0);                            \
                    set_is_red(_gsk_at->parent->parent, 1);                    \
                    GSK_RBTREE_ROTATE_LEFT (top,type,parent,left_child,right_child, _gsk_at->parent->parent);\
                  }                                                            \
              }                                                                \
          }                                                                    \
        set_is_red((top), 0);                                                  \
      }                                                                        \
  }G_STMT_END

#define GSK_RBTREE_REMOVE_(top,type,is_red,set_is_red,parent,left_child,right_child,comparator, node) \
/* Algorithms:273. */                                                         \
G_STMT_START{                                                                 \
  type _gsk_rb_del_z = (node);                                                \
  type _gsk_rb_del_x;                                                         \
  type _gsk_rb_del_y;                                                         \
  type _gsk_rb_del_nullpar = NULL;	/* Used to emulate sentinel nodes */  \
  gboolean _gsk_rb_del_fixup;                                                 \
  if (_gsk_rb_del_z->left_child == NULL || _gsk_rb_del_z->right_child == NULL)\
    _gsk_rb_del_y = _gsk_rb_del_z;                                            \
  else                                                                        \
    {                                                                         \
      GSK_RBTREE_NEXT_ (top,type,is_red,set_is_red,parent,left_child,right_child,comparator,\
                        _gsk_rb_del_z, _gsk_rb_del_y);                         \
    }                                                                         \
  _gsk_rb_del_x = _gsk_rb_del_y->left_child ? _gsk_rb_del_y->left_child       \
                                            : _gsk_rb_del_y->right_child;     \
  if (_gsk_rb_del_x)                                                          \
    _gsk_rb_del_x->parent = _gsk_rb_del_y->parent;                            \
  else                                                                        \
    _gsk_rb_del_nullpar = _gsk_rb_del_y->parent;                               \
  if (!_gsk_rb_del_y->parent)                                                 \
    top = _gsk_rb_del_x;                                                      \
  else                                                                        \
    {                                                                         \
      if (_gsk_rb_del_y == _gsk_rb_del_y->parent->left_child)                 \
	_gsk_rb_del_y->parent->left_child = _gsk_rb_del_x;                    \
      else                                                                    \
	_gsk_rb_del_y->parent->right_child = _gsk_rb_del_x;                   \
    }                                                                         \
  _gsk_rb_del_fixup = !is_red(_gsk_rb_del_y);                                 \
  if (_gsk_rb_del_y != _gsk_rb_del_z)                                         \
    {                                                                         \
      set_is_red(_gsk_rb_del_y, is_red(_gsk_rb_del_z));                       \
      _gsk_rb_del_y->left_child = _gsk_rb_del_z->left_child;                  \
      _gsk_rb_del_y->right_child = _gsk_rb_del_z->right_child;                \
      _gsk_rb_del_y->parent = _gsk_rb_del_z->parent;                          \
      if (_gsk_rb_del_y->parent)                                              \
	{                                                                     \
	  if (_gsk_rb_del_y->parent->left_child == _gsk_rb_del_z)             \
	    _gsk_rb_del_y->parent->left_child = _gsk_rb_del_y;                \
	  else                                                                \
	    _gsk_rb_del_y->parent->right_child = _gsk_rb_del_y;               \
	}                                                                     \
      else                                                                    \
	top = _gsk_rb_del_y;                                                  \
                                                                              \
      if (_gsk_rb_del_y->left_child)                                          \
	_gsk_rb_del_y->left_child->parent = _gsk_rb_del_y;                    \
      if (_gsk_rb_del_y->right_child)                                         \
	_gsk_rb_del_y->right_child->parent = _gsk_rb_del_y;                   \
      if (_gsk_rb_del_nullpar == _gsk_rb_del_z)                               \
	_gsk_rb_del_nullpar = _gsk_rb_del_y;                                  \
    }                                                                         \
  if (_gsk_rb_del_fixup)                                                      \
    {                                                                         \
      /* delete fixup (Algorithms, p 274) */                                  \
      while (_gsk_rb_del_x != top && !is_red (_gsk_rb_del_x))                 \
        {                                                                     \
          type _gsk_rb_del_xparent = _gsk_rb_del_x ? _gsk_rb_del_x->parent    \
                                                   : _gsk_rb_del_nullpar;     \
          if (_gsk_rb_del_x == _gsk_rb_del_xparent->left_child)               \
            {                                                                 \
              type _gsk_rb_del_w = _gsk_rb_del_xparent->right_child;          \
              if (_gsk_rb_del_w != NULL && is_red (_gsk_rb_del_w))            \
                {                                                             \
                  set_is_red (_gsk_rb_del_w, 0);                              \
                  set_is_red (_gsk_rb_del_xparent, 1);                        \
                  GSK_RBTREE_ROTATE_LEFT (top,type,parent,left_child,right_child, _gsk_rb_del_xparent); \
                  _gsk_rb_del_w = _gsk_rb_del_xparent->right_child;           \
                }                                                             \
              if (!(_gsk_rb_del_w->left_child && is_red (_gsk_rb_del_w->left_child))    \
               && !(_gsk_rb_del_w->right_child && is_red (_gsk_rb_del_w->right_child))) \
                {                                                             \
                  set_is_red (_gsk_rb_del_w, 1);                              \
                  _gsk_rb_del_x = _gsk_rb_del_xparent;                        \
                }                                                             \
              else                                                            \
                {                                                             \
                  if (!(_gsk_rb_del_w && is_red (_gsk_rb_del_w->right_child)))\
                    {                                                         \
                      if (_gsk_rb_del_w->left_child)                          \
                        set_is_red (_gsk_rb_del_w->left_child, 0);            \
                      set_is_red (_gsk_rb_del_w, 1);                          \
                      GSK_RBTREE_ROTATE_RIGHT (top,type,parent,left_child,right_child, _gsk_rb_del_w); \
                      _gsk_rb_del_w = _gsk_rb_del_xparent->right_child;       \
                    }                                                         \
                  set_is_red (_gsk_rb_del_w, is_red (_gsk_rb_del_xparent));   \
                  set_is_red (_gsk_rb_del_xparent, 0);                        \
                  set_is_red (_gsk_rb_del_w->right_child, 0);                 \
                  GSK_RBTREE_ROTATE_LEFT (top,type,parent,left_child,right_child, _gsk_rb_del_xparent);\
                  _gsk_rb_del_x = top;                                        \
                }                                                             \
            }                                                                 \
          else                                                                \
            {                                                                 \
              type _gsk_rb_del_w = _gsk_rb_del_xparent->left_child;           \
              if (_gsk_rb_del_w && is_red (_gsk_rb_del_w))                    \
                {                                                             \
                  set_is_red (_gsk_rb_del_w, 0);                              \
                  set_is_red (_gsk_rb_del_xparent, 1);                        \
                  GSK_RBTREE_ROTATE_RIGHT (top,type,parent,left_child,right_child, _gsk_rb_del_xparent);\
                  _gsk_rb_del_w = _gsk_rb_del_xparent->left_child;            \
                }                                                             \
              if (!(_gsk_rb_del_w->right_child && is_red (_gsk_rb_del_w->right_child)) \
               && !(_gsk_rb_del_w->left_child && is_red (_gsk_rb_del_w->left_child))) \
                {                                                             \
                  set_is_red (_gsk_rb_del_w, 1);                              \
                  _gsk_rb_del_x = _gsk_rb_del_xparent;                        \
                }                                                             \
              else                                                            \
                {                                                             \
                  if (!(_gsk_rb_del_w && is_red (_gsk_rb_del_w->left_child))) \
                    {                                                         \
                      set_is_red (_gsk_rb_del_w->right_child, 0);             \
                      set_is_red (_gsk_rb_del_w, 1);                          \
                      GSK_RBTREE_ROTATE_LEFT (top,type,parent,left_child,right_child, _gsk_rb_del_w);\
                      _gsk_rb_del_w = _gsk_rb_del_xparent->left_child;        \
                    }                                                         \
                  set_is_red (_gsk_rb_del_w, is_red (_gsk_rb_del_xparent));   \
                  set_is_red (_gsk_rb_del_xparent, 0);                        \
                  if (_gsk_rb_del_w->left_child)                              \
                    set_is_red (_gsk_rb_del_w->left_child, 0);                \
                  GSK_RBTREE_ROTATE_RIGHT (top,type,parent,left_child,right_child, _gsk_rb_del_xparent);\
                  _gsk_rb_del_x = top;                                        \
                }                                                             \
            }                                                                 \
        }                                                                     \
      if (_gsk_rb_del_x != NULL)                                              \
        set_is_red(_gsk_rb_del_x, 0);                                         \
    }                                                                         \
  _gsk_rb_del_z->left_child = NULL;                                           \
  _gsk_rb_del_z->right_child = NULL;                                          \
  _gsk_rb_del_z->parent = NULL;                                               \
}G_STMT_END

#define GSK_RBTREE_LOOKUP_COMPARATOR_(top,type,is_red,set_is_red,parent,left_child,right_child,comparator, \
                                      key,key_comparator,out)                  \
  G_STMT_START{                                                                \
    type _gsk_lookup_at = (top);                                               \
    while (_gsk_lookup_at)                                                     \
      {                                                                        \
        int _gsk_compare_rv;                                                   \
        key_comparator(key,_gsk_lookup_at,_gsk_compare_rv);                    \
        if (_gsk_compare_rv < 0)                                               \
          _gsk_lookup_at = _gsk_lookup_at->left_child;                         \
        else if (_gsk_compare_rv > 0)                                          \
          _gsk_lookup_at = _gsk_lookup_at->right_child;                        \
        else                                                                   \
          break;                                                               \
      }                                                                        \
    out = _gsk_lookup_at;                                                      \
  }G_STMT_END
#define GSK_RBTREE_LOOKUP_(top,type,is_red,set_is_red,parent,left_child,right_child,comparator, key,out) \
  GSK_RBTREE_LOOKUP_COMPARATOR_(top,type,is_red,set_is_red,parent,left_child,right_child,comparator, key,comparator,out)

/* these macros don't need the is_red/set_is_red macros, nor the comparator,
   so omit them, to keep the lines a bit shorter. */
#define GSK_RBTREE_ROTATE_RIGHT(top,type,parent,left_child,right_child, node) \
  GSK_RBTREE_ROTATE_LEFT(top,type,parent,right_child,left_child, node)
#define GSK_RBTREE_ROTATE_LEFT(top,type,parent,left_child,right_child, node) \
G_STMT_START{                                                              \
  type _gsk_rot_x = (node);                                                \
  type _gsk_rot_y = _gsk_rot_x->right_child;                               \
                                                                           \
  _gsk_rot_x->right_child = _gsk_rot_y->left_child;                        \
  if (_gsk_rot_y->left_child)                                              \
    _gsk_rot_y->left_child->parent = _gsk_rot_x;                           \
  _gsk_rot_y->parent = _gsk_rot_x->parent;                                 \
  if (_gsk_rot_x->parent == NULL)                                          \
    top = _gsk_rot_y;                                                      \
  else if (_gsk_rot_x == _gsk_rot_x->parent->left_child)                   \
    _gsk_rot_x->parent->left_child = _gsk_rot_y;                           \
  else                                                                     \
    _gsk_rot_x->parent->right_child = _gsk_rot_y;                          \
  _gsk_rot_y->left_child = _gsk_rot_x;                                     \
  _gsk_rot_x->parent = _gsk_rot_y;                                         \
}G_STMT_END

/* iteration */
#define GSK_RBTREE_NEXT_(top,type,is_red,set_is_red,parent,left_child,right_child,comparator, in, out)  \
G_STMT_START{                                                              \
  type _gsk_next_at = (in);                                                \
  g_assert (_gsk_next_at != NULL);                                         \
  if (_gsk_next_at->right_child != NULL)                                   \
    {                                                                      \
      _gsk_next_at = _gsk_next_at->right_child;                            \
      while (_gsk_next_at->left_child != NULL)                             \
        _gsk_next_at = _gsk_next_at->left_child;                           \
      out = _gsk_next_at;                                                  \
    }                                                                      \
  else                                                                     \
    {                                                                      \
      type _gsk_next_parent = (in)->parent;                                \
      while (_gsk_next_parent != NULL                                      \
          && _gsk_next_at == _gsk_next_parent->right_child)                \
        {                                                                  \
          _gsk_next_at = _gsk_next_parent;                                 \
          _gsk_next_parent = _gsk_next_parent->parent;                     \
        }                                                                  \
      out = _gsk_next_parent;                                              \
    }                                                                      \
}G_STMT_END

/* prev is just next with left/right child reversed. */
#define GSK_RBTREE_PREV_(top,type,is_red,set_is_red,parent,left_child,right_child,comparator, in, out)  \
  GSK_RBTREE_NEXT_(top,type,is_red,set_is_red,parent,right_child,left_child,comparator, in, out)

#endif
