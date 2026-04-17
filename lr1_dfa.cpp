#include "common.h"

// ==================== 内部辅助函数声明 ====================
namespace {
    // 比较两个项目集是否完全相同（用于状态去重）
    bool is_same_item_set(const unordered_set<LR1Item>& set1, 
                          const unordered_set<LR1Item>& set2);
    
    // 查找已存在的状态（返回状态ID，-1表示未找到）
    int find_existing_state(const unordered_set<LR1Item>& items);
    
    // 打印单个项目（调试用）
    void print_lr1_item(const LR1Item& item);
}

// ==================== 模块3核心函数：构造 LR(1) DFA ====================
void build_lr1_dfa() {
    DEBUG_PRINT("开始构造 LR(1) 自动机...");
    lr_dfa.states.clear();
    lr_dfa.start_state = -1;

    // ==================== 步骤 1：构造初始状态 I0 ====================
    DEBUG_PRINT("构造初始状态 I0...");
    
    // 1.1 初始项目：[S' → ·S, $]
    LR1Item initial_item;
    initial_item.prod_id = 0;          // 0号是增广产生式 S'→S
    initial_item.dot_pos = 0;          // 点在最左边
    initial_item.lookahead = T_EOF;    // 预测符是 $ (T_EOF)

    unordered_set<LR1Item> initial_items;
    initial_items.insert(initial_item);

    // 1.2 对初始项目集做闭包
    unordered_set<LR1Item> closure_items = closure(initial_items);

    // 1.3 创建初始状态 I0
    LR1State state0;
    state0.id = 0;
    state0.items = closure_items;
    lr_dfa.states.push_back(state0);
    lr_dfa.start_state = 0;

    DEBUG_PRINT("初始状态 I0 构造完成，项目数: " << closure_items.size());

    // ==================== 步骤 2：迭代构造所有状态（子集构造法） ====================
    queue<int> state_queue; // 待处理状态队列
    state_queue.push(0);
    int next_state_id = 1;

    while (!state_queue.empty()) {
        int current_id = state_queue.front();
        state_queue.pop();
        LR1State& current_state = lr_dfa.states[current_id];

        DEBUG_PRINT("处理状态 I" << current_id << "...");

        // 收集该状态中所有可能的转移符号 X
        unordered_set<int> symbols;
        for (const LR1Item& item : current_state.items) {
            Production& prod = productions[item.prod_id];
            // 如果点不在最后，说明有转移符号
            if (item.dot_pos < prod.right.size()) {
                int X = prod.right[item.dot_pos];
                symbols.insert(X);
            }
        }

        // 对每个转移符号 X，计算 Goto(I, X)
        for (int X : symbols) {
            DEBUG_PRINT("  计算 Goto(I" << current_id << ", " << symbol_name_map[X] << ")...");

            // 2.1 计算 Goto 转移后的项目集
            unordered_set<LR1Item> goto_items = goto_trans(current_state.items, X);
            
            if (goto_items.empty()) {
                continue;
            }

            // 2.2 对转移后的项目集做闭包
            unordered_set<LR1Item> new_items = closure(goto_items);

            // 2.3 检查是否已存在相同的状态
            int existing_id = find_existing_state(new_items);
            
            if (existing_id != -1) {
                // 状态已存在，只需记录转移边
                current_state.transitions[X] = existing_id;
                DEBUG_PRINT("  状态已存在，复用 I" << existing_id);
            } else {
                // 状态不存在，创建新状态
                LR1State new_state;
                new_state.id = next_state_id++;
                new_state.items = new_items;
                lr_dfa.states.push_back(new_state);
                
                // 记录转移边
                current_state.transitions[X] = new_state.id;
                
                // 新状态加入队列
                state_queue.push(new_state.id);
                
                DEBUG_PRINT("  创建新状态 I" << new_state.id << "，项目数: " << new_items.size());
            }
        }
    }

    DEBUG_PRINT("LR(1) DFA 构造完成，共 " << lr_dfa.states.size() << " 个状态");
    
    // 调试输出 DFA
    #if DEBUG
    print_lr_dfa();
    #endif
}

// ==================== 核心运算 1：Closure 闭包运算 ====================
unordered_set<LR1Item> closure(const unordered_set<LR1Item>& items) {
    unordered_set<LR1Item> result = items;
    queue<LR1Item> item_queue;

    // 初始化队列
    for (const LR1Item& item : items) {
        item_queue.push(item);
    }

    while (!item_queue.empty()) {
        LR1Item item = item_queue.front();
        item_queue.pop();

        Production& prod = productions[item.prod_id];

        // 检查点的位置：如果点在最后，或者点后面是终结符，跳过
        if (item.dot_pos >= prod.right.size()) {
            continue;
        }

        int B = prod.right[item.dot_pos]; // 点后面的符号 B

        // 如果 B 是非终结符，需要扩展
        if (IS_NON_TERM(B)) {
            // 1. 计算 β：点后面的剩余符号串（B 之后的部分）
            vector<int> beta;
            for (size_t i = item.dot_pos + 1; i < prod.right.size(); i++) {
                beta.push_back(prod.right[i]);
            }

            // 2. 计算 βa：beta + [当前预测符 a]
            vector<int> beta_a = beta;
            beta_a.push_back(item.lookahead);

            // 3. 计算 First(βa) 作为新的预测符集
            set<int> lookaheads = compute_first_of_string(beta_a);

            // 4. 遍历所有 B 的产生式，生成新项目
            for (size_t prod_id = 0; prod_id < productions.size(); prod_id++) {
                Production& b_prod = productions[prod_id];
                if (b_prod.left == B) {
                    // 对每个预测符 b ∈ First(βa)
                    for (int b : lookaheads) {
                        // 跳过 ε（预测符不能是 ε）
                        if (b == -1) continue;

                        LR1Item new_item;
                        new_item.prod_id = prod_id;
                        new_item.dot_pos = 0; // 点在最左边
                        new_item.lookahead = b;

                        // 如果新项目不在结果集中，加入
                        if (result.find(new_item) == result.end()) {
                            result.insert(new_item);
                            item_queue.push(new_item);
                        }
                    }
                }
            }
        }
    }

    return result;
}

// ==================== 核心运算 2：Goto 转移运算 ====================
unordered_set<LR1Item> goto_trans(const unordered_set<LR1Item>& items, int sym) {
    unordered_set<LR1Item> result;

    for (const LR1Item& item : items) {
        Production& prod = productions[item.prod_id];

        // 检查点后面的符号是否是 sym
        if (item.dot_pos < prod.right.size() && prod.right[item.dot_pos] == sym) {
            // 点右移一位
            LR1Item new_item = item;
            new_item.dot_pos++;
            result.insert(new_item);
        }
    }

    return result;
}

// ==================== 内部辅助函数实现 ====================
namespace {
    bool is_same_item_set(const unordered_set<LR1Item>& set1, 
                          const unordered_set<LR1Item>& set2) {
        if (set1.size() != set2.size()) {
            return false;
        }

        // 检查 set1 中的每个项目是否都在 set2 中
        for (const LR1Item& item : set1) {
            if (set2.find(item) == set2.end()) {
                return false;
            }
        }

        return true;
    }

    int find_existing_state(const unordered_set<LR1Item>& items) {
        for (size_t i = 0; i < lr_dfa.states.size(); i++) {
            if (is_same_item_set(items, lr_dfa.states[i].items)) {
                return i;
            }
        }
        return -1;
    }

    void print_lr1_item(const LR1Item& item) {
        Production& prod = productions[item.prod_id];
        cout << "  [" << symbol_name_map[prod.left] << " → ";
        
        for (size_t i = 0; i < prod.right.size(); i++) {
            if (i == item.dot_pos) {
                cout << "· ";
            }
            cout << symbol_name_map[prod.right[i]] << " ";
        }
        
        if (item.dot_pos == prod.right.size()) {
            cout << "· ";
        }
        
        cout << ", " << symbol_name_map[item.lookahead] << "]" << endl;
    }
}