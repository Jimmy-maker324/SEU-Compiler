#include "common.h"

// ==================== 内部宏定义 ====================
#define EPSILON -1  // 空串 ε 的特殊编号（不在终结符/非终结符范围内）

// ==================== 内部辅助函数声明 ====================
namespace {
    // 检查一个符号的 First 集是否包含 ε
    bool has_epsilon(int sym);
    
    // 打印单个 First 集（调试用）
    void print_first_set_of(int sym, const set<int>& fs);
}

// ==================== 模块2核心函数：计算所有符号的 First 集 ====================
void compute_first_sets() {
    DEBUG_PRINT("开始计算 First 集...");

    // ==================== 步骤 1：初始化 ====================
    first_set.clear();
    
    // 1.1 终结符的 First 集就是它自己
    for (auto& pair : token_map) {
        int term = pair.second;
        first_set[term].insert(term);
        DEBUG_PRINT("终结符 " << symbol_name_map[term] << " 的 First 集初始化为 { " << symbol_name_map[term] << " }");
    }
    
    // 1.2 非终结符的 First 集初始化为空集
    for (auto& pair : non_term_map) {
        int non_term = pair.second;
        first_set[non_term] = set<int>(); // 初始为空
        DEBUG_PRINT("非终结符 " << symbol_name_map[non_term] << " 的 First 集初始化为空");
    }

    // ==================== 步骤 2：不动点迭代 ====================
    bool changed = true;
    int iteration = 0;
    
    while (changed) {
        changed = false;
        iteration++;
        DEBUG_PRINT("第 " << iteration << " 轮迭代...");

        // 遍历所有产生式
        for (size_t prod_id = 0; prod_id < productions.size(); prod_id++) {
            Production& prod = productions[prod_id];
            int A = prod.left; // 产生式左部 A
            vector<int>& rhs = prod.right; // 产生式右部 X1 X2 ... Xn

            // 跳过增广产生式的打印（太啰嗦）
            if (prod_id != 0) {
                DEBUG_PRINT("  处理产生式: " << symbol_name_map[A] << " → ...");
            }

            // ==================== 核心逻辑：处理右部 ====================
            bool all_have_epsilon = true; // 标记是否所有 Xi 都能推导出 ε
            
            for (size_t i = 0; i < rhs.size(); i++) {
                int Xi = rhs[i];
                
                // 2.1 将 First(Xi) 中除了 ε 的所有符号加入 First(A)
                set<int>& first_Xi = first_set[Xi];
                for (int t : first_Xi) {
                    if (t != EPSILON) {
                        // 检查是否已经存在，避免重复添加
                        if (first_set[A].find(t) == first_set[A].end()) {
                            first_set[A].insert(t);
                            changed = true;
                            DEBUG_PRINT("    新增符号到 First(" << symbol_name_map[A] << "): " << symbol_name_map[t]);
                        }
                    }
                }

                // 2.2 检查 First(Xi) 是否包含 ε
                if (!has_epsilon(Xi)) {
                    all_have_epsilon = false;
                    break; // 不包含 ε，停止处理后续符号
                }
            }

            // 2.3 如果右部所有符号都能推导出 ε，则将 ε 加入 First(A)
            if (all_have_epsilon) {
                if (first_set[A].find(EPSILON) == first_set[A].end()) {
                    first_set[A].insert(EPSILON);
                    changed = true;
                    DEBUG_PRINT("    新增 ε 到 First(" << symbol_name_map[A] << ")");
                }
            }
        }
    }

    DEBUG_PRINT("First 集计算完成，共迭代 " << iteration << " 轮");
    
    // 调试输出完整 First 集
    #if DEBUG
    print_first_set();
    #endif
}

// ==================== 模块2辅助函数：计算一个符号串的 First 集 ====================
set<int> compute_first_of_string(const vector<int>& syms) {
    set<int> result;
    
    if (syms.empty()) {
        // 空串的 First 集是 {ε}
        result.insert(EPSILON);
        return result;
    }

    bool all_have_epsilon = true;
    
    for (size_t i = 0; i < syms.size(); i++) {
        int sym = syms[i];
        set<int>& first_sym = first_set[sym];

        // 1. 将 First(sym) 中除了 ε 的符号加入结果
        for (int t : first_sym) {
            if (t != EPSILON) {
                result.insert(t);
            }
        }

        // 2. 检查是否包含 ε
        if (!has_epsilon(sym)) {
            all_have_epsilon = false;
            break;
        }
    }

    // 3. 如果所有符号都包含 ε，结果中加入 ε
    if (all_have_epsilon) {
        result.insert(EPSILON);
    }

    return result;
}

// ==================== 内部辅助函数实现 ====================
namespace {
    bool has_epsilon(int sym) {
        // 查找 first_set[sym] 中是否包含 EPSILON
        if (first_set.find(sym) == first_set.end()) {
            return false;
        }
        return first_set[sym].count(EPSILON) > 0;
    }

    void print_first_set_of(int sym, const set<int>& fs) {
        cout << "  First(" << symbol_name_map[sym] << ") = { ";
        bool first = true;
        for (int t : fs) {
            if (!first) cout << ", ";
            first = false;
            if (t == EPSILON) {
                cout << "ε";
            } else {
                cout << symbol_name_map[t];
            }
        }
        cout << " }" << endl;
    }
}