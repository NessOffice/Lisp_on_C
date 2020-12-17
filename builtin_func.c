#include "ds.c"

lval* lval_eval_sexpr(lenv* e, lval* v);
lval* lval_pop(lval* v, int i);
lval* lval_take(lval* v, int i);
lval* builtin_head(lenv* e, lval* a);
lval* builtin_tail(lenv* e, lval* a);
lval* builtin_list(lenv* e, lval* a);
lval* builtin_eval(lenv* e, lval* a);
lval* builtin_join(lenv* e, lval* a);
lval* builtin_cons(lenv* e, lval* a);
lval* builtin_len(lenv* e, lval* a);
lval* lval_join(lval* x, lval* y);
lval* builtin_op(lenv* e, lval* a, char* op);
lval* lval_eval(lenv* e, lval *v);

lval *lval_eval_sexpr(lenv* e, lval* v) {
    for(int i = 0;i < v->count;i++) {
        v->cell[i] = lval_eval(e, v->cell[i]);
    }

    for(int i = 0;i < v->count;i++) {
        if(v->cell[i]->type == LVAL_ERR) {
            return lval_take(v, i);
        }
    }

    /* special judge */
    if(v->count == 0) {return v;}
    if(v->count == 1) {return lval_take(v, 0);}

    lval* f = lval_pop(v, 0);
    if(f->type != LVAL_FUN) {
        lval_del(f); lval_del(v);
        return lval_err("first element is not a function");
    }

    lval* result = f->fun(e, v);
    lval_del(f);
    return result;
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

#define LASSERT(args, cond, fmt, ...) \
    if(!(cond)) { \
        lval* err = lval_err(fmt, ##__VA_ARGS__); \
        lval_del(args); \
        return err; \
    }
#define LASSERT_TYPE(func, args, index, expect) \
  LASSERT(args, args->cell[index]->type == expect, \
    "Function '%s' passed incorrect type for argument %i. Got %s, Expected %s.", \
    func, index, ltype_name(args->cell[index]->type), ltype_name(expect))
#define LASSERT_NUM(func, args, num) \
  LASSERT(args, args->count == num, \
    "Function '%s' passed incorrect number of arguments. Got %i, Expected %i.", \
    func, args->count, num)
#define LASSERT_NOT_EMPTY(func, args, index) \
  LASSERT(args, args->cell[index]->count != 0, \
    "Function '%s' passed {} for argument %i.", func, index);

lval* builtin_head(lenv* e, lval* a) {
    LASSERT_NUM("head", a, 1);
    LASSERT_TYPE("head", a, 0, LVAL_QEXPR);
    LASSERT_NOT_EMPTY("head", a, 0);
    
    lval* v = lval_take(a, 0);
    while(v->count > 1) {lval_del(lval_pop(v, 1));}
    return v;
}
lval* builtin_tail(lenv* e, lval* a) {
    LASSERT_NUM("head", a, 1);
    LASSERT_TYPE("head", a, 0, LVAL_QEXPR);
    LASSERT_NOT_EMPTY("head", a, 0);
    
    lval* v = lval_take(a, 0);
    lval_del(lval_pop(v, 0));
    return v;
}
lval* builtin_list(lenv* e, lval* a) {
    a->type = LVAL_QEXPR;
    return a;
}
lval* builtin_eval(lenv* e, lval* a) {
    LASSERT_NUM("eval", a, 1);
    LASSERT_TYPE("eval", a, 0, LVAL_QEXPR);

    lval* x = lval_take(a, 0);
    x->type = LVAL_SEXPR;
    return lval_eval(e, x);
}
lval* builtin_join(lenv* e, lval* a) {
    for(int i = 0;i < a->count;i++) {
        LASSERT_TYPE("join", a, i, LVAL_QEXPR);
    }

    lval* x = lval_pop(a, 0);
    while(a->count) {
        x = lval_join(x, lval_pop(a, 0));
    }
    lval_del(a);
    return x;
}
lval* lval_join(lval* x, lval* y) {
    while(y->count) {
        x = lval_add(x, lval_pop(y, 0));
    }
    lval_del(y);
    return x;
}
lval* builtin_cons(lenv* e, lval* a) {
    LASSERT_NUM("cons", a, 2);
    LASSERT_TYPE("cons", a, 1, LVAL_QEXPR);

    lval* x = lval_pop(a, 0);
    lval* y = lval_pop(a, 0);
    y->count++;
    y->cell = realloc(y->cell, sizeof(lval*) * y->count);
    memmove(&y->cell[1], &y->cell[0], sizeof(lval*) * (y->count-1));
    y->cell[0] = x;
    lval_del(a);
    return y;
}
lval* builtin_len(lenv* e,lval* a) {
    LASSERT_NUM("len", a, 1);
    LASSERT_TYPE("len", a, 0, LVAL_QEXPR);
    
    lval* x = lval_num(a->cell[0]->count);
    lval_del(a);
    return x;
}
lval* builtin_add(lenv* e, lval* a) {return builtin_op(e, a, "+");}
lval* builtin_sub(lenv* e, lval* a) {return builtin_op(e, a, "-");}
lval* builtin_mul(lenv* e, lval* a) {return builtin_op(e, a, "*");}
lval* builtin_div(lenv* e, lval* a) {return builtin_op(e, a, "/");}
lval* builtin_op(lenv* e, lval* a, char* op) {
    /* Ensure all arguments are numbers */
    for (int i = 0; i < a->count; i++) {
        LASSERT_TYPE(op, a, i, LVAL_NUM);
    }
    lval* x = lval_pop(a, 0);

    /* special judge: unary negation */
    if ((strcmp(op, "-") == 0) && a->count == 0) {
        x->num = -x->num;
    }

    while (a->count > 0) {
        lval* y = lval_pop(a, 0);

        if (strcmp(op, "+") == 0) { x->num += y->num; }
        if (strcmp(op, "-") == 0) { x->num -= y->num; }
        if (strcmp(op, "*") == 0) { x->num *= y->num; }
        if (strcmp(op, "/") == 0) {
            if (y->num == 0) {
                lval_del(x); lval_del(y);
                x = lval_err("Division By Zero!"); break;
            }
            x->num /= y->num;
        }
        lval_del(y);
  }
  lval_del(a);
  return x;
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

lval* builtin_def(lenv* e, lval* a) {
    LASSERT_TYPE("def", a, 0, LVAL_QEXPR);
    
    lval* syms = a->cell[0];
    for(int i = 0;i < syms->count;i++) {
        LASSERT(a, syms->cell[i]->type == LVAL_SYM,
            "Function 'def' can't define non-symbol");
    }

    LASSERT_NUM("def", a, syms->count+1);
    
    for(int i = 0;i < syms->count;i++) {
        lenv_put(e, syms->cell[i], a->cell[i+1]);
    }
    lval_del(a);
    return lval_sexpr();
}

void lenv_add_builtin(lenv* e, char* name, lbuiltin func) {
    lval* k = lval_sym(name);
    lval* v = lval_fun(func);
    lenv_put(e, k, v);
    lval_del(k); lval_del(v);
}
void lenv_add_builtins(lenv* e) {
    lenv_add_builtin(e, "list", builtin_list);
    lenv_add_builtin(e, "head", builtin_head);
    lenv_add_builtin(e, "tail", builtin_tail);
    lenv_add_builtin(e, "eval", builtin_eval);
    lenv_add_builtin(e, "join", builtin_join);
    lenv_add_builtin(e, "cons", builtin_cons);
    lenv_add_builtin(e, "len", builtin_len);

    lenv_add_builtin(e, "+", builtin_add);
    lenv_add_builtin(e, "-", builtin_sub);
    lenv_add_builtin(e, "*", builtin_mul);
    lenv_add_builtin(e, "/", builtin_div);

    lenv_add_builtin(e, "def", builtin_def);
}