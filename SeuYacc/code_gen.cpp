#include "common.h"
#include <iomanip>

// ==================== 内部辅助函数声明 ====================
namespace {
    // 生成文件头部（包含声明段用户代码）
    void generate_header(ofstream& out);
    
    // 生成 YYSTYPE 定义
    void generate_yystype(ofstream& out);
    
    // 生成分析表（硬编码为 C 数组）
    void generate_tables(ofstream& out);
    
    // 生成 LR 总控程序 yyparse()
    void generate_yyparse(ofstream& out);
    
    // 生成错误处理函数 yyerror()
    void generate_yyerror(ofstream& out);
    
    // 生成文件尾部（包含用户子程序）
    void generate_footer(ofstream& out);
    
    // 辅助：转义字符串中的特殊字符
    string escape_string(const string& s);
}

// ==================== 模块6核心函数：生成 yyparse.c ====================
void generate_yyparse_c(const string& output_filename) {
    DEBUG_PRINT("开始生成 " << output_filename << "...");

    ofstream out(output_filename);
    if (!out.is_open()) {
        cerr << "❌ 无法打开输出文件: " << output_filename << endl;
        return;
    }

    // ==================== 按顺序生成各部分 ====================
    generate_header(out);
    generate_yystype(out);
    generate_tables(out);
    generate_yyparse(out);
    generate_yyerror(out);
    generate_footer(out);

    out.close();
    DEBUG_PRINT("✅ " << output_filename << " 生成成功！");
}

// ==================== 内部辅助函数实现 ====================
namespace {
    void generate_header(ofstream& out) {
        out << "/* =========================================" << endl;
        out << "   本文件由 SeuYacc 自动生成" << endl;
        out << "   东南大学编译原理课程设计" << endl;
        out << "   ========================================= */" << endl;
        out << endl;

        // 1. 标准头文件
        out << "#include <stdio.h>" << endl;
        out << "#include <stdlib.h>" << endl;
        out << "#include <string.h>" << endl;
        out << endl;

        // 2. 拷贝声明段用户代码 (%{ ... %})
        out << "/* --- 用户声明段代码 --- */" << endl;
        if (!decl_user_code.empty()) {
            out << "%{" << endl;
            out << decl_user_code << endl;
            out << "%}" << endl;
        }
        out << endl;

        // 3. 外部函数声明
        out << "/* --- 外部函数声明 --- */" << endl;
        out << "extern int yylex(void);" << endl;
        out << "extern char* yytext;" << endl;
        out << "extern int yylineno;" << endl;
        out << endl;
    }

    void generate_yystype(ofstream& out) {
        out << "/* --- 语义值类型定义 --- */" << endl;
        out << "#ifndef YYSTYPE" << endl;
        out << "#define YYSTYPE int" << endl; // 课程设计简化版，默认 int
        out << "#endif" << endl;
        out << endl;
        out << "typedef union {" << endl;
        out << "    int ival;" << endl;
        out << "    char* sval;" << endl;
        out << "} YYSTYPE;" << endl;
        out << endl;
        out << "YYSTYPE yylval;" << endl; // 全局语义值
        out << endl;
    }

    void generate_tables(ofstream& out) {
        out << "/* --- LR 分析表定义 --- */" << endl;
        out << "/* Action 类型 */" << endl;
        out << "#define ACTION_SHIFT  1" << endl;
        out << "#define ACTION_REDUCE 2" << endl;
        out << "#define ACTION_ACCEPT 3" << endl;
        out << "#define ACTION_ERROR  4" << endl;
        out << endl;

        // 1. 生成 Action 表
        int num_states = action_table.size();
        int num_terms = action_table.empty() ? 0 : action_table[0].size();

        out << "/* Action 表: [状态][终结符] = {类型, 数值} */" << endl;
        out << "static int action_table[" << num_states << "][" << num_terms << "][2] = {" << endl;
        
        for (int i = 0; i < num_states; i++) {
            out << "    {";
            for (int j = 0; j < num_terms; j++) {
                Action& a = action_table[i][j];
                int type_code = 0;
                switch(a.type) {
                    case ACTION_SHIFT: type_code = 1; break;
                    case ACTION_REDUCE: type_code = 2; break;
                    case ACTION_ACCEPT: type_code = 3; break;
                    default: type_code = 4; break;
                }
                out << "{" << type_code << "," << (a.num >= 0 ? a.num : -1) << "}";
                if (j != num_terms - 1) out << ",";
            }
            out << "}";
            if (i != num_states - 1) out << ",";
            out << endl;
        }
        out << "};" << endl;
        out << endl;

        // 2. 生成 Goto 表
        int num_non_terms = goto_table.empty() ? 0 : goto_table[0].size();
        out << "/* Goto 表: [状态][非终结符索引] = 下一状态 */" << endl;
        out << "static int goto_table[" << num_states << "][" << num_non_terms << "] = {" << endl;
        
        for (int i = 0; i < num_states; i++) {
            out << "    {";
            for (int j = 0; j < num_non_terms; j++) {
                out << goto_table[i][j];
                if (j != num_non_terms - 1) out << ",";
            }
            out << "}";
            if (i != num_states - 1) out << ",";
            out << endl;
        }
        out << "};" << endl;
        out << endl;

        // 3. 生成产生式信息表
        out << "/* 产生式信息: [产生式编号] = {左部非终结符索引, 右部长度} */" << endl;
        out << "static int prod_info[" << prod_info.size() << "][2] = {" << endl;
        for (size_t i = 0; i < prod_info.size(); i++) {
            // 左部非终结符转换为索引
            int left_idx = prod_info[i].left - NON_TERM_BASE;
            out << "    {" << left_idx << "," << prod_info[i].right_len << "}";
            if (i != prod_info.size() - 1) out << ",";
            out << endl;
        }
        out << "};" << endl;
        out << endl;
    }

    void generate_yyparse(ofstream& out) {
        out << "/* --- LR 总控程序 --- */" << endl;
        out << "int yyparse(void) {" << endl;
        out << "    /* 栈定义 */" << endl;
        out << "    #define STACK_SIZE 1024" << endl;
        out << "    int state_stack[STACK_SIZE];" << endl;
        out << "    int sym_stack[STACK_SIZE];" << endl;
        out << "    YYSTYPE val_stack[STACK_SIZE];" << endl;
        out << "    int sp = 0; // 栈指针" << endl;
        out << endl;

        out << "    /* 初始化 */" << endl;
        out << "    state_stack[sp] = 0; // 初始状态 0" << endl;
        out << "    sym_stack[sp] = 0;   // 初始符号 $" << endl;
        out << "    sp++;" << endl;
        out << endl;

        out << "    int token = yylex(); // 读取第一个 Token" << endl;
        out << endl;

        out << "    /* 主循环 */" << endl;
        out << "    while (1) {" << endl;
        out << "        int cur_state = state_stack[sp - 1];" << endl;
        out << "        int act_type = action_table[cur_state][token][0];" << endl;
        out << "        int act_num = action_table[cur_state][token][1];" << endl;
        out << endl;

        out << "        switch (act_type) {" << endl;
        out << "            case ACTION_SHIFT: /* 移进 */" << endl;
        out << "                state_stack[sp] = act_num;" << endl;
        out << "                sym_stack[sp] = token;" << endl;
        out << "                val_stack[sp] = yylval;" << endl;
        out << "                sp++;" << endl;
        out << "                token = yylex();" << endl;
        out << "                break;" << endl;
        out << endl;

        out << "            case ACTION_REDUCE: /* 归约 */" << endl;
        out << "                {" << endl;
        out << "                    int prod_id = act_num;" << endl;
        out << "                    int left_idx = prod_info[prod_id][0];" << endl;
        out << "                    int right_len = prod_info[prod_id][1];" << endl;
        out << endl;
        out << "                    /* 弹出栈 */" << endl;
        out << "                    sp -= right_len;" << endl;
        out << endl;
        out << "                    /* 这里插入语义动作（课程设计可扩展） */" << endl;
        out << "                    /* $$ = ...; */" << endl;
        out << endl;
        out << "                    /* 查 Goto 表 */" << endl;
        out << "                    int prev_state = state_stack[sp - 1];" << endl;
        out << "                    int next_state = goto_table[prev_state][left_idx];" << endl;
        out << endl;
        out << "                    /* 压入新状态和非终结符 */" << endl;
        out << "                    state_stack[sp] = next_state;" << endl;
        out << "                    sym_stack[sp] = left_idx + " << NON_TERM_BASE << ";" << endl;
        out << "                    /* val_stack[sp] = ...; */" << endl;
        out << "                    sp++;" << endl;
        out << "                }" << endl;
        out << "                break;" << endl;
        out << endl;

        out << "            case ACTION_ACCEPT: /* 接受 */" << endl;
        out << "                return 0; /* 成功 */" << endl;
        out << endl;

        out << "            case ACTION_ERROR: /* 错误 */" << endl;
        out << "            default:" << endl;
        out << "                yyerror(\"语法错误\");" << endl;
        out << "                return 1; /* 失败 */" << endl;
        out << "        }" << endl;
        out << "    }" << endl;
        out << "}" << endl;
        out << endl;
    }

    void generate_yyerror(ofstream& out) {
        out << "/* --- 错误处理函数 --- */" << endl;
        out << "void yyerror(const char* msg) {" << endl;
        out << "    fprintf(stderr, \"[语法错误] 行号: %d, 信息: %s, 当前Token: %s\\n\"," << endl;
        out << "            yylineno, msg, yytext);" << endl;
        out << "}" << endl;
        out << endl;
    }

    void generate_footer(ofstream& out) {
        out << "/* --- 用户子程序代码 --- */" << endl;
        if (!user_sub_code.empty()) {
            out << user_sub_code << endl;
        }
        out << endl;

        // 可选：生成一个简单的 main 函数
        out << "/* --- 可选主函数（可删除） --- */" << endl;
        out << "int main() {" << endl;
        out << "    printf(\"=== MiniC 语法分析器启动 ===\\n\");" << endl;
        out << "    int ret = yyparse();" << endl;
        out << "    if (ret == 0) {" << endl;
        out << "        printf(\"=== 解析成功 ===\\n\");" << endl;
        out << "    } else {" << endl;
        out << "        printf(\"=== 解析失败 ===\\n\");" << endl;
        out << "    }" << endl;
        out << "    return ret;" << endl;
        out << "}" << endl;
    }

    string escape_string(const string& s) {
        string res;
        for (char c : s) {
            switch(c) {
                case '\\': res += "\\\\"; break;
                case '\"': res += "\\\""; break;
                case '\n': res += "\\n"; break;
                case '\t': res += "\\t"; break;
                default: res += c;
            }
        }
        return res;
    }
}