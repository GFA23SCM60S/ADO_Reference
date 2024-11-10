#ifndef BTREE_MGR_H
#define BTREE_MGR_H

#include "dberror.h"
#include "tables.h"
#include "const.h"
// #include "data_structures.h"

// 1250 * 8KB(PAGE_SIZE is 8196) = 10MB, assuming we will need 10MB for every pool, 1259 is the closest prime number to 1250
#define HASH_LEN 1259

#define HASH_MULTIPLIER 31

#define MAX_LEVEL 100

// linked list BNode
typedef struct BNode {
  void *data; // general pointer to be used with different data types
  struct BNode *next;
  struct BNode *previous;
} BNode;

// key, value combination for hashmap
typedef struct HM_Comb {
  int key;
  void *val;
} HM_Comb;

// hashmap
typedef struct HashMap {
  BNode *tbl[HASH_LEN]; // table of linked list to solve hashmap collision
} HashMap;

// smart sorted array
typedef struct dynamicArr {
  int size;
  int fill;
  int *elems; // extra extenstion void * with DataType keyType
} dynamicArr;


typedef struct BT_Node {
  int size; // values size
  int isLeaf;
  int pageNum;
  dynamicArr *vals;
  dynamicArr *childrenPages;
  dynamicArr *leafRIDPages;
  dynamicArr *leafRIDSlots;
  // tree pointers
  struct BT_Node **children; // in all but in leaf
  struct BT_Node *parent; // in all but in root
  struct BT_Node *left; // in all but in root
  struct BT_Node *right; // in all but in root
} BT_Node;


// structure for accessing btrees
typedef struct BTreeHandle {
  DataType keyType;
  char *idxId;
  int size;
  int numEntries;
  int numNodes;
  int depth;
  int whereIsRoot;
  int nextPage;
  BT_Node *root;
  void *mgmtData;
} BTreeHandle;

typedef struct BT_ScanHandle {
  BTreeHandle *tree;
  void *mgmtData;
} BT_ScanHandle;

typedef struct ScanMgmtInfo {
  BT_Node *currentNode;
  int elementIndex;
} ScanMgmtInfo;

typedef struct NodeSplitConfig {
    int leftFill;      // Number of elements in left node
    int rightFill;     // Number of elements in right node
    int totalElements; // Total elements after insertion
} NodeSplitConfig;





// BNode functions
BT_Node *createBTNode(int size, int isLeaf, int pageNum);

// init and shutdown index manager
extern RC initIndexManager (void *mgmtData);
extern RC shutdownIndexManager ();

// create, destroy, open, and close an btree index
extern RC createBtree (char *idxId, DataType keyType, int n);
extern RC openBtree (BTreeHandle **tree, char *idxId);
extern RC closeBtree (BTreeHandle *tree);
extern RC deleteBtree (char *idxId);

// access information about a b-tree
extern RC getNumNodes (BTreeHandle *tree, int *result);
extern RC getNumEntries (BTreeHandle *tree, int *result);
extern RC getKeyType (BTreeHandle *tree, DataType *result);

// index access
extern RC findKey (BTreeHandle *tree, Value *key, RID *result);
extern RC insertKey (BTreeHandle *tree, Value *key, RID rid);
extern RC deleteKey (BTreeHandle *tree, Value *key);
extern RC openTreeScan (BTreeHandle *tree, BT_ScanHandle **handle);
extern RC nextEntry (BT_ScanHandle *handle, RID *result);
extern RC closeTreeScan (BT_ScanHandle *handle);

// debug and test functions
extern char *printTree (BTreeHandle *tree);

#endif // BTREE_MGR_H
