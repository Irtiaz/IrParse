#ifndef ITEM_ELEMENT_H_43534
#define ITEM_ELEMENT_H_43534

#include "IntSet.h"
#include "Symbol.h"

typedef struct {
    int variableIndex;
    int ruleIndex;
    int dotIndex;
    IntSet *allowedFollowSet;
} ItemElement;

ItemElement *createItemElement(int variableIndex, int ruleIndex, int dotIndex, IntSet *allowedFollowSet);
ItemElement *getNextElement(ItemElement *element, int ***rules);
int elementsAreSame(ItemElement *element1, ItemElement *element2);
int elementsHaveSameBody(ItemElement *element1, ItemElement *element2);
void printItemElement(ItemElement *element, Symbol *symbols, int ***rules);
void destroyItemElement(ItemElement *itemElement);

#endif
