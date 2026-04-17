#include "common.h"

// ==================== 内部辅助函数声明 ====================
namespace {
    // 初始化分析表（分配内存并填充 ERROR）
    void init_parsing_tables();
    
    // 处理单个状态的 Action 表项
    void process_state_for_action(LR1State& state);
    
    // 处理单个状态的 Goto 表项
    void process_state_for_goto(LR1State& state);
    
    // 填充 prod_info 数组（供代码生成使用）
    void fill_prod_info();
}

// ==================== 模块5核心函数：构造 LR 分析表 ====================
void build_parsing_table() {
    DEBUG_PRINT("开始构造 Action/Goto 分析表...");

    // ==================== 步骤 1：初始化 ====================
    init_parsing_tables();
    fill_prod_info();

    // ==================== 步骤 2：遍历所有状态，填充表 ====================
    for (size_t i = 0; i < lr_dfa.states.size(); i++) {
        LR1State& state = lr_dfa.states[i];
        DEBUG_PRINT("处理状态 I" << state.id << "...");

        // 2.1 填充 Action 表
        process_state_for_action(state);

        // 2.2 填充 Goto 表
        process_state_for_goto(state);
    }

    DEBUG_PRINT("分析表构造完成！");
    DEBUG_PRINT("  Action 表大小: " << action_table.size() << " x " << (action_table.empty() ? 0 : action_table[0].size()));
    DEBUG_PRINT("  Goto 表大小: " << goto_table.size() << " x " << (goto_table.empty() ? 0 : goto_table[0].size()));

    // 调试输出分析表信息
    #if DEBUG
    print_parsing_table();
    #endif
}

// ==================== 冲突解决函数（课程设计核心） ====================
Action resolve_conflict(const Action& cur_action, const Action& new_action) {
    DEBUG_PRINT("⚠️  检测到冲突！正在解决...");

    // 情况 1：移进-归约冲突 (Shift-Reduce)
    if ((cur_action.type == ACTION_SHIFT && new_action.type == ACTION_REDUCE) ||
        (cur_action.type == ACTION_REDUCE && new_action.type == ACTION_SHIFT)) {
        
        Action shift_action, reduce_action;
        if (cur_action.type == ACTION_SHIFT) {
            shift_action = cur_action;
            reduce_action = new_action;
        } else {
            shift_action = new_action;
            reduce_action = cur_action;
        }

        // 获取归约产生式和移进符号
        int prod_id = reduce_action.num;
        // Production& prod = productions[prod_id];
        // int shift_sym = -1; // 这里需要根据上下文获取移进符号，简化版我们用优先级规则
        (void)prod_id; // 避免未使用警告

        // 简化版冲突解决：
        // 1. 如果产生式有优先级，比较优先级
        // 2. 否则，默认移进优先（或者按产生式顺序）
        DEBUG_PRINT("  移进-归约冲突：默认按优先级/结合性处理");
        
        // 课程设计简化：如果没有显式优先级，默认移进优先（匹配 Yacc 默认行为）
        return shift_action;
    }

    // 情况 2：归约-归约冲突 (Reduce-Reduce)
    else if (cur_action.type == ACTION_REDUCE && new_action.type == ACTION_REDUCE) {
        DEBUG_PRINT("  归约-归约冲突：选择先定义的产生式");
        // 规则：选择产生式编号较小的（先定义的优先）
        if (cur_action.num < new_action.num) {
            return cur_action;
        } else {
            return new_action;
        }
    }

    // 其他情况：返回当前动作
    return cur_action;
}

// ==================== 内部辅助函数实现 ====================
namespace {
    void init_parsing_tables() {
        int num_states = lr_dfa.states.size();
        int num_terminals = TERMINAL_MAX + 1; // 终结符 0~TERMINAL_MAX
        int num_non_terms = 0;

        // 计算非终结符数量（找到最大的非终结符编号）
        for (auto& pair : non_term_map) {
            int id = pair.second;
            if (id > num_non_terms) {
                num_non_terms = id;
            }
        }
        num_non_terms = num_non_terms - NON_TERM_BASE + 1; // 转换为索引

        // 1. 初始化 Action 表
        action_table.resize(num_states);
        for (int i = 0; i < num_states; i++) {
            action_table[i].resize(num_terminals);
            for (int j = 0; j < num_terminals; j++) {
                action_table[i][j].type = ACTION_ERROR;
                action_table[i][j].num = -1;
            }
        }

        // 2. 初始化 Goto 表
        goto_table.resize(num_states);
        for (int i = 0; i < num_states; i++) {
            goto_table[i].resize(num_non_terms, -1); // -1 表示无转移
        }

        DEBUG_PRINT("分析表初始化完成");
        DEBUG_PRINT("  状态数: " << num_states);
        DEBUG_PRINT("  终结符数: " << num_terminals);
        DEBUG_PRINT("  非终结符数: " << num_non_terms);
    }

    void process_state_for_action(LR1State& state) {
        int state_id = state.id;

        for (const LR1Item& item : state.items) {
            Production& prod = productions[item.prod_id];

            // ==================== 情况 1：点不在最后，可能是移进 ====================
            if ((size_t)item.dot_pos < prod.right.size()) {
                int X = prod.right[item.dot_pos];
                if (IS_TERMINAL(X)) {
                    // 查找转移边
                    if (state.transitions.find(X) != state.transitions.end()) {
                        int next_state = state.transitions[X];
                        
                        Action new_action;
                        new_action.type = ACTION_SHIFT;
                        new_action.num = next_state;

                        // 检查冲突
                        Action& cur_action = action_table[state_id][X];
                        if (cur_action.type != ACTION_ERROR) {
                            cur_action = resolve_conflict(cur_action, new_action);
                        } else {
                            cur_action = new_action;
                        }

                        DEBUG_PRINT("  Action[" << state_id << "][" << symbol_name_map[X] << "] = Shift " << next_state);
                    }
                }
            }

            // ==================== 情况 2：点在最后，是归约或接受 ====================
            else {
                int lookahead = item.lookahead;

                // 2.1 接受动作（0号产生式 S'→S·）
                if (item.prod_id == 0) {
                    Action new_action;
                    new_action.type = ACTION_ACCEPT;
                    new_action.num = 0;
                    
                    Action& cur_action = action_table[state_id][lookahead];
                    if (cur_action.type != ACTION_ERROR) {
                        cur_action = resolve_conflict(cur_action, new_action);
                    } else {
                        cur_action = new_action;
                    }

                    DEBUG_PRINT("  Action[" << state_id << "][" << symbol_name_map[lookahead] << "] = Accept");
                }

                // 2.2 归约动作
                else {
                    Action new_action;
                    new_action.type = ACTION_REDUCE;
                    new_action.num = item.prod_id;

                    Action& cur_action = action_table[state_id][lookahead];
                    if (cur_action.type != ACTION_ERROR) {
                        cur_action = resolve_conflict(cur_action, new_action);
                    } else {
                        cur_action = new_action;
                    }

                    DEBUG_PRINT("  Action[" << state_id << "][" << symbol_name_map[lookahead] << "] = Reduce " << item.prod_id);
                }
            }
        }
    }

    void process_state_for_goto(LR1State& state) {
        int state_id = state.id;

        for (auto& trans : state.transitions) {
            int X = trans.first;
            int next_state = trans.second;

            if (IS_NON_TERM(X)) {
                // 转换为 Goto 表的索引（减去 NON_TERM_BASE）
                int idx = X - NON_TERM_BASE;
                if (idx >= 0 && idx < (int)goto_table[state_id].size()) {
                    goto_table[state_id][idx] = next_state;
                    DEBUG_PRINT("  Goto[" << state_id << "][" << symbol_name_map[X] << "] = " << next_state);
                }
            }
        }
    }

    void fill_prod_info() {
        prod_info.resize(productions.size());
        for (size_t i = 0; i < productions.size(); i++) {
            Production& prod = productions[i];
            prod_info[i].left = prod.left;
            prod_info[i].right_len = prod.right.size();
        }
        DEBUG_PRINT("prod_info 填充完成，共 " << prod_info.size() << " 条产生式");
    }
}