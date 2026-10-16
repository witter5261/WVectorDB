# WVectorDB

A minimal vector database built from scratch for learning AI engineering.

当前仓库处于 **Phase 0：规格与设计审查**。此阶段先把需求、设计和协作规则写清楚，暂不实现核心 C++ 代码。

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

目录中的 .gitkeep 只是为了让空目录进入 Git；开始实现后会被实际文件替换。

## 当前状态

- GitHub：witter5261/WVectorDB
- 分支：main
- 当前阶段：Phase 0，等待 Codex 审查 SPEC.md 和 DESIGN.md
- 核心实现：尚未开始
