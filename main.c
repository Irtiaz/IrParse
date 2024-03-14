#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Symbol.h"

char **split(char *string, const char *delimeter);
char *strdup(const char *string);
int getIndexOfSymbol(Symbol *symbols, const char *symbolName);
void parseFromGrammarFile(const char *grammarFileName, Symbol **symbolsArray, int ****ruleBodiesArray);
int *concat(int *arr1, int *arr2);
int *flattenWithSingleRule(int *previousRule, int *laterRule);
void flattenWithMultipleRules(int ***result, int **previousRules, int *laterRule);
void printGrammarInfo(Symbol *symbols, int ***ruleBodies, int ruleBreakPoint);
void eliminateLeftRecursion(Symbol *originalSymbols, int ***originalRules, Symbol **modifiedSymbols, int ****rulesAfterElimination);
void freeRuleBodies(int ***ruleBodies);
Symbol *copySymbols(Symbol *symbols);
int ***copyRules(int ***rules);

int main(void) {
    Symbol *symbols;
    int ***ruleBodies;

    parseFromGrammarFile("left-recursion.txt", &symbols, &ruleBodies);
    printGrammarInfo(symbols, ruleBodies, arrlen(ruleBodies));

    {
        Symbol *modifiedSymbols;
        int ***modifiedRules;
        eliminateLeftRecursion(symbols, ruleBodies, &modifiedSymbols, &modifiedRules);
        
        puts("\nLeft recursion eliminated");
        printGrammarInfo(modifiedSymbols, modifiedRules, arrlen(ruleBodies));

        arrfree(modifiedSymbols);
        freeRuleBodies(modifiedRules);
    }

    arrfree(symbols);
    freeRuleBodies(ruleBodies);

    return 0;
}

void freeRuleBodies(int ***ruleBodies) {
    int i;
    for (i = 0; i < arrlen(ruleBodies); ++i) {
        int j;
        for (j = 0; j < arrlen(ruleBodies[i]); ++j) {
            arrfree(ruleBodies[i][j]);
        }
        arrfree(ruleBodies[i]);
    }
    arrfree(ruleBodies);
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

void parseFromGrammarFile(const char *grammarFileName, Symbol **symbolsArray, int ****ruleBodiesArray) {
    Symbol *symbols = NULL;
    int ***ruleBodies = NULL;

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

        fscanf(grammarFile, "%[^(\r)?\n]s", line);
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


        fscanf(grammarFile, "%[^(\r)?\n]s", line);
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
                arrput(ruleBodies, NULL);
            }
        }

        {
            int *dummyStartToActualStartRuleDerivation = NULL;
            arrput(dummyStartToActualStartRuleDerivation, 1);
            arrput(ruleBodies[0], dummyStartToActualStartRuleDerivation);
        }

        while (fscanf(grammarFile, "%[^(\r)?\n]s", line) != EOF) {
            char **array = split(line, " ");
            int *ruleDerivationArray = NULL;
            int i;

            int variableIndex = getIndexOfSymbol(symbols, array[0]);
            free(array[0]);

            for (i = 1; i < arrlen(array); ++i) {
                arrput(ruleDerivationArray, getIndexOfSymbol(symbols, array[i]));
                free(array[i]);
            }

            arrput(ruleBodies[variableIndex], ruleDerivationArray);

            arrfree(array);
            fgetc(grammarFile);
        }

        fclose(grammarFile);
    }

    *symbolsArray = symbols;
    *ruleBodiesArray = ruleBodies;
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

void printGrammarInfo(Symbol *symbols, int ***ruleBodies, int ruleBreakPoint) {
    int eofSymbolIndex = getIndexOfSymbol(symbols, "$");

    {
        int i;
        for (i = 0; i < arrlen(symbols); ++i) {
            printf("%s : %s\n", symbols[i].name, symbols[i].isTerminal? "terminal" : "non-terminal");
        }
    }

    {
        int i;
        for (i = 0; i < arrlen(ruleBodies); ++i) {
            int **bodies = ruleBodies[i];
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
