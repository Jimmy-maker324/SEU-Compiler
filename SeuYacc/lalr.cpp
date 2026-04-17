#include "common.h"

// ==================== 内部辅助数据结构与函数声明 ====================
namespace {
    // LR(1) 项目的“核心”：忽略预测符，只看产生式和点位置
    struct ItemCore {
        int prod_id;
        int dot_pos;

        // 必须重载 ==，用于 unordered_set/unordered_map
        bool operator==(const ItemCore& other) const {
            return prod_id == other.prod_id && dot_pos == other.dot_pos;
        }
    };

    // 特化 hash 函数，用于 unordered_map
    struct CoreHash {
        size_t operator()(const ItemCore& core) const {
            return hash<int>()(core.prod_id) ^ (hash<int>()(core.dot_pos) << 1);
        }
    };

    // 提取一个 LR(1) 项目的核心
    ItemCore get_item_core(const LR1Item& item);
    
    // 提取一个状态的核心集合（用于分组）
    unordered_set<ItemCore, CoreHash> get_state_core(const LR1State& state);
    
    // 比较两个状态是否具有相同的核心
    bool has_same_core(const LR1State& s1, const LR1State& s2);
}

// ==================== 模块4核心函数：合并 LALR(1) 同心项 ====================
void merge_lalr_dfa() {
    DEBUG_PRINT("开始合并 LALR(1) 同心项...");
    size_t original_state_count = lr_dfa.states.size();
    
    if (original_state_count == 0) {
        DEBUG_PRINT("LR DFA 为空，无需合并");
        return;
    }

    // ==================== 步骤 1：按核心对状态分组 ====================
    DEBUG_PRINT("步骤 1：按核心分组...");
    
    vector<vector<int>> core_groups; // 每个元素是一组同核心的状态 ID 列表
    vector<bool> processed(original_state_count, false);

    for (size_t i = 0; i < original_state_count; i++) {
        if (processed[i]) continue;

        // 新建一个核心组
        vector<int> group;
        group.push_back((int)i);
        processed[i] = true;

        // 查找所有同核心的状态
        for (size_t j = i + 1; j < original_state_count; j++) {
            if (!processed[j] && has_same_core(lr_dfa.states[i], lr_dfa.states[j])) {
                group.push_back((int)j);
                processed[j] = true;
            }
        }

        core_groups.push_back(group);
        DEBUG_PRINT("  找到核心组，包含 " << group.size() << " 个状态");
    }

    DEBUG_PRINT("共找到 " << core_groups.size() << " 个核心组");

    // ==================== 步骤 2：构建旧状态 ID -> 新状态 ID 的映射 ====================
    DEBUG_PRINT("步骤 2：构建状态映射...");
    
    vector<int> old_to_new_id(original_state_count, -1);
    vector<LR1State> new_states;

    for (size_t group_idx = 0; group_idx < core_groups.size(); group_idx++) {
        vector<int>& group = core_groups[group_idx];
        int new_id = (int)group_idx;

        // 标记旧 ID 到新 ID 的映射
        for (int old_id : group) {
            old_to_new_id[old_id] = new_id;
        }

        // ==================== 步骤 3：合并同核心组的项目（预测符合并） ====================
        LR1State new_state;
        new_state.id = new_id;

        // 【关键修复】使用 unordered_map 而不是 map，因为我们只需要哈希，不需要排序
        unordered_map<ItemCore, unordered_set<int>, CoreHash> core_to_lookaheads;

        for (int old_id : group) {
            LR1State& old_state = lr_dfa.states[old_id];
            for (const LR1Item& item : old_state.items) {
                ItemCore core = get_item_core(item);
                // 将预测符加入集合
                core_to_lookaheads[core].insert(item.lookahead);
            }
        }

        // 从 map 重建新的项目集
        for (auto& pair : core_to_lookaheads) {
            // 【修复】pair.first 是 const 的，用 const reference 接收
            const ItemCore& core = pair.first;
            unordered_set<int>& lookaheads = pair.second;

            for (int lookahead : lookaheads) {
                LR1Item new_item;
                new_item.prod_id = core.prod_id;
                new_item.dot_pos = core.dot_pos;
                new_item.lookahead = lookahead;
                new_state.items.insert(new_item);
            }
        }

        new_states.push_back(new_state);
        DEBUG_PRINT("  新状态 I" << new_id << " 合并完成，项目数: " << new_state.items.size());
    }

    // ==================== 步骤 4：重建转移边 ====================
    DEBUG_PRINT("步骤 4：重建转移边...");
    
    for (size_t group_idx = 0; group_idx < core_groups.size(); group_idx++) {
        vector<int>& group = core_groups[group_idx];
        int new_id = (int)group_idx;
        LR1State& new_state = new_states[new_id];

        // 取组内第一个旧状态的转移边作为参考（同核心组的转移边核心一定相同）
        int representative_old_id = group[0];
        LR1State& old_state = lr_dfa.states[representative_old_id];

        for (auto& trans : old_state.transitions) {
            int sym = trans.first;
            int old_target_id = trans.second;
            int new_target_id = old_to_new_id[old_target_id];

            if (new_target_id != -1) {
                new_state.transitions[sym] = new_target_id;
            }
        }
    }

    // ==================== 步骤 5：更新全局 DFA ====================
    lr_dfa.states.swap(new_states);
    lr_dfa.start_state = old_to_new_id[lr_dfa.start_state]; // 更新起始状态

    DEBUG_PRINT("LALR(1) 合并完成！");
    DEBUG_PRINT("  原始状态数: " << original_state_count);
    DEBUG_PRINT("  合并后状态数: " << lr_dfa.states.size());
    DEBUG_PRINT("  减少了: " << (original_state_count - lr_dfa.states.size()) << " 个状态");

    // 调试输出新的 DFA
    #if DEBUG
    print_lr_dfa();
    #endif
}

// ==================== 内部辅助函数实现 ====================
namespace {
    ItemCore get_item_core(const LR1Item& item) {
        ItemCore core;
        core.prod_id = item.prod_id;
        core.dot_pos = item.dot_pos;
        return core;
    }

    unordered_set<ItemCore, CoreHash> get_state_core(const LR1State& state) {
        unordered_set<ItemCore, CoreHash> cores;
        for (const LR1Item& item : state.items) {
            cores.insert(get_item_core(item));
        }
        return cores;
    }

    bool has_same_core(const LR1State& s1, const LR1State& s2) {
        // 快速检查：项目数量不同，核心一定不同
        if (s1.items.size() != s2.items.size()) {
            return false;
        }

        // 提取核心集合进行比较
        unordered_set<ItemCore, CoreHash> core1 = get_state_core(s1);
        unordered_set<ItemCore, CoreHash> core2 = get_state_core(s2);

        if (core1.size() != core2.size()) {
            return false;
        }

        // 检查 core1 中的每个核心是否都在 core2 中
        for (const ItemCore& c : core1) {
            if (core2.find(c) == core2.end()) {
                return false;
            }
        }

        return true;
    }
}