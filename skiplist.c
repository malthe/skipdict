#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "skiplist.h"

#define SWAP(x, y, T) do { T temp##x##y = x; x = y; y = temp##x##y; } while (0)

skiplistNode *slCreateNode(int level, double score, void *obj) {
    skiplistNode *n = calloc(1, sizeof(*n) + level * sizeof(struct skiplistLevel));
    n->score = score;
    n->obj = obj;
    return n;
}

skiplist *slCreate(int maxlevel) {
    int j;
    skiplist *sl;

    sl = malloc(sizeof(*sl));
    sl->level = 1;
    sl->maxlevel = maxlevel;
    sl->length = 0;
    sl->header = slCreateNode(maxlevel, 0, NULL);
    for (j=0; j < maxlevel; j++) {
        sl->header->level[j].forward = NULL;
        sl->header->level[j].span = 0;
    }
    sl->header->backward = NULL;
    sl->tail = NULL;
    return sl;
}

void slFree(skiplist *sl) {
    skiplistNode *node = sl->header->level[0].forward, *next;

    free(sl->header);
    while(node) {
        next = node->level[0].forward;
        free(node);
        node = next;
    }
    free(sl);
}

void slInsert(skiplist *sl, double score, void *obj, int level) {
    skiplistNode *update[sl->maxlevel], *x;
    unsigned int rank[sl->maxlevel];
    int i;

    x = sl->header;
    for (i = sl->level-1; i >= 0; i--) {
        /* store rank that is crossed to reach the insert position */
        rank[i] = i == (sl->level-1) ? 0 : rank[i+1];
        while (x->level[i].forward &&
            (x->level[i].forward->score < score ||
                (x->level[i].forward->score == score &&
                 x->level[i].forward->obj < obj))) {
            rank[i] += x->level[i].span;
            x = x->level[i].forward;
        }
        update[i] = x;
    }
    /* we assume the key is not already inside, since we allow duplicated
     * scores, and the re-insertion of score and object should never
     * happen since the caller of slInsert() should test in the hash table
     * if the element is already inside or not. */
    if (level > sl->level) {
        for (i = sl->level; i < level; i++) {
            rank[i] = 0;
            update[i] = sl->header;
            update[i]->level[i].span = sl->length;
        }
        sl->level = level;
    }
    x = slCreateNode(sl->maxlevel, score, obj);
    for (i = 0; i < level; i++) {
        x->level[i].forward = update[i]->level[i].forward;
        update[i]->level[i].forward = x;

        /* update span covered by update[i] as x is inserted here */
        x->level[i].span = update[i]->level[i].span - (rank[0] - rank[i]);
        update[i]->level[i].span = (rank[0] - rank[i]) + 1;
    }

    /* increment span for untouched levels */
    for (i = level; i < sl->level; i++) {
        update[i]->level[i].span++;
    }

    x->backward = (update[0] == sl->header) ? NULL : update[0];
    if (x->level[0].forward) {
        x->level[0].forward->backward = x;
    } else {
        sl->tail = x;
    }
    sl->length++;
}

/* Internal function used by slDelete, slDeleteByScore */
void slDeleteNode(skiplist *sl, skiplistNode *x, skiplistNode **update) {
    int i;
    for (i = 0; i < sl->level; i++) {
        if (update[i]->level[i].forward == x) {
            update[i]->level[i].span += x->level[i].span - 1;
            update[i]->level[i].forward = x->level[i].forward;
        } else {
            update[i]->level[i].span -= 1;
        }
    }
    if (x->level[0].forward) {
        x->level[0].forward->backward = x->backward;
    } else {
        sl->tail = x->backward;
    }
    while(sl->level > 1 && sl->header->level[sl->level-1].forward == NULL)
        sl->level--;
    sl->length--;
}

/* Delete an element with matching score/object from the skiplist. */
int slDelete(skiplist *sl, double score, void *obj, double change) {
    skiplistNode *update[sl->maxlevel], *x, *y;
    int i;

    x = sl->header;
    for (i = sl->level-1; i >= 0; i--) {
        while (x->level[i].forward &&
            (x->level[i].forward->score < score ||
                (x->level[i].forward->score == score &&
                 x->level[i].forward->obj < obj)))
            x = x->level[i].forward;
        update[i] = x;
    }
    /* We may have multiple elements with the same score, what we need
     * is to find the element with both the right score and object. */
    x = x->level[0].forward;
    if (x && score == x->score && x->obj == obj) {
        if (change > 0) {
            y = x->level[0].forward;
            if (y && score + change < y->score) {
                x->score = score + change;
                return 2;
            }
        }
        slDeleteNode(sl, x, update);
        free(x);
        return 1;
    } else {
        return 0; /* not found */
    }
    return 0; /* not found */
}

/* Delete all the elements with rank between start and end from the skiplist.
 * Start and end are inclusive. Note that start and end need to be 1-based */
unsigned long slDeleteByRank(skiplist *sl, unsigned int start, unsigned int end, slDeleteCb cb, void* ud) {
    skiplistNode *update[sl->maxlevel], *x;
    unsigned long traversed = 0, removed = 0;
    int i;

    x = sl->header;
    for (i = sl->level-1; i >= 0; i--) {
        while (x->level[i].forward && (traversed + x->level[i].span) < start) {
            traversed += x->level[i].span;
            x = x->level[i].forward;
        }
        update[i] = x;
    }

    traversed++;
    x = x->level[0].forward;
    while (x && traversed <= end) {
        skiplistNode *next = x->level[0].forward;
        slDeleteNode(sl,x,update);
        cb(ud, x->obj);
        free(x);
        removed++;
        traversed++;
        x = next;
    }
    return removed;
}

/* Find the rank for an element by both score and key.
 * Returns 0 when the element cannot be found, rank otherwise.
 * Note that the rank is 1-based due to the span of sl->header to the
 * first element. */
unsigned long slGetRank(skiplist *sl, double score, void *obj) {
    skiplistNode *x;
    unsigned long rank = 0;
    int i;

    x = sl->header;
    for (i = sl->level-1; i >= 0; i--) {
        while (x->level[i].forward &&
            (x->level[i].forward->score < score ||
                (x->level[i].forward->score == score &&
                 x->level[i].forward->obj <= obj))) {
            rank += x->level[i].span;
            x = x->level[i].forward;
        }

        /* x might be equal to sl->header, so test if obj is non-NULL */
        if (x->obj && x->obj == obj) {
            return rank;
        }
    }
    return 0;
}

/* Finds an element by its rank. The rank argument needs to be 1-based. */
skiplistNode* slGetNodeByRank(skiplistNode* node, int level, unsigned long rank)
{
    unsigned long traversed = 0;
    int i;

    for (i = level; i >= 0; i--) {
        while (node->level[i].forward && (traversed + node->level[i].span) <= rank)
        {
            traversed += node->level[i].span;
            node = node->level[i].forward;
        }
        if (traversed == rank) {
            return node;
        }
    }

    return NULL;
}

/* range [min, max], left & right both include */
/* Returns if there is a part of the dict in range. */
int slIsInRange(skiplist *sl, double min, double max) {
    skiplistNode *x;

    /* Test for ranges that will always be empty. */
    if(min > max) {
        return 0;
    }
    x = sl->tail;
    if (x == NULL || x->score < min)
        return 0;

    x = sl->header->level[0].forward;
    if (x == NULL || x->score > max)
        return 0;
    return 1;
}

/* Find the first node that is contained in the specified range.
 * Returns NULL when no element is contained in the range. */
skiplistNode *slFirstInRange(skiplist *sl, double min, double max) {
    skiplistNode *x;
    int i;

    /* If everything is out of range, return early. */
    if (!slIsInRange(sl,min, max)) return NULL;

    x = sl->header;
    for (i = sl->level-1; i >= 0; i--) {
        /* Go forward while *OUT* of range. */
        while (x->level[i].forward && x->level[i].forward->score < min)
                x = x->level[i].forward;
    }

    /* This is an inner range, so the next node cannot be NULL. */
    x = x->level[0].forward;
    return x;
}

/* Find the last node that is contained in the specified range.
 * Returns NULL when no element is contained in the range. */
skiplistNode *slLastInRange(skiplist *sl, double min, double max) {
    skiplistNode *x;
    int i;

    /* If everything is out of range, return early. */
    if (!slIsInRange(sl, min, max)) return NULL;

    x = sl->header;
    for (i = sl->level-1; i >= 0; i--) {
        /* Go forward while *IN* range. */
        while (x->level[i].forward &&
            x->level[i].forward->score <= max)
                x = x->level[i].forward;
    }

    /* This is an inner range, so this node cannot be NULL. */
    return x;
}

unsigned long slLength(skiplist *sl)
{
    return sl->length;
}

skiplistiter *slIterNew(skiplist *sl, skiplistNode* head)
{
    skiplistiter *it = calloc(1, sizeof(struct skiplistiter));
    if (it) {
        it->parent = sl;
        it->node = head;
        it->forward = 1;
        it->min = it->node->score;
        it->max = sl->tail->score;
    }
    return it;
}

skiplistiter *slIterNewFromHead(skiplist *sl)
{
    return slIterNew(sl, sl->header->level[0].forward);
}

skiplistiter *slIterNewFromRange(skiplist *sl, double min, double max)
{
    skiplistiter *it = calloc(1, sizeof(struct skiplistiter));
    if (it) {
        int reversed = min > max;
        it->parent = sl;
        it->forward = (reversed) ? 0 : 1;
        if (reversed) {
            SWAP(min, max, double);
            it->node = slLastInRange(sl, min, max);
        } else {
            it->node = slFirstInRange(sl, min, max);
        }
        it->min = min;
        it->max = max;
    }
    return it;
}

void slIterDel(skiplistiter *it)
{
    free(it);
}

int slIterGet(skiplistiter *it, double *score, const void **obj)
{
    int err = -1;
    if (it &&
        it->node &&
        it->parent &&
        it->node != it->parent->header &&
        it->min <= it->node->score &&
        it->max >= it->node->score) {
        *score = it->node->score;
        *obj = it->node->obj;
        err = 0;
    }
    return err;
}

int slIterNext(skiplistiter *it)
{
    int err = -1;
    if (it && it->node) {
        if (it->forward) {
            it->node = it->node->level[0].forward;
        } else {
            it->node = it->node->backward;
        }
        err = 0;
    }
    return err;
}
