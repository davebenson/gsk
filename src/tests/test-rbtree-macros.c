#include "../gskrbtreemacros.h"
#include "../gskmempool.h"

typedef struct _TreeNode TreeNode;
struct _TreeNode
{
  gboolean red;
  TreeNode *parent;
  TreeNode *child_left, *child_right;
  guint value;
};
#define COMPARE_INT_WITH_TREE_NODE(a,b,rv) rv = ((a<b->value) ? -1 : (a>b->value) ? 1 : 0)
#define COMPARE_TREE_NODES(a,b,rv)         COMPARE_INT_WITH_TREE_NODE(a->value,b,rv)
#define TREE_NODE_IS_RED(node)             ((node)->red)
#define TREE_NODE_SET_IS_RED(node, val)   ((node)->red = (val))

#define TREE(ptop)  (*(ptop)),TreeNode*,TREE_NODE_IS_RED,TREE_NODE_SET_IS_RED, \
                    parent, child_left, child_right, COMPARE_TREE_NODES

GskMemPoolFixed tree_node_pool = GSK_MEM_POOL_FIXED_STATIC_INIT(sizeof(TreeNode));

static gboolean
add_tree (TreeNode **ptop,
          guint      v)
{
  TreeNode *node = gsk_mem_pool_fixed_alloc (&tree_node_pool);
  TreeNode *extant;
  node->value = v;
  GSK_RBTREE_INSERT (TREE(ptop), node, extant);
  if (extant == NULL)
    return FALSE;
  gsk_mem_pool_fixed_free (&tree_node_pool, node);
  return TRUE;
}

static gboolean
test_tree (TreeNode **ptop,
           guint      v)
{
  TreeNode *found;
  GSK_RBTREE_LOOKUP_COMPARATOR (TREE(ptop), v, COMPARE_INT_WITH_TREE_NODE, found);
  return found != NULL;
}

static gboolean
del_tree (TreeNode **ptop,
          guint      v)
{
  TreeNode *found;
  GSK_RBTREE_LOOKUP_COMPARATOR (TREE(ptop), v, COMPARE_INT_WITH_TREE_NODE, found);
  if (found == NULL)
    return FALSE;
  GSK_RBTREE_REMOVE (TREE(ptop), found);
  gsk_mem_pool_fixed_free (&tree_node_pool, found);
  return TRUE;
}

int main()
{
  TreeNode *tree = NULL;
  g_assert (!add_tree (&tree, 1));
  g_assert (!add_tree (&tree, 2));
  g_assert (!add_tree (&tree, 3));
  g_assert ( add_tree (&tree, 1));
  g_assert ( add_tree (&tree, 2));
  g_assert ( add_tree (&tree, 3));
  g_assert (!test_tree (&tree, 0));
  g_assert ( test_tree (&tree, 1));
  g_assert ( test_tree (&tree, 2));
  g_assert ( test_tree (&tree, 3));
  g_assert (!test_tree (&tree, 4));
  g_assert (!del_tree  (&tree, 0));
  g_assert ( del_tree  (&tree, 2));
  g_assert (!del_tree  (&tree, 4));
  g_assert (!test_tree (&tree, 0));
  g_assert ( test_tree (&tree, 1));
  g_assert (!test_tree (&tree, 2));
  g_assert ( test_tree (&tree, 3));
  g_assert (!test_tree (&tree, 4));
  g_assert ( add_tree (&tree, 1));
  g_assert (!add_tree (&tree, 2));
  g_assert ( add_tree (&tree, 3));
  g_assert ( del_tree  (&tree, 1));
  g_assert ( del_tree  (&tree, 2));
  g_assert ( del_tree  (&tree, 3));
  g_assert (tree == NULL);
  return 0;
}

