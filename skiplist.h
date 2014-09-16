#include <stdlib.h>

typedef struct skiplistNode {
    void *obj;
    double score;
    struct skiplistNode *backward;
    struct skiplistLevel {
        struct skiplistNode *forward;
        unsigned int span;
    }level[];
} skiplistNode;

typedef struct skiplist {
    struct skiplistNode *header, *tail;
    unsigned long length;
    int level;
    int maxlevel;
} skiplist;

typedef struct skiplistiter {
    skiplist *parent;
    skiplistNode *node;
    int forward;
    double min;
    double max;
} skiplistiter;

typedef void (*slDeleteCb) (void *ud, void *obj);
void* slCreateObj(const char* ptr, size_t length);
void slFreeObj(void *obj);

skiplist *slCreate(int maxlevel);
void slFree(skiplist *sl);
void slDump(skiplist *sl);

void slInsert(skiplist *sl, double score, void *obj, int level);
int slDelete(skiplist *sl, double score, void *obj, double change);
unsigned long slLength(skiplist *sl);
unsigned long slDeleteByRank(skiplist *sl, unsigned int start, unsigned int end, slDeleteCb cb, void* ud);

unsigned long slGetRank(skiplist *sl, double score, void *o);
skiplistNode* slGetNodeByRank(skiplistNode* x, int level, unsigned long rank);
skiplistNode *slFirstInRange(skiplist *sl, double min, double max);
skiplistNode *slLastInRange(skiplist *sl, double min, double max);

skiplistiter *slIterNew(skiplist *sl, skiplistNode* head);
skiplistiter* slIterNewFromHead(skiplist *sl);
skiplistiter *slIterNewFromRange(skiplist *sl, double min, double max);
void slIterDel(skiplistiter *it);
int slIterGet(skiplistiter *it, double *score, const void **obj);
int slIterNext(skiplistiter *it);
