#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

#include <stdio.h>

#define TOKEN_NAME_MAX_LENGTH 30

typedef struct {
    char *key;
    int value;
} StringHashMapItem;

typedef struct {
    int variableColumnIndex;
    int popCount;
} RuleDescriptor;

char tokens[][TOKEN_NAME_MAX_LENGTH] = {
    "id", "+", "id"
};
int tokenIndex = 0;

char *getNextToken();

int main(void) {
    FILE *parseTableFile;
    StringHashMapItem *tokenMap;
    int rowCount, columnCount;
    RuleDescriptor *descriptors;
    int **table;

    parseTableFile = fopen("parse-table.txt", "r");
    if (!parseTableFile) {
        fprintf(stderr, "Could not find parse table file\n");
        exit(1);
    }
    
    sh_new_strdup(tokenMap);
    {
        int numberOfTokens, tokenStart;
        fscanf(parseTableFile, "%d %d", &numberOfTokens, &tokenStart);

        {
            int i;
            for (i = 0; i < numberOfTokens; ++i) {
                char tokenName[TOKEN_NAME_MAX_LENGTH];
                fscanf(parseTableFile, "%s", tokenName);
                
                shput(tokenMap, tokenName, tokenStart++);
            }
        }

        shput(tokenMap, "#", tokenStart++);
        shput(tokenMap, "$", tokenStart++);

        columnCount = tokenStart;
    }
    
    {
        int ruleDescriptorCount;
        fscanf(parseTableFile, "%d", &ruleDescriptorCount);

        descriptors = (RuleDescriptor *)malloc(sizeof(RuleDescriptor) * ruleDescriptorCount);
        
        {
            int i;
            for (i = 0; i < ruleDescriptorCount; ++i) {
                fscanf(parseTableFile, "%d %d", &descriptors[i].variableColumnIndex, &descriptors[i].popCount);
            }
        }    
    }

    {
        fscanf(parseTableFile, "%d", &rowCount); 
        
        table = (int **)malloc(rowCount * sizeof(int *));
        {
            int row;
            for (row = 0; row < rowCount; ++row) {
                table[row] = (int *)malloc(columnCount * sizeof(int));
            }
        }

        {
            int row;
            for (row = 0; row < rowCount; ++row) {
                int column;
                for (column = 0; column < columnCount; ++column) {
                    fscanf(parseTableFile, "%d", &table[row][column]);
                }
            }
        }
    }

    {
        int row;
        for (row = 0; row < rowCount; ++row) {
            free(table[row]);
        }
        free(table);
    }


    free(descriptors);
    shfree(tokenMap);
    fclose(parseTableFile);

    return 0;
}

char *getNextToken() {
    return tokens[tokenIndex++];
}
