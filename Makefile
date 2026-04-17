# 编译器与标准
CXX = g++
CXXFLAGS = -std=c++11 -Wall -finput-charset=UTF-8 -fexec-charset=UTF-8
TARGET = SeuYacc

# 所有源码文件
SRCS = main.cpp common.cpp grammar.cpp first_set.cpp lr1_dfa.cpp lalr.cpp parsing_table.cpp code_gen.cpp
OBJS = $(SRCS:.cpp=.o)

# 主目标
all: $(TARGET)

# 链接生成可执行文件
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# 编译每个 cpp 文件
%.o: %.cpp common.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 清理编译产物
clean:
	rm -f $(OBJS) $(TARGET)
	rm -f yyparse.c