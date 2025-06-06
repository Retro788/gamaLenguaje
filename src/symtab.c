#include "symtab.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

Symbol symtab[MAX_VARS];
int num_vars = 0;

int lookup_symbol(const char *name) {
    for (int i = 0; i < num_vars; i++) {
        if (strcmp(symtab[i].name, name) == 0)
            return i;
    }
    return -1;
}

int add_symbol(const char *name) {
    if (num_vars >= MAX_VARS) {
        fprintf(stderr, "Error: demasiadas variables (>= %d).\n", MAX_VARS);
        exit(1);
    }
    int idx = lookup_symbol(name);
    if (idx != -1)
        return idx;
    strcpy(symtab[num_vars].name, name);
    symtab[num_vars].value = 0;
    symtab[num_vars].is_defined = 0;
    num_vars++;
    return num_vars - 1;
}

void set_symbol_value(const char *name, int val) {
    int idx = lookup_symbol(name);
    if (idx < 0)
        idx = add_symbol(name);
    symtab[idx].value = val;
    symtab[idx].is_defined = 1;
}

int get_symbol_value(const char *name) {
    int idx = lookup_symbol(name);
    if (idx < 0) {
        fprintf(stderr, "Error: variable '%s' no declarada.\n", name);
        exit(1);
    }
    if (symtab[idx].is_defined == 0) {
        fprintf(stderr, "Error: variable '%s' no inicializada.\n", name);
        exit(1);
    }
    return symtab[idx].value;
}
