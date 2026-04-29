# README.md
# SeuYacc — 东南大学编译原理课程设计
**LR(1) / LALR(1) 语法分析器自动生成工具**

---

## 项目简介
SeuYacc 是仿照经典 Yacc 实现的**语法分析器生成器**，基于 LR(1) / LALR(1) 算法，使用 C++ 从零开发。
支持解析 `.y` 格式文法文件，自动生成可直接编译运行的 C 语言语法分析程序 `yyparse.c`。

适用于：**东南大学编译原理专题实践 / 课程设计**

---

## 功能特性
- 解析 Yacc 风格文法文件（`%{...%}`、`%% ... %%`）
- 计算符号 **FIRST 集**
- 构造 **LR(1) 项目集规范族（DFA）**
- 支持 **LALR(1) 同心项合并优化**
- 自动生成 **Action / Goto 分析表**
- 自动解决**移进-归约、归约-归约冲突**
- 输出可独立编译的语法分析器 `yyparse.c`
- 支持调试输出：FIRST 集、DFA 状态、分析表
- 带行号的语法错误定位与提示

---

## 项目结构
```
SEU-Compiler/
└── SeuYacc/
    ├── common.h          全局宏、数据结构、类型定义
    ├── common.cpp        全局变量、工具函数、调试打印
    ├── grammar.cpp       .y 文法文件解析
    ├── first_set.cpp     FIRST 集计算（不动点迭代）
    ├── lr1_dfa.cpp       LR(1) DFA 构造（Closure / Goto）
    ├── lalr.cpp          LALR(1) 同心项合并优化
    ├── parsing_table.cpp 分析表生成 + 冲突处理
    ├── code_gen.cpp      生成 yyparse.c 语法分析器
    ├── main.cpp          主程序、命令行参数、流程调度
    ├── token.h           终结符编号定义
    └── yyparse.c         自动生成的语法分析器（示例）
```

---

## 编译与运行
### 1. 编译 SeuYacc
```bash
g++ *.cpp -o SeuYacc -std=c++11 -O2 -finput-charset=UTF-8 -fexec-charset=UTF-8

```

### 2. 基本用法（注意：Windows终端需先运行”chcp 65001“）
```bash
./SeuYacc <input.y> <output.c>
```

示例：
```bash
./SeuYacc minic.y yyparse.c
```

### 3. 开启 LALR(1) 优化
```bash
./SeuYacc --lalr minic.y yyparse.c
```

### 4. 显示调试信息
```bash
./SeuYacc --print-first --print-dfa --print-table minic.y yyparse.c
```

---

## 生成分析器使用方法
1. 使用 Flex 生成词法分析器 `lex.yy.c`
2. 编译语法分析器
```bash
gcc yyparse.c lex.yy.c -o parser
```
3. 运行测试
```bash
./parser < test.c
```

---

## 实现流程
1. 解析 `.y` 文法文件
2. 构造增广文法
3. 计算 FIRST 集
4. 构造 LR(1) DFA
5.（可选）LALR(1) 合并
6. 生成 Action/Goto 分析表
7. 解决移进-归约/归约-归约冲突
8. 输出可运行的 `yyparse.c`

---

## 课程设计亮点
- 模块化清晰，便于写报告、画架构图
- 注释完整、逻辑标准、符合教学实验要求
- 纯 C++ 实现，无第三方依赖
- 支持 MiniC 语言完整文法
- 可直接扩展语义动作、中间代码生成

---

## 作者信息
- **GitHub**：Jimmy-maker324
- **项目**：SEU-Compiler
- **学校**：东南大学
- **课程**：编译原理专题实践

---
