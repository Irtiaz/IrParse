#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Symbol.h"
#include "IntSet.h"
#include "ItemElement.h"

char **split(char *string, const char *delimeter);
char *strdup(const char *string);
int getIndexOfSymbol(Symbol *symbols, const char *symbolName);
void parseFromGrammarFile(const char *grammarFileName, Symbol **symbolsArray, int ****rulesArray);
int *concat(int *arr1, int *arr2);
int *flattenWithSingleRule(int *previousRule, int *laterRule);
void flattenWithMultipleRules(int ***result, int **previousRules, int **laterRuleAddress);
void printGrammarInfo(Symbol *symbols, int ***rules, int ruleBreakPoint);
void eliminateLeftRecursion(Symbol *originalSymbols, int ***originalRules, Symbol **modifiedSymbols, int ****rulesAfterElimination);
void freeRules(int ***rules);
Symbol *copySymbols(Symbol *symbols);
int ***copyRules(int ***rules);
int isNullRule(IntSet *nullables, int *rule);
IntSet *fixNullables(Symbol *originalSymbols, int ***originalRules, int ****rulesReturn);
void addCombinationOfNullableRemovedRules(int ***result, int *rule, IntSet *nullables);
IntSet *firstOf(IntSet **result, int symbolIndex, Symbol *symbols, int ***rules);
IntSet **getFirstSetArray(Symbol *symbols, int ***rules);
IntSet *getFirstOfSubRule(Symbol *symbols, int *rule, int startIndex, IntSet **firstSetArray);
int itemElementExistsInItem(ItemElement *element, ItemElement **item);
ItemElement **getClosureOf(ItemElement *root, Symbol *symbols, int ***rules, IntSet **firstSetArray);
IntSet *getFollowOfSingleRunClosure(ItemElement *root, Symbol *symbols, int ***rules, IntSet **firstSetArray);
ItemElement **getSingleRunClosure(ItemElement *root, Symbol *symbols, int ***rules, IntSet **firstSetArray);
void printItem(ItemElement **item, Symbol *symbols, int ***rules);
void destroyItem(ItemElement **item);
int itemsAreEssentiallySame(ItemElement **item1, ItemElement **item2);
int itemElementMatchesIn(ItemElement **item, ItemElement *itemElement);
void mergeItem(ItemElement ***destinationAddress, ItemElement **source);
int getMatchingItem(ItemElement ***items, ItemElement **item);
void traverseItemAndResolveGotos(ItemElement ****itemsAddress, int ***gotosAddress, int *traverseIndexAddress, Symbol *symbols, int ***rules, IntSet **firstSetArray);
void printItems(ItemElement ***items, Symbol *symbols, int ***rules);
void destroyItems(ItemElement ***items);
void freeGotos(int **gotos);
void printGotos(int **gotos, Symbol *symbols);
ItemElement **mergeClosures(ItemElement **itemElements, Symbol *symbols, int ***rules, IntSet **firstSetArray);
ItemElement **getNexts(ItemElement **item, int symbolIndex, int ***rules);
ItemElement **getMergedClosure(ItemElement **item, int symbolIndex, Symbol *symbols, int ***rules, IntSet **firstSetArray);
ItemElement *getElementByRuleAndDot(ItemElement **item, int variableIndex, int ruleIndex, int dotIndex);
void propagateFollow(int itemIndex, ItemElement ***items, int **gotos, int ***rules);
int getSingleRuleIndex(int ***rules, int variableIndex, int ruleIndex);
void logItemsAndGotos(FILE *logFile, ItemElement ***items, int **gotos, Symbol *symbols, int ***rules);
void logSymbols(FILE *logFile, Symbol *symbols);
void logRules(FILE *logFile, int ***rules);
int getReducer(ItemElement **item, int symbolIndex, int ***rules);
void logParseTable(const char *logFileName, Symbol *symbols, int ***rules, ItemElement ***items, int **gotos);


int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Wrong usage! Correct usage: ./a.out grammar-file.txt\n");
        exit(1);
    }
    else {
        Symbol *symbols;
        int ***rules;
        IntSet **firstSetArray;

        ItemElement ***items = NULL;
        int **gotos = NULL;

        parseFromGrammarFile(argv[1], &symbols, &rules);
        printGrammarInfo(symbols, rules, arrlen(rules));

        firstSetArray = getFirstSetArray(symbols, rules);
        {
            int traverseIndex = 0;

            IntSet *followSetForRoot = createIntSet();
            ItemElement *element;
            putInSet(followSetForRoot, arrlen(symbols) - 1);
            element = createItemElement(0, 0, 0, followSetForRoot);
            destroyIntSet(followSetForRoot);

            arrput(items, getClosureOf(element, symbols, rules, firstSetArray));

            while (traverseIndex < arrlen(items)) traverseItemAndResolveGotos(&items, &gotos, &traverseIndex, symbols, rules, firstSetArray);

            printItems(items, symbols, rules);
            printGotos(gotos, symbols);
        }

        {
            int i;
            for (i = 0; i < arrlen(firstSetArray); ++i) destroyIntSet(firstSetArray[i]);
        }

        logParseTable("parse-table.txt", symbols, rules, items, gotos);

        arrfree(firstSetArray);
        destroyItems(items);
        freeGotos(gotos);
        arrfree(symbols);
        freeRules(rules);

        return 0;
    }
}

void freeRules(int ***rules) {
    int i;
    for (i = 0; i < arrlen(rules); ++i) {
        int j;
        for (j = 0; j < arrlen(rules[i]); ++j) {
            arrfree(rules[i][j]);
        }
        arrfree(rules[i]);
    }
    arrfree(rules);
}


char **split(char *string, const char *delimeter) {
    char **result = NULL;
    char *copyOfString = strdup(string);
    char *token = strtok(copyOfString, delimeter);

    while (token != NULL) {
        arrput(result, strdup(token));
        token = strtok(NULL, delimeter);
    }

    free(copyOfString);

    return result;
}


char *strdup(const char *string) {
    char *copy = (char *)malloc(strlen(string) + 1);
    strcpy(copy, string);
    return copy;
}


int getIndexOfSymbol(Symbol *symbols, const char *symbolName) {
    int i;
    for (i = 0; i < arrlen(symbols); ++i) {
        if (strcmp(symbols[i].name, symbolName) == 0) return i;
    }
    return -1;
}

void parseFromGrammarFile(const char *grammarFileName, Symbol **symbolsArray, int ****rulesArray) {
    Symbol *symbols = NULL;
    int ***rules = NULL;

    {
        Symbol dummyStart = {"_S", 0};

        arrput(symbols, dummyStart);
    }

    {
        FILE *grammarFile;
        char line[1000];
        grammarFile = fopen(grammarFileName, "r");

        if (!grammarFile) {
            fprintf(stderr, "Can not find %s\n", grammarFileName);
            exit(1);
        }

        fscanf(grammarFile, "%[^\r\n]s", line);
        fgetc(grammarFile);
        {
            char **array = split(line, " ");
            int i;
            for (i = 0; i < arrlen(array); ++i) {
                Symbol variable;
                strcpy(variable.name, array[i]);
                variable.isTerminal = 0;

                arrput(symbols, variable);

                free(array[i]);
            }
            arrfree(array);
        }


        fscanf(grammarFile, "%[^\r\n]s", line);
        fgetc(grammarFile);
        {
            char **array = split(line, " ");
            int i;
            for (i = 0; i < arrlen(array); ++i) {
                Symbol variable;
                strcpy(variable.name, array[i]);
                variable.isTerminal = 1;

                arrput(symbols, variable);

                free(array[i]);
            }
            arrfree(array);
        }

        {
            Symbol epsilon = {"#", 1};
            Symbol endOfFile = {"$", 1};

            arrput(symbols, epsilon);
            arrput(symbols, endOfFile);
        }

        {
            int i;
            for (i = 0; i < arrlen(symbols) && !symbols[i].isTerminal; ++i) {
                arrput(rules, NULL);
            }
        }

        {
            int *dummyStartToActualStartRuleDerivation = NULL;
            arrput(dummyStartToActualStartRuleDerivation, 1);
            arrput(rules[0], dummyStartToActualStartRuleDerivation);
        }

        while (fscanf(grammarFile, "%[^\r\n]s", line) != EOF) {
            char **array = split(line, " ");
            int *ruleDerivationArray = NULL;
            int i;

            int variableIndex = getIndexOfSymbol(symbols, array[0]);
            free(array[0]);

            for (i = 1; i < arrlen(array); ++i) {
                arrput(ruleDerivationArray, getIndexOfSymbol(symbols, array[i]));
                free(array[i]);
            }

            arrput(rules[variableIndex], ruleDerivationArray);

            arrfree(array);
            fgetc(grammarFile);
        }

        fclose(grammarFile);
    }

    *symbolsArray = symbols;
    *rulesArray = rules;
}

int *concat(int *arr1, int *arr2) {
    int *arr = NULL;
    int i;
    for (i = 0; i < arrlen(arr1); ++i) {
        arrput(arr, arr1[i]);
    }
    for (i = 0; i < arrlen(arr2); ++i) {
        arrput(arr, arr2[i]);
    }
    return arr;
}

int *flattenWithSingleRule(int *previousRule, int *laterRule) {
    int *flattenedRule;
    flattenedRule = concat(previousRule, laterRule);
    return flattenedRule;
}

void flattenWithMultipleRules(int ***result, int **previousRules, int **laterRuleAddress) {
    int i;
    int  *laterRule = *laterRuleAddress;
    arrdel(laterRule, 0);
    for (i = 0; i < arrlen(previousRules); ++i) {
        arrput(*result, flattenWithSingleRule(previousRules[i], laterRule));
    }
    *laterRuleAddress = laterRule;
}

void printGrammarInfo(Symbol *symbols, int ***rules, int ruleBreakPoint) {
    int eofSymbolIndex = getIndexOfSymbol(symbols, "$");

    {
        int i;
        for (i = 0; i < arrlen(symbols); ++i) {
            printf("%s : %s\n", symbols[i].name, symbols[i].isTerminal? "terminal" : "non-terminal");
        }
    }

    {
        int i;
        for (i = 0; i < arrlen(rules); ++i) {
            int **bodies = rules[i];
            {
                int k;
                printf("%s -> ", symbols[i < ruleBreakPoint? i : eofSymbolIndex + 1 + i - ruleBreakPoint].name);
                for (k = 0; k < arrlen(bodies); ++k) {
                    int *body = bodies[k];
                    {
                        int j;
                        for (j = 0; j < arrlen(body); ++j) {
                            printf("%s ", symbols[body[j]].name);
                        }
                    }
                    if (k < arrlen(bodies) - 1) printf(" | ");
                }
            }
            printf("\n");
        }
    }
}


Symbol *copySymbols(Symbol *symbols) {
    Symbol *copy = NULL; 
    int i;
    for (i = 0; i < arrlen(symbols); ++i) {
        arrput(copy, symbols[i]);
    }
    return copy;
}
int ***copyRules(int ***rules) {
    int ***copy = NULL;
    int i, j, k;
    for (i = 0; i < arrlen(rules); ++i) {
        arrput(copy, NULL);
        for (j = 0; j < arrlen(rules[i]); ++j) {
            arrput(copy[i], NULL);
            for (k = 0; k < arrlen(rules[i][j]); ++k) {
                arrput(copy[i][j], rules[i][j][k]);
            }
        }
    }
    return copy;
}


void eliminateLeftRecursion(Symbol *originalSymbols, int ***originalRules, Symbol **modifiedSymbols, int ****rulesAfterElimination) {
    Symbol *symbols = copySymbols(originalSymbols);
    int ***rules = copyRules(originalRules);
    int variableIndex;

    for (variableIndex = 0; variableIndex < arrlen(originalRules); ++variableIndex) {
        int ruleIndex;
        int runs;
        for (runs = 0; runs < variableIndex; ++runs) {
            int **modifiedRules = NULL;
            for (ruleIndex = 0; ruleIndex < arrlen(rules[variableIndex]); ++ruleIndex) {
                int *rule = rules[variableIndex][ruleIndex];
                int firstVariableInRule = rule[0];
                if (firstVariableInRule < variableIndex) {
                    flattenWithMultipleRules(&modifiedRules, rules[firstVariableInRule], &rule);
                    arrfree(rule);
                }
                else arrput(modifiedRules, rule);

            }
            arrfree(rules[variableIndex]);
            rules[variableIndex] = modifiedRules;
        }

        {
            int **recursiveRules = NULL;
            for (ruleIndex = arrlen(rules[variableIndex]) - 1; ruleIndex >= 0; --ruleIndex) {
                int *rule = rules[variableIndex][ruleIndex];
                int firstVariableInRule = rule[0];
                if (firstVariableInRule == variableIndex) {
                    arrdel(rule, 0);
                    arrput(recursiveRules, rule);
                    arrdel(rules[variableIndex], ruleIndex);
                }
            }

            if (arrlen(recursiveRules)) {
                Symbol extraSymbol;
                extraSymbol.name[0] = '\0';
                strcpy(extraSymbol.name, symbols[variableIndex].name);
                strcat(extraSymbol.name, "'");
                extraSymbol.isTerminal = 0; 
                arrput(symbols, extraSymbol);

                {
                    int i;
                    for (i = 0; i < arrlen(rules[variableIndex]); ++i) {
                        arrput(rules[variableIndex][i], arrlen(symbols) - 1);
                    }
                }

                {
                    int i;
                    int *nullRule = NULL;
                    arrput(nullRule, arrlen(originalSymbols) - 2);
                    for (i = 0; i < arrlen(recursiveRules); ++i) {
                        arrput(recursiveRules[i], arrlen(symbols) - 1);
                    }
                    arrput(recursiveRules, nullRule);
                    arrput(rules, recursiveRules);
                }
            }

        }

    }

    *modifiedSymbols = symbols;
    *rulesAfterElimination = rules;
}

int isNullRule(IntSet *nullables, int *rule) {
    int i;
    for (i = 0; i < arrlen(rule); ++i) {
        if (!existsInSet(nullables, rule[i])) return 0;
    }
    return 1;
}

IntSet *fixNullables(Symbol *originalSymbols, int ***originalRules, int ****rulesReturn) {
    Symbol *symbols = copySymbols(originalSymbols);
    int ***rules = copyRules(originalRules);
    IntSet *nullables = createIntSet();

    IntSet *nextIterationNullables = createIntSet();
    putInSet(nextIterationNullables, arrlen(symbols) - 2);

    while (getLengthOfSet(nextIterationNullables)) {
        IntSet *newlyFoundNullables = createIntSet();

        int variableIndex;
        for (variableIndex = 1; variableIndex < arrlen(rules); ++variableIndex) {
            int ruleIndex;
            for (ruleIndex = arrlen(rules[variableIndex]) - 1; ruleIndex >= 0; --ruleIndex) {
                if (isNullRule(nullables, rules[variableIndex][ruleIndex]) || isNullRule(nextIterationNullables, rules[variableIndex][ruleIndex])) {
                    putInSet(newlyFoundNullables, variableIndex);
                    putInSet(nullables, variableIndex);
                    if (arrlen(rules[variableIndex][ruleIndex]) == 1 && rules[variableIndex][ruleIndex][0] == arrlen(symbols) - 2) {
                        arrfree(rules[variableIndex][ruleIndex]);
                        arrdel(rules[variableIndex], ruleIndex);
                    }
                }
            }
        }

        destroyIntSet(nextIterationNullables);
        nextIterationNullables = newlyFoundNullables;
    }

    destroyIntSet(nextIterationNullables);


    if (existsInSet(nullables, 1)) {
        int *nullRule = NULL;
        arrput(nullRule, arrlen(symbols) - 2);
        arrput(rules[1], nullRule);
    }


    {
        int variableIndex;
        for (variableIndex = 0; variableIndex < arrlen(rules); ++variableIndex) {
            int ruleIndex;
            int **modifiedRules = NULL;
            for (ruleIndex = 0; ruleIndex < arrlen(rules[variableIndex]); ++ruleIndex) {
                addCombinationOfNullableRemovedRules(&modifiedRules, rules[variableIndex][ruleIndex], nullables);
            }

            arrfree(rules[variableIndex]);
            rules[variableIndex] = modifiedRules;
        }
    }

    *rulesReturn = rules;
    arrfree(symbols);

    return nullables;
}


void addCombinationOfNullableRemovedRules(int ***result, int *rule, IntSet *nullables) {
    arrput(*result, rule);
    {
        int i;
        for (i = 0; i < arrlen(rule); ++i) {
            if (existsInSet(nullables, rule[i])) {
                {
                    int j;
                    int *copy = NULL;
                    for (j = 0; j < arrlen(rule); ++j) {
                        if (j != i) arrput(copy, rule[j]);
                    }
                    if (arrlen(copy)) addCombinationOfNullableRemovedRules(result, copy, nullables);
                }
            }
        }
    }
}

IntSet *firstOf(IntSet **result, int symbolIndex, Symbol *symbols, int ***rules) {
    if (getLengthOfSet(result[symbolIndex])) return result[symbolIndex];
    else {
        if (symbols[symbolIndex].isTerminal) {
            putInSet(result[symbolIndex], symbolIndex);
        }
        else {
            int ruleIndex;
            for (ruleIndex = 0; ruleIndex < arrlen(rules[symbolIndex]); ++ruleIndex) {
                int *rule = rules[symbolIndex][ruleIndex];
                int askNextVariable = 1;
                int nextVariable = 0;
                int emptySymbol = getIndexOfSymbol(symbols, "#");
                while (askNextVariable && nextVariable < arrlen(rule)) {
                    IntSet *firstOfNextVariable = firstOf(result, rule[nextVariable], symbols, rules);
                    int *firstArray = getContentsOfSet(firstOfNextVariable);
                    askNextVariable = 0;
                    {
                        int i;
                        for (i = 0; i < arrlen(firstArray); ++i) {
                            if (firstArray[i] != emptySymbol) putInSet(result[symbolIndex], firstArray[i]);
                            else askNextVariable = 1;
                        }
                    }
                    arrfree(firstArray);
                    if (askNextVariable) ++nextVariable;
                }
                if (nextVariable >= arrlen(rule)) putInSet(result[symbolIndex], emptySymbol);
            }
        }
        return result[symbolIndex];
    }
}

IntSet **getFirstSetArray(Symbol *symbols, int ***rules) {
    IntSet **result = NULL; 

    int ***nullableRemovedRules;
    Symbol *leftRecursionEliminatedSymbols;
    int ***leftRecursionEliminatedRules;

    IntSet *nullables = fixNullables(symbols, rules, &nullableRemovedRules);
    printf("----------> length of nullables = %d\n", getLengthOfSet(nullables));
    eliminateLeftRecursion(symbols, nullableRemovedRules, &leftRecursionEliminatedSymbols, &leftRecursionEliminatedRules);

    {
        int i;
        for (i = 0; i < arrlen(symbols); ++i) arrput(result, createIntSet());
    }

    {
        int i;
        for (i = 0; i < arrlen(symbols); ++i) {
            (void)firstOf(result, i, leftRecursionEliminatedSymbols, leftRecursionEliminatedRules);
        }
    }


    {
        int emptySymbol = getIndexOfSymbol(symbols, "#");
        int *nullablesArray = getContentsOfSet(nullables);
        int i;
        for (i = 0; i < arrlen(nullablesArray); ++i) {
            putInSet(result[nullablesArray[i]], emptySymbol);
        }

        arrfree(nullablesArray);
    }


    destroyIntSet(nullables);
    arrfree(leftRecursionEliminatedSymbols);
    freeRules(leftRecursionEliminatedRules);
    freeRules(nullableRemovedRules);

    return result;
}


IntSet *getFirstOfSubRule(Symbol *symbols, int *rule, int startIndex, IntSet **firstSetArray) {
    IntSet *result = createIntSet();
    int askNextVariable = 1;
    int nextVariable = startIndex;
    while (askNextVariable && nextVariable < arrlen(rule)) {
        IntSet *firstOfNextVariable = firstSetArray[rule[nextVariable]];
        int *contents = getContentsOfSet(firstOfNextVariable);
        int i;
        askNextVariable = 0;
        for (i = 0; i < arrlen(contents); ++i) {
            if (contents[i] != arrlen(symbols) - 2) putInSet(result, contents[i]);
            else {
                askNextVariable = 1;
                ++nextVariable;
            }
        }
        arrfree(contents);
    }

    if (nextVariable >= arrlen(rule)) putInSet(result, arrlen(symbols) - 2);
    return result;
}

int itemElementExistsInItem(ItemElement *element, ItemElement **item) {
    int i;
    for (i = 0; i < arrlen(item); ++i) {
        if (elementsAreSame(item[i], element)) return 1;
    }
    return 0;
}

IntSet *getFollowOfSingleRunClosure(ItemElement *root, Symbol *symbols, int ***rules, IntSet **firstSetArray) {
    int *rule = rules[root->variableIndex][root->ruleIndex];
    if (root->dotIndex < arrlen(rule) - 1) {
        IntSet *firstSet = getFirstOfSubRule(symbols, rule, root->dotIndex + 1, firstSetArray);
        if (existsInSet(firstSet, arrlen(symbols) - 2)) {
            int *itemsInActualFollow = getContentsOfSet(root->allowedFollowSet);
            int i;

            removeFromSet(firstSet, arrlen(symbols) - 2);
            for (i = 0; i < arrlen(itemsInActualFollow); ++i) {
                putInSet(firstSet, itemsInActualFollow[i]);
            }

            arrfree(itemsInActualFollow);
        }
        return firstSet;
    }
    else return createCopyOfIntSet(root->allowedFollowSet);
}

ItemElement **getSingleRunClosure(ItemElement *root, Symbol *symbols, int ***rules, IntSet **firstSetArray) {
    if (root->dotIndex >= arrlen(rules[root->variableIndex][root->ruleIndex])) return NULL;
    {
        int symbolIndexUnderDot = rules[root->variableIndex][root->ruleIndex][root->dotIndex];
        Symbol symbolUnderDot = symbols[symbolIndexUnderDot];
        if (symbolUnderDot.isTerminal) return NULL;
        else {
            ItemElement **result = NULL; 
            IntSet *follow = getFollowOfSingleRunClosure(root, symbols, rules, firstSetArray);

            int ruleIndex;
            for (ruleIndex = 0; ruleIndex < arrlen(rules[symbolIndexUnderDot]); ++ruleIndex) {
                ItemElement *closureElement = createItemElement(symbolIndexUnderDot, ruleIndex, 0, follow);
                arrput(result, closureElement);
            }

            destroyIntSet(follow);
            return result;
        }
    }
}

ItemElement **getClosureOf(ItemElement *root, Symbol *symbols, int ***rules, IntSet **firstSetArray) {
    ItemElement **item = NULL; 
    arrput(item, root);

    if (root->dotIndex >= arrlen(rules[root->variableIndex][root->ruleIndex])) return item;

    {
        int found = 1;
        int currentLength = 0;
        while (found) {
            int startIndex = currentLength;
            currentLength = arrlen(item);
            found = 0;
            {
                int elementIndex;
                for (elementIndex = startIndex; elementIndex < currentLength; ++elementIndex) {
                    ItemElement *element = item[elementIndex];
                    ItemElement **singleRunClosure = getSingleRunClosure(element, symbols, rules, firstSetArray);
                    int closureIndex;
                    for (closureIndex = 0; closureIndex < arrlen(singleRunClosure); ++closureIndex) {
                        ItemElement *closure = singleRunClosure[closureIndex];
                        int checkIndex;
                        for (checkIndex = 0; checkIndex < currentLength; ++checkIndex) {
                            if (elementsAreSame(item[checkIndex], closure)) {
                                break;
                            }
                        }
                        if (checkIndex >= currentLength) {

                            int bodyMatch = itemElementMatchesIn(item, closure);
                            if (bodyMatch < 0) {
                                arrput(item, closure);
                                found = 1;
                            }
                            else if (!isSuperSet(item[bodyMatch]->allowedFollowSet, closure->allowedFollowSet)) {
                                mergeItemElementFollows(item[bodyMatch], closure);
                                destroyItemElement(closure);
                                closure = NULL;
                                found = 1;
                            }
                            else {
                                destroyItemElement(closure);
                                closure = NULL;
                            }
                        }
                        else {
                            destroyItemElement(closure);
                            closure = NULL;
                        }
                    }
                    arrfree(singleRunClosure);
                }
            }
        }
    }

    return item;
}

void printItem(ItemElement **item, Symbol *symbols, int ***rules) {
    int i;
    for (i = 0; i < arrlen(item); ++i) {
        printItemElement(item[i], symbols, rules);
    }
}

void destroyItem(ItemElement **item) {
    int i;
    for (i = 0; i < arrlen(item); ++i) {
        destroyItemElement(item[i]);
        item[i] = NULL;
    }
    arrfree(item);
}

int itemElementMatchesIn(ItemElement **item, ItemElement *itemElement) {
    int i;
    for (i = 0; i < arrlen(item); ++i) {
        ItemElement *sourceElement = item[i];
        if (elementsHaveSameBody(sourceElement, itemElement)) return i;
    }
    return -1;
}

int itemsAreEssentiallySame(ItemElement **item1, ItemElement **item2) {
    if (arrlen(item1) != arrlen(item2)) return 0;
    else {
        int i;
        for (i = 0; i < arrlen(item1); ++i) {
            ItemElement *element1 = item1[i];
            if (itemElementMatchesIn(item2, element1) == -1) return 0;
        }
        return 1;
    }
}

void mergeItem(ItemElement ***destinationAddress, ItemElement **source) {
    ItemElement **destination = *destinationAddress;
    int sourceIndex;
    for (sourceIndex = 0; sourceIndex < arrlen(source); ++sourceIndex) {
        int destinationIndex = itemElementMatchesIn(destination, source[sourceIndex]);
        if (destinationIndex < 0) {
            arrput(destination, createItemElement(source[sourceIndex]->variableIndex, source[sourceIndex]->ruleIndex, source[sourceIndex]->dotIndex, source[sourceIndex]->allowedFollowSet));
        }
        else {
            mergeItemElementFollows(destination[destinationIndex], source[sourceIndex]);
        }
    }
    *destinationAddress = destination;
}

int getMatchingItem(ItemElement ***items, ItemElement **item) {
    int i;
    for (i = 0; i < arrlen(items); ++i) {
        if (itemsAreEssentiallySame(items[i], item)) return i;
    }
    return -1;
}

void traverseItemAndResolveGotos(ItemElement ****itemsAddress, int ***gotosAddress, int *traverseIndexAddress, Symbol *symbols, int ***rules, IntSet **firstSetArray) {
    ItemElement **item = (*itemsAddress)[*traverseIndexAddress];
    int *gotoArray = NULL; 

    {
        int i;
        for (i = 0; i < arrlen(symbols); ++i) {
            arrput(gotoArray, -1);
        }
    }

    {
        int symbolIndex;
        for (symbolIndex = 1; symbolIndex < arrlen(symbols); ++symbolIndex) {
            ItemElement **closureItem = getMergedClosure(item, symbolIndex, symbols, rules, firstSetArray);
            if (closureItem) {
                int previousMatchIndex = getMatchingItem(*itemsAddress, closureItem);

                if (previousMatchIndex >= 0) {
                    mergeItem(&((*itemsAddress)[previousMatchIndex]), closureItem);
                    destroyItem(closureItem);
                    propagateFollow(previousMatchIndex, *itemsAddress, *gotosAddress, rules);


                    gotoArray[symbolIndex] = previousMatchIndex;
                }
                else {
                    arrput(*itemsAddress, closureItem);
                    gotoArray[symbolIndex] = arrlen(*itemsAddress) - 1;
                }
            }
        }

        arrput(*gotosAddress, gotoArray);
        *traverseIndexAddress = *traverseIndexAddress + 1;
    }
}

void printItems(ItemElement ***items, Symbol *symbols, int ***rules) {
    int i;
    for (i = 0; i < arrlen(items); ++i) {
        printf("\n\n______________________\nI%d\n______________________\n", i);
        printItem(items[i], symbols, rules);
        printf("______________________\n\n");
    }
}

void destroyItems(ItemElement ***items) {
    int i;
    for (i = 0; i < arrlen(items); ++i) destroyItem(items[i]);
    arrfree(items);
}

void freeGotos(int **gotos) {
    int i;
    for (i = 0; i < arrlen(gotos); ++i) arrfree(gotos[i]);
    arrfree(gotos);
}

void printGotos(int **gotos, Symbol *symbols) {
    int i;
    puts("GOTOS\n___________");
    for (i = 0; i < arrlen(gotos); ++i) {
        int j;
        printf("\n");
        for (j = 0; j < arrlen(gotos[i]); ++j) {
            if (gotos[i][j] >= 0) printf("I%d ----%s---> I%d\n", i, symbols[j].name, gotos[i][j]);
        }
    }
}

void logSymbols(FILE *logFile, Symbol *symbols) {
    int i;
    for (i = 0; i < arrlen(symbols); ++i) {
        if (symbols[i].isTerminal) break;
    }

    fprintf(logFile, "%ld %d\n", arrlen(symbols) - i - 2, i - 1);

    for (; i < arrlen(symbols) - 2; ++i) {
        if (symbols[i].isTerminal) fprintf(logFile, "%s\n", symbols[i].name);
    }
}

void logRules(FILE *logFile, int ***rules) {
    int sum = 0;
    int i;
    for (i = 1; i < arrlen(rules); ++i) {
        int j;
        for (j = 0; j < arrlen(rules[i]); ++j) ++sum;
    }

    fprintf(logFile, "%d\n", sum);

    for (i = 1; i < arrlen(rules); ++i) {
        int j;
        for (j = 0; j < arrlen(rules[i]); ++j) {
            fprintf(logFile, "%d %ld\n", i - 1, arrlen(rules[i][j]));
        }
    }
}

int getSingleRuleIndex(int ***rules, int variableIndex, int ruleIndex) {
    int index = 0;
    int i;
    for (i = 0; i <= variableIndex; ++i) {
        int j;
        for (j = 0; j < (i < variableIndex? arrlen(rules[i]) : ruleIndex); ++j) ++index;
    }
    return index;
}

int getReducer(ItemElement **item, int symbolIndex, int ***rules) {
    int i;
    int answer = -1;
    for (i = 0; i < arrlen(item); ++i) {
        ItemElement *element = item[i];
        int *rule = rules[element->variableIndex][element->ruleIndex];
        if (element->dotIndex == arrlen(rule) && existsInSet(element->allowedFollowSet, symbolIndex)) {
            if (answer != -1) {
                puts("REDUCE-REDUCE CONFLICT FOUND");
                return answer;
            }
            else answer = getSingleRuleIndex(rules, element->variableIndex, element->ruleIndex);
        }
    }
    return answer;
}

void logItemsAndGotos(FILE *logFile, ItemElement ***items, int **gotos, Symbol *symbols, int ***rules) {
    int itemIndex;
    fprintf(logFile, "%ld\n", arrlen(items));
    for (itemIndex = 0; itemIndex < arrlen(items); ++itemIndex) {
        ItemElement **item = items[itemIndex];
        int symbolIndex;
        for (symbolIndex = 1; symbolIndex < arrlen(symbols); ++symbolIndex) {
            if (symbols[symbolIndex].isTerminal) {
                int shiftValue = gotos[itemIndex][symbolIndex];
                int reduceValue = getReducer(item, symbolIndex, rules);
                
                if (shiftValue >= 0 && reduceValue >= 0) puts("SHIFT-REDUCE CONFLICT FOUND");

                if (shiftValue >= 0) fprintf(logFile, "%d ", shiftValue);
                else {
                    if (reduceValue >= 0) fprintf(logFile, "%d ", -reduceValue);
                    else fprintf(logFile, "0 ");
                }
            }
            else {
                fprintf(logFile, "%d ", gotos[itemIndex][symbolIndex]);
            }
        }
        fprintf(logFile, "\n");
    }
}

void logParseTable(const char *logFileName, Symbol *symbols, int ***rules, ItemElement ***items, int **gotos) {
    FILE *logFile = fopen(logFileName, "w");

    if (!logFile) {
        fprintf(stderr, "Could not open log file : %s\n", logFileName);
        exit(1);
    }

    logSymbols(logFile, symbols);
    logRules(logFile, rules);
    logItemsAndGotos(logFile, items, gotos, symbols, rules);

    fclose(logFile);
}

ItemElement **mergeClosures(ItemElement **itemElements, Symbol *symbols, int ***rules, IntSet **firstSetArray) {
    int i;
    ItemElement **result;
    
    if (!itemElements) return NULL;

    result = getClosureOf(itemElements[0], symbols, rules, firstSetArray);
    for (i = 1; i < arrlen(itemElements); ++i) {
        ItemElement **closure = getClosureOf(itemElements[i], symbols, rules, firstSetArray);
        mergeItem(&result, closure);
        destroyItem(closure);
    }
    return result;
}

ItemElement **getNexts(ItemElement **item, int symbolIndex, int ***rules) {
    ItemElement **result = NULL;
    int elementIndex;
    for (elementIndex = 0; elementIndex < arrlen(item); ++elementIndex) {
        ItemElement *element = item[elementIndex];
        int *rule = rules[element->variableIndex][element->ruleIndex];
        if (element->dotIndex < arrlen(rule) && rule[element->dotIndex] == symbolIndex) {
            arrput(result, getNextElement(element, rules));
        }
    }
    return result;
}

ItemElement **getMergedClosure(ItemElement **item, int symbolIndex, Symbol *symbols, int ***rules, IntSet **firstSetArray) {
    ItemElement **nexts = getNexts(item, symbolIndex, rules);
    if (!nexts) return NULL;
    else {
        ItemElement **closure = mergeClosures(nexts, symbols, rules, firstSetArray);
        arrfree(nexts);
        return closure;
    }
}

ItemElement *getElementByRuleAndDot(ItemElement **item, int variableIndex, int ruleIndex, int dotIndex) {
    int elementIndex;
    for (elementIndex = 0; elementIndex < arrlen(item); ++elementIndex) {
        ItemElement *element = item[elementIndex];
        if (element->variableIndex == variableIndex && element->ruleIndex == ruleIndex && element->dotIndex == dotIndex) return element;
    }
    return NULL;
}

void propagateFollow(int itemIndex, ItemElement ***items, int **gotos, int ***rules) {
    ItemElement **item = items[itemIndex];
    int elementIndex;
    
    int *nextPropagations = NULL;

    for (elementIndex = 0; elementIndex < arrlen(item); ++elementIndex) {
        ItemElement *element = item[elementIndex];
        int *rule = rules[element->variableIndex][element->ruleIndex];
        if (element->dotIndex < arrlen(rule)) {
            int symbolIndexUnderDot = rule[element->dotIndex];
            if (itemIndex < arrlen(gotos) && gotos[itemIndex][symbolIndexUnderDot] >= 0) {
                int nextItemIndex = gotos[itemIndex][symbolIndexUnderDot];
                ItemElement **nextItem = items[nextItemIndex];
                ItemElement *correspondingElement = getElementByRuleAndDot(nextItem, element->variableIndex, element->ruleIndex, element->dotIndex + 1);
                if (correspondingElement) {
                    if (!isSuperSet(correspondingElement->allowedFollowSet, element->allowedFollowSet)) {
                        int *elementFollows = getContentsOfSet(element->allowedFollowSet);
                        int i;
                        for (i = 0; i < arrlen(elementFollows); ++i) {
                            putInSet(correspondingElement->allowedFollowSet, elementFollows[i]);
                        }
                        arrfree(elementFollows);

                        arrput(nextPropagations, nextItemIndex);
                    }
                }
                else {
                    fprintf(stderr, "Could not find corresponding element! Something went wrong perhaps\n");
                    exit(1);
                }
            }
        }
    }
    
    {
        int i;
        for (i = 0; i < arrlen(nextPropagations); ++i) {
            propagateFollow(nextPropagations[i], items, gotos, rules);
        }
        arrfree(nextPropagations);
    }

}
