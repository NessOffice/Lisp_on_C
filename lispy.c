/* --------------------------------------------
** Copyright (c) 2020, NessOffice
** All rights reserved.
**
** Environment: Windows
** Author: Ness
** Progress: Chapter 13
-------------------------------------------- */
const char *lispy_version = "0.0.0.0.9";

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "builtin_func.c"
#include "util.c"

int main() {
    mpc_parser_t* Number   = mpc_new("number");
    mpc_parser_t* Symbol   = mpc_new("symbol");
    mpc_parser_t* Sexpr    = mpc_new("sexpr");
    mpc_parser_t* Qexpr    = mpc_new("qexpr");
    mpc_parser_t* Expr     = mpc_new("expr");
    mpc_parser_t* Lispy    = mpc_new("lispy");

    mpca_lang(MPCA_LANG_DEFAULT,
    "                                                        \
        number   : /-?[0-9]+/ ;                              \
        symbol   : /[a-zA-Z0-9_+\\-*\\/\\\\%=<>!&]+/ ;       \
        sexpr    : '(' <expr>* ')' ;                         \
        qexpr    : '{' <expr>* '}' ;                         \
        expr     : <number> | <symbol> | <sexpr> | <qexpr> ; \
        lispy    : /^/ <expr>* /$/ ;                         \
    ",
    Number, Symbol, Sexpr, Qexpr, Expr, Lispy);

    println("Lispy Version %s", lispy_version);
    println("Press ctrl+z to exit\n");

    lenv* e = lenv_new();
    lenv_add_builtins(e);

    while(1) {
        char *input = readline("lispy> ");
        if(input == NULL) {println("Bye."); break;}

        mpc_result_t r;
        if(mpc_parse("<stdin>", input, Lispy, &r)) {
            lval* x = lval_eval(e, lval_read(r.output));
            lval_println(x);
            lval_del(x);
            mpc_ast_delete(r.output);
        } else {
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }
        free(input);
    }
    mpc_cleanup(6, Number, Symbol, Sexpr, Qexpr, Expr, Lispy);

	return 0;
}
