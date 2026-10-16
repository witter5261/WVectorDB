# AI Log

## 2026-10-16 — Phase 0 初始化

### 已确认

- 本地仓库为 C:/Users/22117/Desktop/WVectorDB。
- Git 分支为 main，远程为 https://github.com/witter5261/WVectorDB.git。
- 项目目标是从零实现一个用于学习 AI 工程的简化 Vector Database。
- 当前先建立 Idea → Spec → Design → Implementation → Test → Benchmark 的工作流。
- MVP 草案采用固定维度 float 向量、整数 ID、平方 L2 距离、精确线性扫描和确定性 top-k 排序。
- 持久化、ANN、并发、网络和分布式能力明确列为非目标。

## 2026-10-16 — Codex Phase 0 审查与决策

### 已决定

- 当前阶段命名为 Phase 1：可编译骨架；MVP 是目标版本，不再把 MVP 当作阶段编号。
- 工具链采用 C++20 和 CMake 3.20+。
- VectorId 固定为 std::uint64_t，ID 0 合法；k 使用 std::size_t。
- 参数错误使用 std::invalid_argument；erase 对不存在 ID 返回 false。
- Collection 使用连续记录容器加 ID 到位置的索引；删除采用 swap-and-pop。
- 插入先完整校验，失败时保持旧记录不变。
- 距离输入为 float，累加为 double，输出为 float；拒绝 NaN 和无穷值。
- Phase 3 先实现完整收集后排序，只有 benchmark 证明必要时才加入大小为 k 的堆。
- 初期测试不依赖第三方框架，使用自检可执行文件并接入 CTest。
- benchmark 使用 steady_clock、固定随机种子、CSV 和结果校验。

### 审查结论

Spec 和 Design 的核心方向一致，已具备进入 Phase 1 骨架的条件。完整插入、删除、校验和搜索仍必须等待后续阶段，不能在本阶段宣称 MVP 完成。

## 2026-10-16 — Phase 1 可编译骨架

### 已完成

- 增加 CMakeLists.txt。
- 增加公共类型：VectorId、Vector、SearchResult。
- 增加 Collection 的构造、dimension 和 size 骨架接口。
- 增加无第三方依赖的 smoke test，并注册 CTest。
- 增加 .gitignore，避免提交本地 build 输出。

### 验证

- 已安装 CMake 4.4.4 到 C:/Users/22117/Documents/Codex/tools/cmake-4.4.4/bin/cmake.exe，并加入当前用户 PATH。
- 已使用 g++ 16.1.0 和 C++20 手动构建并运行 smoke test，输出为 Phase 1 smoke test passed。\n- CMake 配置尝试因系统缺少 MinGW Makefiles 所需的 make/ninja 生成器而停止；CMake 本身已可执行。
- 首次编译发现头文件与实现的 noexcept 声明不一致，已统一后重新编译通过。\n- 尚未实现 insert、erase、search，也没有宣称 MVP 完成。

### 下一步

进入 Phase 2：先实现 Collection 生命周期和输入校验，再补齐对应自动化测试。
