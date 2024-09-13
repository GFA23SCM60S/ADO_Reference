#ifndef BTREE_MGR_H
#define BTREE_MGR_H

#include "dberror.h"
#include "tables.h"

#define HASHMAP_LEN 1259

// linked list BNode
typedef struct BNode {
  void *data; // general pointer to be used with different data types
  struct BNode *next;
  struct BNode *previous;
} BNode;


typedef struct hashmapStructure {
  int key;
  void *val;
} hashmapStructure;

// hashmap
typedef struct HashMap {
  BNode *tbl[HASHMAP_LEN];
} HashMap;


typedef struct dynamicArr {
  int dynamicArrSize;
  int fill;
  int *elements; 
} dynamicArr;


typedef struct BT_Node {
  int size; 
  int isLeaf;
  int pageNum;
  dynamicArr *value;
  dynamicArr *childPages;
  dynamicArr *leafRIDPages;
  dynamicArr *leafRIDSlots;
  // tree pointers
  struct BT_Node **childNode; 
  struct BT_Node *parent; 
  struct BT_Node *left; 
  struct BT_Node *right; 
} BT_Node;



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
