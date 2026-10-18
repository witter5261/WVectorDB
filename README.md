# WVectorDB

A minimal vector database built from scratch for learning AI engineering.

当前仓库处于 **Phase 6：最小持久化**。Collection 生命周期、精确 top-k 搜索、批量 API 和 Benchmark 已完成，正在加入可测试的版本化文件保存与加载。

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

Release 构建推荐使用 CMake Preset（需要 Ninja）：

    cmake --preset release
    cmake --build --preset release
    ctest --preset release

当前已安装 CMake 4.4.4，但系统尚未安装 Ninja；在生成器就绪前，可使用 g++ 16.1.0 和 `-O2 -DNDEBUG` 手动验证 Release-like 构建。

最小示例目标为 `wvectordb_basic_search`，源文件位于 `examples/basic_search.cpp`。它创建二维 Collection、插入三条记录并查询 top-2。

## 当前状态

- GitHub：witter5261/WVectorDB
- 分支：main
- 当前阶段：Phase 6，最小持久化
- 基准程序：`benchmark/benchmark.cpp`
- 最近结果：`benchmark/results/phase4_baseline.csv`（Phase 5 批量实验：`phase5_batch_insert.csv` 和 `phase5_batch_search.csv`）
- 核心 MVP 实现：Collection 生命周期、精确 top-k 和 partial_sort 搜索已完成

## Phase 5 当前结果

- 已加入 `insert_batch`：批量操作原子提交，失败时原 Collection 不变。
- 已加入 `search_batch`：结果顺序与查询顺序对应，复用单条搜索校验规则。
- 批量 Benchmark 表明快照复制是小批次插入的瓶颈，当前已改为只备份受影响记录的回滚式实现。
- `search_batch` 当前主要提供批量 API 便利，Benchmark 没有显示稳定加速。
- `save/load` 使用带魔数和版本号的 little-endian 二进制格式；当前不承诺崩溃期间的原子替换。
- CMake Preset 已加入 Release 配置；实际构建仍等待 Ninja。

## 当前设计结论

- `records_` 保存连续记录，`positions_` 保存 ID 到下标的映射。
- 删除使用 swap-and-pop，因此记录的物理位置不稳定；搜索结果依靠 distance 和 ID 排序保证确定性。
- 距离使用平方 L2，输入为 float，使用 double 累加；`k < N` 使用 `partial_sort`，`k >= N` 使用完整排序。
- 功能测试和 Benchmark 都使用独立结果校验；Benchmark 数据保存在 `benchmark/results/`。

## Phase 5 计划

先完成 MVP 复盘、CMake Release 构建入口和最小使用示例，再从持久化、批量操作和 ANN 三个方向中选择下一项能力。选择会写回 SPEC.md，并附带明确的非目标和验收标准。
