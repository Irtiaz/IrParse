#ifndef INT_SET_H_3253425
#define INT_SET_H_3253425

typedef struct IntSetStruct IntSet;

IntSet *createIntSet(void);

void putInSet(IntSet *set, int value);
int existsInSet(IntSet *set, int value);
int *getContents(IntSet *set);

void destroyIntSet(IntSet *set);

#endif
