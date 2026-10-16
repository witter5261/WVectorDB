# AI Log

## 2026-10-16 — Phase 0 初始化

### 已确认

- 本地仓库为 C:/Users/22117/Desktop/AI-Projects/WVectorDB。
- Git 分支为 main，远程为 https://github.com/witter5261/WVectorDB.git。
- 项目目标是从零实现一个用于学习 AI 工程的简化 Vector Database。
- 当前先建立 Idea → Spec → Design → Implementation → Test → Benchmark 的工作流。
- 本次只创建规范文档和目录骨架，不写核心 C++ 实现。
- MVP 草案采用固定维度 float 向量、整数 ID、平方 L2 距离、精确线性扫描和确定性 top-k 排序。
- 持久化、ANN、并发、网络和分布式能力明确列为非目标。

### 为什么这样定

先固定一个可解释的正确性基线，才能在后续加入索引或优化时比较结果。把边界和非目标写清楚，可以防止 Codex 在需求不完整时自行扩大工程范围。

### 待 Codex 审查

- C++/CMake 最低版本；
- 错误表达方式；
- 存储和删除策略；
- 测试框架；
- benchmark 计时和输出格式。

### 本次交付证据

- 已建立 SPEC.md、DESIGN.md、AGENTS.md、TODO.md；
- 已建立 src/、include/、tests/、benchmark/；
- 尚未编译，因为当前阶段没有 C++ 实现或构建文件。
