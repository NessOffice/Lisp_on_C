/* data structure */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "libs/mpc.h"
#include "lassert.c"

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

    /* Basic */
    union {
        long num;
        char* err;
        char* sym;
    };

    /* Function */
    lbuiltin builtin;
    lenv* env;
    lval* formals;
    lval* body;

    /* Expression */
    int count;
    struct lval** cell;
};

/* constructor, copy constructor, destructor */
lval *lval_num(long x);
lval *lval_err(char* fmt, ...);
lval *lval_sym(char *s);
lval *lval_sexpr();
lval *lval_qexpr();
lval* lval_fun(lbuiltin func);
lval* lval_lambda(lval* formals, lval* body);
lval* lval_copy(lval* v);
int lval_eq(lval* x, lval* y);
void lval_del(lval *v);

/* construct lval from mpt_ast_t */
lval *lval_read_num(mpc_ast_t* t);
lval *lval_read(mpc_ast_t* t);

lval *lval_add(lval* v, lval* x);
lval* lval_pop(lval* v, int i); // get i-th element and drop it from the list
lval* lval_take(lval* v, int i); // get i-th element, and drop others
lval* lval_join(lval* x, lval* y);

void lval_expr_print(lval* v, char open, char close);
void lval_print(lval* v);
void lval_println(lval* v);

lval *lval_eval_sexpr(lenv* e, lval* v);
lval* lval_eval(lenv* e, lval *v);
lval* lval_call(lenv* e, lval* f, lval* a); // use {x & xs} to carry arbitary number of arguments

struct lenv {
    lenv* par;
    int count;
    char** syms;
    lval** vals;
};

lenv* lenv_new();
lenv* lenv_copy(lenv* e);
void lenv_del(lenv* e);
lval* lenv_get(lenv* e, lval* k); // get variable from local to global
void lenv_put(lenv* e, lval* k, lval* v); // define variable in local scope
void lenv_def(lenv* e, lval* k, lval* v); // define variable in global scope

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
    v->builtin = func;
    return v;
}
lval* lval_lambda(lval* formals, lval* body) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_FUN;
    v->builtin = NULL;
    v->env = lenv_new();
    v->formals = formals;
    v->body = body;
    return v;
}
lval* lval_copy(lval* v) {
    lval* x = malloc(sizeof(lval));
    x->type = v->type;
    switch (v->type)
    {
        case LVAL_FUN:
            if(v->builtin) {
                x->builtin = v->builtin;
            } else {
                x->builtin = NULL;
                x->env = lenv_copy(v->env);
                x->formals = lval_copy(v->formals);
                x->body = lval_copy(v->body);
            }
            break;
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
int lval_eq(lval* x, lval* y) {
    if(x->type != y->type) {return 0;}
    switch(x->type)
    {
        case LVAL_NUM: return (x->num == y->num);
        case LVAL_ERR: return (strcmp(x->err, y->err) == 0);
        case LVAL_SYM: return (strcmp(x->sym, y->sym) == 0);
        case LVAL_FUN:
            if(x->builtin || y->builtin) {
                return (x->builtin == y->builtin);
            } else {
                return lval_eq(x->formals, y->formals)
                    && lval_eq(x->body, y->body);
            }
        case LVAL_QEXPR:
        case LVAL_SEXPR:
            if(x->count != y->count) {return 0;}
            for(int i = 0;i < x->count;i++) {
                if(!lval_eq(x->cell[i], y->cell[i])) {return 0;}
            }
            return 1;
        default:
            break;
    }
    return 0;
}
void lval_del(lval *v) {
    switch(v->type) {
        case LVAL_NUM: break;
        case LVAL_ERR: free(v->err); break;
        case LVAL_SYM: free(v->sym); break;
        case LVAL_FUN:
            if(!v->builtin) {
                lenv_del(v->env);
                lval_del(v->formals);
                lval_del(v->body);
            }
            break;
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
lval* lval_pop(lval* v, int i) {
    lval* x = v->cell[i];

    memmove(&v->cell[i], &v->cell[i+1], sizeof(lval*) * (v->count-i-1));
    v->count--;
    v->cell = realloc(v->cell, sizeof(lval*) * v->count);
    return x;
}
lval* lval_take(lval* v, int i) {
    lval* x = lval_pop(v, i);
    lval_del(v);
    return x;
}
lval* lval_join(lval* x, lval* y) {
    while(y->count) {
        x = lval_add(x, lval_pop(y, 0));
    }
    lval_del(y);
    return x;
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
        case LVAL_FUN:
            if(v->builtin) {
                printf("<function>");
            } else {
                printf("(\\ ");
                lval_print(v->formals);
                putchar(' ');
                lval_print(v->body);
                putchar(')');
            }
            break;
        case LVAL_SEXPR: lval_expr_print(v, '(', ')'); break;
        case LVAL_QEXPR: lval_expr_print(v, '{', '}'); break;
    }
}
void lval_println(lval* v) {lval_print(v); putchar('\n');}

lval *lval_eval_sexpr(lenv* e, lval* v) {
    for(int i = 0;i < v->count;i++) {
        v->cell[i] = lval_eval(e, v->cell[i]);
    }

    for(int i = 0;i < v->count;i++) {
        if(v->cell[i]->type == LVAL_ERR) {
            return lval_take(v, i);
        }
    }

    if(v->count == 0) {return v;}
    if(v->count == 1) {return lval_take(v, 0);}

    lval* f = lval_pop(v, 0);
    if(f->type != LVAL_FUN) {
        lval_del(f); lval_del(v);
        return lval_err("first element is not a function");
    }

    lval* result = lval_call(e, f, v);
    lval_del(f);
    return result;
}
lval* lval_eval(lenv* e, lval *v) {
    if(v->type == LVAL_SYM) {
        lval* x = lenv_get(e, v);
        lval_del(v);
        return x;
    }
    if(v->type == LVAL_SEXPR) {return lval_eval_sexpr(e, v);}
    return v;
}
lval* lval_call(lenv* e, lval* f, lval* a) {
    if(f->builtin) {return f->builtin(e, a);}

    int given = a->count;
    int total = f->formals->count;

    while (a->count) {
        if (f->formals->count == 0) {
            lval_del(a);
            return lval_err(
                "Function passed too many arguments. "
                "Got %i, Expected %i.", given, total);
        }
        lval* sym = lval_pop(f->formals, 0);
        if (strcmp(sym->sym, "&") == 0) {
            if (f->formals->count != 1) {
                lval_del(a);
                return lval_err("Function format invalid. "
                    "Symbol '&' not followed by single symbol.");
            }

            lval* nsym = lval_pop(f->formals, 0);
            a->type = LVAL_QEXPR;
            lenv_put(f->env, nsym, a);
            lval_del(sym); lval_del(nsym);
            break;
        }
        lval* val = lval_pop(a, 0);
        lenv_put(f->env, sym, val);
        lval_del(sym); lval_del(val);
    }
    if (f->formals->count > 0
    && strcmp(f->formals->cell[0]->sym, "&") == 0) {
        if (f->formals->count != 2) {
            return lval_err("Function format invalid. "
                "Symbol '&' not followed by single symbol.");
        }
        lval_del(lval_pop(f->formals, 0));

        lval* sym = lval_pop(f->formals, 0);
        lval* val = lval_qexpr();

        lenv_put(f->env, sym, val);
        lval_del(sym); lval_del(val);
    }

    if (f->formals->count == 0) {
        f->env->par = e;
        lval* fbody = lval_copy(f->body);
        fbody->type = LVAL_SEXPR; // must be QEXPR before
        return lval_eval(f->env, fbody);
    } else {
        return lval_copy(f);
    }
}

lenv* lenv_new() {
    lenv* e = malloc(sizeof(lenv));
    e->par = NULL;
    e->count = 0;
    e->syms = NULL;
    e->vals = NULL;
    return e;
}
lenv* lenv_copy(lenv* e) {
    lenv* n = malloc(sizeof(lenv));
    n->par = e->par;
    n->count = e->count;
    n->syms = malloc(sizeof(char*) * n->count);
    n->vals = malloc(sizeof(lval*) * n->count);
    for (int i = 0; i < e->count; i++) {
        n->syms[i] = malloc(strlen(e->syms[i]) + 1);
        strcpy(n->syms[i], e->syms[i]);
        n->vals[i] = lval_copy(e->vals[i]);
    }
  return n;
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
    return e->par
        ? lenv_get(e->par, k)
        : lval_err("Unbounded Symbol '%s'", k->sym);
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
void lenv_def(lenv* e, lval* k, lval* v) {
    while(e->par) {e = e->par;}
    lenv_put(e, k, v);
}