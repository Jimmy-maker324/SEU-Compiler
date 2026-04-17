#include "common.h"
#include <cctype>

// ==================== 内部辅助函数声明 ====================
namespace {
    string trim(const string& s);
    vector<string> split(const string& s);
}

// ==================== 【全新重写】核心解析函数 ====================
bool parse_yacc_file(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Cannot open file: " << filename << endl;
        return false;
    }

    // 第一步：先把整个文件读入内存字符串，方便处理
    string buffer;
    char ch;
    while (file.get(ch)) buffer += ch;
    file.close();

    size_t pos = 0;
    int line = 1;

    // 预定义一些基础符号
    symbol_name_map[T_EOF] = "$";
    token_map["int"] = T_INT;
    token_map["void"] = T_VOID;
    symbol_name_map[T_INT] = "int";
    symbol_name_map[T_VOID] = "void";

    // ==================== 阶段 1：找 %{ ... %} ====================
    DEBUG_PRINT("Stage 1: Looking for %{ ... %}");
    size_t p1 = buffer.find("%{");
    if (p1 != string::npos) {
        size_t p2 = buffer.find("%}", p1 + 2);
        if (p2 != string::npos) {
            decl_user_code = buffer.substr(p1 + 2, p2 - p1 - 2);
            DEBUG_PRINT("Read C code OK, len=" << decl_user_code.size());
            pos = p2 + 2; // 【关键】跳到 %} 后面
        }
    }

    // ==================== 阶段 2：找第一个 %%，进入规则段 ====================
    DEBUG_PRINT("Stage 2: Looking for first %%");
    size_t p_rules_start = buffer.find("%%", pos);
    if (p_rules_start == string::npos) {
        cerr << "Error: Cannot find first %%" << endl;
        return false;
    }
    pos = p_rules_start + 2; // 跳到第一个 %% 后面

    // ==================== 阶段 3：找第二个 %%，规则段结束 ====================
    DEBUG_PRINT("Stage 3: Looking for second %%");
    size_t p_rules_end = buffer.find("%%", pos);
    if (p_rules_end == string::npos) {
        cerr << "Error: Cannot find second %%" << endl;
        return false;
    }

    // 【核心】只提取两个 %% 之间的内容作为规则！
    string rules_str = buffer.substr(pos, p_rules_end - pos);
    DEBUG_PRINT("Rules section extracted, len=" << rules_str.size());

    // ==================== 阶段 4：解析规则字符串 ====================
    DEBUG_PRINT("Stage 4: Parsing rules");
    size_t rpos = 0;
    string current_left;

    while (rpos < rules_str.size()) {
        // 跳过空白
        while (rpos < rules_str.size() && isspace(rules_str[rpos])) {
            if (rules_str[rpos] == '\n') line++;
            rpos++;
        }
        if (rpos >= rules_str.size()) break;

        // 读取左部（直到遇到 :）
        string left;
        while (rpos < rules_str.size() && rules_str[rpos] != ':') {
            if (!isspace(rules_str[rpos])) left += rules_str[rpos];
            rpos++;
        }
        rpos++; // 跳过 ':'

        if (left.empty()) left = current_left;
        else {
            current_left = left;
            if (origin_start_symbol == -1) {
                origin_start_symbol = add_non_term(left);
                DEBUG_PRINT("Set start symbol: " << left);
            }
        }

        // 读取右部（直到遇到 ; 或 |）
        string right;
        while (rpos < rules_str.size()) {
            char c = rules_str[rpos];
            if (c == ';' || c == '|') {
                // 处理这条产生式
                right = trim(right);
                if (!right.empty()) {
                    Production prod;
                    prod.left = add_non_term(left);
                    vector<string> syms = split(right);
                    for (string& s : syms) {
                        if (s.empty()) continue;
                        int id = -1;
                        if (token_map.count(s)) id = token_map[s];
                        else id = add_non_term(s);
                        if (id != -1) prod.right.push_back(id);
                    }
                    productions.push_back(prod);
                    DEBUG_PRINT("Add prod: " << left << " -> " << right);
                }
                right.clear();
                if (c == ';') { rpos++; break; }
                if (c == '|') { rpos++; } // 继续读下一个候选式
            } else {
                right += c;
                rpos++;
            }
        }
    }

    // ==================== 阶段 5：读取用户子程序（第二个 %% 之后） ====================
    user_sub_code = buffer.substr(p_rules_end + 2);
    DEBUG_PRINT("User code section read, len=" << user_sub_code.size());

    // ==================== 最后：构造增广文法 ====================
    string aug_name = "S'";
    aug_start_symbol = add_non_term(aug_name);
    Production aug_prod;
    aug_prod.left = aug_start_symbol;
    aug_prod.right.push_back(origin_start_symbol);
    productions.insert(productions.begin(), aug_prod);
    DEBUG_PRINT("Augmented grammar done, total prods=" << productions.size());

    return true;
}

// ==================== 辅助函数实现 ====================
namespace {
    string trim(const string& s) {
        if(s.empty()) return s;
        size_t st = s.find_first_not_of(" \t\n\r");
        size_t ed = s.find_last_not_of(" \t\n\r");
        return st==string::npos ? "" : s.substr(st, ed-st+1);
    }
    vector<string> split(const string& s) {
        vector<string> res;
        stringstream ss(s); string w;
        while(ss >> w) res.push_back(w);
        return res;
    }
}

// ==================== 公共函数 ====================
void add_token(const string& name, int id) {
    token_map[name] = id;
    symbol_name_map[id] = name;
}
int add_non_term(const string& name) {
    if(non_term_map.count(name)) return non_term_map[name];
    static int next_id = NON_TERM_BASE;
    int id = next_id++;
    non_term_map[name] = id;
    symbol_name_map[id] = name;
    return id;
}