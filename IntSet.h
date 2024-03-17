#ifndef INT_SET_H_3253425
#define INT_SET_H_3253425

typedef struct IntSetStruct IntSet;

IntSet *createIntSet(void);

IntSet *createCopyOfIntSet(IntSet *source);
void putInSet(IntSet *set, int value);
void removeFromSet(IntSet *set, int value);
int existsInSet(IntSet *set, int value);
int *getContentsOfSet(IntSet *set);
int getLengthOfSet(IntSet *set);
int isSuperSet(IntSet *possibleSuperSet, IntSet *otherSet);

void destroyIntSet(IntSet *set);

#endif
