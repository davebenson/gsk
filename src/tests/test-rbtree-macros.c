#include <string.h>
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

/* --- RCB Tree Test --- */
/* Add all integers in the range [0, N) to
   the rbc tree, then iterate the tree
   and compute the index for each node. */
typedef struct _RBCTreeNode RBCTreeNode;
struct _RBCTreeNode
{
  guint tn_count;
  RBCTreeNode *tn_left, *tn_right, *tn_parent;
  gboolean red;
  guint value;
};

#define RBCTREENODE_GET_COUNT(n) n->tn_count
#define RBCTREENODE_SET_COUNT(n,v) n->tn_count = v
#define RBCTREE(tp)     (tp), RBCTreeNode*, \
                        TREE_NODE_IS_RED, TREE_NODE_SET_IS_RED, \
                        RBCTREENODE_GET_COUNT, RBCTREENODE_SET_COUNT, \
                        tn_parent, tn_left, tn_right, COMPARE_TREE_NODES
static void
shuffle (RBCTreeNode *node_array,
         guint n_nodes)
{
  guint i;
  for (i = 0; i < n_nodes; i++)
    {
      guint r = g_random_int_range (i, n_nodes);
      guint tmp = node_array[i].value;
      node_array[i].value = node_array[r].value;
      node_array[r].value = tmp;
    }
}

static void
validate_counts (RBCTreeNode *xxx)
{
  static guint depth = 0;
  g_assert (xxx->tn_count == (xxx->tn_left ? xxx->tn_left->tn_count : 0)
                       + (xxx->tn_right ? xxx->tn_right->tn_count : 0)
                       + 1);
  depth++;
  if (xxx->tn_left)
    validate_counts (xxx->tn_left);
  if (xxx->tn_right)
    validate_counts (xxx->tn_right);
  depth--;
}

static void
test_random_rbcint_tree  (guint n)
{
  RBCTreeNode *arr = g_new (RBCTreeNode, n);
  RBCTreeNode *top = NULL;
  guint i;

#if 1
  memset (arr, 0xed, sizeof (RBCTreeNode) * n);
#endif

  /* generate a random permutation
     (from http://www.techuser.net/randpermgen.html) */
  for (i = 0; i < n; i++)
    arr[i].value = i;
  shuffle (arr, n);

  for (i = 0; i < n; i++)
    {
      RBCTreeNode *conflict;
      GSK_RBCTREE_INSERT (RBCTREE (top), arr + i, conflict);
      g_assert (conflict == NULL);
      validate_counts (top);
    }
  for (i = 0; i < n; i++)
    {
      guint v = arr[i].value;
      RBCTreeNode *n;
      guint tv;
      GSK_RBCTREE_LOOKUP_COMPARATOR (RBCTREE (top), v, COMPARE_INT_WITH_TREE_NODE, n);
      g_assert (n == arr + i);
      GSK_RBCTREE_GET_NODE_INDEX (RBCTREE (top), n, tv);
      //g_message ("node index for value %u, index %u is apparently %u (should == value %u)", v,i,tv,v);
      g_assert (v == tv);
    }
  shuffle (arr, n);
  for (i = 0; i < n; i++)
    GSK_RBCTREE_REMOVE (RBCTREE (top), arr + i);
  /* XXX: we really need to test the counts DURING the removal!!!!!! */
}

int main()
{
  TreeNode *tree = NULL;
  TreeNode *node;
  guint i;
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

  GSK_RBTREE_FIRST (TREE(&tree), node);
  g_assert (node == NULL);
  GSK_RBTREE_LAST (TREE(&tree), node);
  g_assert (node == NULL);

  /* Construct tree with odd numbers 1..999 inclusive */
  for (i = 1; i <= 999; i += 2)
    g_assert (!add_tree (&tree, i));

  GSK_RBTREE_FIRST (TREE(&tree), node);
  g_assert (node != NULL);
  g_assert (node->value == 1);
  GSK_RBTREE_LAST (TREE(&tree), node);
  g_assert (node != NULL);
  g_assert (node->value == 999);

  for (i = 1; i <= 999; i += 2)
    {
      g_assert (test_tree (&tree, i));
      g_assert (!test_tree (&tree, i+1));
    }
  for (i = 0; i <= 999; i++)
    {
      GSK_RBTREE_SUPREMUM_COMPARATOR (TREE(&tree), i, COMPARE_INT_WITH_TREE_NODE, node);
      g_assert (node);
      g_assert (node->value == (i%2)?i:(i+1));
    }
  GSK_RBTREE_SUPREMUM_COMPARATOR (TREE(&tree), 1000, COMPARE_INT_WITH_TREE_NODE, node);
  g_assert (node==NULL);
  for (i = 1; i <= 1000; i++)
    {
      TreeNode *node;
      GSK_RBTREE_INFIMUM_COMPARATOR (TREE(&tree), i, COMPARE_INT_WITH_TREE_NODE, node);
      g_assert (node);
      g_assert (node->value == (i%2)?i:(i-1));
    }
  GSK_RBTREE_INFIMUM_COMPARATOR (TREE(&tree), 0, COMPARE_INT_WITH_TREE_NODE, node);
  g_assert (node==NULL);
  for (i = 1; i <= 999; i += 2)
    g_assert (del_tree (&tree, i));


  /* random rbctree test */
  g_printerr ("Testing RBC-tree macros... ");
  for (i = 0; i < 100; i++)
    test_random_rbcint_tree (10);
  g_printerr (".");
  for (i = 0; i < 10; i++)
    test_random_rbcint_tree (100);
  g_printerr (".");
  for (i = 0; i < 2; i++)
    {
      test_random_rbcint_tree (1000);
      g_printerr (".");
    }
  g_printerr ("done.\n");

  return 0;
}

