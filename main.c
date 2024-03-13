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

int main(void) {
    Symbol *symbols;
    int ***ruleBodies;

    parseFromGrammarFile("grammar.txt", &symbols, &ruleBodies);

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
                for (k = 0; k < arrlen(bodies); ++k) {
                    int *body = bodies[k];
                    printf("%s -> ", symbols[i].name);
                    {
                        int j;
                        for (j = 0; j < arrlen(body); ++j) {
                            printf("%s", symbols[body[j]].name);
                        }
                    }
                    printf("\n");
                }
            }
        }
    }
    
    arrfree(symbols);
    {
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

    return 0;
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
        Symbol dummyStart = {"S'", 0};
        Symbol epsilon = {"#", 1};
        Symbol endOfFile = {"$", 1};

        arrput(symbols, dummyStart);
        arrput(symbols, epsilon);
        arrput(symbols, endOfFile);
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
            int i;
            for (i = 0; i < arrlen(symbols); ++i) {
                arrput(ruleBodies, NULL);
            }
        }
        
        {
            int *dummyStartToActualStartRuleDerivation = NULL;
            arrput(dummyStartToActualStartRuleDerivation, 3);
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
