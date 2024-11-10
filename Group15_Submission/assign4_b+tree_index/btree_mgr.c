#include "btree_mgr.h"
#include "string.h"
#include "storage_mgr.h"
#include "buffer_mgr.h"
#include <stdarg.h>
#include <unistd.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>


// Function prototypes
static int hash(int key);
static BNode* createNode(int key, void *val);

HashMap* hmInit(void) {
    HashMap *hm = calloc(1, sizeof(HashMap));
    return hm;  // calloc initializes all pointers to NULL
}

static int hash(int key) {
    unsigned h = HASH_MULTIPLIER;
    return ((h * HASH_LEN) ^ (key * HASH_LEN)) % HASH_LEN;
}

static BNode* createNode(int key, void *val) {
    HM_Comb *comb = malloc(sizeof(HM_Comb));
    if (!comb) return NULL;
    comb->key = key;
    comb->val = val;

    BNode *newNode = malloc(sizeof(BNode));
    if (!newNode) {
        free(comb);
        return NULL;
    }
    newNode->data = comb;
    newNode->next = NULL;
    newNode->previous = NULL;
    return newNode;
}

HM_Comb* getComb(HashMap *hm, int key) {
    int index = hash(key);
    for (BNode *current = hm->tbl[index]; current; current = current->next) {
        HM_Comb *comb = (HM_Comb*)current->data;
        if (comb->key == key) {
            return comb;
        }
    }
    return NULL;
}

bool hmInsert(HashMap *hm, int key, void *val) {
    int index = hash(key);
    for (BNode *current = hm->tbl[index]; current; current = current->next) {
        HM_Comb *comb = (HM_Comb*)current->data;
        if (comb->key == key) {
            comb->val = val;  // Update existing value
            return false;  // Key already existed
        }
    }

    BNode *newNode = createNode(key, val);
    if (!newNode) return false;  // Memory allocation failed

    newNode->next = hm->tbl[index];
    if (hm->tbl[index]) {
        hm->tbl[index]->previous = newNode;
    }
    hm->tbl[index] = newNode;
    return true;  // New key inserted
}

void* hmGet(HashMap *hm, int key) {
    HM_Comb *comb = getComb(hm, key);
    return comb ? comb->val : NULL;
}

bool hmDelete(HashMap *hm, int key) {
    int index = hash(key);
    BNode *current = hm->tbl[index];
    BNode *prev = NULL;

    while (current != NULL) {
        HM_Comb *comb = (HM_Comb *)current->data;
        if (comb->key == key) {
            if (prev == NULL) {
                hm->tbl[index] = current->next;
            } else {
                prev->next = current->next;
            }

            if (current->next != NULL) {
                current->next->previous = prev;
            }

            free(comb);
            free(current);
            return true;
        }
        prev = current;
        current = current->next;
    }
    return false;
}

void hmDestroy(HashMap *hm) {
    for (int i = 0; i < HASH_LEN; i++) {
        BNode *current = hm->tbl[i];
        while (current != NULL) {
            BNode *next = current->next;
            free(current->data);
            free(current);
            current = next;
        }
    }
    free(hm);
}

int dynamicArrSearch(dynamicArr *arr, int elem, int *fitOn) {
    if (arr->fill == 0) {
        *fitOn = 0;
        return -1;
    }

    int low = 0;
    int high = arr->fill - 1;

    while (low <= high) {
        int mid = low + (high - low) / 2;  // Avoid potential overflow

        if (elem == arr->elems[mid]) {
            // Find the first occurrence of the element
            while (mid > 0 && elem == arr->elems[mid - 1]) {
                mid--;
            }
            *fitOn = mid;
            return mid;
        }

        if (elem < arr->elems[mid]) {
            high = mid - 1;
        } else {
            low = mid + 1;
        }
    }

    *fitOn = low;
    return -1;
}

dynamicArr* saInit(int size) {
    dynamicArr *arr = malloc(sizeof(dynamicArr));
    if (arr == NULL) return NULL;

    arr->elems = malloc(size * sizeof(int));
    if (arr->elems == NULL) {
        free(arr);
        return NULL;
    }

    arr->size = size;
    arr->fill = 0;
    return arr;
}

void saDestroy(dynamicArr *arr) {
    if (arr != NULL) {
        free(arr->elems);
        free(arr);
    }
}

int saInsertAt(dynamicArr *arr, int elem, int index) {
    if (arr == NULL || arr->fill >= arr->size || index > arr->fill) {
        return -1;
    }

    if (index < arr->fill) {
        memmove(&arr->elems[index + 1], &arr->elems[index], (arr->fill - index) * sizeof(int));
    }

    arr->elems[index] = elem;
    arr->fill++;
    return index;
}

int saInsert(dynamicArr *arr, int elem) {
    if (arr->fill >= arr->size) {
        return -1;  // Array is full
    }

    int fitOn;
    int index = dynamicArrSearch(arr, elem, &fitOn);

    if (index < 0) {
        // Element not found, insert at fitOn
        memmove(&arr->elems[fitOn + 1], &arr->elems[fitOn], (arr->fill - fitOn) * sizeof(int));
        arr->elems[fitOn] = elem;
        arr->fill++;
    }

    return fitOn;
}

void saDeleteAt(dynamicArr *arr, int index, int count) {
    if (index < 0 || index >= arr->fill || count <= 0) {
        return;  // Invalid input
    }

    count = (index + count > arr->fill) ? (arr->fill - index) : count;
    memmove(&arr->elems[index], &arr->elems[index + count], (arr->fill - index - count) * sizeof(int));
    arr->fill -= count;
}

int saDeleteOne(dynamicArr *arr, int elem) {
    int fitOn;
    int index = dynamicArrSearch(arr, elem, &fitOn);

    if (index >= 0) {
        saDeleteAt(arr, index, 1);
    }

    return index;
}

int saDeleteAll(dynamicArr *arr, int elem) {
    int fitOn;
    int index = dynamicArrSearch(arr, elem, &fitOn);

    if (index >= 0) {
        int count = 1;
        while (index + count < arr->fill && elem == arr->elems[index + count]) {
            count++;
        }
        saDeleteAt(arr, index, count);
    }

    return index;
}

inline void saEmpty(dynamicArr *arr) {
    arr->fill = 0;
}

void freePointers(int count, ...) {
    va_list pointerList;
    va_start(pointerList, count);

    for (int i = 0; i < count; i++) {
        void *ptr = va_arg(pointerList, void *);
        free(ptr);
    }

    va_end(pointerList);
}

void freePointer(int count, ...) {
    va_list args;
    va_start(args, count);

    for (int i = 0; i < count; i++) {
        void *ptr = va_arg(args, void *);
        free(ptr);
    }

    va_end(args);
}

// Static helper function prototypes
static void initializeNodeCommonFields(BT_Node *node, int size, int isLeaf, int pageNum);
static void initializeNonLeafNode(BT_Node *node, int size);
static void initializeLeafNode(BT_Node *node, int size);
static void destroyNodeCommonFields(BT_Node *node);
static void destroyNonLeafNode(BT_Node *node);
static void destroyLeafNode(BT_Node *node);
static void printLeafNodeData(BT_Node *node, char *result);
static void printNonLeafNodeData(BT_Node *node, char *result);

BT_Node *createBTNode(int size, int isLeaf, int pageNum) {
    BT_Node *targetNode = new(BT_Node);
    initializeNodeCommonFields(targetNode, size, isLeaf, pageNum);

    if (!isLeaf) {
        initializeNonLeafNode(targetNode, size);
    } else {
        initializeLeafNode(targetNode, size);
    }

    return targetNode;
}

static void initializeNodeCommonFields(BT_Node *node, int size, int isLeaf, int pageNum) {
    node->right = NULL;
    node->left = NULL;
    node->parent = NULL;
    node->size = size;
    node->pageNum = pageNum;
    node->vals = saInit(size);
    node->isLeaf = isLeaf;
}

static void initializeNonLeafNode(BT_Node *node, int size) {
    node->childrenPages = saInit(size + 1);
    node->children = newArray(BT_Node *, size + 1);
}

static void initializeLeafNode(BT_Node *node, int size) {
    node->leafRIDPages = saInit(size);
    node->leafRIDSlots = saInit(size);
}

void destroyBTNode(BT_Node *node) {
    destroyNodeCommonFields(node);

    if (!node->isLeaf) {
        destroyNonLeafNode(node);
    } else {
        destroyLeafNode(node);
    }

    free(node);
}

static void destroyNodeCommonFields(BT_Node *node) {
    saDestroy(node->vals);
}

static void destroyNonLeafNode(BT_Node *node) {
    saDestroy(node->childrenPages);
    free(node->children);
}

static void destroyLeafNode(BT_Node *node) {
    saDestroy(node->leafRIDPages);
    saDestroy(node->leafRIDSlots);
}

RC printNodeHelper(BT_Node *node, char *result) {
    if (node == NULL) {
        sprintf(result + strlen(result), "NULL BNode !!\n");
        return RC_GENERAL_ERROR;
    }

    sprintf(result + strlen(result), "(%d)[", node->pageNum);

    if (node->isLeaf) {
        printLeafNodeData(node, result);
    } else {
        printNonLeafNodeData(node, result);
    }

    sprintf(result + strlen(result), "]\n");
    return RC_OK;
}

static void printLeafNodeData(BT_Node *node, char *result) {
    for (int i = 0; i < node->vals->fill; i++) {
        sprintf(result + strlen(result), "%d.%d,%d",
                node->leafRIDPages->elems[i],
                node->leafRIDSlots->elems[i],
                node->vals->elems[i]);

        if (i < node->vals->fill - 1) {
            sprintf(result + strlen(result), ",");
        }
    }
}

static void printNonLeafNodeData(BT_Node *node, char *result) {
    int i;
    for (i = 0; i < node->vals->fill; i++) {
        sprintf(result + strlen(result), "%d,%d,",
                node->childrenPages->elems[i],
                node->vals->elems[i]);
    }
    sprintf(result + strlen(result), "%d", node->childrenPages->elems[i]);
}

// Static helper function prototypes
static RC pinAndGetPage(BTreeHandle *tree, BM_PageHandle **handleOfPage, int pageNum);
static void readNodeHeader(char *ptr, intptr_t *leafIs, intptr_t *filler);
static void readNodeData(BT_Node *node, char *ptr, int filler, int leafIs);
static void readLeafNodeData(BT_Node *node, char *ptr, int index);
static void readNonLeafNodeData(BT_Node *node, char *ptr, int index);
static void writeNodeHeader(char *ptr, BT_Node *node);
static void writeNodeData(char *ptr, BT_Node *node);
static RC markDirtyAndUnpin(BTreeHandle *tree, BM_PageHandle *page);

RC readNode(BT_Node **bTreeNode, BTreeHandle *tree, int pageNum) {
    RC error;
    BM_PageHandle *handleOfPage;

    if ((error = pinAndGetPage(tree, &handleOfPage, pageNum)) != RC_OK) {
        return error;
    }

    char *ptr = handleOfPage->data;
    intptr_t leafIs, filler;
    readNodeHeader(ptr, &leafIs, &filler);

    BT_Node *_bTreeNode = createBTNode(tree->size, leafIs, pageNum);
    readNodeData(_bTreeNode, ptr + BYTES_BT_HEADER_LEN, filler, leafIs);

    error = unpinPage(tree->mgmtData, handleOfPage);
    free(handleOfPage);
    *bTreeNode = _bTreeNode;
    return error;
}

static RC pinAndGetPage(BTreeHandle *tree, BM_PageHandle **handleOfPage, int pageNum) {
    *handleOfPage = new(BM_PageHandle);
    RC pinResult = pinPage(tree->mgmtData, *handleOfPage, pageNum);
    if (pinResult != RC_OK) {
        free(*handleOfPage);
        return pinResult;
    }
    return RC_OK;
}

static void readNodeHeader(char *ptr, intptr_t *leafIs, intptr_t *filler) {
    *leafIs = (intptr_t)ptr;
    ptr += SIZE_INT;
    *filler = (intptr_t)ptr;
}

static void readNodeData(BT_Node *node, char *ptr, int filler, int leafIs) {
    for (int q = 0; q < filler; q++) {
        if (leafIs) {
            readLeafNodeData(node, ptr, q);
        } else {
            readNonLeafNodeData(node, ptr, q);
        }
        ptr += 3 * SIZE_INT;
    }
    if (!leafIs) {
        int childPage;
        memcpy(&childPage, ptr, SIZE_INT);
        saInsertAt(node->childrenPages, childPage, filler);
    }
}

static void readLeafNodeData(BT_Node *node, char *ptr, int index) {
    int ridPage, ridSlot, val;
    memcpy(&ridPage, ptr, SIZE_INT);
    memcpy(&ridSlot, ptr + SIZE_INT, SIZE_INT);
    memcpy(&val, ptr + 2 * SIZE_INT, SIZE_INT);
    saInsertAt(node->vals, val, index);
    saInsertAt(node->leafRIDPages, ridPage, index);
    saInsertAt(node->leafRIDSlots, ridSlot, index);
}

static void readNonLeafNodeData(BT_Node *node, char *ptr, int index) {
    int childPage, val;
    memcpy(&childPage, ptr, SIZE_INT);
    memcpy(&val, ptr + SIZE_INT, SIZE_INT);
    saInsertAt(node->vals, val, index);
    saInsertAt(node->childrenPages, childPage, index);
}

RC writeNode(BT_Node *node, BTreeHandle *tree) {
    RC err;
    BM_PageHandle *page;

    if ((err = pinAndGetPage(tree, &page, node->pageNum)) != RC_OK) {
        return err;
    }

    char *ptr = page->data;
    writeNodeHeader(ptr, node);
    writeNodeData(ptr + BYTES_BT_HEADER_LEN, node);

    err = markDirtyAndUnpin(tree, page);
    forceFlushPool(tree->mgmtData);
    free(page);
    return err;
}

static void writeNodeHeader(char *ptr, BT_Node *node) {
    memcpy(ptr, &node->isLeaf, SIZE_INT);
    ptr += SIZE_INT;
    memcpy(ptr, &node->vals->fill, SIZE_INT);
}

static void writeNodeData(char *ptr, BT_Node *node) {
    for (int i = 0; i < node->vals->fill; i++) {
        int index = (node->isLeaf) ? i : (node->vals->fill - i - 1);
        memcpy(ptr, (node->isLeaf) ? &node->leafRIDPages->elems[index] : &node->childrenPages->elems[index], SIZE_INT);
        ptr += SIZE_INT;
        memcpy(ptr, (node->isLeaf) ? &node->leafRIDSlots->elems[index] : &node->vals->elems[index], SIZE_INT);
        ptr += SIZE_INT;
    }
    if (!node->isLeaf) {
        memcpy(ptr, &node->childrenPages->elems[node->vals->fill], SIZE_INT);
    } else {
        memcpy(ptr, &node->leafRIDPages->elems[node->vals->fill], SIZE_INT);
    }
}

static RC markDirtyAndUnpin(BTreeHandle *tree, BM_PageHandle *page) {
    RC err = markDirty(tree->mgmtData, page);
    if (err == RC_OK) {
        err = unpinPage(tree->mgmtData, page);
    }
    return err;
}

// Static helper function prototypes
static void printLeafNode(BT_Node *node);
static void printNonLeafNode(BT_Node *node);
static RC loadBtreeNodesRecursive(BTreeHandle *tree, BT_Node *root, BT_Node **leftOnLevel, int level);
static RC readNodeAndSetParent(BT_Node **child, BTreeHandle *tree, int pageNum, BT_Node *parent);
static void updateSiblingPointersForLoading(BT_Node *left, BT_Node *current);
static BT_Node *findLeafNode(BTreeHandle *tree, int key);
static RC initializeLeftOnLevel(BT_Node **leftOnLevel, int depth);
static RC writeTreeHeaderToPage(BTreeHandle *tree, char *ptr);

void printNode(BT_Node *node) {
    if (!node) {
        printf("NULL BNode !!\n");
        return;
    }
    printf("\nDetails of node==>\n");
    printf("Is Leaf : %d\t Size: %d\t Filled: %d\t pageNum: %d\nNodeData:\t [ ",
           node->isLeaf, node->size, node->vals->fill, node->pageNum);

    if (node->isLeaf) {
        printLeafNode(node);
    } else {
        printNonLeafNode(node);
    }

    printf("\n------------\n");
}

static void printLeafNode(BT_Node *node) {
    for (int i = 0; i < node->vals->fill; i++) {
        printf("%d.%d , ", node->childrenPages->elems[i], node->leafRIDSlots->elems[i]);
        printf("<%d>", node->vals->elems[i]);
        if (i + 1 < node->vals->fill) {
            printf(" , ");
        }
    }
    printf("%d ]", node->childrenPages->elems[node->vals->fill]);
}

static void printNonLeafNode(BT_Node *node) {
    for (int i = 0; i < node->vals->fill; i++) {
        printf("%d , ", node->childrenPages->elems[i + 1]);
        printf("<%d>", node->vals->elems[i]);
        if (i + 1 < node->vals->fill) {
            printf(" , ");
        }
    }
    printf("%d ]", node->childrenPages->elems[node->vals->fill]);
}

RC loadBtreeNodes(BTreeHandle *tree, BT_Node *root, BT_Node **leftOnLevel, int level) {
    return loadBtreeNodesRecursive(tree, root, leftOnLevel, level);
}

static RC loadBtreeNodesRecursive(BTreeHandle *tree, BT_Node *root, BT_Node **leftOnLevel, int level) {
    BT_Node *left = leftOnLevel[level];
    RC err;

    if (root->isLeaf == 0) {
        for (int i = 0; i < root->childrenPages->fill; i++) {
            if ((err = readNodeAndSetParent(&root->children[i], tree, root->childrenPages->elems[i], root))) {
                return err;
            }

            updateSiblingPointersForLoading(left, root->children[i]);
            left = root->children[i];
            leftOnLevel[level] = left;

            if ((err = loadBtreeNodesRecursive(tree, root->children[i], leftOnLevel, level + 1))) {
                return err;
            }
        }
    }
    return RC_OK;
}

static RC readNodeAndSetParent(BT_Node **child, BTreeHandle *tree, int pageNum, BT_Node *parent) {
    RC err = readNode(child, tree, pageNum);
    if (err == RC_OK) {
        (*child)->parent = parent;
    }
    return err;
}

static void updateSiblingPointersForLoading(BT_Node *left, BT_Node *current) {
    if (left != NULL) {
        left->right = current;
        current->left = left;
    }
}

BT_Node *findNodeByKey(BTreeHandle *tree, int key) {
    return findLeafNode(tree, key);
}

static BT_Node *findLeafNode(BTreeHandle *tree, int key) {
    BT_Node *current = tree->root;
    int index, fitOn;
    while(current != NULL && !current->isLeaf) {
        index = dynamicArrSearch(current->vals, key, &fitOn);
        if (index >= 0) {
            fitOn += 1;
        }
        current = current->children[fitOn];
    }
    return current;
}

RC loadBtree(BTreeHandle *tree) {
    RC err;
    tree->root = NULL;
    if (tree->depth) {
        if ((err = readNode(&tree->root, tree, tree->whereIsRoot))) {
            return err;
        }
        BT_Node **leftOnLevel = newArray(BT_Node *, tree->depth);
        if ((err = initializeLeftOnLevel(leftOnLevel, tree->depth))) {
            free(leftOnLevel);
            return err;
        }
        err = loadBtreeNodes(tree, tree->root, leftOnLevel, 0);
        free(leftOnLevel);
        return err;
    }
    return RC_OK;
}

static RC initializeLeftOnLevel(BT_Node **leftOnLevel, int depth) {
    for (int i = 0; i < depth; i++) {
        leftOnLevel[i] = NULL;
    }
    return RC_OK;
}

RC writeBtreeHeader(BTreeHandle *tree) {
    RC err;
    BM_BufferPool *bm = tree->mgmtData;
    BM_PageHandle *page = new(BM_PageHandle);
    if (RC_OK != (err = pinPage(bm, page, 0))) {
        freePointer(1, page);
        return err;
    }

    err = writeTreeHeaderToPage(tree, page->data);

    if (err == RC_OK) {
        err = markDirty(bm, page);
    }
    if (err == RC_OK) {
        err = unpinPage(bm, page);
    }
    forceFlushPool(bm);
    freePointer(1, page);
    return err;
}

static RC writeTreeHeaderToPage(BTreeHandle *tree, char *ptr) {
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
    return RC_OK;
}

// Function prototypes
static BT_Node* createNewRoot(BTreeHandle* tree, BT_Node* left);
static RC insertIntoNonFullParent(BT_Node* parent, BT_Node* right, int index, BTreeHandle* tree);
static RC handleParentOverflow(BTreeHandle* tree, BT_Node* parent, BT_Node* right, int key);
static BT_Node* createOverflowedNode(BTreeHandle* tree, BT_Node* parent);
static void insertIntoOverflowedNode(BT_Node* overflowed, BT_Node* right, int key, int index);
static BT_Node* splitOverflowedNode(BTreeHandle* tree, BT_Node* parent, BT_Node* overflowed);
static void updateSiblingPointersForSplitting(BT_Node* parent, BT_Node* rightParent);

RC insPropagateParent(BTreeHandle *tree, BT_Node *left, BT_Node *right, int key) {
    BT_Node *parent = left->parent;
    int index;

    if (parent == NULL) {
        parent = createNewRoot(tree, left);
    }

    right->parent = parent;
    left->parent = parent;
    index = saInsert(parent->vals, key);

    if (index >= 0) {
        return insertIntoNonFullParent(parent, right, index, tree);
    } else {
        return handleParentOverflow(tree, parent, right, key);
    }
}

static BT_Node* createNewRoot(BTreeHandle* tree, BT_Node* left) {
    BT_Node* parent = createBTNode(tree->size, 0, tree->nextPage);
    saInsertAt(parent->childrenPages, left->pageNum, 0);
    parent->children[0] = left;
    tree->nextPage++;
    tree->whereIsRoot = parent->pageNum;
    tree->numNodes++;
    tree->depth++;
    tree->root = parent;
    writeBtreeHeader(tree);
    return parent;
}

static RC insertIntoNonFullParent(BT_Node* parent, BT_Node* right, int index, BTreeHandle* tree) {
    index += 1; // next position is the pointer
    saInsertAt(parent->childrenPages, right->pageNum, index);
    for (int i = parent->vals->fill; i > index; i--) {
        parent->children[i] = parent->children[i - 1];
    }
    parent->children[index] = right;
    return writeNode(parent, tree);
}

static RC handleParentOverflow(BTreeHandle* tree, BT_Node* parent, BT_Node* right, int key) {
    BT_Node* overflowed = createOverflowedNode(tree, parent);
    int index = saInsert(overflowed->vals, key);
    insertIntoOverflowedNode(overflowed, right, key, index);

    BT_Node* rightParent = splitOverflowedNode(tree, parent, overflowed);

    key = rightParent->vals->elems[0];
    saDeleteAt(rightParent->vals, 0, 1);

    updateSiblingPointersForSplitting(parent, rightParent);

    writeNode(parent, tree);
    writeNode(rightParent, tree);
    writeBtreeHeader(tree);

    return insPropagateParent(tree, parent, rightParent, key);
}

static BT_Node* createOverflowedNode(BTreeHandle* tree, BT_Node* parent) {
    const int newSize = tree->size + 1;
    BT_Node* overflowed = createBTNode(newSize, 0, -1);

    // Copy node properties
    overflowed->vals->fill = parent->vals->fill;
    overflowed->childrenPages->fill = parent->childrenPages->fill;

    // Copy values and children information
    const size_t valueSize = SIZE_INT * parent->vals->fill;
    const size_t pageSize = SIZE_INT * parent->childrenPages->fill;
    const size_t ptrSize = sizeof(BT_Node*) * parent->childrenPages->fill;

    memcpy(overflowed->vals->elems, parent->vals->elems, valueSize);
    memcpy(overflowed->childrenPages->elems, parent->childrenPages->elems, pageSize);
    memcpy(overflowed->children, parent->children, ptrSize);

    return overflowed;
}

static void insertIntoOverflowedNode(BT_Node* overflowed, BT_Node* right, int key, int index) {
    const int insertPos = index + 1;

    // Insert the new page number
    saInsertAt(overflowed->childrenPages, right->pageNum, insertPos);

    // Shift existing children pointers
    for (int i = overflowed->childrenPages->fill - 1; i > insertPos; i--) {
        overflowed->children[i] = overflowed->children[i - 1];
    }

    // Insert the new child pointer
    overflowed->children[insertPos] = right;
}

static BT_Node* splitOverflowedNode(BTreeHandle* tree, BT_Node* parent, BT_Node* overflowed) {
    // Calculate split points
    const int leftFill = overflowed->vals->fill / 2;
    const int rightFill = overflowed->vals->fill - leftFill;
    const int leftPtrSize = leftFill + 1;

    // Create right node
    BT_Node* rightParent = createBTNode(tree->size, 0, tree->nextPage);
    tree->nextPage++;
    tree->numNodes++;

    // Update left node (parent)
    parent->vals->fill = leftFill;
    parent->childrenPages->fill = leftPtrSize;

    // Copy data to left node (parent)
    memcpy(parent->vals->elems,
           overflowed->vals->elems,
           SIZE_INT * leftFill);
    memcpy(parent->childrenPages->elems,
           overflowed->childrenPages->elems,
           SIZE_INT * leftPtrSize);
    memcpy(parent->children,
           overflowed->children,
           sizeof(BT_Node*) * leftPtrSize);

    // Update right node
    rightParent->vals->fill = rightFill;
    rightParent->childrenPages->fill = overflowed->childrenPages->fill - leftPtrSize;

    // Copy data to right node
    memcpy(rightParent->vals->elems,
           overflowed->vals->elems + leftFill,
           SIZE_INT * rightFill);
    memcpy(rightParent->childrenPages->elems,
           overflowed->childrenPages->elems + leftPtrSize,
           SIZE_INT * rightParent->childrenPages->fill);
    memcpy(rightParent->children,
           overflowed->children + leftPtrSize,
           sizeof(BT_Node*) * rightParent->childrenPages->fill);

    // Clean up and return
    destroyBTNode(overflowed);
    return rightParent;
}

static void updateSiblingPointersForSplitting(BT_Node* parent, BT_Node* rightParent) {
    rightParent->right = parent->right;
    if (rightParent->right != NULL) {
        rightParent->right->left = rightParent;
    }
    parent->right = rightParent;
    rightParent->left = parent;
}

void freeNodes(BT_Node *root) {
    if (root == NULL) {
        return;
    }

    BT_Node *leaf = root;
    while (!leaf->isLeaf) {
        leaf = leaf->children[0];
    }

    BT_Node *parent = leaf->parent;
    BT_Node *next;

    while (true) {
        while (leaf != NULL) {
            next = leaf->right;
            destroyBTNode(leaf);
            leaf = next;
        }

        if (parent == NULL) {
            break;
        }

        leaf = parent;
        parent = leaf->parent;
    }
}

RC initIndexManager(void *mgmtData) {
    return RC_OK;
}

RC shutdownIndexManager() {
    return RC_OK;
}

RC createBtree(char *idxId, DataType keyType, int n) {
    const int maxNodesPerPage = (PAGE_SIZE - BYTES_BT_HEADER_LEN) / (3 * SIZE_INT);

    if (n > maxNodesPerPage) {
        return RC_IM_N_TO_LAGE;
    }

    RC rc = createPageFile(idxId);
    if (rc != RC_OK) {
        return rc;
    }

    SM_FileHandle *fileHandle = malloc(sizeof(SM_FileHandle));
    if (fileHandle == NULL) {
        return RC_MEM_ALLOCATION_ERROR;
    }

    rc = openPageFile(idxId, fileHandle);
    if (rc != RC_OK) {
        free(fileHandle);
        return rc;
    }

    char *headerData = calloc(PAGE_SIZE, sizeof(char));
    if (headerData == NULL) {
        closePageFile(fileHandle);
        free(fileHandle);
        return RC_MEM_ALLOCATION_ERROR;
    }

    char *currentPtr = headerData;
    int headerValues[] = {n, keyType, 0, 0, 0, 0, 1};
    int numHeaderValues = sizeof(headerValues) / sizeof(headerValues[0]);

    for (int i = 0; i < numHeaderValues; ++i) {
        memcpy(currentPtr, &headerValues[i], SIZE_INT);
        currentPtr += SIZE_INT;
    }

    rc = writeBlock(0, fileHandle, headerData);

    free(headerData);
    closePageFile(fileHandle);
    free(fileHandle);

    return rc;
}


RC openBtree(BTreeHandle **tree, char *idxId) {
    BTreeHandle *newTree = malloc(sizeof(BTreeHandle));
    if (!newTree) return RC_ERROR;

    BM_BufferPool *bufferPool = malloc(sizeof(BM_BufferPool));
    if (!bufferPool) {
        free(newTree);
        return RC_ERROR;
    }

    RC rc = initBufferPool(bufferPool, idxId, PER_IDX_BUF_SIZE, RS_LRU, NULL);
    if (rc != RC_OK) {
        free(bufferPool);
        free(newTree);
        return rc;
    }

    BM_PageHandle *pageHandle = malloc(sizeof(BM_PageHandle));
    if (!pageHandle) {
        shutdownBufferPool(bufferPool);
        free(bufferPool);
        free(newTree);
        return RC_ERROR;
    }

    rc = pinPage(bufferPool, pageHandle, 0);
    if (rc != RC_OK) {
        free(pageHandle);
        shutdownBufferPool(bufferPool);
        free(bufferPool);
        free(newTree);
        return rc;
    }

    char *dataPtr = pageHandle->data;

    newTree->idxId = idxId;
    newTree->mgmtData = bufferPool;

    int *treeMembers[] = {
        &newTree->size,
        (int*)&newTree->keyType,
        &newTree->whereIsRoot,
        &newTree->numNodes,
        &newTree->numEntries,
        &newTree->depth,
        &newTree->nextPage
    };

    for (int i = 0; i < sizeof(treeMembers) / sizeof(treeMembers[0]); ++i) {
        memcpy(treeMembers[i], dataPtr, SIZE_INT);
        dataPtr += SIZE_INT;
    }

    rc = unpinPage(bufferPool, pageHandle);
    free(pageHandle);

    if (rc != RC_OK) {
        closeBtree(newTree);
        return rc;
    }

    rc = loadBtree(newTree);
    if (rc != RC_OK) {
        closeBtree(newTree);
        return rc;
    }

    *tree = newTree;
    return RC_OK;
}

RC closeBtree(BTreeHandle *tree) {
    if (!tree) return RC_INVALID_PARAMETER;

    if (tree->mgmtData) {
        shutdownBufferPool(tree->mgmtData);
        free(tree->mgmtData);
    }

    freeNodes(tree->root);
    free(tree);

    return RC_OK;
}

RC deleteBtree(char *idxId) {
    if (!idxId) return RC_INVALID_PARAMETER;

    if (access(idxId, F_OK) == -1) {
        return RC_FILE_NOT_FOUND;
    }

    if (unlink(idxId) == -1) {
        return RC_FS_ERROR;
    }

    return RC_OK;
}

RC getNumNodes(BTreeHandle *tree, int *result) {
    if (tree == NULL || result == NULL) {
        return RC_INVALID_PARAMETER;
    }

    *result = tree->numNodes;
    return RC_OK;
}

RC getNumEntries(BTreeHandle *tree, int *result) {
    if (tree == NULL || result == NULL) {
        return RC_INVALID_PARAMETER;
    }

    *result = tree->numEntries;
    return RC_OK;
}

RC getKeyType(BTreeHandle *tree, DataType *result) {
    if (tree == NULL || result == NULL) {
        return RC_INVALID_PARAMETER;
    }

    *result = tree->keyType;
    return RC_OK;
}

RC findKey(BTreeHandle *tree, Value *key, RID *result) {
    if (tree == NULL || key == NULL || result == NULL) {
        return RC_INVALID_PARAMETER;
    }

    BT_Node *targetNode = findNodeByKey(tree, key->v.intV);
    if (targetNode == NULL) {
        return RC_IM_KEY_NOT_FOUND;
    }

    int targetIndex;
    int foundIndex = dynamicArrSearch(targetNode->vals, key->v.intV, &targetIndex);

    if (foundIndex >= 0) {
        result->page = targetNode->leafRIDPages->elems[foundIndex];
        result->slot = targetNode->leafRIDSlots->elems[foundIndex];
        return RC_OK;
    }

    return RC_IM_KEY_NOT_FOUND;
}

static RC initializeNewNode(BTreeHandle *tree, BT_Node *targetNode);
static RC insertIntoNonFullNode(BTreeHandle *tree, BT_Node *targetNode, Value *key, RID rid, int index);
static RC splitAndInsertIntoFullNode(BTreeHandle *tree, BT_Node *targetNode, Value *key, RID rid);
static void copyNodeData(BT_Node *dest, BT_Node *src, int start, int count);

RC insertKey(BTreeHandle *tree, Value *key, RID rid)
{
    BT_Node *targetNode = findNodeByKey(tree, key->v.intV);

    if (targetNode == NULL)
    {
        targetNode = createBTNode(tree->size, 1, tree->nextPage);
        initializeNewNode(tree, targetNode);
    }

    int target = 0;
    int i = dynamicArrSearch(targetNode->vals, key->v.intV, &target);
    if (i >= 0)
    {
        return RC_IM_KEY_ALREADY_EXISTS;
    }

    i = saInsert(targetNode->vals, key->v.intV);

    if (i >= 0)
    {
        return insertIntoNonFullNode(tree, targetNode, key, rid, i);
    }
    else
    {
        return splitAndInsertIntoFullNode(tree, targetNode, key, rid);
    }
}

static RC initializeNewNode(BTreeHandle *tree, BT_Node *targetNode)
{
    tree->root = targetNode;
    tree->whereIsRoot = targetNode->pageNum;
    tree->nextPage = tree->nextPage + 1;
    tree->numNodes = tree->numNodes + 1;
    tree->depth = tree->depth + 1;
    writeBtreeHeader(tree);
    return RC_OK;
}

static RC insertIntoNonFullNode(BTreeHandle *tree, BT_Node *targetNode, Value *key, RID rid, int index)
{
    saInsertAt(targetNode->leafRIDPages, rid.page, index);
    saInsertAt(targetNode->leafRIDSlots, rid.slot, index);
    tree->numEntries = tree->numEntries + 1;
    writeBtreeHeader(tree);
    return RC_OK;
}

static RC splitAndInsertIntoFullNode(BTreeHandle *tree, BT_Node *targetNode, Value *key, RID rid)
{
    BT_Node *dividable = createBTNode(tree->size + 1, 1, -1);
    copyNodeData(dividable, targetNode, 0, targetNode->vals->fill);

    int i = saInsert(dividable->vals, key->v.intV);
    saInsertAt(dividable->leafRIDPages, rid.page, i);
    saInsertAt(dividable->leafRIDSlots, rid.slot, i);

    int lFill = ceil((float)dividable->vals->fill / 2);
    int rFill = dividable->vals->fill - lFill;
    BT_Node *rightChild = createBTNode(tree->size, 1, tree->nextPage);
    tree->nextPage++;
    tree->numNodes++;

    copyNodeData(targetNode, dividable, 0, lFill);
    copyNodeData(rightChild, dividable, lFill, rFill);

    destroyBTNode(dividable);

    rightChild->right = targetNode->right;
    if (rightChild->right != NULL)
    {
        rightChild->right->left = rightChild;
    }
    targetNode->right = rightChild;
    rightChild->left = targetNode;

    writeNode(rightChild, tree);
    writeNode(targetNode, tree);
    insPropagateParent(tree, targetNode, rightChild, rightChild->vals->elems[0]);

    tree->numEntries = tree->numEntries + 1;
    writeBtreeHeader(tree);
    return RC_OK;
}

static void copyNodeData(BT_Node *dest, BT_Node *src, int start, int count)
{
    dest->leafRIDSlots->fill = count;
    dest->leafRIDPages->fill = count;
    dest->vals->fill = count;

    memcpy(dest->vals->elems, src->vals->elems + start, SIZE_INT * count);
    memcpy(dest->leafRIDPages->elems, src->leafRIDPages->elems + start, SIZE_INT * count);
    memcpy(dest->leafRIDSlots->elems, src->leafRIDSlots->elems + start, SIZE_INT * count);
}

RC deleteKey(BTreeHandle *tree, Value *key) {
    BT_Node *targetNode = findNodeByKey(tree, key->v.intV);
    if (targetNode == NULL) {
        return RC_OK;
    }

    int target;
    int i = dynamicArrSearch(targetNode->vals, key->v.intV, &target);
    if (i < 0) {
        return RC_OK;
    }

    saDeleteAt(targetNode->vals, i, 1);
    saDeleteAt(targetNode->leafRIDPages, i, 1);
    saDeleteAt(targetNode->leafRIDSlots, i, 1);
    tree->numEntries--;

    writeNode(targetNode, tree);
    writeBtreeHeader(tree);
    return RC_OK;
}

RC openTreeScan(BTreeHandle *tree, BT_ScanHandle **handle) {
    if (tree->root == NULL) {
        return RC_IM_NO_MORE_ENTRIES;
    }

    ScanMgmtInfo *scanMgmDetails = malloc(sizeof(ScanMgmtInfo));
    BT_ScanHandle *bt_scanHandle = malloc(sizeof(BT_ScanHandle));

    if (!scanMgmDetails || !bt_scanHandle) {
        free(scanMgmDetails);
        free(bt_scanHandle);
        return RC_ERROR;
    }

    bt_scanHandle->tree = tree;
    scanMgmDetails->currentNode = tree->root;

    while (!scanMgmDetails->currentNode->isLeaf) {
        scanMgmDetails->currentNode = scanMgmDetails->currentNode->children[0];
    }

    bt_scanHandle->mgmtData = scanMgmDetails;
    scanMgmDetails->elementIndex = 0;
    *handle = bt_scanHandle;

    return RC_OK;
}

RC closeTreeScan(BT_ScanHandle *handle) {
    if (handle) {
        free(handle->mgmtData);
        free(handle);
    }
    return RC_OK;
}


RC nextEntry(BT_ScanHandle *handle, RID *result) {
    ScanMgmtInfo *scanMgmtInfo = handle->mgmtData;

    // Check for overflow and move to next node if necessary
    if (scanMgmtInfo->elementIndex >= scanMgmtInfo->currentNode->leafRIDPages->fill) {
        if (scanMgmtInfo->elementIndex == scanMgmtInfo->currentNode->vals->fill) {
            if (scanMgmtInfo->currentNode->right == NULL) {
                return RC_IM_NO_MORE_ENTRIES;
            }
            scanMgmtInfo->currentNode = scanMgmtInfo->currentNode->right;
        }
        scanMgmtInfo->elementIndex = 0;
    }

    // Update the result
    result->page = scanMgmtInfo->currentNode->leafRIDPages->elems[scanMgmtInfo->elementIndex];
    result->slot = scanMgmtInfo->currentNode->leafRIDSlots->elems[scanMgmtInfo->elementIndex];
    scanMgmtInfo->elementIndex++;

    return RC_OK;
}

static int calculateBufferSize(BTreeHandle *tree) {
    return (tree->numNodes * tree->size * CHARS_PER_NODE) +
           (tree->size) +
           PADDING_CHARS +
           (tree->numNodes * NEWLINE_CHARS);
}

static BT_Node* getLeftmostNodeAtLevel(BT_Node* root, int level) {
    BT_Node* current = root;

    for (int i = 0; i <= level; i++) {
        if (current == NULL || current->children == NULL) {
            return NULL;
        }
        current = current->children[0];
    }

    return current;
}

char* printTree(BTreeHandle *tree) {
    if (tree == NULL || tree->root == NULL) {
        return newCharArr(1);  // Return empty string for empty tree
    }

    // Allocate buffer with calculated size
    char* result = newCharArr(calculateBufferSize(tree));
    if (result == NULL) {
        return NULL;  // Handle allocation failure
    }

    BT_Node* currentNode = tree->root;
    int currentLevel = 0;

    // Traverse the tree level by level
    while (currentNode != NULL) {
        // Print current node
        printNodeHelper(currentNode, result);

        // Determine next node to process
        if (currentNode->isLeaf) {
            // Move right at leaf level
            currentNode = currentNode->right;
        }
        else if (currentNode->right == NULL) {
            // Move down to next level's leftmost node
            currentNode = getLeftmostNodeAtLevel(tree->root, ++currentLevel);
        }
        else {
            // Move right at current level
            currentNode = currentNode->right;
        }
    }

    return result;
}
