#ifndef PRODUCTION_RULE_BODY_H_345345234
#define PRODUCTION_RULE_BODY_H_345345234

#define DERIVATION_MAX_LENGTH 20

typedef struct {
    int variableIndex;
    int derivationArray[DERIVATION_MAX_LENGTH];
    int derivationArrayLength;
} ProductionRuleBody;

#endif
