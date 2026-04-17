%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "token.h"
#include "yylval.h"

// 外部变量/函数声明（来自Lex）
extern int yylex(void);
extern int yylineno;
extern char* yytext;

// 错误处理函数声明
void yyerror(const char* s);

// 调试开关：开启后会打印语法解析过程
#define YYDEBUG 1
%}

/* ========== 语义值联合体，与yylval.h完全一致 ========== */
%union {
    int ival;
    char *sval;
}

/* ========== Token声明，与token.h完全匹配 ========== */
%token T_EOF
%token T_INT T_VOID T_IF T_ELSE T_WHILE T_RETURN T_BREAK T_CONTINUE
%token <sval> T_IDENT T_STRING
%token <ival> T_INTEGER
%token T_PLUS T_MINUS T_MULT T_DIV
%token T_ASSIGN T_EQ T_NE T_LT T_LE T_GT T_GE
%token T_AND T_OR T_NOT
%token T_LPAREN T_RPAREN T_LBRACE T_RBRACE T_LBRACKET T_RBRACKET T_SEMI T_COMMA

/* ========== 运算符优先级与结合性（从上到下优先级递增） ========== */
%right T_ASSIGN          // 赋值右结合
%left T_OR               // 逻辑或左结合
%left T_AND              // 逻辑与左结合
%left T_EQ T_NE          // 相等性比较左结合
%left T_LT T_LE T_GT T_GE // 关系比较左结合
%left T_PLUS T_MINUS     // 加减左结合
%left T_MULT T_DIV       // 乘除左结合
%right T_NOT UMINUS      // 逻辑非、一元负号右结合
%nonassoc LOWER_THAN_ELSE // 解决if-else悬挂二义性
%nonassoc T_ELSE

/* ========== 语法开始符号 ========== */
%start program

%%

/* ========== 1. 程序入口：全局声明（变量/函数） ========== */
program:
    /* 空程序 */
    | program global_decl
    ;

global_decl:
    var_decl    { printf("[行号:%d] 全局变量声明\n", yylineno); }
    | func_def  { printf("[行号:%d] 函数定义完成\n", yylineno); }
    ;

/* ========== 2. 类型说明符 ========== */
type_spec:
    T_INT   { $$ = T_INT; }
    | T_VOID{ $$ = T_VOID; }
    ;

/* ========== 3. 变量声明（全局/局部通用） ========== */
var_decl:
    type_spec T_IDENT T_SEMI
    {
        printf("[行号:%d] 变量声明: 类型=%s, 名称=%s\n", 
               yylineno, ($1 == T_INT) ? "int" : "void", $2);
        free($2); // 释放strdup的内存
    }
    | type_spec T_IDENT T_LBRACKET T_INTEGER T_RBRACKET T_SEMI
    {
        printf("[行号:%d] 数组声明: 类型=%s, 名称=%s, 长度=%d\n", 
               yylineno, ($1 == T_INT) ? "int" : "void", $2, $4);
        free($2);
    }
    ;

/* ========== 4. 函数定义 ========== */
func_def:
    type_spec T_IDENT T_LPAREN param_list T_RPAREN compound_stmt
    {
        printf("[行号:%d] 函数头: 返回值=%s, 函数名=%s\n", 
               yylineno, ($1 == T_INT) ? "int" : "void", $2);
        free($2);
    }
    ;

/* ========== 5. 函数参数列表 ========== */
param_list:
    T_VOID
    {
        printf("[行号:%d] 参数列表: 无参数\n", yylineno);
    }
    | param_decl_list
    ;

param_decl_list:
    param_decl
    | param_decl_list T_COMMA param_decl
    ;

param_decl:
    type_spec T_IDENT
    {
        printf("[行号:%d] 参数: 类型=%s, 名称=%s\n", 
               yylineno, ($1 == T_INT) ? "int" : "void", $2);
        free($2);
    }
    | type_spec T_IDENT T_LBRACKET T_RBRACKET
    {
        printf("[行号:%d] 数组参数: 类型=%s[], 名称=%s\n", 
               yylineno, ($1 == T_INT) ? "int" : "void", $2);
        free($2);
    }
    ;

/* ========== 6. 复合语句（代码块） ========== */
compound_stmt:
    T_LBRACE local_decls stmt_list T_RBRACE
    {
        printf("[行号:%d] 代码块结束\n", yylineno);
    }
    ;

local_decls:
    /* 空 */
    | local_decls var_decl
    ;

stmt_list:
    /* 空 */
    | stmt_list stmt
    ;

/* ========== 7. 语句类型 ========== */
stmt:
    expr_stmt   { printf("[行号:%d] 表达式语句\n", yylineno); }
    | compound_stmt
    | if_stmt
    | while_stmt
    | return_stmt
    | break_stmt
    | continue_stmt
    | error T_SEMI { yyerrok; /* 错误恢复：遇到分号继续解析 */ }
    ;

/* 7.1 表达式语句 */
expr_stmt:
    expression T_SEMI
    | T_SEMI { printf("[行号:%d] 空语句\n", yylineno); }
    ;

/* 7.2 if-else语句（解决悬挂else二义性） */
if_stmt:
    T_IF T_LPAREN expression T_RPAREN stmt %prec LOWER_THAN_ELSE
    {
        printf("[行号:%d] if语句（无else）\n", yylineno);
    }
    | T_IF T_LPAREN expression T_RPAREN stmt T_ELSE stmt
    {
        printf("[行号:%d] if-else语句\n", yylineno);
    }
    ;

/* 7.3 while循环语句 */
while_stmt:
    T_WHILE T_LPAREN expression T_RPAREN stmt
    {
        printf("[行号:%d] while循环语句\n", yylineno);
    }
    ;

/* 7.4 return语句 */
return_stmt:
    T_RETURN T_SEMI
    {
        printf("[行号:%d] return语句（无返回值）\n", yylineno);
    }
    | T_RETURN expression T_SEMI
    {
        printf("[行号:%d] return语句（带返回值）\n", yylineno);
    }
    ;

/* 7.5 break/continue语句 */
break_stmt:
    T_BREAK T_SEMI
    {
        printf("[行号:%d] break语句\n", yylineno);
    }
    ;

continue_stmt:
    T_CONTINUE T_SEMI
    {
        printf("[行号:%d] continue语句\n", yylineno);
    }
    ;

/* ========== 8. 表达式（优先级从低到高） ========== */
expression:
    assign_expr
    ;

assign_expr:
    logic_or_expr
    | T_IDENT T_ASSIGN assign_expr
    {
        printf("[行号:%d] 赋值操作: %s = ...\n", yylineno, $1);
        free($1);
    }
    | T_IDENT T_LBRACKET expression T_RBRACKET T_ASSIGN assign_expr
    {
        printf("[行号:%d] 数组赋值: %s[...] = ...\n", yylineno, $1);
        free($1);
    }
    ;

logic_or_expr:
    logic_and_expr
    | logic_or_expr T_OR logic_and_expr
    {
        printf("[行号:%d] 逻辑或运算 ||\n", yylineno);
    }
    ;

logic_and_expr:
    equality_expr
    | logic_and_expr T_AND equality_expr
    {
        printf("[行号:%d] 逻辑与运算 &&\n", yylineno);
    }
    ;

equality_expr:
    relational_expr
    | equality_expr T_EQ relational_expr
    {
        printf("[行号:%d] 相等比较 ==\n", yylineno);
    }
    | equality_expr T_NE relational_expr
    {
        printf("[行号:%d] 不等比较 !=\n", yylineno);
    }
    ;

relational_expr:
    additive_expr
    | relational_expr T_LT additive_expr
    {
        printf("[行号:%d] 小于比较 <\n", yylineno);
    }
    | relational_expr T_LE additive_expr
    {
        printf("[行号:%d] 小于等于比较 <=\n", yylineno);
    }
    | relational_expr T_GT additive_expr
    {
        printf("[行号:%d] 大于比较 >\n", yylineno);
    }
    | relational_expr T_GE additive_expr
    {
        printf("[行号:%d] 大于等于比较 >=\n", yylineno);
    }
    ;

additive_expr:
    multiplicative_expr
    | additive_expr T_PLUS multiplicative_expr
    {
        printf("[行号:%d] 加法运算 +\n", yylineno);
    }
    | additive_expr T_MINUS multiplicative_expr
    {
        printf("[行号:%d] 减法运算 -\n", yylineno);
    }
    ;

multiplicative_expr:
    unary_expr
    | multiplicative_expr T_MULT unary_expr
    {
        printf("[行号:%d] 乘法运算 *\n", yylineno);
    }
    | multiplicative_expr T_DIV unary_expr
    {
        printf("[行号:%d] 除法运算 /\n", yylineno);
    }
    ;

unary_expr:
    postfix_expr
    | T_MINUS unary_expr %prec UMINUS
    {
        printf("[行号:%d] 一元负号运算\n", yylineno);
    }
    | T_NOT unary_expr
    {
        printf("[行号:%d] 逻辑非运算 !\n", yylineno);
    }
    ;

postfix_expr:
    primary_expr
    | postfix_expr T_LBRACKET expression T_RBRACKET
    {
        printf("[行号:%d] 数组下标访问\n", yylineno);
    }
    | postfix_expr T_LPAREN arg_list T_RPAREN
    {
        printf("[行号:%d] 函数调用\n", yylineno);
    }
    ;

primary_expr:
    T_IDENT
    {
        printf("[行号:%d] 标识符: %s\n", yylineno, $1);
        free($1);
    }
    | T_INTEGER
    {
        printf("[行号:%d] 整数常量: %d\n", yylineno, $1);
    }
    | T_STRING
    {
        printf("[行号:%d] 字符串常量: \"%s\"\n", yylineno, $1);
        free($1);
    }
    | T_LPAREN expression T_RPAREN
    {
        printf("[行号:%d] 括号表达式\n", yylineno);
    }
    ;

/* ========== 9. 函数调用参数列表 ========== */
arg_list:
    /* 无参数 */
    | assign_expr
    | arg_list T_COMMA assign_expr
    ;

%%

/* ========== 错误处理函数 ========== */
void yyerror(const char* s) {
    fprintf(stderr, "[语法错误] 行号:%d, 错误信息:%s, 当前Token:%s\n", 
            yylineno, s, yytext);
}

/* ========== 主函数 ========== */
int main(int argc, char* argv[]) {
    printf("===== MiniC 语法分析器启动 =====\n\n");
    // 开启调试模式（可选）
    // yydebug = 1;
    int ret = yyparse();
    printf("\n===== 语法分析结束 =====\n");
    if (ret == 0) {
        printf("✅ 解析成功：无语法错误\n");
    } else {
        printf("❌ 解析失败：存在语法错误\n");
    }
    return ret;
}