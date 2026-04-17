#ifndef COMMON_H
#define COMMON_H

// ==================== 1. 标准库头文件（全覆盖所有模块需求） ====================
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <set>
#include <stack>
#include <queue>
#include <algorithm>
#include <functional>
#include <cstring>
#include <cstdlib>
#include <limits>

using namespace std;

// ==================== 2. 调试开关（打开可输出各模块调试信息） ====================
#define DEBUG 1
#if DEBUG
#define DEBUG_PRINT(x) cout << "[DEBUG] " << x << endl
#else
#define DEBUG_PRINT(x)
#endif

// ==================== 3. 核心宏定义 ====================
// 符号编号分区：终结符0~999，非终结符从1000开始，彻底避免编号冲突
#define TERMINAL_MAX    999
#define NON_TERM_BASE   1000

// 终结符/非终结符判断宏
#define IS_TERMINAL(sym)    (sym <= TERMINAL_MAX)
#define IS_NON_TERM(sym)    (sym >= NON_TERM_BASE)

// 输入结束符（对应文法中的$）
#define END_MARKER      T_EOF

// ==================== 4. 统一Token编号（和minic.l、token.h完全一致） ====================
enum TokenID {
    T_EOF         = 0,    // 文件结束符（$）

    // 关键字
    T_INT         = 1,
    T_VOID        = 2,
    T_IF          = 3,
    T_ELSE        = 4,
    T_WHILE       = 5,
    T_RETURN      = 6,
    T_BREAK       = 7,
    T_CONTINUE    = 8,

    // 带语义值的Token
    T_IDENT       = 10,   // 标识符
    T_INTEGER     = 11,   // 整数常量
    T_STRING      = 12,   // 字符串常量

    // 算术运算符
    T_PLUS        = 20,   // +
    T_MINUS       = 21,   // -
    T_MULT        = 22,   // *
    T_DIV         = 23,   // /

    // 赋值与比较运算符
    T_ASSIGN      = 30,   // =
    T_EQ          = 31,   // ==
    T_NE          = 32,   // !=
    T_LT          = 33,   // <
    T_LE          = 34,   // <=
    T_GT          = 35,   // >
    T_GE          = 36,   // >=

    // 逻辑运算符
    T_AND         = 40,   // &&
    T_OR          = 41,   // ||
    T_NOT         = 42,   // !

    // 分隔符
    T_LPAREN      = 50,   // (
    T_RPAREN      = 51,   // )
    T_LBRACE      = 52,   // {
    T_RBRACE      = 53,   // }
    T_LBRACKET    = 54,   // [
    T_RBRACKET    = 55,   // ]
    T_SEMI        = 56,   // ;
    T_COMMA       = 57    // ,
};

// 运算符结合性枚举
enum AssocType {
    ASSOC_LEFT,     // 左结合
    ASSOC_RIGHT,    // 右结合
    ASSOC_NONASSOC  // 无结合（解决悬挂else二义性）
};

// Action表动作类型枚举
enum ActionType {
    ACTION_SHIFT,   // 移进
    ACTION_REDUCE,  // 归约
    ACTION_ACCEPT,  // 接受
    ACTION_ERROR    // 错误
};

// ==================== 5. 核心数据结构定义 ====================
/**
 * @brief 优先级与结合性信息（对应.y文件中的%left/%right/%nonassoc）
 */
struct Precedence {
    int level;                // 优先级等级（数字越大，优先级越高）
    AssocType assoc;          // 结合性
};

/**
 * @brief 文法产生式结构
 */
struct Production {
    int left;                 // 左部非终结符编号
    vector<int> right;        // 右部符号串（终结符/非终结符编号）
    string semantic_action;   // 产生式对应的语义动作代码
    int prec_level;           // 产生式优先级（用于解决冲突）

    Production() : left(-1), prec_level(-1) {}
};

/**
 * @brief LR(1)项目结构
 */
struct LR1Item {
    int prod_id;              // 产生式编号（对应productions数组的下标）
    int dot_pos;              // 点的位置（0=最左侧，等于右部长度=点在最右侧）
    int lookahead;            // 向前看预测符（终结符编号）

    // 重载==运算符，用于unordered_set去重
    bool operator==(const LR1Item& other) const {
        return prod_id == other.prod_id 
            && dot_pos == other.dot_pos 
            && lookahead == other.lookahead;
    }
};

// 特化std::hash，让LR1Item可放入unordered_set（C++11及以上必须）
namespace std {
    template<> struct hash<LR1Item> {
        size_t operator()(const LR1Item& item) const {
            // 哈希组合，避免冲突
            size_t h1 = hash<int>()(item.prod_id);
            size_t h2 = hash<int>()(item.dot_pos);
            size_t h3 = hash<int>()(item.lookahead);
            return h1 ^ (h2 << 1) ^ (h3 << 2);
        }
    };
}

/**
 * @brief LR(1) DFA状态结构
 */
struct LR1State {
    int id;                              // 状态唯一编号
    unordered_set<LR1Item> items;       // 该状态的所有LR(1)项目
    map<int, int> transitions;          // 状态转移边：<符号编号, 下一状态编号>

    LR1State() : id(-1) {}
};

/**
 * @brief LR(1) DFA整体结构
 */
struct LR1DFA {
    vector<LR1State> states;            // 所有状态集合
    int start_state;                     // 初始状态编号

    LR1DFA() : start_state(-1) {}
};

/**
 * @brief Action表项结构
 */
struct Action {
    ActionType type;         // 动作类型
    int num;                 // 移进=目标状态编号；归约=产生式编号

    Action() : type(ACTION_ERROR), num(-1) {}
};

/**
 * @brief 产生式精简信息（归约时使用，对应parsing_table.h接口）
 */
struct ProdInfo {
    int left;            // 左部非终结符编号
    int right_len;       // 右部符号个数（归约时弹出栈的长度）
};

// ==================== 6. 全局变量声明（所有模块共享，在common.cpp中定义） ====================
// 符号映射表
extern map<string, int> token_map;          // Token名 -> 编号
extern map<string, int> non_term_map;       // 非终结符名 -> 编号
extern map<int, string> symbol_name_map;    // 符号编号 -> 符号名（调试/错误提示用）

// 文法核心信息
extern vector<Production> productions;      // 所有产生式（0号固定为增广产生式 S'→S）
extern map<int, Precedence> precedence_map; // 符号 -> 优先级与结合性
extern int origin_start_symbol;              // 原文法开始符号
extern int aug_start_symbol;                 // 增广文法开始符号 S'

// 文法预处理结果
extern map<int, set<int>> first_set;         // 符号编号 -> First集

// LR自动机结果
extern LR1DFA lr_dfa;                         // LR(1)/LALR(1) DFA

// 分析表结果（严格匹配接口约定）
extern vector<vector<Action>> action_table;  // Action表：[状态编号][终结符编号]
extern vector<vector<int>> goto_table;       // Goto表：[状态编号][非终结符编号]
extern vector<ProdInfo> prod_info;           // 产生式精简信息：[产生式编号]

// .y文件用户代码
extern string decl_user_code;                 // %{ %} 包裹的声明段代码
extern string user_sub_code;                  // 末尾的用户子程序代码

// 错误处理全局变量
extern int yylineno;                           // 当前解析行号

// ==================== 7. 各模块函数声明（按Yacc六大模块划分） ====================
// -------------------- 模块1：.y文件解析（grammar.cpp） --------------------
/**
 * @brief 解析.y输入文件，提取文法所有信息
 * @param filename .y文件路径
 * @return 解析成功返回true，失败返回false
 */
bool parse_yacc_file(const string& filename);

/**
 * @brief 添加终结符Token
 * @param name Token名
 * @param id 分配的编号
 */
void add_token(const string& name, int id);

/**
 * @brief 添加非终结符
 * @param name 非终结符名
 * @return 分配的编号
 */
int add_non_term(const string& name);

// -------------------- 模块2：文法预处理+First集计算（first_set.cpp） --------------------
/**
 * @brief 计算所有符号的First集
 */
void compute_first_sets();

/**
 * @brief 计算一个符号串的First集
 * @param syms 符号串
 * @return First集结果
 */
set<int> compute_first_of_string(const vector<int>& syms);

// -------------------- 模块3：LR(1) DFA构造（lr1_dfa.cpp） --------------------
/**
 * @brief 构造LR(1)项目集规范族（DFA）
 */
void build_lr1_dfa();

/**
 * @brief 对LR(1)项目集做闭包运算
 * @param items 待闭包的项目集
 * @return 闭包后的完整项目集
 */
unordered_set<LR1Item> closure(const unordered_set<LR1Item>& items);

/**
 * @brief Goto转移运算
 * @param items 源项目集
 * @param sym 转移符号
 * @return 转移后的项目集
 */
unordered_set<LR1Item> goto_trans(const unordered_set<LR1Item>& items, int sym);

// -------------------- 模块4：LALR(1)优化（lalr.cpp） --------------------
/**
 * @brief 合并LR(1)同心项，生成LALR(1) DFA
 */
void merge_lalr_dfa();

// -------------------- 模块5：Action/Goto分析表构造（parsing_table.cpp） --------------------
/**
 * @brief 构造LR分析表
 */
void build_parsing_table();

/**
 * @brief 解决移进-归约/归约-归约冲突
 * @param cur_action 当前已有的动作
 * @param new_action 新的待插入动作
 * @return 最终确定的动作
 */
Action resolve_conflict(const Action& cur_action, const Action& new_action);

// -------------------- 模块6：yyparse.c代码生成（code_gen.cpp） --------------------
/**
 * @brief 生成可编译的yyparse.c语法分析器
 * @param output_filename 输出文件路径
 */
void generate_yyparse_c(const string& output_filename);

// -------------------- 通用工具函数（common.cpp） --------------------
/**
 * @brief 错误信息打印
 * @param msg 错误信息
 */
void yyerror(const string& msg);

/**
 * @brief 打印调试信息：First集
 */
void print_first_set();

/**
 * @brief 打印调试信息：LR(1) DFA
 */
void print_lr_dfa();

/**
 * @brief 打印调试信息：分析表
 */
void print_parsing_table();

#endif // COMMON_H