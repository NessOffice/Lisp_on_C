#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "libs/mpc.h"

typedef enum LVAL_TYPE { LVAL_NUM, LVAL_ERR, LVAL_SYM, LVAL_SEXPR, LVAL_QEXPR } LVAL_TYPE;
typedef struct lval {
    LVAL_TYPE type;
    union {
        long num;
        char* err;
        char* sym;
    };
    int count;
    struct lval** cell;
} lval;

/* constructor begin */
lval *lval_num(long x);
lval *lval_err(char *m);
lval *lval_sym(char *s);
lval *lval_sexpr();
void lval_del(lval *v);

lval *lval_num(long x) {
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_NUM;
    v->num = x;
    return v;
}
lval *lval_err(char *m) {
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_ERR;
    v->err = malloc(strlen(m) + 1);
    strcpy(v->err, m);
    return v;
}
lval *lval_sym(char *s) {
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_SYM;
    v->err = malloc(strlen(s) + 1);
    strcpy(v->err, s);
    return v;
}
lval *lval_sexpr() {
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_SEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;
}
lval *lval_qexpr() {
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_QEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;
}
void lval_del(lval *v) {
    switch(v->type) {
        case LVAL_NUM: break;
        case LVAL_ERR: free(v->err); break;
        case LVAL_SYM: free(v->sym); break;
        case LVAL_QEXPR:
        case LVAL_SEXPR:
            for(int i = 0;i < v->count;i++) {
                lval_del(v->cell[i]);
            }
            free(v->cell);
            break;
    }
    free(v);
}
/* constructor end  */

lval *lval_read_num(mpc_ast_t* t);
lval *lval_read(mpc_ast_t* t);
lval *lval_add(lval* v, lval* x);
void lval_expr_print(lval* v, char open, char close);
void lval_print(lval* v);
void lval_println(lval* v);

lval *lval_read_num(mpc_ast_t* t) {
    errno = 0;
    long x = strtol(t->contents, NULL, 10);
    return errno != ERANGE
        ? lval_num(x)
        : lval_err("invalid number");
}
lval *lval_read(mpc_ast_t* t) {
    if(strstr(t->tag, "number")) {return lval_read_num(t);}
    if(strstr(t->tag, "symbol")) {return lval_sym(t->contents);}

    /* If root (>) or sexpr|qexpr then create empty list */
    lval *x = NULL;
    if(strcmp(t->tag, ">") == 0) {x = lval_sexpr();}
    if(strstr(t->tag, "sexpr"))  {x = lval_sexpr();}
    if(strstr(t->tag, "qexpr"))  {x = lval_qexpr();}

    for(int i = 0;i < t->children_num;i++) {
        if(strcmp(t->children[i]->contents, "(") == 0) {continue;}
        if(strcmp(t->children[i]->contents, ")") == 0) {continue;}
        if(strcmp(t->children[i]->contents, "{") == 0) {continue;}
        if(strcmp(t->children[i]->contents, "}") == 0) {continue;}
        if(strcmp(t->children[i]->tag,  "regex") == 0) {continue;}
        x = lval_add(x, lval_read(t->children[i]));
    }
    return x;
}
lval *lval_add(lval* v, lval* x) {
    v->count++;
    v->cell = realloc(v->cell, sizeof(lval*) * v->count);
    v->cell[v->count-1] = x;
    return v;
}
void lval_expr_print(lval* v, char open, char close) {
    putchar(open);
    for(int i = 0;i < v->count;i++) {
        lval_print(v->cell[i]);
        if(i != (v->count-1)) {
            putchar(' ');
        }
    }
    putchar(close);
}
void lval_print(lval* v) {
    switch(v->type) {
        case LVAL_NUM:   printf("%li", v->num); break;
        case LVAL_ERR:   printf("Error: %s", v->err); break;
        case LVAL_SYM:   printf("%s", v->sym); break;
        case LVAL_SEXPR: lval_expr_print(v, '(', ')'); break;
        case LVAL_QEXPR: lval_expr_print(v, '{', '}'); break;
    }
}
void lval_println(lval* v) { lval_print(v); putchar('\n'); }