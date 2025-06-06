/**************************************************************
 * analyzer.c
 *
 * Mini‐analizador sintáctico (parser de descenso recursivo)
 * para verificar declaraciones de variables del estilo:
 *
 *   Int   a = 8, J = 3, K ;
 *   Char  x, y = 100 ;
 *   Float z = 3 ;
 *
 * Gramática (BNF pura):
 *
 *   <program>       ::= <definir> EOF
 *
 *   <definir>       ::= <tipo> <lista_vars> ';'
 *
 *   <tipo>          ::= 'Int' | 'Char' | 'Float'
 *
 *   <lista_vars>    ::= <var_decl> <lista_vars_tail>
 *
 *   <lista_vars_tail>::= ',' <var_decl> <lista_vars_tail> | ε
 *
 *   <var_decl>      ::= IDENT [ '=' NUM ]
 *
 * Tokens léxicos:
 *   - PALABRAS RESERVADAS: "Int", "Char", "Float"
 *   - IDENT: (Letra) (Letra|Dígito)*
 *   - NUM: (Dígito)+
 *   - ','  → TOK_COMMA
 *   - '='  → TOK_ASSIGN
 *   - ';'  → TOK_SEMI
 *   - EOF  → TOK_EOF
 *   - Cualquier otro carácter no reconocido → TOK_UNKNOWN
 *
 * Para compilar:
 *   gcc -Wall -o analyzer analyzer.c
 *
 * Para ejecutar (por ejemplo, a partir de un fichero input.txt):
 *   ./analyzer < input.txt
 *
 * Si la declaración respeta la gramática, imprimirá “OK”.
 * En caso contrario, mostrará un mensaje “Error de sintaxis...”
 * y terminará con exit(1).
 *
 **************************************************************/

 #define _CRT_SECURE_NO_WARNINGS

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <ctype.h>
 
 #define MAX_TOKENS      1024
 #define MAX_LEXEME_LEN   128
 
 /*--------------------------------------------------------------
  * Enumeración de los tipos de token que el parser reconoce
  *-------------------------------------------------------------*/
 typedef enum {
     TOK_INT,       // "Int"
     TOK_CHAR,      // "Char"
     TOK_FLOAT,     // "Float"
     TOK_IDENT,     // identificador genérico (letra + dígitos/letras)
     TOK_NUM,       // constante entera (dígito+)
     TOK_COMMA,     // ','
     TOK_ASSIGN,    // '='
     TOK_SEMI,      // ';'
     TOK_EOF,       // fin de fichero
     TOK_UNKNOWN    // cualquier otro símbolo no reconocido
 } TokenType;
 
 /*--------------------------------------------------------------
  * Estructura de un token: su tipo y su lexema (texto)
  *-------------------------------------------------------------*/
 typedef struct {
     TokenType type;
     char      lexeme[MAX_LEXEME_LEN];
 } Token;
 
 /*--------------------------------------------------------------
  * Arreglo global de tokens y contadores:
  *   tokens[0..num_tokens-1] = lista de tokens hallados
  *   num_tokens = número real de tokens
  *   cur_token  = índice del token “actual” para el parser
  *-------------------------------------------------------------*/
 static Token tokens[MAX_TOKENS];
 static int   num_tokens = 0;
 static int   cur_token  = 0;
 
 /*--------------------------------------------------------------
  * next_char(): lee un carácter (getchar) de stdin; devuelve EOF
  * si ya no hay más.
  * unget_char(c): “devuelve” c al buffer, para leerlo luego.
  *-------------------------------------------------------------*/
 static int next_char(void) {
     int c = getchar();
     return (c == EOF ? EOF : c);
 }
 
 static void unget_char(int c) {
     if (c != EOF) {
         ungetc(c, stdin);
     }
 }
 
 /*--------------------------------------------------------------
  * add_token(type, lex): añade un token al arreglo global.
  *-------------------------------------------------------------*/
 static void add_token(TokenType type, const char *lex) {
     if (num_tokens >= MAX_TOKENS) {
         fprintf(stderr, "Error: demasiados tokens (>= %d).\n", MAX_TOKENS);
         exit(1);
     }
     tokens[num_tokens].type = type;
     strncpy(tokens[num_tokens].lexeme, lex, MAX_LEXEME_LEN - 1);
     tokens[num_tokens].lexeme[MAX_LEXEME_LEN - 1] = '\0';
     num_tokens++;
 }
 
 /*--------------------------------------------------------------
  * yylex(): reconocedor léxico. Lee la entrada, omite espacios,
  * tabulaciones y saltos de línea, y reconoce uno a uno:
  *   - Palabra que empiece con letra → IDENT o “Int”/“Char”/“Float”
  *   - Secuencia de dígitos → NUM
  *   - Símbolos ','  '='  ';'
  *   - EOF → TOK_EOF
  *   - Cualquier otro carácter → TOK_UNKNOWN
  * Cada vez que reconoce un token, lo agrega a tokens[] y devuelve
  * el TokenType correspondiente. Cuando alcanza EOF, devuelve TOK_EOF.
  *-------------------------------------------------------------*/
 static TokenType yylex(void) {
     int c;
     char buffer[MAX_LEXEME_LEN];
     int len;
 
     // 1) Saltar espacios en blanco y saltos de línea:
     do {
         c = next_char();
     } while (c == ' ' || c == '\t' || c == '\n' || c == '\r');
 
     if (c == EOF) {
         return TOK_EOF;
     }
 
     // 2) Si comienza con letra, leemos toda la secuencia de letras/dígitos:
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
 
         // Comprobar si es palabra reservada:
         if (strcmp(buffer, "Int") == 0) {
             add_token(TOK_INT, buffer);
             return TOK_INT;
         }
         if (strcmp(buffer, "Char") == 0) {
             add_token(TOK_CHAR, buffer);
             return TOK_CHAR;
         }
         if (strcmp(buffer, "Float") == 0) {
             add_token(TOK_FLOAT, buffer);
             return TOK_FLOAT;
         }
         // Si no, es identificador
         add_token(TOK_IDENT, buffer);
         return TOK_IDENT;
     }
 
     // 3) Si comienza con dígito, leemos dígitos para NUM:
     if (isdigit(c)) {
         len = 0;
         do {
             if (len < MAX_LEXEME_LEN - 1) {
                 buffer[len++] = (char)c;
             }
             c = next_char();
         } while (isdigit(c));
         buffer[len] = '\0';
         unget_char(c);
 
         add_token(TOK_NUM, buffer);
         return TOK_NUM;
     }
 
     // 4) Símbolos simples: ','  '='  ';'
     switch (c) {
         case ',':
             add_token(TOK_COMMA, ",");
             return TOK_COMMA;
         case '=':
             add_token(TOK_ASSIGN, "=");
             return TOK_ASSIGN;
         case ';':
             add_token(TOK_SEMI, ";");
             return TOK_SEMI;
         default:
             // Cualquier otro carácter se marca como desconocido:
             buffer[0] = (char)c;
             buffer[1] = '\0';
             add_token(TOK_UNKNOWN, buffer);
             return TOK_UNKNOWN;
     }
 }
 
 /*--------------------------------------------------------------
  * tokenize_input(): lee toda la entrada hasta EOF, llamando a
  * yylex() repetidamente. Cuando yylex() devuelve TOK_EOF, sale
  * del bucle y añade un token TOK_EOF en el arreglo.
  *-------------------------------------------------------------*/
 static void tokenize_input(void) {
     TokenType t;
     do {
         t = yylex();
     } while (t != TOK_EOF);
     // Insertamos explícitamente un token TOK_EOF final:
     add_token(TOK_EOF, "EOF");
 }
 
 /*--------------------------------------------------------------
  * lookahead(): devuelve el tipo del token en tokens[cur_token]
  *-------------------------------------------------------------*/
 static TokenType lookahead(void) {
     if (cur_token < num_tokens) {
         return tokens[cur_token].type;
     }
     return TOK_EOF;
 }
 
 /*--------------------------------------------------------------
  * match(expected): si lookahead()==expected, consume el token
  * (cur_token++). Si no coincide, imprime un error y sale(1).
  *-------------------------------------------------------------*/
 static void match(TokenType expected) {
     if (lookahead() == expected) {
         cur_token++;
     } else {
         fprintf(stderr,
                 "Error de sintaxis: se esperaba token %d, pero vino %d "
                 "(lexema=\"%s\").\n",
                 expected,
                 lookahead(),
                 tokens[cur_token].lexeme);
         exit(1);
     }
 }
 
 /*==============================================================
  *               SECCIÓN SINTÁCTICA (PARSER)
  *=============================================================*/
 
 /* Prototipos (una función por cada no terminal) */
 static void parse_program(void);
 static void parse_definir(void);
 static void parse_tipo(void);
 static void parse_lista_vars(void);
 static void parse_lista_vars_tail(void);
 static void parse_var_decl(void);
 
 /*
  * <program> ::= <definir> EOF
  */
 static void parse_program(void) {
     parse_definir();
     match(TOK_EOF);
 }
 
 /*
  * <definir> ::= <tipo> <lista_vars> ';'
  */
 static void parse_definir(void) {
     parse_tipo();
     parse_lista_vars();
     match(TOK_SEMI);
 }
 
 /*
  * <tipo> ::= 'Int' | 'Char' | 'Float'
  */
 static void parse_tipo(void) {
     TokenType t = lookahead();
     if (t == TOK_INT || t == TOK_CHAR || t == TOK_FLOAT) {
         // Consumimos el token del tipo
         cur_token++;
     } else {
         fprintf(stderr,
                 "Error de sintaxis en <tipo>: se esperaba 'Int', 'Char' o 'Float' "
                 "pero vino \"%s\".\n",
                 tokens[cur_token].lexeme);
         exit(1);
     }
 }
 
 /*
  * <lista_vars> ::= <var_decl> <lista_vars_tail>
  */
 static void parse_lista_vars(void) {
     parse_var_decl();
     parse_lista_vars_tail();
 }
 
 /*
  * <lista_vars_tail> ::= ',' <var_decl> <lista_vars_tail> | ε
  */
 static void parse_lista_vars_tail(void) {
     if (lookahead() == TOK_COMMA) {
         match(TOK_COMMA);
         parse_var_decl();
         parse_lista_vars_tail();
     }
     // Sino, aceptamos vacío y retornamos
 }
 
 /*
  * <var_decl> ::= IDENT [ '=' NUM ]
  */
 static void parse_var_decl(void) {
     if (lookahead() == TOK_IDENT) {
         // Podrías almacenar aquí variables en la tabla de símbolos
         cur_token++;
         if (lookahead() == TOK_ASSIGN) {
             match(TOK_ASSIGN);
             if (lookahead() == TOK_NUM) {
                 cur_token++;
             } else {
                 fprintf(stderr,
                         "Error de sintaxis en <var_decl>: tras '=' se esperaba <NUM> "
                         "pero vino \"%s\".\n",
                         tokens[cur_token].lexeme);
                 exit(1);
             }
         }
     } else {
         fprintf(stderr,
                 "Error de sintaxis en <var_decl>: se esperaba IDENT "
                 "pero vino \"%s\".\n",
                 tokens[cur_token].lexeme);
         exit(1);
     }
 }
 
 /*==============================================================
  *                       MAIN
  *=============================================================*/
 int main(void) {
     // 1) Tokenizamos toda la entrada
     tokenize_input();
 
     // 2) Iniciamos el parser
     cur_token = 0;
     parse_program();
 
     // 3) Si no hubo error, imprimimos OK
     printf("OK\n");
     return 0;
 }
 