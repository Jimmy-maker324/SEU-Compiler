%{
#include <stdio.h>
void yyerror(const char* s) { fprintf(stderr, "Error\n"); }
extern int yylex(void);
%}

%token A B C
%start program

%%

program:
    stmt_list
    ;

stmt_list:
    stmt
    | stmt_list stmt
    ;

stmt:
    A
    | B
    | C
    ;

%%

int main() {
    yyparse();
    return 0;
}