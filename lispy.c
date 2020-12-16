/* --------------------------------------------
** Environment: Windows
** Author: Ness
** Date: 2020/12/16 11:31
** Progress: Chapter6
** Testcases:
**     + 5 (* 2 2)
**     hello
**     / 1dog
-------------------------------------------- */
const char *lispy_version = "0.0.0.0.2";

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "libs/mpc.h"
#include "util.c"

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
            mpc_ast_print(r.output);
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
