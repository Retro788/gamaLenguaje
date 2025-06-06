#ifndef SYMTAB_H
#define SYMTAB_H

#include "lexer.h"

#define MAX_VARS 256

typedef struct {
    char name[MAX_LEXEME_LEN];
    int value;
    int is_defined;
} Symbol;

extern Symbol symtab[MAX_VARS];
extern int num_vars;

int lookup_symbol(const char *name);
int add_symbol(const char *name);
void set_symbol_value(const char *name, int val);
int get_symbol_value(const char *name);

#endif
