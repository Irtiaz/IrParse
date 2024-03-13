#ifndef PRODUCTION_RULE_H_3453423
#define PRODUCTION_RULE_H_3453423

#include "IntSet.h"

#define MAX_FOLLOW_SYMBOL_LENGTH 100

typedef struct {
    int bodyIndex;
    int dotIndex;
    IntSet *allowedFollowSymbolIndices;
} ProductionRule;

#endif
