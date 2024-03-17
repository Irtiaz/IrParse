#include <stdlib.h>
#include "stb_ds.h"
#include "IntSet.h"

struct Item {
    int key;
    int value;
};

struct IntSetStruct {
    struct Item *hash;
};

IntSet *createIntSet(void) {
    IntSet *set = (IntSet *)malloc(sizeof(IntSet));
    set->hash = NULL;
    return set;
}

IntSet *createCopyOfIntSet(IntSet *source) {
    int *items = getContentsOfSet(source);
    IntSet *result = createIntSet();
    int i;
    for (i = 0; i < arrlen(items); ++i) {
        putInSet(result, items[i]);
    }
    arrfree(items);
    return result;
}

void putInSet(IntSet *set, int value) {
    hmput(set->hash, value, 1);
}

void removeFromSet(IntSet *set, int value) {
    hmput(set->hash, value, 0);
}

int existsInSet(IntSet *set, int value) {
    return hmget(set->hash, value) != 0;
}

int *getContentsOfSet(IntSet *set) {
    int *array = NULL;
    int i;
    for (i = 0; i < hmlen(set->hash); ++i) {
        arrput(array, set->hash[i].key);
    }
    return array;
}

int getLengthOfSet(IntSet *set) {
    return hmlen(set->hash);
}

int isSuperSet(IntSet *possibleSuperSet, IntSet *otherSet) {
    int *contents = getContentsOfSet(otherSet);
    int i;
    for (i = 0; i < arrlen(contents); ++i) {
        if (!existsInSet(possibleSuperSet, contents[i])) break;    
    }
    arrfree(contents);

    return i >= getLengthOfSet(otherSet);
}

void destroyIntSet(IntSet *set) {
    hmfree(set->hash);
    free(set);
}
