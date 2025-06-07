/**************************************************************
 * analyzer.c
 *
 * Mini‐analizador sintáctico + intérprete (parser de descenso recursivo)
 * para un lenguaje muy sencillo que reconoce:
 *
 *   - Declaración de variables:   Entero a = 8, b, c = 5;
 *   - Salida (Imprimir):          Imprimir( a + b );
 *   - Entrada (Leer):             Leer( x );
 *   - Asignación/ariméticas:      x = y * (z + 2) - 5;
 *   - Condicional (Si/Sino):      Si ( x < 10 ) Imprimir(x); Sino x = 0;
 *   - Bucle (Mientras):           Mientras ( x > 0 ) { Imprimir(x); x = x - 1; }
 *   - Bloques:                    { stmt1; stmt2; ... }
 *
 * Gramática (BNF):
 *
 *   <program>        ::= <stmt_list> EOF
 *
 *   <stmt_list>      ::= <stmt> <stmt_list> | ε
 *
 *   <stmt>           ::= <decl_stmt>
 *                     | <print_stmt>
 *                     | <read_stmt>
 *                     | <assign_stmt>
 *                     | <if_stmt>
 *                     | <while_stmt>
 *                     | <block_stmt>
 *
 *   <decl_stmt>      ::= <type> <var_list> ';'
 *   <type>           ::= 'Entero' | 'Caracter' | 'Flotante'
 *   <var_list>       ::= <var_decl> ( ',' <var_decl> )*
 *   <var_decl>       ::= IDENT [ '=' <expr> ]
 *
 *   <print_stmt>     ::= 'Imprimir' '(' <expr> ')' ';'
 *   <read_stmt>      ::= 'Leer' '(' IDENT ')' ';'
 *
 *   <assign_stmt>    ::= IDENT '=' <expr> ';'
 *
 *   <if_stmt>        ::= 'Si' '(' <expr> ')' <stmt> [ 'Sino' <stmt> ]
 *   <while_stmt>     ::= 'Mientras' '(' <expr> ')' <stmt>
 *
 *   <block_stmt>     ::= '{' <stmt_list> '}'
 *
 *   <expr>           ::= <rel_expr>
 *   <rel_expr>       ::= <add_expr> ( ( '==' | '!=' | '<' | '>' | '<=' | '>=' ) <add_expr> )*
 *   <add_expr>       ::= <mul_expr> ( ( '+' | '-' ) <mul_expr> )*
 *   <mul_expr>       ::= <unary_expr> ( ( '*' | '/' ) <unary_expr> )*
 *   <unary_expr>     ::= [ '-' ] <primary>
 *   <primary>        ::= '(' <expr> ')' | NUM | IDENT
 *
 * Tokens léxicos:
 *   - IDENT: (Letra) (Letra|Dígito)*
 *   - NUM:   (Dígito)+
 *   - Palabras reservadas: Entero, Caracter, Flotante, Imprimir, Leer, Si, Sino, Mientras
 *   - Símbolos: ',' ';' '(' ')' '{' '}' 
 *   - Operadores: '+' '-' '*' '/' '%' '^'
 *   - Relacionales: '==' '!=' '<' '>' '<=' '>='
 *   - Asignación: '='
 *   - EOF  → TOK_EOF
 *   - Cualquier otro → TOK_UNKNOWN
 *
 * Para compilar en Windows (MinGW-w64):
 *   1) Asegúrate de que analyzer.c no tenga BOM. 
 *      Si usas VS Code, guárdalo como UTF-8 sin BOM. 
 *      O bien, ábrelo con el Bloc de notas y “Guardar como… → ANSI”.
 *
 *   2) Abre CMD (no PowerShell), navega a la carpeta:
 *      cd C:\Users\Retro788\Desktop\gamaLenguaje
 *
 *   3) Compila con:
 *      gcc -Wall -o analyzer analyzer.c
 *
 *   4) Ejecuta:
 *      analyzer.exe [archivo.txt]
 *      Si no especificas archivo, escribe tu programa en la consola
 *      y pulsa Ctrl+Z ⏎ al terminar. También puedes pasar el
 *      archivo por redirección con "analyzer.exe < archivo.txt".
 *
 * Si todo es correcto, el intérprete leerá tu input (línea por línea)
 * e imprimirá los resultados correspondientes.  
 *
 **************************************************************/


 #define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
 
 /*==============================================================
  *                       DEFINICIONES GLOBALES
  *=============================================================*/
 
#define MAX_TOKENS       2048
#define MAX_LEXEME_LEN    128
#define MAX_VARS          256
#define MAX_SOURCE_LEN  65536
 
 /*--------------------------------------------------------------
  * Tipo de datos para variables en la tabla de símbolos. 
  * Para simplificar, todas son enteros (Entero, Caracter, Flotante 
  * → se almacenan como int). 
  *-------------------------------------------------------------*/
 typedef struct {
     char   name[MAX_LEXEME_LEN];  // Identificador
     int    value;                 // Valor
     int    is_defined;            // 0 = no existe aún, 1 = ya existe
 } Symbol;
 
 /*--------------------------------------------------------------
  * Vector global para guardar variables: 
  *   symtab[0..num_vars-1] 
  *-------------------------------------------------------------*/
 static Symbol symtab[MAX_VARS];
 static int    num_vars = 0;
 
 /*--------------------------------------------------------------
  * Enumeración de tokens (TOK_XXX) 
  *-------------------------------------------------------------*/
 typedef enum {
     // palabras reservadas
     TOK_INT,       // “Entero”
     TOK_CHAR,      // “Caracter”
     TOK_FLOAT,     // “Flotante”
    TOK_PRINT,     // “Imprimir”
    TOK_READ,      // “Leer”
    TOK_IF,        // “Si”
    TOK_ELSE,      // “Sino”
    TOK_WHILE,     // “Mientras”
    TOK_VAR,       // "var"
    TOK_CONST,     // "const"
    TOK_ITEMS,     // "items"
    TOK_ITEM,      // "item"
 
     // identificador y número
    TOK_IDENT,     // identificador: letra( letra|dígito )*
    TOK_NUM,       // número: dígito+
    TOK_STRING,    // cadena entre comillas
 
     // operadores y símbolos
     TOK_COMMA,     // ‘,’
     TOK_SEMI,      // ‘;’
     TOK_LPAREN,    // ‘(’
     TOK_RPAREN,    // ‘)’
     TOK_LBRACE,    // ‘{’
     TOK_RBRACE,    // ‘}’
 
     TOK_ASSIGN,    // ‘=’
     TOK_EQ,        // ‘==’
     TOK_NEQ,       // ‘!=’
     TOK_LT,        // ‘<’
     TOK_LE,        // ‘<=’
     TOK_GT,        // ‘>’
     TOK_GE,        // ‘>=’
 
     TOK_PLUS,      // ‘+’
     TOK_MINUS,     // ‘-’
    TOK_MULT,      // ‘*’
    TOK_DIV,       // ‘/’
    TOK_MOD,       // '%'
    TOK_POW,       // '^'
 
     TOK_EOF,       // fin de archivo 
     TOK_UNKNOWN    // cualquier otro
 } TokenType;
 
 /*--------------------------------------------------------------
  * Un token consta de su tipo y su lexema (texto). 
  *-------------------------------------------------------------*/
 typedef struct {
     TokenType type;
     char      lexeme[MAX_LEXEME_LEN];
 } Token;
 
 /*--------------------------------------------------------------
  * Vector global de tokens (producidos por el lexer) y 
  * contadores:
  *
  *   tokens[0..num_tokens-1]   = lista de tokens 
  *   num_tokens = # real de tokens
  *   cur_token  = índice del token “actual” para el parser
  *-------------------------------------------------------------*/
static Token tokens[MAX_TOKENS];
static int   num_tokens = 0;
static int   cur_token  = 0;

/* Buffer para guardar el codigo fuente leido */
static char source_buffer[MAX_SOURCE_LEN];
static int  source_len = 0;

/* Buffer para capturar la salida de ejecucion */
#define MAX_EXEC_LEN 65536
static char exec_buffer[MAX_EXEC_LEN];
static int  exec_len = 0;

/*--------------------------------------------------------------
 * Nombre textual de cada token (para depuración y .obj)
 *-------------------------------------------------------------*/
static const char *token_name(TokenType t) {
    switch (t) {
        case TOK_INT:    return "TOK_INT";
        case TOK_CHAR:   return "TOK_CHAR";
        case TOK_FLOAT:  return "TOK_FLOAT";
        case TOK_PRINT:  return "TOK_PRINT";
        case TOK_READ:   return "TOK_READ";
        case TOK_IF:     return "TOK_IF";
        case TOK_ELSE:   return "TOK_ELSE";
        case TOK_WHILE:  return "TOK_WHILE";
        case TOK_VAR:    return "TOK_VAR";
        case TOK_CONST:  return "TOK_CONST";
        case TOK_ITEMS:  return "TOK_ITEMS";
        case TOK_ITEM:   return "TOK_ITEM";
        case TOK_IDENT:  return "TOK_IDENT";
        case TOK_NUM:    return "TOK_NUM";
        case TOK_STRING: return "TOK_STRING";
        case TOK_COMMA:  return "TOK_COMMA";
        case TOK_SEMI:   return "TOK_SEMI";
        case TOK_LPAREN: return "TOK_LPAREN";
        case TOK_RPAREN: return "TOK_RPAREN";
        case TOK_LBRACE: return "TOK_LBRACE";
        case TOK_RBRACE: return "TOK_RBRACE";
        case TOK_ASSIGN: return "TOK_ASSIGN";
        case TOK_EQ:     return "TOK_EQ";
        case TOK_NEQ:    return "TOK_NEQ";
        case TOK_LT:     return "TOK_LT";
        case TOK_LE:     return "TOK_LE";
        case TOK_GT:     return "TOK_GT";
        case TOK_GE:     return "TOK_GE";
        case TOK_PLUS:   return "TOK_PLUS";
        case TOK_MINUS:  return "TOK_MINUS";
        case TOK_MULT:   return "TOK_MULT";
        case TOK_DIV:    return "TOK_DIV";
        case TOK_MOD:    return "TOK_MOD";
        case TOK_POW:    return "TOK_POW";
        case TOK_EOF:    return "TOK_EOF";
        default:         return "TOK_UNKNOWN";
    }
}

/*--------------------------------------------------------------
 * Registrar salida en exec_buffer
 *-------------------------------------------------------------*/
static void append_exec_output(const char *text) {
    size_t len = strlen(text);
    if (exec_len + len >= MAX_EXEC_LEN)
        len = MAX_EXEC_LEN - exec_len - 1;
    if (len > 0) {
        memcpy(exec_buffer + exec_len, text, len);
        exec_len += len;
        exec_buffer[exec_len] = '\0';
    }
}

/*--------------------------------------------------------------
 * Escribir los tokens en un archivo .obj
 *-------------------------------------------------------------*/
/* helper predicates for token categorization */
static int is_reserved(TokenType t) {
    switch (t) {
        case TOK_INT: case TOK_CHAR: case TOK_FLOAT:
        case TOK_PRINT: case TOK_READ: case TOK_IF:
        case TOK_ELSE: case TOK_WHILE:
        case TOK_VAR: case TOK_CONST: case TOK_ITEMS: case TOK_ITEM:
            return 1;
        default:
            return 0;
    }
}

static int is_symbol(TokenType t) {
    return (t == TOK_COMMA || t == TOK_SEMI || t == TOK_LPAREN ||
            t == TOK_RPAREN || t == TOK_LBRACE || t == TOK_RBRACE);
}

static int is_operator(TokenType t) {
    switch (t) {
        case TOK_PLUS: case TOK_MINUS: case TOK_MULT: case TOK_DIV:
        case TOK_MOD:  case TOK_POW:   case TOK_ASSIGN:
        case TOK_EQ:   case TOK_NEQ:   case TOK_LT: case TOK_LE:
        case TOK_GT:   case TOK_GE:
            return 1;
        default:
            return 0;
    }
}

static void write_lexical_sections(FILE *f) {
    fprintf(f, "-- Palabras reservadas --\n");
    for (int i = 0; i < num_tokens; i++)
        if (is_reserved(tokens[i].type))
            fprintf(f, "%s\t%s\n", token_name(tokens[i].type), tokens[i].lexeme);

    fprintf(f, "\n-- Identificadores --\n");
    for (int i = 0; i < num_tokens; i++)
        if (tokens[i].type == TOK_IDENT)
            fprintf(f, "%s\t%s\n", token_name(tokens[i].type), tokens[i].lexeme);

    fprintf(f, "\n-- Numeros --\n");
    for (int i = 0; i < num_tokens; i++)
        if (tokens[i].type == TOK_NUM)
            fprintf(f, "%s\t%s\n", token_name(tokens[i].type), tokens[i].lexeme);

    fprintf(f, "\n-- Cadenas --\n");
    for (int i = 0; i < num_tokens; i++)
        if (tokens[i].type == TOK_STRING)
            fprintf(f, "%s\t%s\n", token_name(tokens[i].type), tokens[i].lexeme);

    fprintf(f, "\n-- Operadores --\n");
    for (int i = 0; i < num_tokens; i++)
        if (is_operator(tokens[i].type))
            fprintf(f, "%s\t%s\n", token_name(tokens[i].type), tokens[i].lexeme);

    fprintf(f, "\n-- Simbolos --\n");
    for (int i = 0; i < num_tokens; i++)
        if (is_symbol(tokens[i].type))
            fprintf(f, "%s\t%s\n", token_name(tokens[i].type), tokens[i].lexeme);
}

static void write_tokens_to_obj(const char *filename,
                                const char *parse_result,
                                const char *exec_output) {
    FILE *f = fopen(filename, "w");
    if (!f) {
        perror("fopen");
        exit(1);
    }
    fprintf(f, "=== Codigo fuente ===\n");
    fwrite(source_buffer, 1, source_len, f);
    if (source_len == 0 || source_buffer[source_len-1] != '\n')
        fputc('\n', f);
    fprintf(f, "\n=== Lexer ===\n");
    write_lexical_sections(f);
    fprintf(f, "\n=== Parser ===\n%s\n", parse_result);
    fprintf(f, "\n=== Ejecucion ===\n");
    fputs(exec_output, f);
    fclose(f);
}
 
 
 /*==============================================================
  *                   FUNCIONES DE TABLA DE SÍMBOLOS
  *=============================================================*/
 
 /**
  * lookup_symbol(nombre):
  *   Busca si existe ya una variable con nombre “nombre” 
  *   en symtab. Si la encuentra, devuelve su índice [0..num_vars-1].
  *   Si no existe, devuelve -1.
  */
 static int lookup_symbol(const char *nombre) {
     for (int i = 0; i < num_vars; i++) {
         if (strcmp(symtab[i].name, nombre) == 0) {
             return i;
         }
     }
     return -1;
 }
 
 /**
  * add_symbol(nombre):
  *   Agrega una nueva variable a la tabla de símbolos con 
  *   valor 0 e is_defined=0. Devuelve el índice donde la insertó. 
  *   Si ya existe o si no hay espacio, aborta con error.
  */
 static int add_symbol(const char *nombre) {
     if (num_vars >= MAX_VARS) {
         fprintf(stderr, "Error: demasiadas variables (>= %d).\n", MAX_VARS);
         exit(1);
     }
     int idx = lookup_symbol(nombre);
     if (idx != -1) {
         // Ya existe
         return idx;
     }
     strcpy(symtab[num_vars].name, nombre);
     symtab[num_vars].value = 0;
     symtab[num_vars].is_defined = 0;
     num_vars++;
     return num_vars - 1;
 }
 
 /**
  * set_symbol_value(nombre, val):
  *   Busca la variable “nombre” en la tabla. Si no existe, la crea
  *   y luego le asigna el valor “val”. Si existe, simplemente actualiza
  *   su valor. Marca is_defined=1.
  */
 static void set_symbol_value(const char *nombre, int val) {
     int idx = lookup_symbol(nombre);
     if (idx < 0) {
         idx = add_symbol(nombre);
     }
     symtab[idx].value = val;
     symtab[idx].is_defined = 1;
 }
 
 /**
  * get_symbol_value(nombre):
  *   Devuelve el valor entero de la variable “nombre”. Si no existe
  *   o no fue inicializada (is_defined=0), da error y termina.
  */
 static int get_symbol_value(const char *nombre) {
     int idx = lookup_symbol(nombre);
     if (idx < 0) {
         fprintf(stderr, "Error: variable '%s' no declarada.\n", nombre);
         exit(1);
     }
     if (symtab[idx].is_defined == 0) {
         fprintf(stderr, "Error: variable '%s' no inicializada.\n", nombre);
         exit(1);
     }
     return symtab[idx].value;
 }
 
 
 /*==============================================================
  *                      ANALIZADOR LÉXICO
  *=============================================================*/
 
 /**
  * next_char():
  *   Lee un carácter de stdin (getchar). Devuelve EOF si ya no hay nada.
  */
static FILE *input_stream = NULL;

static int next_char(void) {
    int c = fgetc(input_stream);
    if (c != EOF && source_len < MAX_SOURCE_LEN - 1) {
        source_buffer[source_len++] = (char)c;
        source_buffer[source_len] = '\0';
    }
    return (c == EOF ? EOF : c);
}
 
 /**
  * unget_char(c):
  *   “Devuelve” c al flujo de entrada, para que next_char lo lea de nuevo.
  */
static void unget_char(int c) {
    if (c != EOF) {
        ungetc(c, input_stream);
        if (source_len > 0 && source_buffer[source_len-1] == (char)c) {
            source_len--;
            source_buffer[source_len] = '\0';
        }
    }
}
 
 /**
  * add_token(type, lexe):
  *   Agrega al arreglo “tokens” un nuevo token con tipo “type” y texto “lexe”.
  */
 static void add_token(TokenType type, const char *lexe) {
     if (num_tokens >= MAX_TOKENS) {
         fprintf(stderr, "Error: demasiados tokens (>= %d).\n", MAX_TOKENS);
         exit(1);
     }
     tokens[num_tokens].type = type;
     strncpy(tokens[num_tokens].lexeme, lexe, MAX_LEXEME_LEN - 1);
     tokens[num_tokens].lexeme[MAX_LEXEME_LEN - 1] = '\0';
     num_tokens++;
 }
 
 /**
  * yylex():
  *   Reconoce un solo token de la entrada estándar y lo añade a tokens[].
  *   Retorna el TokenType. Espacios/tab/newline se saltan:
  *    - Palabra que empieza con letra    → IDENT o palabra reservada
  *    - Secuencia de dígitos             → NUM
  *    - “==”, “!=”, “<=”, “>=”            → TOK_EQ/TOK_NEQ/TOK_LE/TOK_GE
  *    - ‘<’, ‘>’, ‘=’ (asign.)           → TOK_LT/TOK_GT/TOK_ASSIGN
 *    - Símbolos simples: ',', ';', '(', ')', '{', '}'
 *    - Operadores: '+', '-', '*', '/', '%', '^'
  *    - EOF → TOK_EOF
  *    - Cualquier otro → TOK_UNKNOWN
  */
/* Prototipos de funciones de reconocimiento léxico */
static TokenType lex_identifier_or_keyword(int first);
static TokenType lex_number(int first);
static TokenType lex_string(void);
static TokenType lex_relop_or_assign(int first);
static TokenType lex_symbol(int first);
static TokenType lex_operator(int first);

static TokenType yylex(void) {
    int c;

    /* 1) Saltar espacios y nuevas líneas */
    do {
        c = next_char();
    } while (c == ' ' || c == '\t' || c == '\n' || c == '\r');

    if (c == EOF)
        return TOK_EOF;

    if (isalpha(c))
        return lex_identifier_or_keyword(c);

    if (isdigit(c))
        return lex_number(c);

    if (c == '"')
        return lex_string();

    if (c == '=' || c == '!' || c == '<' || c == '>')
        return lex_relop_or_assign(c);

    if (c == ',' || c == ';' || c == '(' || c == ')' || c == '{' || c == '}')
        return lex_symbol(c);

    if (c == '+' || c == '-' || c == '*' || c == '/' || c == '%' || c == '^')
        return lex_operator(c);

    /* Cualquier otro carácter → TOK_UNKNOWN */
    char buf[2];
    buf[0] = (char)c;
    buf[1] = '\0';
    add_token(TOK_UNKNOWN, buf);
    return TOK_UNKNOWN;
}

/* === Implementación de cada componente léxico === */

static TokenType lex_identifier_or_keyword(int first) {
    char buffer[MAX_LEXEME_LEN];
    int len = 0;
    int c = first;
    do {
        if (len < MAX_LEXEME_LEN - 1)
            buffer[len++] = (char)c;
        c = next_char();
    } while (isalpha(c) || isdigit(c));
    buffer[len] = '\0';
    unget_char(c);

    char tmp[MAX_LEXEME_LEN];
    for (int i = 0; i < len; i++)
        tmp[i] = (char)tolower((unsigned char)buffer[i]);
    tmp[len] = '\0';

    if (strcmp(tmp, "entero") == 0) {
        add_token(TOK_INT, buffer);
        return TOK_INT;
    }
    if (strcmp(tmp, "caracter") == 0) {
        add_token(TOK_CHAR, buffer);
        return TOK_CHAR;
    }
    if (strcmp(tmp, "flotante") == 0) {
        add_token(TOK_FLOAT, buffer);
        return TOK_FLOAT;
    }
    if (strcmp(tmp, "imprimir") == 0) {
        add_token(TOK_PRINT, buffer);
        return TOK_PRINT;
    }
    if (strcmp(tmp, "leer") == 0) {
        add_token(TOK_READ, buffer);
        return TOK_READ;
    }
    if (strcmp(tmp, "si") == 0) {
        add_token(TOK_IF, buffer);
        return TOK_IF;
    }
    if (strcmp(tmp, "sino") == 0) {
        add_token(TOK_ELSE, buffer);
        return TOK_ELSE;
    }
    if (strcmp(tmp, "mientras") == 0) {
        add_token(TOK_WHILE, buffer);
        return TOK_WHILE;
    }
    if (strcmp(tmp, "var") == 0) {
        add_token(TOK_VAR, buffer);
        return TOK_VAR;
    }
    if (strcmp(tmp, "const") == 0) {
        add_token(TOK_CONST, buffer);
        return TOK_CONST;
    }
    if (strcmp(tmp, "items") == 0) {
        add_token(TOK_ITEMS, buffer);
        return TOK_ITEMS;
    }
    if (strcmp(tmp, "item") == 0) {
        add_token(TOK_ITEM, buffer);
        return TOK_ITEM;
    }

    add_token(TOK_IDENT, buffer);
    return TOK_IDENT;
}

static TokenType lex_number(int first) {
    char buffer[MAX_LEXEME_LEN];
    int len = 0;
    int c = first;
    do {
        if (len < MAX_LEXEME_LEN - 1)
            buffer[len++] = (char)c;
        c = next_char();
    } while (isdigit(c));
    buffer[len] = '\0';
    unget_char(c);
    add_token(TOK_NUM, buffer);
    return TOK_NUM;
}

static TokenType lex_string(void) {
    char buffer[MAX_LEXEME_LEN];
    int len = 0;
    int c;
    while ((c = next_char()) != '"' && c != EOF) {
        if (len < MAX_LEXEME_LEN - 1)
            buffer[len++] = (char)c;
    }
    buffer[len] = '\0';
    if (c != '"') {
        add_token(TOK_UNKNOWN, buffer);
        return TOK_UNKNOWN;
    }
    add_token(TOK_STRING, buffer);
    return TOK_STRING;
}

static TokenType lex_relop_or_assign(int first) {
    int next = next_char();
    if (first == '=') {
        if (next == '=') {
            add_token(TOK_EQ, "==");
            return TOK_EQ;
        }
        unget_char(next);
        add_token(TOK_ASSIGN, "=");
        return TOK_ASSIGN;
    }
    if (first == '!') {
        if (next == '=') {
            add_token(TOK_NEQ, "!=");
            return TOK_NEQ;
        }
        unget_char(next);
        add_token(TOK_UNKNOWN, "!");
        return TOK_UNKNOWN;
    }
    if (first == '<') {
        if (next == '=') {
            add_token(TOK_LE, "<=");
            return TOK_LE;
        }
        unget_char(next);
        add_token(TOK_LT, "<");
        return TOK_LT;
    }
    if (first == '>') {
        if (next == '=') {
            add_token(TOK_GE, ">=");
            return TOK_GE;
        }
        unget_char(next);
        add_token(TOK_GT, ">");
        return TOK_GT;
    }
    unget_char(next);
    char buf[2] = { (char)first, '\0' };
    add_token(TOK_UNKNOWN, buf);
    return TOK_UNKNOWN;
}

static TokenType lex_symbol(int first) {
    switch (first) {
        case ',': add_token(TOK_COMMA, ","); return TOK_COMMA;
        case ';': add_token(TOK_SEMI, ";"); return TOK_SEMI;
        case '(': add_token(TOK_LPAREN, "("); return TOK_LPAREN;
        case ')': add_token(TOK_RPAREN, ")"); return TOK_RPAREN;
        case '{': add_token(TOK_LBRACE, "{"); return TOK_LBRACE;
        case '}': add_token(TOK_RBRACE, "}"); return TOK_RBRACE;
    }
    char buf[2] = { (char)first, '\0' };
    add_token(TOK_UNKNOWN, buf);
    return TOK_UNKNOWN;
}

static TokenType lex_operator(int first) {
    switch (first) {
        case '+': add_token(TOK_PLUS, "+"); return TOK_PLUS;
        case '-': add_token(TOK_MINUS, "-"); return TOK_MINUS;
        case '*': add_token(TOK_MULT, "*"); return TOK_MULT;
        case '/': add_token(TOK_DIV, "/"); return TOK_DIV;
        case '%': add_token(TOK_MOD, "%"); return TOK_MOD;
        case '^': add_token(TOK_POW, "^"); return TOK_POW;
    }
    char buf[2] = { (char)first, '\0' };
    add_token(TOK_UNKNOWN, buf);
    return TOK_UNKNOWN;
}
 
 /**
  * tokenize_input():
  *   Lee toda la entrada estándar (stdin) hasta EOF, llamando a yylex()
  *   repetidamente. Cuando yylex() devuelve TOK_EOF, sale del bucle y
  *   añade al final un único token TOK_EOF.
  */
 static void tokenize_input(void) {
     TokenType t;
     do {
         t = yylex();
     } while (t != TOK_EOF);
     add_token(TOK_EOF, "EOF");
 }
 
 
 /*==============================================================
  *                  FUNCIONES AUXILIARES DEL PARSER
  *=============================================================*/
 
 /**
  * lookahead():
  *   Devuelve el TokenType de tokens[cur_token], o TOK_EOF si
  *   cur_token >= num_tokens.
  */
 static TokenType lookahead(void) {
     if (cur_token < num_tokens) {
         return tokens[cur_token].type;
     }
     return TOK_EOF;
 }
 
 /**
  * match(expected):
  *   Si lookahead()==expected, avanza cur_token++ (consume el token).
  *   Si no coincide, imprime un mensaje de error y termina con exit(1).
  */
static void match(TokenType expected) {
    if (lookahead() == expected) {
        cur_token++;
    } else {
        fprintf(stderr,
                "Error de sintaxis: se esperaba %s, pero vino %s ('%s').\n",
                token_name(expected),
                token_name(lookahead()),
                tokens[cur_token].lexeme);
        exit(1);
    }
}
 
 /**
  * expect_ident():
  *   Verifica que el token actual sea TOK_IDENT. Si lo es, devuelve
  *   el lexema (tokens[cur_token].lexeme) y avanza cur_token++. Si no,
  *   error.
  */
 static char *expect_ident(void) {
     if (lookahead() == TOK_IDENT) {
         char *name = tokens[cur_token].lexeme;
         cur_token++;
         return name;
     } else {
         fprintf(stderr,
                 "Error de sintaxis: se esperaba IDENT, "
                 "pero vino '%s'.\n",
                 tokens[cur_token].lexeme);
         exit(1);
     }
     return NULL; // solo para evitar warning
 }
 
 
 /*==============================================================
  *          PARSER DE EXPRESIONES (EVALUACIÓN EN TIEMPO REAL)
  *=============================================================*/
 
 /**
  * Prototipos (se definen más abajo):
  */
 static int parse_expr(void);
 static int parse_rel_expr(void);
 static int parse_add_expr(void);
static int parse_mul_expr(void);
static int parse_pow_expr(void);
 static int parse_unary_expr(void);
 static int parse_primary(void);
 
 /*
  * parse_expr():
  *   En esta gramática, <expr> ::= <rel_expr>
  */
 static int parse_expr(void) {
     return parse_rel_expr();
 }
 
 /*
  * <rel_expr> ::= <add_expr> { ( '==' | '!=' | '<' | '>' | '<=' | '>=' ) <add_expr> }
  */
 static int parse_rel_expr(void) {
     int left = parse_add_expr();
 
     while (1) {
         TokenType t = lookahead();
         if (t == TOK_EQ || t == TOK_NEQ || t == TOK_LT ||
             t == TOK_GT || t == TOK_LE || t == TOK_GE) {
             cur_token++;
             int right = parse_add_expr();
             switch (t) {
                 case TOK_EQ:  left = (left == right); break;
                 case TOK_NEQ: left = (left != right); break;
                 case TOK_LT:  left = (left < right);  break;
                 case TOK_GT:  left = (left > right);  break;
                 case TOK_LE:  left = (left <= right); break;
                 case TOK_GE:  left = (left >= right); break;
                 default: break;
             }
         } else {
             break;
         }
     }
     return left;
 }
 
 /*
  * <add_expr> ::= <mul_expr> { ( '+' | '-' ) <mul_expr> }
  */
 static int parse_add_expr(void) {
     int left = parse_mul_expr();
 
     while (1) {
         TokenType t = lookahead();
         if (t == TOK_PLUS || t == TOK_MINUS) {
             cur_token++;
             int right = parse_mul_expr();
             if (t == TOK_PLUS) {
                 left = left + right;
             } else {
                 left = left - right;
             }
         } else {
             break;
         }
     }
     return left;
 }
 
 /*
  * <mul_expr> ::= <unary_expr> { ( '*' | '/' ) <unary_expr> }
  */
static int parse_pow_expr(void) {
    int left = parse_unary_expr();

    while (lookahead() == TOK_POW) {
        match(TOK_POW);
        int right = parse_unary_expr();
        left = (int)pow(left, right);
    }

    return left;
}

static int parse_mul_expr(void) {
    int left = parse_pow_expr();

    while (1) {
        TokenType t = lookahead();
        if (t == TOK_MULT || t == TOK_DIV || t == TOK_MOD) {
            cur_token++;
            int right = parse_pow_expr();
            if (t == TOK_MULT) {
                left = left * right;
            } else if (t == TOK_DIV) {
                if (right == 0) {
                    fprintf(stderr, "Error: división por cero.\n");
                    exit(1);
                }
                left = left / right;
            } else {
                if (right == 0) {
                    fprintf(stderr, "Error: módulo por cero.\n");
                    exit(1);
                }
                left = left % right;
            }
        } else {
            break;
        }
    }
    return left;
}
 
 /*
  * <unary_expr> ::= [ '-' ] <primary>
  */
 static int parse_unary_expr(void) {
     if (lookahead() == TOK_MINUS) {
         cur_token++;
         int val = parse_primary();
         return -val;
     }
     return parse_primary();
 }
 
 /*
  * <primary> ::= '(' <expr> ')' | NUM | IDENT
  */
 static int parse_primary(void) {
     if (lookahead() == TOK_LPAREN) {
         match(TOK_LPAREN);
         int val = parse_expr();
         match(TOK_RPAREN);
         return val;
     } else if (lookahead() == TOK_NUM) {
         int val = atoi(tokens[cur_token].lexeme);
         cur_token++;
         return val;
     } else if (lookahead() == TOK_IDENT) {
         // Variable: obtenemos su valor de la tabla de símbolos
         char *name = tokens[cur_token].lexeme;
         cur_token++;
         return get_symbol_value(name);
     } else {
         fprintf(stderr,
                 "Error de sintaxis en <primary>: se esperaba "
                 "NUM, IDENT o '(', pero vino '%s'.\n",
                 tokens[cur_token].lexeme);
         exit(1);
     }
     return 0; // para evitar warning
 }
 
 
 /*==============================================================
  *         PARSER DE DECLARACIONES (DECL_STMT)
  *=============================================================*/
 
 /*
  * <decl_stmt> ::= <type> <var_list> ';'
  * <type>       ::= 'Entero' | 'Caracter' | 'Flotante'
  * <var_list>   ::= <var_decl> ( ',' <var_decl> )*
  * <var_decl>   ::= IDENT [ '=' <expr> ]
  *
  * Semántica:
  *    - Cada identificador se agrega a la tabla de símbolos con 
  *      is_defined=0 por defecto.
  *    - Si hay “= <expr>”, entonces evaluamos <expr> y asignamos el
  *      valor a la variable, marcando is_defined=1.
  *    - Si no hay “=”, la variable queda definida con valor 0 e
  *      is_defined=0 (error si se usa antes de asignar).
  */
 static void parse_decl_stmt(void) {
     // 1) <type>
     TokenType t = lookahead();
    if (t == TOK_INT || t == TOK_CHAR || t == TOK_FLOAT ||
        t == TOK_VAR || t == TOK_CONST || t == TOK_ITEMS || t == TOK_ITEM) {
        cur_token++;
    } else {
        fprintf(stderr,
                "Error de sintaxis en <decl_stmt>: se esperaba tipo 'Entero', 'Caracter', 'Flotante', 'var', 'const', 'items' o 'item', "
                "pero vino '%s'.\n",
                tokens[cur_token].lexeme);
        exit(1);
    }
 
     // 2) <var_list> ::= <var_decl> (',' <var_decl> )*
     while (1) {
         // <var_decl> ::= IDENT [ '=' <expr> ]
         if (lookahead() == TOK_IDENT) {
             char *varname = tokens[cur_token].lexeme;
             int idx = add_symbol(varname);  // crea o recupera índice
             symtab[idx].is_defined = 0;     // aún no asignado
 
             cur_token++;
             if (lookahead() == TOK_ASSIGN) {
                 match(TOK_ASSIGN);
                 int val = parse_expr();
                 set_symbol_value(varname, val);
             }
         } else {
             fprintf(stderr,
                     "Error de sintaxis en <var_list>: se esperaba IDENT, "
                     "pero vino '%s'.\n",
                     tokens[cur_token].lexeme);
             exit(1);
         }
 
         // Si viene coma, se repite el var_decl
         if (lookahead() == TOK_COMMA) {
             match(TOK_COMMA);
         } else {
             break;
         }
     }
 
     // 3) Consumir ';'
     match(TOK_SEMI);
 }
 
 
 /*==============================================================
  *            PARSER DE INSTRUCCIONES (STMT)
  *=============================================================*/
 
 /*
  * Prototipos recursivos:
  */
 static void parse_stmt(void);
 static void parse_print_stmt(void);
 static void parse_read_stmt(void);
 static void parse_assign_stmt(void);
 static void parse_if_stmt(void);
 static void parse_while_stmt(void);
 static void parse_block_stmt(void);
 
 /*
  * <stmt> ::= <decl_stmt>
  *          | <print_stmt>
  *          | <read_stmt>
  *          | <assign_stmt>
  *          | <if_stmt>
  *          | <while_stmt>
  *          | <block_stmt>
  */
 static void parse_stmt(void) {
     switch (lookahead()) {
         case TOK_INT:
         case TOK_CHAR:
         case TOK_FLOAT:
             parse_decl_stmt();
             break;
 
         case TOK_PRINT:
             parse_print_stmt();
             break;
 
         case TOK_READ:
             parse_read_stmt();
             break;
 
         case TOK_IDENT:
             parse_assign_stmt();
             break;
 
         case TOK_IF:
             parse_if_stmt();
             break;
 
         case TOK_WHILE:
             parse_while_stmt();
             break;
 
         case TOK_LBRACE:
             parse_block_stmt();
             break;
 
         default:
             fprintf(stderr,
                     "Error de sintaxis en <stmt>: token inesperado '%s'.\n",
                     tokens[cur_token].lexeme);
             exit(1);
     }
 }
 
 /*
  * <print_stmt> ::= 'Imprimir' '(' <expr> ')' ';'
  * Semántica: evalúa <expr> y muestra su valor por stdout (seguido de newline).
  */
static void parse_print_stmt(void) {
    match(TOK_PRINT);
    if (lookahead() == TOK_LPAREN) {
        match(TOK_LPAREN);
        if (lookahead() == TOK_STRING) {
            char *texto = tokens[cur_token].lexeme;
            match(TOK_STRING);
            match(TOK_RPAREN);
            match(TOK_SEMI);
            printf("%s\n", texto);
            append_exec_output(texto);
            append_exec_output("\n");
        } else {
            int val = parse_expr();
            match(TOK_RPAREN);
            match(TOK_SEMI);
            printf("%d\n", val);
            char buf[32];
            snprintf(buf, sizeof(buf), "%d\n", val);
            append_exec_output(buf);
        }
    } else if (lookahead() == TOK_LBRACE) {
        match(TOK_LBRACE);
        if (lookahead() == TOK_STRING) {
            char *texto = tokens[cur_token].lexeme;
            match(TOK_STRING);
            match(TOK_RBRACE);
            match(TOK_SEMI);
            printf("%s\n", texto);
            append_exec_output(texto);
            append_exec_output("\n");
        } else if (lookahead() == TOK_IDENT) {
            char *varname = expect_ident();
            match(TOK_RBRACE);
            match(TOK_SEMI);
            int val = get_symbol_value(varname);
            printf("%d\n", val);
            char buf[32];
            snprintf(buf, sizeof(buf), "%d\n", val);
            append_exec_output(buf);
        } else {
            fprintf(stderr, "Error de sintaxis: se esperaba CADENA o IDENT.\n");
            exit(1);
        }
    } else {
        fprintf(stderr, "Error de sintaxis en Imprimir.\n");
        exit(1);
    }
}
 
 /*
  * <read_stmt> ::= 'Leer' '(' IDENT ')' ';'
  * Semántica: lee un entero de stdin y lo asigna a la variable IDENT. 
  */
 static void parse_read_stmt(void) {
     match(TOK_READ);
     match(TOK_LPAREN);
     char *varname = expect_ident();
     match(TOK_RPAREN);
     match(TOK_SEMI);
 
     int x;
     if (scanf("%d", &x) != 1) {
         fprintf(stderr, "Error de runtime: no se pudo leer un entero.\n");
         exit(1);
     }
     set_symbol_value(varname, x);
 }
 
 /*
  * <assign_stmt> ::= IDENT '=' <expr> ';'
  * Semántica: evalúa <expr> y asigna el resultado a la variable.
  */
 static void parse_assign_stmt(void) {
     char *varname = expect_ident();
     match(TOK_ASSIGN);
     int val = parse_expr();
     match(TOK_SEMI);
     set_symbol_value(varname, val);
 }
 
 /*
  * <if_stmt> ::= 'Si' '(' <expr> ')' <stmt> [ 'Sino' <stmt> ]
  *
  * Semántica:
  *   - Si cond ≠ 0: ejecutar <stmt> de la rama THEN y descartar la rama 'Sino'.
  *   - Si cond == 0: descartar <stmt> de la rama THEN y, si existe 'Sino', ejecutar la rama ELSE.
  */
 static void parse_if_stmt(void) {
     match(TOK_IF);           // consume 'Si'
     match(TOK_LPAREN);       // consume '('
     int cond = parse_expr(); // evalúa la condición
     match(TOK_RPAREN);       // consume ')'
 
     if (cond) {
         // === rama THEN ===
         parse_stmt();
         // Si hay 'Sino', descartamos sintácticamente la rama ELSE
         if (lookahead() == TOK_ELSE) {
             match(TOK_ELSE);
             // Ignorar sintácticamente el <stmt> de la rama ELSE:
             if (lookahead() == TOK_INT || lookahead() == TOK_CHAR || lookahead() == TOK_FLOAT) {
                 // decl_stmt → tipo var_list ;
                 cur_token++;
                 do {
                     if (lookahead() == TOK_IDENT) {
                         cur_token++;
                         if (lookahead() == TOK_ASSIGN) {
                             cur_token++;
                             int nivel_p = 0;
                             do {
                                 TokenType tt = lookahead();
                                 if (tt == TOK_LPAREN) nivel_p++;
                                 else if (tt == TOK_RPAREN) nivel_p--;
                                 cur_token++;
                             } while (!(nivel_p == 0 && (lookahead() == TOK_COMMA || lookahead() == TOK_SEMI)));
                         }
                     } else {
                         fprintf(stderr,
                                 "Error de sintaxis al ignorar <decl_stmt> en ELSE: '%s'.\n",
                                 tokens[cur_token].lexeme);
                         exit(1);
                     }
                     if (lookahead() == TOK_COMMA) {
                         match(TOK_COMMA);
                     } else {
                         break;
                     }
                 } while (1);
                 match(TOK_SEMI);
             }
             else if (lookahead() == TOK_PRINT) {
                 // print_stmt → Imprimir ( expr ) ;
                 match(TOK_PRINT);
                 match(TOK_LPAREN);
                 int nivel_p = 0;
                 do {
                     if (lookahead() == TOK_LPAREN) nivel_p++;
                     else if (lookahead() == TOK_RPAREN) nivel_p--;
                     cur_token++;
                 } while (!(nivel_p == 0 && lookahead() == TOK_RPAREN));
                 match(TOK_RPAREN);
                 match(TOK_SEMI);
             }
             else if (lookahead() == TOK_READ) {
                 // read_stmt → Leer ( IDENT ) ;
                 match(TOK_READ);
                 match(TOK_LPAREN);
                 if (lookahead() == TOK_IDENT) cur_token++;
                 match(TOK_RPAREN);
                 match(TOK_SEMI);
             }
             else if (lookahead() == TOK_IDENT) {
                 // assign_stmt → IDENT = expr ;
                 cur_token++;
                 match(TOK_ASSIGN);
                 int nivel_p = 0;
                 do {
                     if (lookahead() == TOK_LPAREN) nivel_p++;
                     else if (lookahead() == TOK_RPAREN) nivel_p--;
                     cur_token++;
                 } while (!(nivel_p == 0 && lookahead() == TOK_SEMI));
                 match(TOK_SEMI);
             }
             else if (lookahead() == TOK_IF) {
                 // if_stmt anidado
                 cur_token++;
                 match(TOK_LPAREN);
                 int nivel_p = 0;
                 do {
                     if (lookahead() == TOK_LPAREN) nivel_p++;
                     else if (lookahead() == TOK_RPAREN) nivel_p--;
                     cur_token++;
                 } while (!(nivel_p == 0 && lookahead() == TOK_RPAREN));
                 match(TOK_RPAREN);
                 parse_if_stmt(); // ignora recursivamente
             }
             else if (lookahead() == TOK_WHILE) {
                 // while_stmt anidado
                 cur_token++;
                 match(TOK_LPAREN);
                 int nivel_p = 0;
                 do {
                     if (lookahead() == TOK_LPAREN) nivel_p++;
                     else if (lookahead() == TOK_RPAREN) nivel_p--;
                     cur_token++;
                 } while (!(nivel_p == 0 && lookahead() == TOK_RPAREN));
                 match(TOK_RPAREN);
                 // Ignorar cuerpo (puede ser bloque o sentencia única)
                 if (lookahead() == TOK_LBRACE) {
                     int brace_nivel = 0;
                     do {
                         if (lookahead() == TOK_LBRACE) brace_nivel++;
                         else if (lookahead() == TOK_RBRACE) brace_nivel--;
                         cur_token++;
                     } while (brace_nivel > 0 && lookahead() != TOK_EOF);
                 } else {
                     parse_stmt(); // descarta la sentencia única
                 }
             }
             else if (lookahead() == TOK_LBRACE) {
                 // bloque_stmt → ignorar todo hasta cerrar '}'
                 int brace_nivel = 0;
                 do {
                     if (lookahead() == TOK_LBRACE) brace_nivel++;
                     else if (lookahead() == TOK_RBRACE) brace_nivel--;
                     cur_token++;
                 } while (brace_nivel > 0 && lookahead() != TOK_EOF);
             }
             else {
                 fprintf(stderr,
                         "Error de sintaxis al ignorar rama ‘Sino’: token '%s'.\n",
                         tokens[cur_token].lexeme);
                 exit(1);
             }
         }
     }
     else {
         // cond == 0 → descartamos la rama THEN y, si hay "Sino", ejecutamos la rama ELSE
         //int saved = cur_token;
         // Ignorar sintácticamente la <sentencia> THEN
         if (lookahead() == TOK_INT || lookahead() == TOK_CHAR || lookahead() == TOK_FLOAT) {
             // decl_stmt
             cur_token++;
             do {
                 if (lookahead() == TOK_IDENT) {
                     cur_token++;
                     if (lookahead() == TOK_ASSIGN) {
                         cur_token++;
                         int nivel_p = 0;
                         do {
                             if (lookahead() == TOK_LPAREN) nivel_p++;
                             else if (lookahead() == TOK_RPAREN) nivel_p--;
                             cur_token++;
                         } while (!(nivel_p == 0 && (lookahead() == TOK_COMMA || lookahead() == TOK_SEMI)));
                     }
                 } else {
                     fprintf(stderr,
                             "Error de sintaxis al ignorar <decl_stmt>: '%s'.\n",
                             tokens[cur_token].lexeme);
                     exit(1);
                 }
                 if (lookahead() == TOK_COMMA) {
                     match(TOK_COMMA);
                 } else {
                     break;
                 }
             } while (1);
             match(TOK_SEMI);
         }
         else if (lookahead() == TOK_PRINT) {
             // print_stmt
             match(TOK_PRINT);
             match(TOK_LPAREN);
             int nivel_p = 0;
             do {
                 if (lookahead() == TOK_LPAREN) nivel_p++;
                 else if (lookahead() == TOK_RPAREN) nivel_p--;
                 cur_token++;
             } while (!(nivel_p == 0 && lookahead() == TOK_RPAREN));
             match(TOK_RPAREN);
             match(TOK_SEMI);
         }
         else if (lookahead() == TOK_READ) {
             // read_stmt
             match(TOK_READ);
             match(TOK_LPAREN);
             if (lookahead() == TOK_IDENT) cur_token++;
             match(TOK_RPAREN);
             match(TOK_SEMI);
         }
         else if (lookahead() == TOK_IDENT) {
             // assign_stmt
             cur_token++;
             match(TOK_ASSIGN);
             int nivel_p = 0;
             do {
                 if (lookahead() == TOK_LPAREN) nivel_p++;
                 else if (lookahead() == TOK_RPAREN) nivel_p--;
                 cur_token++;
             } while (!(nivel_p == 0 && lookahead() == TOK_SEMI));
             match(TOK_SEMI);
         }
         else if (lookahead() == TOK_IF) {
             // if_stmt anidado
             cur_token++;
             match(TOK_LPAREN);
             int nivel_p = 0;
             do {
                 if (lookahead() == TOK_LPAREN) nivel_p++;
                 else if (lookahead() == TOK_RPAREN) nivel_p--;
                 cur_token++;
             } while (!(nivel_p == 0 && lookahead() == TOK_RPAREN));
             match(TOK_RPAREN);
             parse_if_stmt();  // ignora recursivamente
         }
         else if (lookahead() == TOK_WHILE) {
             // while_stmt anidado
             cur_token++;
             match(TOK_LPAREN);
             int nivel_p = 0;
             do {
                 if (lookahead() == TOK_LPAREN) nivel_p++;
                 else if (lookahead() == TOK_RPAREN) nivel_p--;
                 cur_token++;
             } while (!(nivel_p == 0 && lookahead() == TOK_RPAREN));
             match(TOK_RPAREN);
             // ignorar cuerpo
             if (lookahead() == TOK_LBRACE) {
                 int brace_nivel = 0;
                 do {
                     if (lookahead() == TOK_LBRACE) brace_nivel++;
                     else if (lookahead() == TOK_RBRACE) brace_nivel--;
                     cur_token++;
                 } while (brace_nivel > 0 && lookahead() != TOK_EOF);
             } else {
                 parse_stmt();
             }
         }
         else if (lookahead() == TOK_LBRACE) {
             int brace_nivel = 0;
             do {
                 if (lookahead() == TOK_LBRACE) brace_nivel++;
                 else if (lookahead() == TOK_RBRACE) brace_nivel--;
                 cur_token++;
             } while (brace_nivel > 0 && lookahead() != TOK_EOF);
         }
         else {
             fprintf(stderr,
                     "Error de sintaxis al ignorar <sentencia>: '%s'.\n",
                     tokens[cur_token].lexeme);
             exit(1);
         }
 
         // Una vez ignorada la rama THEN, comprobamos si hay 'Sino'
         if (lookahead() == TOK_ELSE) {
             match(TOK_ELSE);
             // Ahora sí ejecutamos la rama ELSE:
             parse_stmt();
         }
         // Si no hay 'Sino', continuamos adelante
     }
 }
 
 /*
  * <while_stmt> ::= 'Mientras' '(' <expr> ')' <stmt>
  * Semántica: evalúa <expr>; mientras ≠0, ejecuta <stmt> y repite.
  */
 static void parse_while_stmt(void) {
     match(TOK_WHILE);
     match(TOK_LPAREN);
 
     // Para “repetir” el bucle, guardamos la posición de cur_token justo después de '('
     int cond_pos = cur_token;
     int valor_cond = parse_expr();
     match(TOK_RPAREN);
 
     if (valor_cond) {
         // Ejecutamos cuerpo (stmt) mientras expr ≠ 0
         int body_pos = cur_token;
         parse_stmt();
         while (1) {
             cur_token = cond_pos;
             int nuevo = parse_expr();
             if (!nuevo) {
                 break;
             }
             match(TOK_RPAREN);
             cur_token = body_pos;
             parse_stmt();
         }
         // ——————————————————————
         // IMPORTANTE: aquí falta consumir el último ')'
         match(TOK_RPAREN);
         // ——————————————————————
     } else {
         // cond == 0 → descartamos el <stmt> que sigue
         int saved = cur_token;
         parse_stmt();      // ignora sintácticamente
         cur_token = saved; // regresamos a después de ')'
         parse_stmt();      // consumimos esa <stmt> (pero no la ejecutamos)
     }
 }
 
 /*
  * <block_stmt> ::= '{' <stmt_list> '}'
  * Semántica: simplemente ejecuta cada stmt hasta cerrar llave
  */
 static void parse_block_stmt(void) {
     match(TOK_LBRACE);
     while (lookahead() != TOK_RBRACE && lookahead() != TOK_EOF) {
         parse_stmt();
     }
     match(TOK_RBRACE);
 }
 
 
 /*==============================================================
  *               PARSER PRINCIPAL DE <program>
  *=============================================================*/
 
 /*
  * <program> ::= <stmt_list> EOF
  */
 static void parse_program(void) {
     while (lookahead() != TOK_EOF) {
         parse_stmt();
     }
     match(TOK_EOF);
 }
 
 
 /*==============================================================
  *                          MAIN
  *=============================================================*/
 
int main(int argc, char *argv[]) {
    input_stream = stdin;
    if (argc > 1) {
        input_stream = fopen(argv[1], "r");
        if (!input_stream) {
            perror("fopen");
            return 1;
        }
    }

    // 1) Tokenizar toda la entrada (en CMD, pulsa Ctrl+Z ⏎ para EOF)
    tokenize_input();

    // 2) Iniciar el parser
    cur_token = 0;
    parse_program();
    // 3) Guardar tokens y resultado del parser
    write_tokens_to_obj("lexico.obj", "OK", exec_buffer);

    // 4) Si no hubo error, imprimimos OK
    printf("OK\n");
    if (input_stream != stdin) {
        fclose(input_stream);
    }
    return 0;
}
 