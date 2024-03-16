#include "ItemElement.h"
#include "IntSet.h"
#include "Symbol.h"
#include "stb_ds.h"

#include <stdlib.h>
#include <stdio.h>


ItemElement *createItemElement(int variableIndex, int ruleIndex, int dotIndex, IntSet *allowedFollowSet) {
    ItemElement *itemElement = (ItemElement *)malloc(sizeof(ItemElement));
    
    itemElement->variableIndex = variableIndex;
    itemElement->ruleIndex = ruleIndex;
    itemElement->dotIndex = dotIndex;
    itemElement->allowedFollowSet = createCopyOfIntSet(allowedFollowSet);

    return itemElement;
}

int elementsAreSame(ItemElement *element1, ItemElement *element2) {
    if (element1->variableIndex != element2->variableIndex || element1->ruleIndex != element2->ruleIndex || element1->dotIndex != element2->dotIndex) return 0;
    else {
        int result = 1;
        int *followSet1 = getContentsOfSet(element1->allowedFollowSet);
        int *followSet2 = getContentsOfSet(element2->allowedFollowSet);
        if (arrlen(followSet1) != arrlen(followSet2)) result = 0;
        else {
            int i;
            for (i = 0; i < arrlen(followSet1); ++i) {
                if (!existsInSet(element2->allowedFollowSet, followSet1[i])) {
                    result = 0;
                    break;
                }
            }
        }

        arrfree(followSet1);
        arrfree(followSet2);
        return result;
    }
}

void printItemElement(ItemElement *element, Symbol *symbols, int ***rules) {
    int *rule = rules[element->variableIndex][element->ruleIndex];
    int i;
    printf("%s -> ", symbols[element->variableIndex].name);
    for (i = 0; i < arrlen(rule); ++i) {
        if (i == element->dotIndex) printf(".");
        printf("%s", symbols[rule[i]].name);
    }
    {
        int *follows = getContentsOfSet(element->allowedFollowSet);
        printf(",");
        for (i = 0; i < arrlen(follows); ++i) {
            printf("%s", symbols[follows[i]].name);
            if (i < arrlen(follows) - 1) printf("/");
        }
        arrfree(follows);
    }
    printf("\n");
}

void destroyItemElement(ItemElement *itemElement) {
    destroyIntSet(itemElement->allowedFollowSet);
    free(itemElement);
}

