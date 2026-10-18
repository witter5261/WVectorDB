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
## 2026-10-17 — insert 异常安全审查

### 现有代码的失败路径

- 新 ID 的 records_.push_back 先于 positions_.emplace；向量复制或 records_ 扩容失败时，标准 vector 操作保持原状态。
- positions_.emplace 分配失败时，标准 unordered_map 插入没有效果；原 catch 会移除刚追加的 Record，因此两个容器可以恢复一致。
- 已有 ID 会先复制 replacement；复制失败时旧 Record 不变。当前 Vector 是 std::vector<float>，默认分配器下的移动赋值不会抛异常。
- 当前代码不存在“positions_.emplace 成功后再向量赋值”的执行路径。原 catch 中 inserted == false 时先 pop、随后再进入 catch 检查，逻辑重复。

### 修改

- insert 现在先把输入复制到 prepared_vector。
- 已有 ID 直接移动 prepared_vector 替换旧向量。
- 新 ID 把 prepared_vector 移入 records_，再写 positions_。
- positions_ 写入失败时只回滚本次追加的 Record，并重新抛出原异常。

### 验证

- 新增失败新 ID 后再次插入的测试，确认没有残留 ID 映射。
- 新增失败已有 ID 后再次覆盖的测试，确认已有映射仍可用。
- g++ 16.1.0、C++20、-Wall -Wextra -Wpedantic 编译通过。
- 输出：Phase 2 insert test passed。

### 局限

当前 Vector 固定为 std::vector<float>，测试无法稳定注入内存分配异常；因此本次测试验证了可控的校验失败和后续状态可用性，容器分配异常依赖标准库的异常保证和代码结构。
## 2026-10-17 — erase 与 swap-and-pop

### 设计决定

- erase 先通过 positions_ 找到被删除记录的下标；不存在的 ID 返回 false，不修改任何状态。
- 删除中间记录时，用 records_ 最后一条记录覆盖被删除位置，再把被移动 ID 的 positions_ 下标更新为新位置。
- 最后 pop_back，并删除被删除 ID 的索引。
- 记录物理顺序不稳定，但后续搜索按 distance、id 排序，因此不依赖物理顺序。

### 验证

- 测试不存在 ID 不改变 size。
- 测试中间、首项、末项删除。
- 删除中间或首项后，再次覆盖被移动的 ID，验证 positions_ 已同步更新。
- g++ 16.1.0、C++20、-Wall -Wextra -Wpedantic 编译通过。
- 输出：Phase 2 Collection lifecycle test passed。

## 2026-10-17 — Phase 3 精确 top-k 搜索

### 设计决定

- 使用平方 L2 距离 `sum((query[i] - vector[i])^2)`。平方根是单调函数，不会改变距离排序；省略它可以减少每个维度的计算开销。
- 输入元素仍为 `float`，中间累加使用 `double`，最后转换为 `float`。如果结果不能表示为有限 `float`，抛出 `std::overflow_error`，避免把不可排序的值放进结果。
- 初版对所有记录线性扫描，收集结果后按 `distance` 升序、`id` 升序完整排序，再截取前 `k` 项。这样得到最容易审查的精确基线，复杂度为 O(ND + N log N)；堆优化和 ANN 索引留到 benchmark 后决定。
- `search` 复用 Collection 的维度和有限值校验，因此非法查询在任何 `k` 值下都会被拒绝。

### 验证

- 覆盖 `k=0`、`k>N`、空 Collection、已知二维距离、重复距离的 ID 排序、错误维度和非有限查询值。
- g++ 16.1.0、C++20、`-Wall -Wextra -Wpedantic` 编译通过。
- 命令：`g++ -std=c++20 -Wall -Wextra -Wpedantic -Iinclude src/collection.cpp tests/smoke_test.cpp -o build/wvectordb_smoke.exe`。
- 输出：`Phase 3 Collection search test passed`。

### 未完成项

- 当前测试使用手工构造的预期值；TODO 中保留独立朴素参考实现比较。
- CMake 配置仍需要 Ninja 或 MinGW Make 生成器；目前使用手动 g++ 命令验证。

## 2026-10-17 — Phase 4 Benchmark 基线

### 基准程序

- 新增 `benchmark/benchmark.cpp`，使用固定随机种子 `20261017` 生成向量和查询。
- 参数矩阵为 `N ∈ {100, 1000, 10000}`、`D ∈ {8, 128}`、`k ∈ {1, 10}`；每组 20 个查询，先 warm-up 2 轮，再测量 7 轮。
- 计时只覆盖 `Collection::search` 循环；构建 Collection、数据生成和正确性校验不计入搜索耗时。
- 每组计时前用独立的朴素参考实现逐查询比较 ID、距离、数量和排序；计时过程中累积 checksum，避免结果被优化掉。
- 输出 CSV 包含 mean、median、min、max、p95（每查询纳秒）和 checksum，文件为 `benchmark/results/phase4_baseline.csv`。

### 实际环境

- 操作系统：Windows 11 家庭版，版本 10.0.26200。
- CPU：Intel Core i7-14650HX，16 核、24 线程。
- 编译器：g++ 16.1.0；C++20；`-O2 -DNDEBUG -Wall -Wextra -Wpedantic`，作为 Release-like 构建。
- CMake 4.4.4 已安装，但当前系统仍没有可用的 MinGW Make/Ninja 生成器，因此本次使用等价优化选项手动构建基准程序。

### 实际结果（median，每次查询）

| N | D | k=1 | k=10 |
|---:|---:|---:|---:|
| 100 | 8 | 2,215 ns | 2,090 ns |
| 100 | 128 | 7,040 ns | 7,795 ns |
| 1,000 | 8 | 39,945 ns | 39,400 ns |
| 1,000 | 128 | 91,850 ns | 93,825 ns |
| 10,000 | 8 | 536,195 ns | 527,765 ns |
| 10,000 | 128 | 1,045,405 ns | 1,065,680 ns |

结果显示：N 增大时耗时明显上升；D 从 8 增加到 128 时耗时约增加 2～3 倍；同一个 N、D 下 k=1 和 k=10 的耗时接近，符合“完整排序后再截取”的实现特征。当前数据足以继续讨论 partial_sort 或 nth_element，但本次不直接修改搜索算法。

### 验证

- Release-like Benchmark 运行完成，输出：`Benchmark completed: benchmark/results/phase4_baseline.csv`。
- 最终 checksum：`4150586.6471549571`。
- 工作区没有执行 Git commit；后续优化前应保留本 CSV 作为比较基线。

## 2026-10-17 — Phase 4 排序算法对比

### 实验设计

- 没有修改 `Collection::search`。Benchmark 层新增 `partial_sort` 和 `nth_element` 对比实现。
- 每组实验让当前 API、完整排序参考、`partial_sort`、`nth_element + 前 k 排序` 共享同一批向量和查询；对比数据写入 `benchmark/results/phase4_sort_comparison.csv`。
- `default_collection` 测量真实 Collection API；`full_sort_reference` 用同一批平铺向量隔离排序算法本身，避免把容器布局差异误认为排序收益。
- 每个变体先与独立参考结果逐查询比较，再进行 2 轮 warm-up 和 7 轮计时；四个变体的 checksum 一致。

### 实际结果（median，每查询）

| N | D | k | default | partial_sort | nth_element |
|---:|---:|---:|---:|---:|---:|
| 1,000 | 8 | 1 | 39,305 ns | 4,470 ns | 10,235 ns |
| 1,000 | 128 | 1 | 89,440 ns | 54,535 ns | 63,175 ns |
| 10,000 | 8 | 1 | 556,610 ns | 48,370 ns | 107,540 ns |
| 10,000 | 128 | 1 | 1,098,485 ns | 601,320 ns | 666,090 ns |
| 10,000 | 8 | 10 | 528,930 ns | 49,940 ns | 102,630 ns |
| 10,000 | 128 | 10 | 1,117,160 ns | 583,360 ns | 633,555 ns |

### 解释

- N=10,000、D=8、k=1 时，`partial_sort` 约为完整排序的 1/11，`nth_element` 约为 1/5；排序成本在总耗时中占比较高。
- D=128 时，距离计算占比上升，`partial_sort` 仍有约 1.8～1.9 倍收益，但不再达到低维度下的数量级差距。
- `partial_sort` 在本次 k=1、10 的实验中通常快于 `nth_element`，因为它直接维护并排序前 k 个结果；`nth_element` 需要一次分割再对前 k 项排序，常数开销更高。
- 当前结果支持后续评估 top-k 优化，但还不能直接把 benchmark 层实现搬进 Collection：替代实现使用同一批平铺向量，而默认 API 使用 records_；应先设计统一的内部候选收集接口，再补充回归和更大 k 的实验。

### 验证

- 编译：g++ 16.1.0、C++20、`-O2 -DNDEBUG -Wall -Wextra -Wpedantic`。
- 运行输出：`Baseline benchmark completed` 和 `Sort comparison completed`。
- baseline checksum：`4150586.6471549571`；四变体比较 checksum：`16602346.588619828`，其中四个变体各自的 checksum 一致。
- 未修改默认 `Collection::search`，未执行 Git commit。

## 2026-10-17 — 将 partial_sort 应用到默认搜索

### 决策

- Benchmark 显示 `partial_sort` 在小 k、大 N 时稳定快于完整排序；因此将默认 `Collection::search` 改为：当 `k < N` 时使用 `partial_sort`，当 `k >= N` 时保留完整 `sort`。
- 比较器仍然使用 distance 升序、ID 升序，确保优化只改变排序过程，不改变结果规则。
- `nth_element` 暂留 Benchmark 对比层，没有直接采用，因为本机实测通常慢于 `partial_sort`，并且极小数据上更容易受到计时噪声影响。

### 回归与优化后结果

- 新增 `k == N` 的测试，确认完整排序分支仍返回全部结果并保持确定性顺序。
- Phase 3 smoke test 通过：`Phase 3 Collection search test passed`。
- 优化后基准数据保存为 `benchmark/results/phase4_baseline_optimized.csv` 和 `benchmark/results/phase4_sort_comparison_optimized.csv`。
- 优化后中位数示例：`N=10000,D=8,k=1` 为 51,165 ns；`N=10000,D=128,k=1` 为 604,625 ns。相对于此前完整排序基线，分别约提升 10.9 倍和 1.8 倍。
- checksum 与优化前一致，说明返回结果没有改变。

### 当前边界

- 这次只优化了 Collection 内部的排序选择，没有改变公共 API，也没有引入 ANN、堆索引或并发。
- 是否继续支持更大的 k、增加阈值策略或改用 `nth_element`，应在更广的参数矩阵和不同机器上重新测量后决定。

## 2026-10-17 — Phase 4 k 扩展实验

### 参数扩展

- 将每个 N 的 k 扩展为 `1、10、min(100,N)、N/2、N`，并自动去重。
- 仍使用 N ∈ {100, 1,000, 10,000}、D ∈ {8, 128}、20 个查询、2 轮 warm-up、7 轮测量。
- 原始结果保存为 `benchmark/results/phase4_k_sweep_baseline.csv` 和 `benchmark/results/phase4_k_sweep_comparison.csv`。

### 代表性结果（median，每查询）

| N | D | k | default partial/full | full_sort_reference | partial_sort |
|---:|---:|---:|---:|---:|---:|
| 1,000 | 8 | 100 | 16,115 ns | 42,815 ns | 17,830 ns |
| 1,000 | 8 | 500 | 36,920 ns | 41,575 ns | 33,565 ns |
| 1,000 | 8 | 1,000 | 42,955 ns | 40,695 ns | 41,995 ns |
| 10,000 | 8 | 100 | 72,910 ns | 589,730 ns | 69,535 ns |
| 10,000 | 8 | 5,000 | 514,830 ns | 592,300 ns | 486,570 ns |
| 10,000 | 8 | 10,000 | 562,540 ns | 580,265 ns | 579,475 ns |
| 10,000 | 128 | 100 | 600,040 ns | 1,116,765 ns | 616,190 ns |
| 10,000 | 128 | 5,000 | 1,051,985 ns | 1,123,610 ns | 1,042,285 ns |
| 10,000 | 128 | 10,000 | 1,137,270 ns | 1,141,435 ns | 1,155,330 ns |

### 阈值决策

- k=N 时默认路径已经切换到完整排序；这是必要的边界，因为 `partial_sort` 在 k=N 时没有筛选收益，部分高维实验还略慢。
- k=N/2 时，partial_sort 在本次 N=10,000 实验中仍略快：D=8 约快 18%，D=128 约快 7%。暂时不增加更早的比例阈值。
- k 较大时 `partial_sort` 的优势会缩小，但没有稳定地比完整排序更慢；N=1,000 的个别配置差异接近测量噪声。
- 因此当前策略保持为 `k < N → partial_sort`、`k >= N → sort`。以后若需要进一步优化，应增加更多机器、更多查询数量和更大 k，再评估按比例切换或使用 `nth_element`。

### 重复运行

- 相同程序和参数再次运行，结果保存为 `benchmark/results/phase4_baseline_run2.csv`。
- 两次运行的 checksum 完全一致：`4150586.6471549571`，说明输入和结果稳定。
- 第二次运行中 `N=10000,D=128` 的 median 为 1,110,230 ns（k=1）和 1,134,310 ns（k=10）；第一次分别为 1,045,405 ns 和 1,065,680 ns。约 6% 的差异说明系统调度、缓存和 CPU 状态会影响绝对耗时，因此报告 median、重复次数和完整 CSV，而不依据单次结果做优化结论。

## 2026-10-17 — 进入 Phase 5：MVP 复盘与下一阶段规划

### 当前设计总结

- `Collection` 使用 `records_` 连续保存 ID 和向量，`positions_` 用 `unordered_map` 提供 ID 到物理下标的映射。
- `insert` 先校验并准备完整向量副本，再修改容器；新记录的索引写入失败时回滚追加记录，已有记录替换失败时保留旧向量。
- `erase` 使用 swap-and-pop，删除中间记录的常数级操作会改变物理顺序，因此 API 不承诺位置稳定。
- `search` 校验查询，使用平方 L2 距离、double 累加和确定性排序；当前 `k < N` 使用 `partial_sort`，`k >= N` 使用完整排序。

### 当前测试与 Benchmark 总结

- smoke test 覆盖构造、插入、覆盖、维度和有限值校验、删除、k 边界、空集合、已知距离和重复距离排序。
- Benchmark 使用独立参考实现逐查询比较 ID、距离、数量和排序，并使用固定种子、warm-up、重复测量、checksum 和 CSV。
- Benchmark 说明 N 增大带来主要的线性扫描成本；D 增大提高距离计算成本；小 k 时 partial_sort 的收益最大；k 接近 N 时收益缩小，k=N 时切回完整排序即可。
- 当前仍没有持久化、批量 API、ANN、并发或网络能力；这些保持为后续候选，不属于已完成 MVP。

### Phase 5 计划

1. 修正文档中的阶段、目录和构建入口描述，使新协作者能按 README 运行项目。
2. 解决 CMake 生成器缺失问题，完成一次 Release configure/build/CTest；在此之前保留 g++ 手动构建证据。
3. 增加一个最小使用示例，展示创建 Collection、插入向量和执行 top-k 搜索。
4. 对持久化、批量操作和 ANN 三个候选方向比较学习收益、工程复杂度、测试成本和对当前数据结构的影响。
5. 选定下一项能力后提升 Spec 版本，并写出明确的目标、非目标、接口和验收标准。

### Phase 5 暂不做的事

- 不因为 Benchmark 已经优化就直接加入 ANN 或复杂索引。
- 不把单机一次 Benchmark 结果当作通用性能承诺。
- 不在下一项能力边界确定前继续扩展公共 API。

## 2026-10-17 — Phase 5：CMake、示例与批量 API

### CMake 与示例

- 新增 `CMakePresets.json`，提供 `release` configure/build/test 入口：`cmake --preset release`、`cmake --build --preset release`、`ctest --preset release`。
- 新增 `examples/basic_search.cpp` 和 `wvectordb_basic_search` CMake 目标，示例创建二维 Collection、插入三条记录并查询 top-2。
- CMake 能识别 Release preset，但实际 configure 被环境阻塞：系统没有 Ninja、MinGW Make 或 MSBuild；CMake 4.4.4 本身可用。
- 使用 g++ 16.1.0、C++20、`-Wall -Wextra -Wpedantic` 手动编译示例和测试均通过；示例输出 `101 distance = 0`、`102 distance = 1`。

### 批量 API 设计

- 新增公开类型 `VectorEntry { VectorId id; Vector vector; }`。
- 新增 `insert_batch(entries)`：先复制当前 Collection，在副本上按顺序复用单条 `insert`，成功后交换内部容器；因此批次失败不会部分更新原对象。
- 同一批次重复 ID 遵循单条覆盖规则，后出现的条目生效；空批次不改变状态。
- 新增 `search_batch(queries, k)`：逐个复用单条 `search`，返回数组下标与查询下标对应；某个查询非法时抛出异常，Collection 不变。
- 初版先使用快照复制证明强异常安全；Benchmark 显示小批次复制整个 Collection 的成本过高，因此改为准备输入、备份受影响旧向量、预留容器并使用回滚日志。该实现保留原子语义，同时避免复制未受影响的记录。

### 验证

- 测试覆盖批量重复 ID、批量搜索顺序、失败批量插入不改变 size、非法批量查询。
- 输出：`Phase 5 Collection batch API test passed`。
- 示例和库代码均使用 Release-like/警告开启的 g++ 编译通过。

## 2026-10-17 — Phase 5 批量 API Benchmark 与优化

### 基准设计

- 新增 `benchmark/batch_benchmark.cpp` 和 `wvectordb_batch_benchmark` 目标。
- 批量插入测量 N ∈ {100, 1,000, 5,000}、D ∈ {8, 128}，批大小为 `1、10、100、N`，每组重复 3 次。
- 批量搜索测量 N ∈ {1,000, 10,000}、D ∈ {8, 128}、k ∈ {1, 10}、Q ∈ {1, 10, 100}，每组重复 7 次。
- 每组数据先用独立参考搜索校验结果，再测量逐个 `search` 循环与 `search_batch`。
- 结果保存为 `benchmark/results/phase5_batch_insert.csv` 和 `benchmark/results/phase5_batch_search.csv`。

### 初版结果与优化决定

- 初版快照实现中，N=5,000、D=8、批大小 1 的插入约为 142,208 ns/条，而单条插入约为 86 ns/条；批大小 N 时约为 77 ns/条。
- 这证明完整 Collection 快照复制是小批次的主要瓶颈，因此实施了回滚式原子更新：只复制输入和将被覆盖的旧向量，不复制未受影响记录。
- 优化后同配置批大小 1 约为 2,158 ns/条，批大小 N 约为 126 ns/条；小批次仍有原子安全成本，但已经从 O(N²D) 级别降到与受影响数据相关的成本。
- `search_batch` 与逐个 `search` 的结果 checksum 一致；Q 较大时常出现额外外层结果容器成本，例如 N=10,000、D=8、k=1、Q=100 时约为 83,491 ns/查询，而逐个循环约为 51,200 ns/查询。因此暂不优化 `search_batch`，它当前主要提供 API 便利和结果聚合，不承诺加速。

### 当前结论

- 批量插入保留原子语义，并采用回滚式实现。
- 批量搜索保持逐查询复用，后续只有在需要批量输出缓冲区或并行执行时再重新设计。
- 回归验证输出：`Phase 5 Collection batch API test passed`；基准程序的所有变体均通过独立参考结果校验。

## 2026-10-18 — Phase 6 最小持久化

### 设计决定

- 下一项能力选择最小持久化，因为当前 Collection 进程结束后数据会消失；持久化比 ANN 更直接地延伸“数据库”目标，也能复用现有 Collection 不变量。
- 文件使用 `WVDB` 魔数、版本号 1、维度、记录数和固定宽度 little-endian 数据；向量元素按 float 位模式写入，不走文本格式。
- `save` 写出当前内存快照；`load` 先读取和校验文件，再通过 `insert_batch` 构造 Collection。
- 当前不实现崩溃期间的原子替换、日志恢复、文件锁或并发写入，这些留作后续范围。

### 实现与验证

- 新增 `Collection::save(path)` 和 `Collection::load(path)`。
- round-trip 测试确认维度、数量、ID、距离和搜索结果保持一致。
- 测试错误魔数和截断文件；输出：`Phase 6 Collection persistence test passed`。
- 手动 g++ C++20、警告开启编译通过；CMake Release preset 仍等待系统安装 Ninja。
