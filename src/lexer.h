#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>

#define MAX_TOKENS 2048
#define MAX_LEXEME_LEN 128

// Tipos de token
typedef enum {
    TOK_INT,
    TOK_CHAR,
    TOK_FLOAT,
    TOK_PRINT,
    TOK_READ,
    TOK_IF,
    TOK_ELSE,
    TOK_WHILE,
    TOK_SUM,
    TOK_SWITCH,
    TOK_CASE,
    TOK_DEFAULT,
    TOK_BREAK,
    TOK_IDENT,
    TOK_NUM,
    TOK_STRING,
    TOK_COMMA,
    TOK_SEMI,
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_LBRACE,
    TOK_RBRACE,
    TOK_COLON,
    TOK_ASSIGN,
    TOK_EQ,
    TOK_NEQ,
    TOK_LT,
    TOK_LE,
    TOK_GT,
    TOK_GE,
    TOK_PLUS,
    TOK_MINUS,
    TOK_MULT,
    TOK_DIV,
    TOK_EOF,
    TOK_UNKNOWN
} TokenType;

typedef struct {
    TokenType type;
    char lexeme[MAX_LEXEME_LEN];
    int line;
} Token;

extern Token tokens[MAX_TOKENS];
extern int num_tokens;
extern int cur_token;

void init_lexer(FILE *fp);
void tokenize_input(void);
TokenType lookahead(void);
void match(TokenType expected);
char *expect_ident(void);

#endif
