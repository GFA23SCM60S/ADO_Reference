#include "btree_mgr.h"
#include "string.h"
#include "storage_mgr.h"
#include "buffer_mgr.h"
#include <stdarg.h>
#include <unistd.h>
#include <math.h>



#include <stdlib.h>
#include <stdio.h>




/*
  - description: Initializes a hash map
  - param: None
  - return: A pointer to the initialized hash map (HashMap *) if successful, or NULL on memory allocation failure
*/
HashMap *hmInit() {
  // Allocate memory for the hash map
  HashMap *hm = malloc(sizeof(HashMap));

  // Check for memory allocation failure
  if (hm == NULL) {
    // Handle memory allocation failure
    return NULL;
  }

  // Initialize each entry in the hash table to NULL
  for (int i = 0; i < HASH_LEN; i++) {
    hm->tbl[i] = NULL;
  }

  // Return the initialized hash map
  return hm;
}


/*
  - description: Hashes a key using the djb2 algorithm-inspired hash function
  - param:
    1. key - The key to be hashed
  - return: The hash value for the given key
*/
int hash(int key) {
  // Choose an initial hash value
  unsigned h = 31;

  // Apply the djb2-inspired hash function
  h = (h * HASH_LEN) ^ (key * HASH_LEN);

  // Return the hash value within the range of the hash table size
  return h % HASH_LEN;
}



/*
  - description: Retrieves a key-value pair from the hash map
  - param:
    1. hm - The hash map handle
    2. key - The key to be found
  - return: A pointer to the key-value pair (hashmapStructure *) if found, or NULL if the key is not present
*/
hashmapStructure *structHM(HashMap *hm, int key) {
  // Calculate the hash value for the given key
  int indx = hash(key);

  // Use a double-pointer to simplify traversal and update
  BNode **current = &(hm->tbl[indx]);

  // Traverse the linked list to find the key-value pair
  while (*current != NULL) {
    // Convert the data to a key-value pair structure
    hashmapStructure *comb = (hashmapStructure *)(*current)->data;

    // Check if the key matches
    if (comb->key == key) {
      // Return the key-value pair if found
      return comb;
    }

    // Move to the next node in the linked list
    current = &((*current)->next);
  }

  // Return NULL if the key is not found in the hash map
  return NULL;
}


/*
  - description: Inserts a key-value pair into the hash map or updates the value if the key already exists
  - param:
    1. hm - The hash map handle
    2. key - The key to be inserted or updated
    3. val - A pointer to the value associated with the key
  - return: 0 if the value is updated, 1 if a new key-value pair is inserted, or an appropriate error code
*/
int insertHM(HashMap *hm, int key, void *val) {
  // Calculate the hash value for the key
  int indx = hash(key);

  // Use a double-pointer to simplify traversal and update
  BNode **current = &(hm->tbl[indx]);

  // Traverse the linked list to find the key-value pair
  while (*current != NULL) {
    // Convert the data to a key-value pair structure
    hashmapStructure *comb = (hashmapStructure *)(*current)->data;

    // Check if the key matches
    if (comb->key == key) {
      // Update the value and return
      comb->val = val;
      return 0; // Updated
    }

    // Move to the next node in the linked list
    current = &((*current)->next);
  }

  // If the key does not exist, create a new key-value pair
  hashmapStructure *comb = malloc(sizeof(hashmapStructure));
  comb->key = key;
  comb->val = val;

  // Create a new node for the key-value pair
  BNode *newNode = malloc(sizeof(BNode));
  newNode->data = comb;
  newNode->next = hm->tbl[indx];
  newNode->previous = NULL;

  // Update the previous pointer of the next node if it exists
  if (hm->tbl[indx] != NULL) {
    hm->tbl[indx]->previous = newNode;
  }

  // Update the linked list pointer in the hash map
  hm->tbl[indx] = newNode;

  return 1; // Inserted
}


/*
  - description: Retrieves the value associated with a key from the hash map
  - param:
    1. hm - The hash map handle
    2. key - The key for which the value is to be retrieved
  - return: A pointer to the value associated with the key, or NULL if the key is not present in the hash map
*/
void *getHM(HashMap *hm, int key) {
  // Retrieve the key-value pair associated with the key
  hashmapStructure *comb = structHM(hm, key);

  // If the key is not present, return NULL
  if (comb == NULL) {
    return NULL;
  }

  // Return the value associated with the key
  return comb->val;
}


/*
  - description: Deletes a key-value pair from the hash map
  - param:
    1. hm - The hash map handle
    2. key - The key for which the key-value pair is to be deleted
  - return: 1 if the key-value pair is successfully deleted, 0 if the key is not found
*/
int deleteHM(HashMap *hm, int key) {
  // Calculate the hash value for the key
  int indx = hash(key);

  // Use a double-pointer to simplify traversal and update
  BNode **current = &(hm->tbl[indx]);

  // Traverse the linked list to find the key-value pair
  while (*current != NULL) {
    // Convert the data to a key-value pair structure
    hashmapStructure *comb = (hashmapStructure *)(*current)->data;

    // Check if the key matches
    if (comb->key == key) {
      // Update the linked list pointer to skip the current node
      *current = (*current)->next;

      // Update the previous pointer of the next node if it exists
      if (*current != NULL) {
        (*current)->previous = NULL;
      }

      // Free the memory for the key-value pair and the node
      free(comb);
      free(*current);

      return 1; // Deleted
    }

    // Move to the next node in the linked list
    current = &((*current)->next);
  }

  // Return 0 if the key is not found in the hash map
  return 0; // Not found
}


/*
  - description: Deletes all nodes of the hash map and releases associated resources
  - param:
    1. hm - The hash map handle
*/
void destroyHM(HashMap *hm) {
  // Iterate through each hash index in the hash map
  for (int i = 0; i < HASH_LEN; i++) {
    // Use a double-pointer to simplify traversal and update
    BNode **current = &(hm->tbl[i]);

    // Traverse the linked list and delete nodes
    while (*current != NULL) {
      // Use a temporary pointer for the node to be deleted
      BNode *deletable = *current;

      // Move to the next node in the linked list
      *current = (*current)->next;

      // Free the memory for the key-value pair data and the node
      free(deletable->data);
      free(deletable);
    }
  }

  // Free the memory for the hash map itself
  free(hm);
}




/*
  - description: Performs binary search on a dynamic array to find the first occurrence of an element
  - param:
    1. arr - The dynamic array handle
    2. elem - The element to be searched
    3. fitOn - A pointer to an integer that will store the position where the element can be fit
  - return: The position of the first occurrence of the element if found, -1 otherwise
*/
int dynamicArrSearch(dynamicArr *arr, int elem, int *fitOn) {
  int low = 0;
  int high = arr->fill - 1;

  // Handle the case of an empty array
  if (high < 0) {
    (*fitOn) = low;
    return -1;
  }

  int position;
  while (1) {
    position = (low + high) / 2;

    if (elem == arr->elements[position]) {
      // Find the first occurrence of the element
      while (position > 0 && elem == arr->elements[position - 1]) {
        position--;
      }

      // Store the position where the element can be fit
      (*fitOn) = position;
      return position;
    }

    // If the search space is reduced to a single element
    if (low >= high) {
      // Check if the element should be placed after the current element
      if (elem > arr->elements[low]) {
        low++;
      }

      // Store the position where the element can be fit
      (*fitOn) = low;

      // Element not found
      return -1;
    }

    // Update the search space based on the comparison with the current element
    if (elem < arr->elements[position]) {
      high = position - 1;
    } else {
      low = position + 1;
    }
  }
}


dynamicArr *initArr(int size) {
  dynamicArr *newDynamicArr = new(dynamicArr); // TODO free, Done in destroyArr
  newDynamicArr->elements = newIntArr(size); // TODO free, Done in destroyArr
  newDynamicArr->dynamicArrSize = size;
  newDynamicArr->fill = 0;
  return newDynamicArr;
}


void destroyArr(dynamicArr *dynamicArray) {
  free(dynamicArray->elements);
  free(dynamicArray);
}

int InsertAtArr(dynamicArr *dynamicArray, int element, int insertIndex) {
  if (dynamicArray->dynamicArrSize > dynamicArray->fill && insertIndex <= dynamicArray->fill) {
    if (insertIndex != dynamicArray->fill) { // Insert at the end
      for (int i = dynamicArray->fill; i > insertIndex; i--) {
        dynamicArray->elements[i] = dynamicArray->elements[i - 1];
      }
    }
    dynamicArray->elements[insertIndex] = element;
    dynamicArray->fill++;
    return insertIndex;
  }
  return -1; // No more space or invalid index
}


int InsertArr(dynamicArr *dynamicArray, int elem) {
  int insertIndex = -1; // Indicates no more space

  // Check if there is space in the array
  if (dynamicArray->dynamicArrSize > dynamicArray->fill) {
    int searchIndex = dynamicArrSearch(dynamicArray, elem, &insertIndex);
    insertIndex = InsertAtArr(dynamicArray, elem, insertIndex);
  }

  return insertIndex;
}


void DeleteAtArr(dynamicArr *dynamicArray, int startIndex, int deleteCount) {
  dynamicArray->fill -= deleteCount;
  for (int i = startIndex; i < dynamicArray->fill; i++) {
    dynamicArray->elements[i] = dynamicArray->elements[i + deleteCount];
  }
}


int DeleteOneArr(dynamicArr *dynamicArray, int element) {
  int empty;
  int index = dynamicArrSearch(dynamicArray, element, &empty);

  if (index >= 0) {
    DeleteAtArr(dynamicArray, index, 1);
  }

  return index;
}


int DeleteAllArr(dynamicArr *dynamicArray, int elem) {
  int empty;
  int firstIndex = dynamicArrSearch(dynamicArray, elem, &empty);

  if (firstIndex >= 0) {
    int count = 1;

    while (firstIndex + count < dynamicArray->fill && elem == dynamicArray->elements[firstIndex + count]) {
      count++;
    }

    DeleteAtArr(dynamicArray, firstIndex, count);
  }

  return firstIndex;
}


void emptyArr(dynamicArr *dynamicArray) {
  dynamicArray->fill = 0;
}



// helpers

void freePointer(int count, ...) {
  // Initialize the variable argument list
  va_list pointerList;
  va_start(pointerList, count);

  // Loop through the arguments and free each pointer
  for (int i = 0; i < count; i++) {
    void *ptr = va_arg(pointerList, void *);
    free(ptr);
  }

  // Clean up the variable argument list
  va_end(pointerList);
}


/*
  - description: Creates a new BT_Node with the specified size, leaf status, and page number
  - param:
    1. size - The size of the BT_Node
    2. isLeaf - Indicates whether the BT_Node is a leaf or not
    3. pageNum - The page number associated with the BT_Node
  - return: A pointer to the newly created BT_Node
*/
BT_Node *createBTNode(int size, int isLeaf, int pageNum)
{
  BT_Node *targeNode = new (BT_Node);
  // Setting up the values
  targeNode->right = NULL;  // first empty
  targeNode->left = NULL;   // first empty
  targeNode->parent = NULL; // first empty
  targeNode->size = size;
  targeNode->pageNum = pageNum;
  targeNode->value = initArr(size); // initialize the array of values
  targeNode->isLeaf = isLeaf;
  if (!isLeaf)
  {                                                      // if it has childNode
    targeNode->childPages = initArr(size + 1);         // +1 because it is the number of childNode
    targeNode->childNode = newArray(BT_Node *, size + 1); // +1 because it is the number of childNode
  }
  else
  {                                         // else it is a leaf
    targeNode->leafRIDPages = initArr(size); // initialize the array of leaf RID pages
    targeNode->leafRIDSlots = initArr(size); // initialize the array of leaf RID slots
  }
  return targeNode; // return the reference to the node
}



/*
  - description: Recursively destroys a B-tree node and releases associated resources
  - param:
    1. btNode - The B-tree node to be destroyed
*/
void destroyBTNode(BT_Node *node)
{
  destroyArr(node->value); // destroy the array of values
  if (!node->isLeaf)
  {                         // if it has childNode
    destroyArr(node->childPages); // destroy the array of childNode pages
    free(node->childNode);           // free the array of childNode
  }
  else
  {                                // else it is a leaf
    destroyArr(node->leafRIDPages); // destroy the array of leaf RID pages
    destroyArr(node->leafRIDSlots); // destroy the array of leaf RID slots
  }
  free(node); // free the node
}

RC printNodeHelper(BT_Node *node, char *result) {
  bool pol = node == NULL;
  if (pol) {
    sprintf(result+ strlen(result),"NULL BNode !!\n");
    return RC_GENERAL_ERROR;
  }
  sprintf(result + strlen(result), "(%d)[", node->pageNum);

  int q;
  for (q = 0; q < node->value->fill; q++) {
      if (q > 0) {
          sprintf(result + strlen(result), ",");
      }

      if (node->isLeaf) {
          // If it is a leaf node
          sprintf(result + strlen(result), "%d.%d,%d", node->leafRIDPages->elements[q], node->leafRIDSlots->elements[q], node->value->elements[q]);
      } else {
          // If it has childNode
          sprintf(result + strlen(result), "%d,%d", node->childPages->elements[q], node->value->elements[q]);
      }
  }

  if (!node->isLeaf && node->value->fill > 0) {
      sprintf(result + strlen(result), ",");
  }

  if (!node->isLeaf) {
      sprintf(result + strlen(result), "%d", node->childPages->elements[q]);
  }

  sprintf(result + strlen(result), "]\n");

  return RC_OK;
}

/*
  - description: Reads a B-tree node from disk into memory
  - param:
    1. bTreeNode - Pointer to the B-tree node that will be created
    2. tree - The B-tree handle
    3. pageNum - The page number of the node to be read
  - return: OK status if successful, or an appropriate error code
*/
RC readNode(BT_Node **bTreeNode, BTreeHandle *tree, int pageNum) {
  RC error;

  // Allocate memory for a page handle
  BM_PageHandle *handleOfPage = new(BM_PageHandle);

  // Attempt to pin the page containing the B-tree node
RC pinResult = pinPage(tree->mgmtData, handleOfPage, pageNum);
if ((error = pinResult) != RC_OK) {
    free(handleOfPage);
    return error;
}
  // Read the header information from the page
  char *poitr = handleOfPage->data;
intptr_t leafIs = (intptr_t)poitr;
poitr += SIZE_INT;
intptr_t filler = (intptr_t)poitr;


  // Skip header bytes of the node
  poitr = handleOfPage->data + BYTES_BT_HEADER_LEN;

  // Create a new B-tree node based on the read information
  BT_Node *_bTreeNode = createBTNode(tree->size, leafIs, pageNum);

  int val, q;

for (q = 0; q < filler; q++) {
    if (!leafIs) {
        int childPage;
        memcpy(&childPage, poitr, SIZE_INT);
        poitr += SIZE_INT;

        int val;
        memcpy(&val, poitr, SIZE_INT);
        poitr += SIZE_INT;

        // Insert values and child pages into the B-tree node
        InsertAtArr(_bTreeNode->value, val, q);
        InsertAtArr(_bTreeNode->childPages, childPage, q);
    } else {
        int ridPage, ridSlot;
        memcpy(&ridPage, poitr, SIZE_INT);
        poitr += SIZE_INT;

        memcpy(&ridSlot, poitr, SIZE_INT);
        poitr += SIZE_INT;

        int val;
        memcpy(&val, poitr, SIZE_INT);
        poitr += SIZE_INT;

        // Insert values, RID pages, and RID slots into the B-tree node
        InsertAtArr(_bTreeNode->value, val, q);
        InsertAtArr(_bTreeNode->leafRIDPages, ridPage, q);
        InsertAtArr(_bTreeNode->leafRIDSlots, ridSlot, q);
    }
}

// Insert the last child page for non-leaf nodes
if (!leafIs) {
    int childPage;
    memcpy(&childPage, poitr, SIZE_INT);
    InsertAtArr(_bTreeNode->childPages, childPage, q
    );
}
  // Unpin the page
  error = unpinPage(tree->mgmtData, handleOfPage);
  free(handleOfPage);

  // Assign the created B-tree node to the output parameter
  *bTreeNode = _bTreeNode;

  return error;
}

RC writeNode(BT_Node *node, BTreeHandle *tree) {
  RC err;
  BM_PageHandle *page = new(BM_PageHandle);

  // Pin the page
  if (RC_OK!=(err = pinPage(tree->mgmtData, page, node->pageNum))) {
    free(page);
    return err;
  }

  char *ptr = page->data;
  memcpy(ptr, &node->isLeaf, SIZE_INT);
  ptr += SIZE_INT;
  memcpy(ptr, &node->value->fill, SIZE_INT);
  ptr = page->data + BYTES_BT_HEADER_LEN; // skip header bytes of the node

  int i;

  // Use ternary operator for the main logic
  for (i = 0; i < node->value->fill; i++) {
    int index = (node->isLeaf) ? i : (node->value->fill - i - 1);

    // Copy child pages or leaf RID pages based on node type
    memcpy(ptr, (node->isLeaf) ? &node->leafRIDPages->elements[index] : &node->childPages->elements[index], SIZE_INT);
    ptr += SIZE_INT;

    // Copy leaf RID slots or values based on node type
    memcpy(ptr, (node->isLeaf) ? &node->leafRIDSlots->elements[index] : &node->value->elements[index], SIZE_INT);
    ptr += SIZE_INT;
  }

  // Additional logic for the last child page
  if (!node->isLeaf) {
    // Only copy the last child page for non-leaf nodes
    memcpy(ptr, &node->childPages->elements[i], SIZE_INT);
  } else {
    // Only copy the last leaf RID page for leaf nodes
    memcpy(ptr, &node->leafRIDPages->elements[i], SIZE_INT);
  }

  // Mark the page as dirty
  int dirty_variable;
  dirty_variable = markDirty(tree->mgmtData, page);
  err = dirty_variable;

  // Unpin the page
  int unpin_page_variable;
  unpin_page_variable = unpinPage(tree->mgmtData, page);
  err = unpin_page_variable;

  // Force flush the buffer pool
  forceFlushPool(tree->mgmtData);

  // Free the allocated memory
  free(page);

  // Return the result
  return err;
}

void printNode(BT_Node *node) {
  if (!node) {
    
    return;
  }

  
  printf("Is Leaf : %d\t Size: %d\t Filled: %d\t pageNum: %d\nNodeData:\t [ ", node->isLeaf, node->size, node->value->fill, node->pageNum);

  int i = 0;

  
  while (i < node->value->fill) {
    int index = (node->isLeaf) ? i : (i + 1);

    // Print child pages or leaf RID pages based on node type
    printf((node->isLeaf) ? "%d.%d , " : "%d , ", node->childPages->elements[index], node->leafRIDSlots->elements[i]);

    // Print values
    printf("<%d>", node->value->elements[i]);

    // Add comma if not the last element
    if (++i < node->value->fill) {
      printf(" , ");
    }
  }

  // Print the last child page or leaf RID page based on node type
  printf((node->isLeaf) ? "%d ]" : "%d ]", node->childPages->elements[i]);

 
}

/*
  - description: Loads B-tree nodes into memory recursively
  - param:
    1. tree - The B-tree handle
    2. root - The root B-tree node
    3. leftOnLevel - Array of B-tree nodes representing the leftmost node on each level
    4. level - The current level of the B-tree
  - return: OK status if successful, or an appropriate error code
*/
RC loadBtreeNodes(BTreeHandle *tree, BT_Node *root, BT_Node **leftOnLevel, int level) {
  RC error;

  if (!root->isLeaf) {
    BT_Node *left = leftOnLevel[level];
    
    for (int i = 0; i < root->childPages->fill; i++) {
      // Read B-tree node from disk
      if ((error = readNode(&root->childNode[i], tree, root->childPages->elements[i]))) {
        return error;
      }

      // Update node relationships
      if (left != NULL) {
        left->right = root->childNode[i];
      }
      root->childNode[i]->left = left;
      left = root->childNode[i];
      root->childNode[i]->parent = root;

      leftOnLevel[level] = left;

      // Recursive call for the next level
      if ((error = loadBtreeNodes(tree, root->childNode[i], leftOnLevel, level + 1))) {
        return error;
      }
    }
  }

  return RC_OK;
}


BT_Node *findNodeByKey(BTreeHandle *tree, int key) {
  BT_Node *rootNode = (*tree).root;
  int indexVal;
  int target;
  while(rootNode != NULL && (*rootNode).isLeaf == 0) {
    indexVal = dynamicArrSearch((*rootNode).value, key, &target);
    if (indexVal >= 0) target += 1;
    rootNode = (*rootNode).childNode[target];
  }
  return rootNode;
}

RC loadBtree(BTreeHandle *tree) {
  RC err;
  tree->root = NULL;
  if (tree->depth) {
    if ((err = readNode(&tree->root, tree, tree->whereIsRoot))) {
      return err;
    }
    BT_Node **leftOnLevel = newArray(BT_Node *, tree->depth);
    for (int i = 0; i < tree->depth; i++) {
      leftOnLevel[i] = NULL;
    }
    err = loadBtreeNodes(tree, tree->root, leftOnLevel, 0);
    free(leftOnLevel);
    return err;
  }
  return RC_OK;
}

/*
  - description: Writes the B-tree header information to disk
  - param:
    1. tree - The B-tree handle
  - return: OK status if successful, or an appropriate error code
*/
RC writeBtreeHeader(BTreeHandle *tree) {
  RC error;

  // Retrieve the buffer pool from the B-tree handle
  BM_BufferPool *bufferPool = tree->mgmtData;

  // Allocate memory for a page handle
  BM_PageHandle *pageHandle = new(BM_PageHandle);

  // Attempt to pin the page containing the B-tree header
  if ((error = pinPage(bufferPool, pageHandle, 0)) != RC_OK) {
    freePointer(1, pageHandle);
    return error;
  }

  // Write B-tree header information to the page
  char *ptr = pageHandle->data;
  memcpy(ptr, &tree->size, SIZE_INT);
  ptr += SIZE_INT;
  memcpy(ptr, &tree->keyType, SIZE_INT);
  ptr += SIZE_INT;
  memcpy(ptr, &tree->whereIsRoot, SIZE_INT);
  ptr += SIZE_INT;
  memcpy(ptr, &tree->numNodes, SIZE_INT);
  ptr += SIZE_INT;
  memcpy(ptr, &tree->numEntries, SIZE_INT);
  ptr += SIZE_INT;
  memcpy(ptr, &tree->depth, SIZE_INT);
  ptr += SIZE_INT;
  memcpy(ptr, &tree->nextPage, SIZE_INT);

  // Mark the page as dirty and unpin it
  error = markDirty(bufferPool, pageHandle);
  error = unpinPage(bufferPool, pageHandle);

  // Force a flush of the buffer pool
  forceFlushPool(bufferPool);

  // Free allocated memory
  freePointer(1, pageHandle);

  return error;
}


/*
  - description: Inserts a key into the B-tree and propagates changes to the parent nodes
  - param:
    1. tree - The B-tree handle
    2. left - The left B-tree node
    3. right - The right B-tree node
    4. key - The key to be inserted
  - return: OK status if successful, or an appropriate error code
*/
RC insPropagateParent(BTreeHandle *tree, BT_Node *left, BT_Node *right, int key) {
  BT_Node *parent = left->parent;
  int index;
  RC error;

  if (parent == NULL) {
    // Create a new parent node
    parent = createBTNode(tree->size, 0, tree->nextPage);
    InsertAtArr(parent->childPages, left->pageNum, 0);
    parent->childNode[0] = left;
    tree->nextPage++;
    tree->whereIsRoot = parent->pageNum;
    tree->numNodes++;
    tree->depth++;
    tree->root = parent;
    writeBtreeHeader(tree);
  }

  right->parent = parent;
  left->parent = parent;

  // Insert key into the parent node
  index = InsertArr(parent->value, key);

  if (index >= 0) {
    index += 1; // next position is the pointer
    InsertAtArr(parent->childPages, right->pageNum, index);

    for (int i = parent->value->fill; i > index; i--) {
      parent->childNode[i] = parent->childNode[i - 1];
    }

    parent->childNode[index] = right;

    return writeNode(parent, tree);
  } else {
    // Parent is full and needs to be split
    BT_Node *overflowed = createBTNode(tree->size + 1, 0, -1);

    // Copy values and pointers from the parent to the overflowed node
    overflowed->value->fill = parent->value->fill;
    overflowed->childPages->fill = parent->childPages->fill;
    memcpy(overflowed->value->elements, parent->value->elements, SIZE_INT * parent->value->fill);
    memcpy(overflowed->childPages->elements, parent->childPages->elements, SIZE_INT * parent->childPages->fill);
    memcpy(overflowed->childNode, parent->childNode, sizeof(BT_Node *) * parent->childPages->fill);

    // Insert the new key and pointer into the overflowed node
    index = InsertArr(overflowed->value, key);
    InsertAtArr(overflowed->childPages, right->pageNum, index + 1);

    for (int i = parent->childPages->fill; i > index + 1; i--) {
      overflowed->childNode[i] = overflowed->childNode[i - 1];
    }

    overflowed->childNode[index + 1] = right;

    int leftFill = overflowed->value->fill / 2;
    int rightFill = overflowed->value->fill - leftFill;
    BT_Node *rightParent = createBTNode(tree->size, 0, tree->nextPage);

    tree->nextPage++;
    tree->numNodes++;

    // Copy values and pointers for left and right childNode of the overflowed node
    parent->value->fill = leftFill;
    parent->childPages->fill = leftFill + 1;
    memcpy(parent->value->elements, overflowed->value->elements, SIZE_INT * leftFill);
    memcpy(parent->childPages->elements, overflowed->childPages->elements, SIZE_INT * parent->childPages->fill);
    memcpy(parent->childNode, overflowed->childNode, sizeof(BT_Node *) * parent->childPages->fill);

    rightParent->value->fill = rightFill;
    rightParent->childPages->fill = overflowed->childPages->fill - parent->childPages->fill;
    memcpy(rightParent->value->elements, overflowed->value->elements + leftFill, SIZE_INT * rightFill);
    memcpy(rightParent->childPages->elements, overflowed->childPages->elements + parent->childPages->fill,
           SIZE_INT * rightParent->childPages->fill);
    memcpy(rightParent->childNode, overflowed->childNode + parent->childPages->fill,
           sizeof(BT_Node *) * rightParent->childPages->fill);

    destroyBTNode(overflowed);

    // Take the median value to the parent
    key = rightParent->value->elements[0];
    DeleteAtArr(rightParent->value, 0, 1);

    // Establish links between nodes
    rightParent->right = parent->right;
    if (rightParent->right != NULL) {
      rightParent->right->left = rightParent;
    }

    parent->right = rightParent;
    rightParent->left = parent;

    writeNode(parent, tree);
    writeNode(rightParent, tree);
    writeBtreeHeader(tree);

    // Recursive call to propagate changes to the parent nodes
    return insPropagateParent(tree, parent, rightParent, key);
  }
}


void freeNodes(BT_Node *root) {
  if(root != NULL){
    BT_Node *child = root;
    while(!(*child).isLeaf) { // Finding the first child
      child = (*child).childNode[0];
    }
    BT_Node *parent = (*child).parent;
    BT_Node *nextNode;
    while (true) {
      while(child != NULL) {
        nextNode = (*child).right;
        destroyBTNode(child);
        child = nextNode;
      }
      if(parent != NULL){
        child = parent;
        parent = (*child).parent;
      }else{
        break;
      }
    }
  }else{
    return ;
  }
}

//functionality

RC initIndexManager (void *mgmtData) {
  return RC_OK;
}

RC shutdownIndexManager () {
  return RC_OK;
}

/*
  - description: Creates a B-tree and initializes the necessary metadata in the file
  - param:
    1. idxId - The identifier for the B-tree file
    2. keyType - The data type of the keys in the B-tree
    3. n - The order of the B-tree
  - return: OK status if successful, or an appropriate error code
*/
RC createBtree (char *idxId, DataType keyType, int n)
{
  int back = (PAGE_SIZE - BYTES_BT_HEADER_LEN) / (3 * SIZE_INT);
  if (n > back) { 
    return RC_IM_N_TO_LAGE;
  }
  RC reco;

  int beco = (reco = createPageFile (idxId));
  if(RC_OK!= beco){
    return reco;
  }

  SM_FileHandle *fileHandels = new(SM_FileHandle);
  int ofp = openPageFile(idxId, fileHandels);
  if (RC_OK != (reco = ofp)) {
    free(fileHandels);
    return reco;
  }

  char *heads = newCleanArray(char, PAGE_SIZE);
  char *pointers = heads;


 int values[] = {n, keyType, 0, 0, 0, 0, 1};

// Calculate the total number of elements in the array
int numElements = sizeof(values) / sizeof(values[0]);

// Use a loop to copy each element to the destination using memcpy
for (int i = 0; i < numElements; ++i) {
    memcpy(pointers, &values[i], SIZE_INT);
    pointers += SIZE_INT;
}

// Write the block to the file
reco = writeBlock(0, fileHandels, heads);

// Check for success or failure
if (reco != RC_OK) {
    // Free allocated memory
    free(fileHandels);
    free(heads);
    return reco;
}

  free(heads);
  reco = closePageFile(fileHandels);
  free(fileHandels);
  return reco;
}

/*
  - description: Opens an existing B-tree and initializes the B-tree handle
  - param:
    1. tree - A pointer to a BTreeHandle pointer that will store the opened B-tree handle
    2. idxId - The identifier for the B-tree file
  - return: OK status if successful, or an appropriate error code
*/
RC openBtree (BTreeHandle **tree, char *idxId){
  BTreeHandle *trees__ = new(BTreeHandle);
  RC err;
  BM_BufferPool *bufferM = NULL;  // Initialize to NULL to safely handle allocation failure

  // Allocate memory for BM_BufferPool and initialize it
  if (!(bufferM = new(BM_BufferPool)) || (err = initBufferPool(bufferM, idxId, PER_IDX_BUF_SIZE, RS_LRU, NULL))) {
    freePointer(2, bufferM, trees__);
    return err;
  }

  BM_PageHandle *handleOfPage = new(BM_PageHandle);

  // Pin the first page using the buffer manager and handle errors
  if (pinPage(bufferM, handleOfPage, 0) != RC_OK) {
    // Free allocated memory for handleOfPage
    freePointer(1, handleOfPage);

    // Clean up and return the error
    freePointer(2, bufferM, trees__);
    return err; // Assuming RC_PIN_ERROR is appropriate for a pinPage error
  }
  char *poitr = handleOfPage->data;

  // Initialize _tree members directly
  trees__->idxId = idxId;
  trees__->mgmtData = bufferM;


  int* treeMembers[] = {
    &trees__->size,
    (int*)&trees__->keyType,
    &trees__->whereIsRoot,
    &trees__->numNodes,
    &trees__->numEntries,
    &trees__->depth,
    &trees__->nextPage
  };

  // Iterate over treeMembers array and copy values from ptr
  for (int i = 0; i < sizeof(treeMembers) / sizeof(treeMembers[0]); ++i) {
    memcpy(treeMembers[i], poitr, SIZE_INT);
    poitr += SIZE_INT;
  }


  // Unpin the page using the buffer manager
  err = unpinPage(bufferM, handleOfPage);
  freePointer(1, handleOfPage);

  if (err != RC_OK) {
    closeBtree(trees__);
    return err;
  }

  // Load the B-tree
  if ((err = loadBtree(trees__)) != RC_OK) {
      closeBtree(trees__);
      return err;
  }

  // Assign the B-tree to the output pointer
  *tree = trees__;

  return RC_OK;

}

/*
  - description: Closes a B-tree, releasing associated resources
  - param:
    1. tree - The B-tree handle
  - return: OK status if successful, or an appropriate error code
*/
RC closeBtree(BTreeHandle *tree) {
    // Shutdown the buffer pool associated with the B-tree
    shutdownBufferPool(tree->mgmtData);

    // Free allocated memory for the B-tree handle and buffer pool
    freePointer(2, tree->mgmtData, tree);

    // Free allocated memory for B-tree nodes
    freeNodes(tree->root);

    // Return OK if the B-tree is successfully closed
    return RC_OK;
}

/*
  - description: Deletes a B-tree file
  - param:
    1. idxId - The identifier for the B-tree file
  - return: RC_FILE_NOT_FOUND if the file does not exist,
            RC_FS_ERROR if there is an error in unlinking,
            or RC_OK if the file is successfully unlinked
*/
RC deleteBtree (char *idxId) {

// Use the ternary operator to check if the file exists and unlink it accordingly.
// Return RC_FILE_NOT_FOUND if the file does not exist,
// RC_FS_ERROR if there is an error in unlinking,
// or RC_OK if the file is successfully unlinked.
return (access(idxId, F_OK) == -1) ? RC_FILE_NOT_FOUND : (unlink(idxId) == -1) ? RC_FS_ERROR : RC_OK;

}

/*
  - description: Retrieves the number of nodes in the B-tree
  - param:
    1. tree - The B-tree handle
    2. result - A pointer to an integer that will store the result (number of nodes)
  - return: OK status if successful, or an appropriate error code
*/
RC getNumNodes(BTreeHandle *tree, int *result) {
    // Pointer to store the result
    int *getNumNodes_variable = result;

    // Assign the value of numNodes from the B-tree handle to the result
    *getNumNodes_variable = tree->numNodes;

    // Return OK status to indicate successful execution
    return RC_OK;
}

/*
  - description: Retrieves the number of entries (key-value pairs) in the B-tree
  - param:
    1. tree - The B-tree handle
    2. result - A pointer to an integer that will store the result (number of entries)
  - return: OK status if successful, or an appropriate error code
*/
RC getNumEntries(BTreeHandle *tree, int *result) {
    // Pointer to store the result
    int *getNumEntries_variable = result;

    // Assign the value of numEntries from the B-tree handle to the result
    *getNumEntries_variable = tree->numEntries;

    // Return OK status to indicate successful execution
    return RC_OK;
}

/*
  - description: Retrieves the data type of the keys in the B-tree
  - param:
    1. tree - The B-tree handle
    2. result - A pointer to a DataType that will store the result (data type of keys)
  - return: OK status if successful, or an appropriate error code
*/
RC getKeyType(BTreeHandle *tree, DataType *result) {
    // Pointer to store the result
    DataType *getKeyType_variable = result;

    // Assign the value of keyType from the B-tree handle to the result
    *getKeyType_variable = tree->keyType;

    // Return OK status to indicate successful execution
    return RC_OK;
}


/*
	- description : Finds the key in the B-tree and returns the corresponding RID (Record ID).
	- param : 
		1. tree - The B-tree handle
		2. key - The key to be found
    3. result -  The pointer to the result RID
	- return : RC_OK if the key is found and the RID is set successfully,
             RC_IM_KEY_NOT_FOUND if the key is not found
*/
RC findKey(BTreeHandle *tree, Value *key, RID *result)
{
  BT_Node *targetNode = findNodeByKey(tree, key->v.intV);
  int target = 0;
  int i = dynamicArrSearch(targetNode->value, key->v.intV,
                         &target); // target is the index of the key
  if (i >= 0)
  {
    result->page = targetNode->leafRIDPages->elements[i];
    result->slot = targetNode->leafRIDSlots->elements[i];
    return RC_OK; // SUCCESS
  }
  return RC_IM_KEY_NOT_FOUND; // RC_IM_KEY_NOT_FOUND because it is an error
}

/*
	- description : Inserts a key-value pair into a B-tree
	- param : 
		1. tree - The B-tree handle
		2. key - The key to be found
    3. rid - The record ID associated with the key
	- return : RC_OK if the insertion is successful, RC_IM_KEY_ALREADY_EXISTS if the key already exists in the B-tree, or an appropriate error code
*/
RC insertKey(BTreeHandle *tree, Value *key, RID rid)
{
  BT_Node *targetNode = findNodeByKey(tree, key->v.intV);

  if (targetNode == NULL)
  { // first check NULL to create a new node
    targetNode = createBTNode(tree->size, 1, tree->nextPage);
    // fill the node
    tree->root = targetNode;
    tree->whereIsRoot = targetNode->pageNum;
    tree->nextPage = tree->nextPage + 1;
    tree->numNodes = tree->numNodes + 1;
    tree->depth = tree->depth + 1;
    writeBtreeHeader(tree); // write the header
  }

  int target = 0;
  int i = dynamicArrSearch(targetNode->value, key->v.intV, &target);
  if (i >= 0)
  {                                  // if it is already there
    return RC_IM_KEY_ALREADY_EXISTS; //  return RC_IM_KEY_ALREADY_EXISTS because it is an error
  }
  // otherwise insert
  i = InsertArr(targetNode->value, key->v.intV);

  if (i >= 0)
  {                                                    // if it is not full
    InsertAtArr(targetNode->leafRIDPages, rid.page, i); // insert the RID page
    InsertAtArr(targetNode->leafRIDSlots, rid.slot, i); // insert the RID slot
    tree->numEntries = tree->numEntries + 1;           // increase the number of entries
    writeBtreeHeader(tree);                            // write the header
    return RC_OK;                                      // SUCCESS
  }
  else
  {                                                           // else it is full and we need to split
    BT_Node *dividable = createBTNode(tree->size + 1, 1, -1); // +1 because it is the number of values
    memcpy(dividable->value->elements, targetNode->value->elements, SIZE_INT * targetNode->value->fill);
    dividable->value->fill = targetNode->value->fill;
    memcpy(dividable->leafRIDPages->elements, targetNode->leafRIDPages->elements, SIZE_INT * targetNode->leafRIDPages->fill);
    dividable->leafRIDPages->fill = targetNode->leafRIDPages->fill;
    memcpy(dividable->leafRIDSlots->elements, targetNode->leafRIDSlots->elements, SIZE_INT * targetNode->leafRIDSlots->fill);
    dividable->leafRIDSlots->fill = targetNode->leafRIDSlots->fill;
    i = InsertArr(dividable->value, key->v.intV);
    InsertAtArr(dividable->leafRIDPages, rid.page, i);
    InsertAtArr(dividable->leafRIDSlots, rid.slot, i);

    // split
    int lFill = ceil((float)dividable->value->fill / 2);
    int rFill = dividable->value->fill - lFill;
    BT_Node *rightChild = createBTNode(tree->size, 1, tree->nextPage); // create the right node
    tree->nextPage++;
    tree->numNodes++;

    // Since eividable is sorted then it is safe to just copy the content
    // copy left child
    targetNode->leafRIDSlots->fill = lFill;
    targetNode->leafRIDPages->fill = targetNode->leafRIDSlots->fill;
    targetNode->value->fill = targetNode->leafRIDPages->fill;
    // copy the content
    memcpy(targetNode->value->elements, dividable->value->elements, SIZE_INT * lFill);
    memcpy(targetNode->leafRIDPages->elements, dividable->leafRIDPages->elements, SIZE_INT * lFill);
    memcpy(targetNode->leafRIDSlots->elements, dividable->leafRIDSlots->elements, SIZE_INT * lFill);

    // copy right child
    rightChild->leafRIDSlots->fill = rFill;
    rightChild->leafRIDPages->fill = rightChild->leafRIDSlots->fill;
    rightChild->value->fill = rightChild->leafRIDPages->fill;
    // copy the content
    memcpy(rightChild->value->elements, dividable->value->elements + lFill, SIZE_INT * rFill);
    memcpy(rightChild->leafRIDPages->elements, dividable->leafRIDPages->elements + lFill, SIZE_INT * rFill);
    memcpy(rightChild->leafRIDSlots->elements, dividable->leafRIDSlots->elements + lFill, SIZE_INT * rFill);

    destroyBTNode(dividable); // destroy the dividable node

    // adjust the pointers
    rightChild->right = targetNode->right;
    if (rightChild->right != NULL)
    {
      rightChild->right->left = rightChild;
    }
    targetNode->right = rightChild;
    rightChild->left = targetNode;
    writeNode(rightChild, tree);
    writeNode(targetNode, tree);
    insPropagateParent(tree, targetNode, rightChild, rightChild->value->elements[0]);

    tree->numEntries = tree->numEntries + 1; // increase the number of entries
    writeBtreeHeader(tree);                  // write the header
    return RC_OK;                            // SUCCESS
  }
  return RC_OK; // SUCCESS
}

/*
	- description :  * Deletes a key from the B-tree.
  This function deletes the specified key from the B-tree. It first searches for the node
  containing the key, and if found, removes the key from the node's values array, as well as
  the corresponding entries in the leafRIDPages and leafRIDSlots arrays. The number of entries
  in the B-tree is decremented by 1. Finally, the modified node and B-tree header are written
  back to disk.
	- param : 
		1. tree - The B-tree handle
		2. key - The key to be found
    3. rid - The record ID associated with the key
	- return : RC_OK if the key is successfully deleted, or an appropriate error code otherwise
*/
RC deleteKey(BTreeHandle *tree, Value *key)
{
  BT_Node *targetNode = findNodeByKey(tree, key->v.intV);
  int target = 0;
  if (targetNode == NULL)
  {               // first check NULL to short circuit
    return RC_OK; // it is not RC_IM_KEY_NOT_FOUND because it is not an
                  // error
  }
  int i = dynamicArrSearch(targetNode->value, key->v.intV,
                         &target); // target is the index of the key
  if (i < 0)
  {
    return RC_OK; // it is not RC_IM_KEY_NOT_FOUND because it is not an
                  // error
  }
  DeleteAtArr((*targetNode).leafRIDPages, i, 1);
  DeleteAtArr((*targetNode).value, i, 1);
  DeleteAtArr((*targetNode).leafRIDSlots, i, 1);
  tree->numEntries = tree->numEntries - 1;
  writeBtreeHeader(tree);
  writeNode(targetNode, tree);
  return RC_OK; // SUCCESS
}


/*
  - description: Opens a B-tree scan and initializes the scan handle
  - param:
    1. tree - The B-tree handle
    2. handle - A pointer to a BT_ScanHandle pointer that will store the scan handle
  - return: RC OK status if successful, or an appropriate error code
*/
RC openTreeScan (BTreeHandle *tree, BT_ScanHandle **handle){
  ScanMgmtInfo *scanMgmDetails = new(ScanMgmtInfo);
  BT_ScanHandle *bt_scanHandle = new(BT_ScanHandle);
  // Updating the current Node to root
  (*scanMgmDetails).currentNode = (*tree).root;
  (*bt_scanHandle).tree = tree;
  // Will only pass when the condition is satisfied 
  while(!(*scanMgmDetails).currentNode->isLeaf) {
    (*scanMgmDetails).currentNode = (*scanMgmDetails).currentNode->childNode[0];
  }
  // Updating the management Data
  (*bt_scanHandle).mgmtData = scanMgmDetails;
  // Updating the element index to 0
  (*scanMgmDetails).elementIndex = 0;
  // updating the handle with the recently changed bt_scanHandle
  *handle = bt_scanHandle;
  return RC_OK;
}

/*
  - description: Closes a B-tree scan and releases associated resources
  - param:
    1. handle - The scan handle to be closed
  - return: OK status if successful, or an appropriate error code
*/
RC closeTreeScan (BT_ScanHandle *handle){
  // Freeing the handle 
  freePointer(2, (*handle).mgmtData, handle);
  return RC_OK;
}


/*
  - description: Moves to the next entry in the B-tree scan and retrieves the RID
  - param:
    1. handle - The scan handle
    2. result - A pointer to a RID that will store the result
  - return: OK status if successful, RC_IM_NO_MORE_ENTRIES if there are no more entries, or an appropriate error code
*/
RC nextEntry (BT_ScanHandle *handle, RID *result){
  ScanMgmtInfo *scanMgmtInfo = handle->mgmtData;
  //Will pass only if the condition is correct
  if((*scanMgmtInfo).elementIndex >= (*scanMgmtInfo).currentNode->leafRIDPages->fill) { // finding the left most leaf
    // Will search next node
    if((*scanMgmtInfo).elementIndex == (*scanMgmtInfo).currentNode->value->fill){
      // Return No More Entries if Right lead is null
      if((*scanMgmtInfo).currentNode->right==NULL) return RC_IM_NO_MORE_ENTRIES;
      else {
        // Will change the current Node to right Node
        (*scanMgmtInfo).currentNode = (*scanMgmtInfo).currentNode->right;
        // Will update the element Index to 0
        (*scanMgmtInfo).elementIndex = 0;
      }
    } else {
      // Will change the current Node to right Node
      (*scanMgmtInfo).currentNode = (*scanMgmtInfo).currentNode->right;
      // Will update the element Index to 0
      (*scanMgmtInfo).elementIndex = 0;
    }
  }
  // Updating the page and slot of the result
  (*result).page = (*scanMgmtInfo).currentNode->leafRIDPages->elements[(*scanMgmtInfo).elementIndex];
  (*result).slot = (*scanMgmtInfo).currentNode->leafRIDSlots->elements[(*scanMgmtInfo).elementIndex];
  // Increment the element Idex
  (*scanMgmtInfo).elementIndex++;
  return RC_OK;
}

char *printTree (BTreeHandle *tree){
  char *result = newCharArr((*tree).numNodes * (*tree).size * 11 + (*tree).size + (*tree).numNodes + 14);
  BT_Node *rootNode = (*tree).root;
  int incVal=0;
  if(result == NULL) return result;
  while(rootNode!=NULL){
    printNodeHelper(rootNode, result);
    if((*rootNode).isLeaf == 0 && (*rootNode).right == NULL){
      BT_Node *temp = (*tree).root;
      for(int i=0; i<=incVal;i++){
        temp=temp->childNode[0];
      }
      rootNode = temp;
      ++incVal;
    }else{
      rootNode = (*rootNode).right;
    }
  }
  return result;
}
