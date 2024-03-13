#ifndef SYMBOL_H_325345
#define SYMBOL_H_325345

#define SYMBOL_NAME_MAX_SIZE 20

typedef struct {
    char name[SYMBOL_NAME_MAX_SIZE];
    int isTerminal;
} Symbol;

#endif
