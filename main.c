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
void flattenWithMultipleRules(int ***result, int **previousRules, int *laterRule);
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

int main(void) {
    Symbol *symbols;
    int ***rules;

    parseFromGrammarFile("grammar.txt", &symbols, &rules);
    printGrammarInfo(symbols, rules, arrlen(rules));

    {
        IntSet **firstSetArray = getFirstSetArray(symbols, rules);
        {
            int i;
            for (i = 0; i < arrlen(symbols); ++i) {
                int *contentsOfFirstSet = getContentsOfSet(firstSetArray[i]);
                int j;
                printf("First(%s) = ", symbols[i].name);
                for (j = 0; j < arrlen(contentsOfFirstSet); ++j) {
                    printf("%s ", symbols[contentsOfFirstSet[j]].name);
                }
                printf("\n");
                arrfree(contentsOfFirstSet); 
            }
        }

        {
            IntSet *followSetForRoot = createIntSet();
            ItemElement *element;
            putInSet(followSetForRoot, arrlen(symbols) - 1);
            element = createItemElement(0, 0, 0, followSetForRoot);
            destroyIntSet(followSetForRoot);
            {
                ItemElement **result = getClosureOf(element, symbols, rules, firstSetArray);
                int resultIndex;
                printItem(result, symbols, rules);
                for (resultIndex = 0; resultIndex < arrlen(result); ++resultIndex) {
                    destroyItemElement(result[resultIndex]);
                }
                arrfree(result);
            }
        }

        {
            int i;
            for (i = 0; i < arrlen(firstSetArray); ++i) destroyIntSet(firstSetArray[i]);
        }
        arrfree(firstSetArray);
    }


    arrfree(symbols);
    freeRules(rules);

    return 0;
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

void flattenWithMultipleRules(int ***result, int **previousRules, int *laterRule) {
    int i;
    arrdel(laterRule, 0);
    for (i = 0; i < arrlen(previousRules); ++i) {
        arrput(*result, flattenWithSingleRule(previousRules[i], laterRule));
    }
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
                            printf("%s", symbols[body[j]].name);
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
                    flattenWithMultipleRules(&modifiedRules, rules[firstVariableInRule], rule);
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
                if (isNullRule(nullables, rules[variableIndex][ruleIndex]) || isNullRule(nullables, rules[variableIndex][ruleIndex])) {
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
    return root->dotIndex < arrlen(rule) - 1? getFirstOfSubRule(symbols, rule, root->dotIndex + 1, firstSetArray) : createCopyOfIntSet(root->allowedFollowSet);
}

ItemElement **getSingleRunClosure(ItemElement *root, Symbol *symbols, int ***rules, IntSet **firstSetArray) {
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

ItemElement **getClosureOf(ItemElement *root, Symbol *symbols, int ***rules, IntSet **firstSetArray) {
   ItemElement **item = NULL; 
   arrput(item, root);

   {
       int found = 1;
       while (found) {
           int currentLength = arrlen(item);
           found = 0;
           {
               int elementIndex;
               for (elementIndex = 0; elementIndex < currentLength; ++elementIndex) {
                   ItemElement *element = item[elementIndex];
                   ItemElement **singleRunClosure = getSingleRunClosure(element, symbols, rules, firstSetArray);
                   int closureIndex;
                   for (closureIndex = 0; closureIndex < arrlen(singleRunClosure); ++closureIndex) {
                       ItemElement *closure = singleRunClosure[closureIndex];
                       int checkIndex;
                       for (checkIndex = 0; checkIndex < arrlen(item); ++checkIndex) {
                           if (elementsAreSame(item[checkIndex], closure)) {
                               break;
                           }
                       }
                       if (checkIndex >= arrlen(item)) {
                           found = 1;
                           arrput(item, closure);
                       }
                       else destroyItemElement(closure);
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
