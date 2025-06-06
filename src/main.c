#include <stdio.h>
#include "lexer.h"
#include "parser.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <archivo.cpp> [tokens.obj]\n", argv[0]);
        return 1;
    }

    FILE *fp = fopen(argv[1], "r");
    if (!fp) {
        perror("fopen");
        return 1;
    }

    init_lexer(fp);
    tokenize_input();

    if (argc >= 3) {
        FILE *out = fopen(argv[2], "w");
        if (!out) {
            perror("fopen");
            return 1;
        }
        for (int i = 0; i < num_tokens; i++) {
            fprintf(out, "%d:\t%d\t%s\n", tokens[i].line, tokens[i].type, tokens[i].lexeme);
        }
        fclose(out);
        rewind(fp);
        init_lexer(fp);
        tokenize_input();
    }

    cur_token = 0;
    parse_program();

    printf("OK\n");
    fclose(fp);
    return 0;
}
