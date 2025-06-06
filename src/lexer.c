#include "lexer.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

Token tokens[MAX_TOKENS];
int num_tokens = 0;
int cur_token = 0;

static FILE *input_fp = NULL;
static int current_line = 1;

static int next_char(void) {
    int c = fgetc(input_fp);
    if (c == '\n')
        current_line++;
    return (c == EOF ? EOF : c);
}

static void unget_char(int c) {
    if (c != EOF) {
        if (c == '\n')
            current_line--;
        ungetc(c, input_fp);
    }
}

static void add_token(TokenType type, const char *lexe) {
    if (num_tokens >= MAX_TOKENS) {
        fprintf(stderr, "Error: demasiados tokens (>= %d).\n", MAX_TOKENS);
        exit(1);
    }
    tokens[num_tokens].type = type;
    strncpy(tokens[num_tokens].lexeme, lexe, MAX_LEXEME_LEN - 1);
    tokens[num_tokens].lexeme[MAX_LEXEME_LEN - 1] = '\0';
    tokens[num_tokens].line = current_line;
    num_tokens++;
}

void init_lexer(FILE *fp) {
    input_fp = fp;
    current_line = 1;
    num_tokens = 0;
    cur_token = 0;
}

static TokenType yylex(void) {
    int c;
    char buffer[MAX_LEXEME_LEN];
    int len;

    do {
        c = next_char();
    } while (c == ' ' || c == '\t' || c == '\n' || c == '\r');

    if (c == EOF) {
        return TOK_EOF;
    }

    if (isalpha(c)) {
        len = 0;
        do {
            if (len < MAX_LEXEME_LEN - 1) {
                buffer[len++] = (char)c;
            }
            c = next_char();
        } while (isalpha(c) || isdigit(c));
        buffer[len] = '\0';
        unget_char(c);

        char tmp[MAX_LEXEME_LEN];
        for (int i = 0; buffer[i]; i++)
            tmp[i] = tolower((unsigned char)buffer[i]);
        tmp[len] = '\0';

        if (strcmp(tmp, "entero") == 0) { add_token(TOK_INT, buffer); return TOK_INT; }
        if (strcmp(tmp, "caracter") == 0) { add_token(TOK_CHAR, buffer); return TOK_CHAR; }
        if (strcmp(tmp, "flotante") == 0) { add_token(TOK_FLOAT, buffer); return TOK_FLOAT; }
        if (strcmp(tmp, "imprimir") == 0) { add_token(TOK_PRINT, buffer); return TOK_PRINT; }
        if (strcmp(tmp, "leer") == 0) { add_token(TOK_READ, buffer); return TOK_READ; }
        if (strcmp(tmp, "suma") == 0) { add_token(TOK_SUM, buffer); return TOK_SUM; }
        if (strcmp(tmp, "si") == 0) { add_token(TOK_IF, buffer); return TOK_IF; }
        if (strcmp(tmp, "sino") == 0) { add_token(TOK_ELSE, buffer); return TOK_ELSE; }
        if (strcmp(tmp, "mientras") == 0) { add_token(TOK_WHILE, buffer); return TOK_WHILE; }
        if (strcmp(tmp, "switch") == 0) { add_token(TOK_SWITCH, buffer); return TOK_SWITCH; }
        if (strcmp(tmp, "caso") == 0) { add_token(TOK_CASE, buffer); return TOK_CASE; }
        if (strcmp(tmp, "predeterminado") == 0) { add_token(TOK_DEFAULT, buffer); return TOK_DEFAULT; }
        if (strcmp(tmp, "romper") == 0) { add_token(TOK_BREAK, buffer); return TOK_BREAK; }

        add_token(TOK_IDENT, buffer);
        return TOK_IDENT;
    }

    if (c == '"') {
        len = 0;
        c = next_char();
        while (c != '"' && c != EOF && c != '\n') {
            if (len < MAX_LEXEME_LEN - 1) buffer[len++] = (char)c;
            c = next_char();
        }
        buffer[len] = '\0';
        if (c != '"') {
            fprintf(stderr, "Error: cadena sin cerrar en linea %d.\n", current_line);
            exit(1);
        }
        add_token(TOK_STRING, buffer);
        return TOK_STRING;
    }

    if (isdigit(c)) {
        len = 0;
        do {
            if (len < MAX_LEXEME_LEN - 1) buffer[len++] = (char)c;
            c = next_char();
        } while (isdigit(c));
        buffer[len] = '\0';
        unget_char(c);
        add_token(TOK_NUM, buffer);
        return TOK_NUM;
    }

    if (c == '=') {
        int next = next_char();
        if (next == '=') { add_token(TOK_EQ, "=="); return TOK_EQ; }
        unget_char(next); add_token(TOK_ASSIGN, "="); return TOK_ASSIGN;
    }
    if (c == '!') {
        int next = next_char();
        if (next == '=') { add_token(TOK_NEQ, "!="); return TOK_NEQ; }
        unget_char(next); add_token(TOK_UNKNOWN, "!"); return TOK_UNKNOWN;
    }
    if (c == '<') {
        int next = next_char();
        if (next == '=') { add_token(TOK_LE, "<="); return TOK_LE; }
        unget_char(next); add_token(TOK_LT, "<"); return TOK_LT;
    }
    if (c == '>') {
        int next = next_char();
        if (next == '=') { add_token(TOK_GE, ">="); return TOK_GE; }
        unget_char(next); add_token(TOK_GT, ">"); return TOK_GT;
    }

    switch (c) {
        case ',': add_token(TOK_COMMA, ","); return TOK_COMMA;
        case ';': add_token(TOK_SEMI, ";"); return TOK_SEMI;
        case '(': add_token(TOK_LPAREN, "("); return TOK_LPAREN;
        case ')': add_token(TOK_RPAREN, ")"); return TOK_RPAREN;
        case '{': add_token(TOK_LBRACE, "{"); return TOK_LBRACE;
        case '}': add_token(TOK_RBRACE, "}"); return TOK_RBRACE;
        case ':': add_token(TOK_COLON, ":"); return TOK_COLON;
        case '+': add_token(TOK_PLUS, "+"); return TOK_PLUS;
        case '-': add_token(TOK_MINUS, "-"); return TOK_MINUS;
        case '*': add_token(TOK_MULT, "*"); return TOK_MULT;
        case '/': add_token(TOK_DIV, "/"); return TOK_DIV;
        default:
            buffer[0] = (char)c; buffer[1] = '\0';
            add_token(TOK_UNKNOWN, buffer);
            return TOK_UNKNOWN;
    }
}

void tokenize_input(void) {
    TokenType t;
    do {
        t = yylex();
    } while (t != TOK_EOF);
    add_token(TOK_EOF, "EOF");
}

TokenType lookahead(void) {
    if (cur_token < num_tokens)
        return tokens[cur_token].type;
    return TOK_EOF;
}

void match(TokenType expected) {
    if (lookahead() == expected) {
        cur_token++;
    } else {
        fprintf(stderr, "Error de sintaxis: se esperaba otro token.\n");
        exit(1);
    }
}

char *expect_ident(void) {
    if (lookahead() == TOK_IDENT) {
        char *name = tokens[cur_token].lexeme;
        cur_token++;
        return name;
    }
    fprintf(stderr, "Error de sintaxis: se esperaba IDENT.\n");
    exit(1);
}
