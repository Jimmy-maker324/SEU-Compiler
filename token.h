#ifndef TOKEN_H
#define TOKEN_H

// 终结符编号
#define T_EOF         0

#define T_INT         1
#define T_VOID        2
#define T_IF          3
#define T_ELSE        4
#define T_WHILE       5
#define T_RETURN      6
#define T_BREAK       7
#define T_CONTINUE    8

#define T_IDENT       10
#define T_INTEGER     11
#define T_STRING      12

#define T_PLUS        20
#define T_MINUS       21
#define T_MULT        22
#define T_DIV         23

#define T_ASSIGN      30
#define T_EQ          31
#define T_NE          32
#define T_LT          33
#define T_LE          34
#define T_GT          35
#define T_GE          36

#define T_AND         40
#define T_OR          41
#define T_NOT         42

#define T_LPAREN      50
#define T_RPAREN      51
#define T_LBRACE      52
#define T_RBRACE      53
#define T_LBRACKET    54
#define T_RBRACKET    55
#define T_SEMI        56
#define T_COMMA       57

#endif