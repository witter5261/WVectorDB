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
- 已使用 g++ 16.1.0 和 C++20 手动构建并运行 smoke test，输出为 Phase 1 smoke test passed。
- CMake 配置尝试因系统缺少 MinGW Makefiles 所需的 make/ninja 生成器而停止；CMake 本身已可执行。
- 首次编译发现头文件与实现的 noexcept 声明不一致，已统一后重新编译通过。
- 尚未实现 insert、erase、search，也没有宣称 MVP 完成。

### 下一步

进入 Phase 2：先实现 Collection 生命周期和输入校验，再补齐对应自动化测试。
## 2026-10-17 — Phase 2 插入与数据结构第一步

### 设计决定

- Collection 使用 records_ 保存连续的 Record；每条 Record 包含 VectorId 和 Vector。
- positions_ 使用 unordered_map<VectorId, std::size_t> 保存 ID 到 records_ 下标的映射。
- 连续记录数组为后续线性搜索提供顺序访问；ID 索引让覆盖操作不需要扫描全部向量。
- insert 先校验维度和有限值，再改变 records_ 或 positions_，避免错误输入留下半更新状态。
- 重复 ID 复制新向量后覆盖旧向量，size 不增加。
- 新记录加入 records_ 后再写入 positions_；索引写入发生异常时回滚刚加入的记录。
- Collection 构造时拒绝 dimension == 0。

### 验证

- g++ 16.1.0，C++20，-Wall -Wextra -Wpedantic 编译通过。
- 测试通过：合法插入、ID 0、重复 ID 覆盖、维度错误、NaN 拒绝和失败插入不改变 size。
- 输出：Phase 2 insert test passed。
- 尚未实现 erase、swap-and-pop 和 search。

### 学习重点

这一步先验证数据结构不变量，再继续增加删除和搜索。后续删除会展示“物理位置不稳定”和“索引必须同步更新”的取舍。
