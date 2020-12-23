/* --------------------------------------------
** Copyright (c) 2020, NessOffice
** All rights reserved.
**
** Environment: Windows
** Author: Ness
** Progress: Chapter 15
-------------------------------------------- */
const char *lispy_version = "0.0.0.1.0";

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "builtin_func.c"
#include "util.c"

int main() {
    Number   = mpc_new("number");
    Symbol   = mpc_new("symbol");
    String   = mpc_new("string");
    Comment  = mpc_new("comment");
    Sexpr    = mpc_new("sexpr");
    Qexpr    = mpc_new("qexpr");
    Expr     = mpc_new("expr");
    Lispy    = mpc_new("lispy");

    mpca_lang(MPCA_LANG_DEFAULT,
    "                                                        \
        number   : /-?[0-9]+/ ;                              \
        symbol   : /[a-zA-Z0-9_+\\-*\\/\\\\%=<>!&]+/ ;       \
        string   : /\"(\\\\.|[^\"])*\"/ ;                    \
        comment  : /;[^\\r\\n]*/ ;                           \
        sexpr    : '(' <expr>* ')' ;                         \
        qexpr    : '{' <expr>* '}' ;                         \
        expr     : <number>  | <symbol> | <string>           \
                 | <comment> | <sexpr> | <qexpr> ;           \
        lispy    : /^/ <expr>* /$/ ;                         \
    ",
    Number, Symbol, String, Comment, Sexpr, Qexpr, Expr, Lispy);

    println("Lispy Version %s", lispy_version);
    println("Press ctrl+z to exit\n");

    lenv* e = lenv_new();
    lenv_add_builtins(e);

    lval* a = lval_sexpr();
    lval_add(a, lval_str("lstd/prelude.lspy"));
    lval* x = builtin_load(e, a);
    lval_del(x);

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
    mpc_cleanup(8, 
        Number, Symbol, String, Comment,
        Sexpr, Qexpr, Expr, Lispy);

	return 0;
}
