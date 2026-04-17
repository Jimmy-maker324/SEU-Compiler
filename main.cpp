#include "common.h"
#include <chrono> 

using namespace std;

// 简单的 RAII 计时器类
class Timer {
public:
    Timer(const string& name) : name_(name), start_(chrono::high_resolution_clock::now()) {}
    ~Timer() {
        auto end = chrono::high_resolution_clock::now();
        auto duration = chrono::duration_cast<chrono::milliseconds>(end - start_).count();
        cout << "   [耗时] " << duration << " ms" << endl;
    }
private:
    string name_;
    chrono::high_resolution_clock::time_point start_;
};

// 打印帮助信息
void print_help(const char* program_name) {
    cout << "========================================" << endl;
    cout << "  SeuYacc - Syntax Parser Generator" << endl;
    cout << "  Southeast University Compiler Design" << endl;
    cout << "========================================" << endl;
    cout << endl;
    cout << "Usage: " << program_name << " [options] <input.y> <output.c>" << endl;
    cout << endl;
    cout << "Options:" << endl;
    cout << "  -h, --help       Show this help message" << endl;
    cout << "  -v, --verbose    Enable verbose debug output" << endl;
    cout << "  --lalr           Enable LALR(1) optimization (default: LR(1))" << endl;
    cout << "  --print-first    Print First sets" << endl;
    cout << "  --print-dfa      Print LR DFA states" << endl;
    cout << "  --print-table    Print parsing table info" << endl;
    cout << endl;
    cout << "Examples:" << endl;
    cout << "  " << program_name << " minic.y yyparse.c" << endl;
    cout << "  " << program_name << " --lalr --print-dfa minic.y yyparse.c" << endl;
}

int main(int argc, char* argv[]) {
    // ==================== 1. 初始化变量 ====================
    string input_file;
    string output_file;
    bool use_lalr = false;
    bool print_first = false;
    bool print_dfa = false;
    bool print_table = false;
    bool verbose = false;

    // ==================== 2. 解析命令行参数 ====================
    if (argc < 2) {
        print_help(argv[0]);
        return 1;
    }

    // 遍历参数
    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            print_help(argv[0]);
            return 0;
        } else if (arg == "-v" || arg == "--verbose") {
            verbose = true;
        } else if (arg == "--lalr") {
            use_lalr = true;
        } else if (arg == "--print-first") {
            print_first = true;
        } else if (arg == "--print-dfa") {
            print_dfa = true;
        } else if (arg == "--print-table") {
            print_table = true;
        } else {
            // 位置参数：输入文件和输出文件
            if (input_file.empty()) {
                input_file = arg;
            } else if (output_file.empty()) {
                output_file = arg;
            } else {
                cerr << "❌ 错误：多余的参数 '" << arg << "'" << endl;
                print_help(argv[0]);
                return 1;
            }
        }
    }

    // 检查必填参数
    if (input_file.empty() || output_file.empty()) {
        cerr << "❌ 错误：必须指定输入文件和输出文件" << endl;
        print_help(argv[0]);
        return 1;
    }

    // ==================== 3. 正式启动 SeuYacc ====================
    cout << R"(
   ____            __  __  __
  / ___| ___ _   _\ \/ / / _| __ _  ___ ___
 | |  _ / _ \ | | |\  / | |_ / _` |/ __/ _ \
 | |_| |  __/ |_| |/  \ |  _| (_| | (_|  __/
  \____|\___|\__,_/_/\_\|_|  \__,_|\___\___|
    )" << endl;
    cout << "========================================" << endl;
    cout << "  Input file: " << input_file << endl;
    cout << "  Output file: " << output_file << endl;
    cout << "  Mode: " << (use_lalr ? "LALR(1)" : "LR(1)") << endl;
    cout << "========================================" << endl;
    cout << endl;

    try {
        // ==================== 模块 1：解析 .y 文件 ====================
        {
            cout << "  [1/6] Parsing .y file..." << endl;
            Timer t("Parsing file");
            if (!parse_yacc_file(input_file)) {
                cerr << "Failed to parse or open file '" << input_file << "'" << endl;
                return 1;
            }
            cout << "   Success: Parsing completed" << endl;
            cout << "   Info: A total of " << productions.size() << " productions were recognized" << endl;
            cout << "   Info: The start symbol is " << symbol_name_map[origin_start_symbol] << endl;
        }
        cout << endl;

        // ==================== 模块 2：计算 First 集 ====================
        {
            cout << "   [2/6] Computing First sets..." << endl;
            Timer t("Computing First sets");
            compute_first_sets();
            cout << "   Success: First sets computed" << endl;
            if (print_first) {
                print_first_set();
            }
        }
        cout << endl;

        // ==================== 模块 3：构造 LR(1) DFA ====================
        {
            cout << "   [3/6] Computing LR(1) DFA..." << endl;
            Timer t("Computing LR(1) DFA");
            build_lr1_dfa();
            cout << "   Success: LR(1) DFA constructed" << endl;
            cout << "   Info: A total of " << lr_dfa.states.size() << " states were generated" << endl;
        }
        cout << endl;

        // ==================== 模块 4（可选）：LALR(1) 合并 ====================
        if (use_lalr) {
            cout << "   [4/6] Merging LALR(1) core items..." << endl;
            Timer t("Merging LALR(1)");
            merge_lalr_dfa();
            cout << "   Success: LALR(1) merging completed" << endl;
            cout << "   Info: " << lr_dfa.states.size() << " states remaining after merging" << endl;
        } else {
            cout << "   [4/6] Skipping LALR(1) optimization" << endl;
        }
        
        if (print_dfa) {
            print_lr_dfa();
        }
        cout << endl;

        // ==================== 模块 5：构造分析表 ====================
        {
            cout << "   [5/6] Computing Action/Goto parsing table..." << endl;
            Timer t("Computing parsing table");
            build_parsing_table();
            cout << "   Success: Parsing table constructed" << endl;
            if (print_table) {
                print_parsing_table();
            }
        }
        cout << endl;

        // ==================== 模块 6：生成 yyparse.c ====================
        {
            cout << "   [6/6] Computing yyparse.c..." << endl;
            Timer t("Computing yyparse.c");
            generate_yyparse_c(output_file);
            cout << "   Success: " << output_file << " has been generated" << endl;
        }
        cout << endl;

        // ==================== 结束 ====================
        cout << "========================================" << endl;
        cout << "  🎉 SeuYacc 执行完毕！" << endl;
        cout << "  输出文件: " << output_file << endl;
        cout << "========================================" << endl;

    } catch (const exception& e) {
        cerr << "\n❌ 发生致命错误：" << e.what() << endl;
        return 1;
    }

    return 0;
}