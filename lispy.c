/* --------------------------------------------
** Environment: Windows
** Author: Ness
** Progress: Chapter 8
** Testcases:
**     / 10 0
**     / 10 2
-------------------------------------------- */
const char *lispy_version = "0.0.0.0.4";

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "libs/mpc.h"
#include "util.c"

typedef enum LVAL_TYPE { LVAL_NUM, LVAL_ERR } LVAL_TYPE;
typedef enum LVAL_ERR_TYPE { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM } LVAL_ERR_TYPE;
typedef long LVAL_VAL_TYPE;

typedef struct {
    LVAL_TYPE type;
    union {
        LVAL_VAL_TYPE num;
        LVAL_ERR_TYPE err;
    };
} lval;

/* constructor begin */
lval lval_num(long x) {
    lval v;
    v.type = LVAL_NUM;
    v.num = x;
    return v;
}
lval lval_err(int x) {
    lval v;
    v.type = LVAL_ERR;
    v.err = x;
    return v;
}
/* constructor end  */

void lval_print(lval v) {
    switch(v.type) {
        case LVAL_NUM: printf("%li", v.num); break;
        case LVAL_ERR:
            switch(v.err) {
                case LERR_DIV_ZERO: printf("Error: division by zero!"); break;
                case LERR_BAD_OP:   printf("Error: Invalid Operator!"); break;
                case LERR_BAD_NUM:  printf("Error: Invalid Number!");   break;
            }
            break;
    }
}
void lval_println(lval v) { lval_print(v); putchar('\n'); }

lval eval_op(char *op, lval x, lval y) {
    if(x.type == LVAL_ERR) {return x;}
    if(y.type == LVAL_ERR) {return y;}

    if(strcmp(op, "+") == 0) {return lval_num(x.num + y.num);}
    if(strcmp(op, "-") == 0) {return lval_num(x.num - y.num);}
    if(strcmp(op, "*") == 0) {return lval_num(x.num * y.num);}
    if(strcmp(op, "/") == 0) {
        return y.num == 0
            ? lval_err(LERR_DIV_ZERO)
            : lval_num(x.num / y.num);
    }
    return lval_err(LERR_BAD_OP);
}

lval eval(mpc_ast_t *t) {
    if(strstr(t->tag, "number")) {
        errno = 0;
        long x = strtol(t->contents, NULL, 10);
        return errno != ERANGE
            ? lval_num(x)
            : lval_err(LERR_BAD_NUM);
    }
    /* The operator is always second child. */
    char* op = t->children[1]->contents;
    lval x = eval(t->children[2]);
    for(int i = 3;strstr(t->children[i]->tag, "expr");i++) {
        x = eval_op(op, x, eval(t->children[i]));
    }
    return x;
}

int main() {
    mpc_parser_t* Number   = mpc_new("number");
    mpc_parser_t* Operator = mpc_new("operator");
    mpc_parser_t* Expr     = mpc_new("expr");
    mpc_parser_t* Lispy    = mpc_new("lispy");

    mpca_lang(MPCA_LANG_DEFAULT,
  "                                                     \
    number   : /-?[0-9]+/ ;                             \
    operator : '+' | '-' | '*' | '/' ;                  \
    expr     : <number> | '(' <operator> <expr>+ ')' ;  \
    lispy    : /^/ <operator> <expr>+ /$/ ;             \
  ",
  Number, Operator, Expr, Lispy);

    println("Lispy Version %s", lispy_version);
    println("Press ctrl+c to exit\n");

    while(1) {
        char *input = readline("lispy> ");

        mpc_result_t r;
        if(mpc_parse("<stdin>", input, Lispy, &r)) {
            // println("%li", eval(r.output));
            lval_println(eval(r.output));
            mpc_ast_delete(r.output);
        } else {
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }
        free(input);
    }
    mpc_cleanup(4, Number, Operator, Expr, Lispy);

	return 0;
}
