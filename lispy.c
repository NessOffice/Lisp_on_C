/* --------------------------------------------
** Environment: Windows
** Author: Ness
** Date: 2020/12/16 12:16
** Progress: Chapter 7
** Testcases:
**     + 5 6
**     - (* 10 10) (+ 1 1 1)
-------------------------------------------- */
const char *lispy_version = "0.0.0.0.3";

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "libs/mpc.h"
#include "util.c"

long eval_op(char *op, long x, long y) {
    if(strcmp(op, "+") == 0) {return x + y;}
    if(strcmp(op, "-") == 0) {return x - y;}
    if(strcmp(op, "*") == 0) {return x * y;}
    if(strcmp(op, "/") == 0) {return x / y;}
    return 0;
}

long eval(mpc_ast_t *t) {
    if(strstr(t->tag, "number")) {
        return atoi(t->contents);
    }
    /* The operator is always second child. */
    char* op = t->children[1]->contents;
    long x = eval(t->children[2]);
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
            println("%li", eval(r.output));
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
