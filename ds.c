/* data structure */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "libs/mpc.h"

struct lval;
struct lenv;
typedef struct lval lval;
typedef struct lenv lenv;

typedef enum LVAL_TYPE {
    LVAL_NUM, LVAL_ERR, LVAL_SYM,
    LVAL_FUN, LVAL_SEXPR, LVAL_QEXPR
} LVAL_TYPE;
char* ltype_name(LVAL_TYPE t) {
    switch(t) {
        case LVAL_FUN: return "Function";
        case LVAL_NUM: return "Number";
        case LVAL_ERR: return "Error";
        case LVAL_SYM: return "Symbol";
        case LVAL_SEXPR: return "S-Expression";
        case LVAL_QEXPR: return "Q-Expression";
        default: return "Unknown";
    }
}

typedef lval*(*lbuiltin)(lenv*, lval*);
struct lval {
    LVAL_TYPE type;
    union {
        long num;
        char* err;
        char* sym;
        lbuiltin fun;
    };
    int count;
    struct lval** cell;
};

lval *lval_num(long x);
lval *lval_err(char* fmt, ...);
lval *lval_sym(char *s);
lval *lval_sexpr();
lval *lval_qexpr();
void lval_del(lval *v);

lval *lval_num(long x) {
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_NUM;
    v->num = x;
    return v;
}
lval *lval_err(char* fmt, ...) {
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_ERR;
    
    va_list va;
    va_start(va, fmt);

    v->err = malloc(512);
    vsnprintf(v->err, 511, fmt, va);
    v->err = realloc(v->err, strlen(v->err) + 1);
    va_end(va);
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
lval* lval_fun(lbuiltin func) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_FUN;
    v->fun = func;
    return v;
}
void lval_del(lval *v) {
    switch(v->type) {
        case LVAL_NUM: break;
        case LVAL_ERR: free(v->err); break;
        case LVAL_SYM: free(v->sym); break;
        case LVAL_FUN: break;
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
        : lval_err("invalid number '%s'!", t->contents);
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
        case LVAL_FUN:   printf("<function>"); break;
        case LVAL_SEXPR: lval_expr_print(v, '(', ')'); break;
        case LVAL_QEXPR: lval_expr_print(v, '{', '}'); break;
    }
}
void lval_println(lval* v) { lval_print(v); putchar('\n'); }

lval* lval_copy(lval* v) {
    lval* x = malloc(sizeof(lval));
    x->type = v->type;
    switch (v->type)
    {
        case LVAL_FUN: x->fun = v->fun; break;
        case LVAL_NUM: x->num = v->num; break;
        case LVAL_ERR:
            x->err = malloc(strlen(v->err) + 1);
            strcpy(x->err, v->err);
            break;
        case LVAL_SYM:
            x->err = malloc(strlen(v->err) + 1);
            strcpy(x->err, v->err);
            break;
        case LVAL_SEXPR:
        case LVAL_QEXPR:
            x->count = v->count;
            x->cell = malloc(sizeof(lval*) * x->count);
            for(int i = 0;i < x->count;i++) {
                x->cell[i] = lval_copy(v->cell[i]);
            }
        default:
            break;
    }
    return x;
}

struct lenv {
    int count;
    char** syms;
    lval** vals;
};
lenv* lenv_new() {
    lenv* e = malloc(sizeof(lenv));
    e->count = 0;
    e->syms = NULL;
    e->vals = NULL;
    return e;
}
void lenv_del(lenv* e) {
    for(int i = 0;i < e->count;i++) {
        free(e->syms[i]);
        lval_del(e->vals[i]);
    }
    free(e->syms);
    free(e->vals);
    free(e);
}
lval* lenv_get(lenv* e, lval* k) {
    for(int i = 0;i < e->count;i++) {
        if(strcmp(e->syms[i], k->sym) == 0) {
            return lval_copy(e->vals[i]);
        }
    }
    return lval_err("unbounded symbol '%s'!", k->sym);
}
void lenv_put(lenv* e, lval* k, lval* v) {
    for(int i = 0;i < e->count;i++) {
        if(strcmp(e->syms[i], k->sym) == 0) {
            lval_del(e->vals[i]);
            e->vals[i] = lval_copy(v);
            return;
        }
    }

    e->count++;
    e->vals = realloc(e->vals, sizeof(lval*) * e->count);
    e->syms = realloc(e->syms, sizeof(char*) * e->count);
    e->vals[e->count - 1] = lval_copy(v);
    e->syms[e->count - 1] = malloc(strlen(k->sym) + 1);
    strcpy(e->syms[e->count - 1], k->sym);
}