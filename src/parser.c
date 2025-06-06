#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"
#include "symtab.h"
 static int parse_expr(void);
static void parse_switch_stmt(void);
static void skip_simple_stmt(void);
 static int parse_rel_expr(void);
 static int parse_add_expr(void);
 static int parse_mul_expr(void);
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
 static int parse_mul_expr(void) {
     int left = parse_unary_expr();
 
     while (1) {
         TokenType t = lookahead();
         if (t == TOK_MULT || t == TOK_DIV) {
             cur_token++;
             int right = parse_unary_expr();
             if (t == TOK_MULT) {
                 left = left * right;
             } else {
                 if (right == 0) {
                     fprintf(stderr, "Error: división por cero.\n");
                     exit(1);
                 }
                 left = left / right;
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
     if (t == TOK_INT || t == TOK_CHAR || t == TOK_FLOAT) {
         cur_token++;
     } else {
         fprintf(stderr,
                 "Error de sintaxis en <decl_stmt>: se esperaba tipo 'Entero', 'Caracter' o 'Flotante', "
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
static void parse_sum_stmt(void);
 
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

        case TOK_SUM:
            parse_sum_stmt();
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
        case TOK_SWITCH:
            parse_switch_stmt();
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
 * <print_stmt> ::=
 *       'Imprimir' '(' ( <expr> | STRING ) ')' ';'
 *     | 'Imprimir' '{' ( <expr> | STRING ) '}' ';'
 * Semántica: evalúa la expresión (o imprime la cadena) y muestra el
 * resultado por stdout seguido de un salto de línea.
 */
static void parse_print_stmt(void) {
    match(TOK_PRINT);
    if (lookahead() == TOK_LPAREN) {
        match(TOK_LPAREN);
        if (lookahead() == TOK_STRING) {
            char *s = tokens[cur_token].lexeme;
            cur_token++;
            match(TOK_RPAREN);
            match(TOK_SEMI);
            printf("%s\n", s);
        } else {
            int val = parse_expr();
            match(TOK_RPAREN);
            match(TOK_SEMI);
            printf("%d\n", val);
        }
    } else if (lookahead() == TOK_LBRACE) {
        match(TOK_LBRACE);
        if (lookahead() == TOK_STRING) {
            char *s = tokens[cur_token].lexeme;
            cur_token++;
            match(TOK_RBRACE);
            match(TOK_SEMI);
            printf("%s\n", s);
        } else {
            int val = parse_expr();
            match(TOK_RBRACE);
            match(TOK_SEMI);
            printf("%d\n", val);
        }
    } else {
        fprintf(stderr,
                "Error de sintaxis en Imprimir: se esperaba '(' o '{' pero vino '%s'.\n",
                tokens[cur_token].lexeme);
        exit(1);
    }
}

/*
 * <sum_stmt> ::= 'Suma' <expr> ';'
 * Muestra el resultado de la expresión.
 */
static void parse_sum_stmt(void) {
    match(TOK_SUM);
    int val = parse_expr();
    match(TOK_SEMI);
    printf("%d\n", val);
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
 
static void skip_simple_stmt(void) {
    if (lookahead() == TOK_LBRACE) {
        int n = 0;
        do {
            if (lookahead() == TOK_LBRACE) n++;
            else if (lookahead() == TOK_RBRACE) n--;
            cur_token++;
        } while (n > 0 && lookahead() != TOK_EOF);
    } else {
        int p = 0;
        do {
            if (lookahead() == TOK_LPAREN) p++;
            else if (lookahead() == TOK_RPAREN) p--;
            cur_token++;
        } while (p > 0 || (lookahead() != TOK_SEMI && lookahead() != TOK_EOF));
        if (lookahead() == TOK_SEMI) cur_token++;
    }
}

static void parse_switch_stmt(void) {
    match(TOK_SWITCH);
    match(TOK_LPAREN);
    int val = parse_expr();
    match(TOK_RPAREN);
    match(TOK_LBRACE);
    int done = 0;
    while (lookahead() == TOK_CASE) {
        match(TOK_CASE);
        if (lookahead() != TOK_NUM) {
            fprintf(stderr, "Error de sintaxis en Caso: se esperaba numero.\n");
            exit(1);
        }
        int cval = atoi(tokens[cur_token].lexeme);
        cur_token++;
        match(TOK_COLON);
        if (!done && val == cval) {
            parse_stmt();
            done = 1;
        } else {
            skip_simple_stmt();
        }
        if (lookahead() == TOK_BREAK) {
            match(TOK_BREAK);
            match(TOK_SEMI);
            break;
        }
    }
    if (lookahead() == TOK_DEFAULT) {
        match(TOK_DEFAULT);
        match(TOK_COLON);
        if (!done) parse_stmt();
        else skip_simple_stmt();
    }
    match(TOK_RBRACE);
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
 void parse_program(void) {
     while (lookahead() != TOK_EOF) {
         parse_stmt();
     }
     match(TOK_EOF);
 }
 
