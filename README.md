# WVectorDB

A minimal vector database built from scratch for learning AI engineering.

当前仓库处于 **Phase 1：可编译骨架**。Phase 0 的规格审查已完成，当前只验证 C++ 类型、CMake 目标和 smoke test，不实现完整的向量插入、删除或搜索。

## 项目路线

Idea → Spec → Design → Implementation → Test → Benchmark → Iteration

- SPEC.md：定义 MVP 要解决的问题、接口行为和验收标准。
- DESIGN.md：说明拟采用的数据结构、模块边界和算法。
- AGENTS.md：给 Codex 的协作规则和阶段门槛。
- AI_LOG.md：记录人和 AI 的关键决策、疑问与实验。
- TODO.md：按阶段维护可执行任务。

## 目录

    include/     公共头文件
    src/         实现代码
    tests/       功能与回归测试
    benchmark/   基准程序、数据和结果
    build/       本地构建输出，不提交到 Git

## 构建与测试

在安装 CMake 3.20 或更高版本后：

    cmake -S . -B build
    cmake --build build
    ctest --test-dir build --output-on-failure

当前已安装 CMake 4.4.4。项目配置还需要 Ninja 或 MinGW Make 生成器；在生成器就绪前，可使用可用的 C++20 编译器手动编译 Phase 1 smoke test。

## 当前状态

- GitHub：witter5261/WVectorDB
- 分支：main
- 当前阶段：Phase 1，可编译骨架
- 核心 MVP 实现：尚未开始
