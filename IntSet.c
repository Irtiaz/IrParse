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

void putInSet(IntSet *set, int value) {
    hmput(set->hash, value, 1);
}


int existsInSet(IntSet *set, int value) {
    return hmget(set->hash, value) != 0;
}

int *getContents(IntSet *set) {
    int *array = NULL;
    int i;
    for (i = 0; i < hmlen(set->hash); ++i) {
        arrput(array, set->hash[i].key);
    }
    return array;
}

void destroyIntSet(IntSet *set) {
    hmfree(set->hash);
    free(set);
}
