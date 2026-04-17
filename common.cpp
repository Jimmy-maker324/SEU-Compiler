#include "common.h"

// ==================== 全局变量定义 ====================
// 符号映射表
map<string, int> token_map;
map<string, int> non_term_map;
map<int, string> symbol_name_map;

// 文法核心信息
vector<Production> productions;
map<int, Precedence> precedence_map;
int origin_start_symbol = -1;
int aug_start_symbol = -1;

// 文法预处理结果
map<int, set<int>> first_set;

// LR自动机结果
LR1DFA lr_dfa;

// 分析表结果
vector<vector<Action>> action_table;
vector<vector<int>> goto_table;
vector<ProdInfo> prod_info;

// .y文件用户代码
string decl_user_code;
string user_sub_code;

// 错误处理全局变量
int yylineno = 1;

// ==================== 通用工具函数实现 ====================
void yyerror(const string& msg) {
    cerr << "[语法错误] 行号:" << yylineno << "，错误信息：" << msg << endl;
}

void print_first_set() {
    cout << "\n===== First 集结果 =====" << endl;
    for (auto& pair : first_set) {
        int sym = pair.first;
        cout << symbol_name_map[sym] << " : { ";
        for (int t : pair.second) {
            cout << symbol_name_map[t] << " ";
        }
        cout << "}" << endl;
    }
}

void print_lr_dfa() {
    cout << "\n===== LR DFA 状态 =====" << endl;
    for (auto& state : lr_dfa.states) {
        cout << "\n状态 I" << state.id << ":" << endl;
        for (auto& item : state.items) {
            Production& prod = productions[item.prod_id];
            cout << "  " << symbol_name_map[prod.left] << " → ";
            for (size_t i = 0; i < prod.right.size(); i++) {
                if (i == item.dot_pos) cout << "· ";
                cout << symbol_name_map[prod.right[i]] << " ";
            }
            if ((size_t)item.dot_pos == prod.right.size()) cout << "· ";
            cout << ", " << symbol_name_map[item.lookahead] << endl;
        }
        if (!state.transitions.empty()) {
            cout << "  转移边：" << endl;
            for (auto& trans : state.transitions) {
                cout << "    " << symbol_name_map[trans.first] << " → I" << trans.second << endl;
            }
        }
    }
}

void print_parsing_table() {
    cout << "\n===== 分析表构造完成 =====" << endl;
    cout << "Action表行数（状态数）：" << action_table.size() << endl;
    cout << "Goto表行数（状态数）：" << goto_table.size() << endl;
    cout << "产生式总数：" << productions.size() << endl;
}