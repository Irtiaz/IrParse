#ifndef INT_SET_H_3253425
#define INT_SET_H_3253425

typedef struct IntSetStruct IntSet;

IntSet *createIntSet(void);

void putInSet(IntSet *set, int value);
void removeFromSet(IntSet *set, int value);
int existsInSet(IntSet *set, int value);
int *getContentsOfSet(IntSet *set);
int getLengthOfSet(IntSet *set);

void destroyIntSet(IntSet *set);

#endif
