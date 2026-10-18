# WVectorDB Design

- 版本：0.3.0
- 对应规格：SPEC.md 0.3.0
- 状态：Phase 6 最小持久化实现中

本文描述可审查的设计和当前骨架边界。MVP 的 Collection 生命周期和精确搜索已经实现，Phase 5 负责复盘交付质量并决定下一项能力。

## 1. 设计原则

1. 先保证行为可解释、结果可复现，再考虑速度。
2. 公共 API 小而稳定，内部结构允许后续替换。
3. 精确线性扫描作为正确性基线；任何未来索引都必须能与它比较。
4. 让维度、ID、距离和排序规则在类型与测试中清楚可见。
5. 不把持久化、并发和网络问题提前混入 MVP。

## 2. 建议模块

    include/wvectordb/
    ├── collection.hpp   Collection 公共 API
    └── types.hpp        VectorId、Vector、SearchResult 等类型

    src/
    └── collection.cpp   Collection 生命周期实现

    tests/
    └── smoke_test.cpp    Collection 生命周期、搜索与批量 API 自检

    benchmark/
    ├── benchmark.cpp     Phase 4 精确搜索基准程序
    └── results/           基准 CSV 结果

最终文件名可在后续审查时调整，但模块职责应保持清晰。

## 3. 公共接口约定

目标 MVP 接口如下：

    class Collection {
    public:
        explicit Collection(std::size_t dimension);
        std::size_t dimension() const noexcept;
        std::size_t size() const noexcept;
        void insert(VectorId id, const Vector& vector);
        void insert_batch(const std::vector<VectorEntry>& entries);
        bool erase(VectorId id) noexcept;
        std::vector<SearchResult> search(
            const Vector& query,
            std::size_t k) const;
        std::vector<std::vector<SearchResult>> search_batch(
            const std::vector<Vector>& queries,
            std::size_t k) const;
        void save(const std::filesystem::path& path) const;
        static Collection load(const std::filesystem::path& path);
    };

Phase 1 提供构造、dimension 和 size 的骨架接口。
Phase 2 已加入 insert 和 erase；Phase 3 加入 search。

- VectorId 是 std::uint64_t，0 合法。
- Vector 是 std::vector<float>。
- SearchResult 包含 VectorId id 和 float distance。
- 参数错误使用 std::invalid_argument。
- erase 不存在的 ID 返回 false。
- API 不承诺记录的物理位置稳定。

批量操作约定：

- `insert_batch` 先复制并校验所有输入，只备份将被覆盖的旧向量；容器预留完成后再修改，失败时删除本批次新增 ID、恢复备份向量，因此不会部分更新原对象。
- 同一批次中的重复 ID 遵循单条覆盖规则，后出现的条目生效。
- 该 MVP 设计以强异常安全和行为一致为优先，批量插入的复制成本集中在输入向量和受影响旧记录；完整 Collection 不再整体复制。
- `search_batch` 逐个复用单条 `search`，返回结果下标与查询下标对应；查询对象是 const，因此失败不会改变 Collection。

## 12. Phase 6 持久化格式

持久化采用最小版本化二进制格式，字段按 little-endian 写入：

```text
4 bytes  magic = WVDB
u32      format version = 1
u64      dimension
u64      record count
repeat record count:
    u64  VectorId
    float[dimension] vector values
```

`save` 只负责写出当前快照，不承诺崩溃期间的原子替换。`load` 在构造 Collection 前读取并检查魔数、版本、维度、数量和向量值；文件中的重复 ID、截断数据和非法值都会被拒绝。浮点数通过固定宽度位模式写入，避免依赖文本格式的舍入差异。

## 4. 数据模型与删除策略

建议内部采用：

- 一个连续记录容器保存 ID 和向量数据，便于顺序扫描；
- 一个 ID 到记录位置的 unordered_map 索引，便于覆盖和删除；
- 覆盖操作先验证新向量，再替换旧向量；
- 删除采用 swap-and-pop，并更新被移动记录的索引；
- 搜索结果不依赖记录的物理位置，而依赖 distance、id 排序。

这种结构让 MVP 的顺序扫描简单可验证，同时避免 tombstone 长期堆积。

## 5. 搜索算法

对每条记录：

1. 校验查询向量已经满足 Collection 维度；
2. 以 double 累加平方差；
3. 将 distance 和 id 作为排序键；
4. 当 `k < N` 时使用 `partial_sort`，只保证前 k 个结果有序；当 `k >= N` 时使用完整排序。
5. `nth_element` 只作为 Benchmark 对比项，暂不作为默认实现。

完整排序复杂度为 O(ND + N log N)；`partial_sort` 的排序部分通常接近 O(N log k)。距离计算仍然是 O(ND)，所以 D 较大时优化排序的收益会下降。后续是否引入堆或其他结构，必须由新的 benchmark 数据决定。

## 6. 数值与边界

- 输入向量元素使用 float，累加使用 double。
- 输入必须是有限值；NaN 会破坏比较排序，必须在边界拒绝。
- 若距离计算结果无法表示为有限 float，必须在 API 层报告错误，而不是返回不可排序的值。
- 平方 L2 不开平方，因此排序结果与欧氏距离一致。
- 两个距离相等时按 ID 打破平局，避免依赖容器迭代顺序。
- 零维 Collection、空查询、超过 k 的请求由输入校验和测试覆盖。

## 7. 错误与资源

- 参数错误使用 std::invalid_argument。
- 不存在的删除使用布尔返回值。
- 内存分配等系统异常不吞掉。
- Collection 拥有自己的向量副本；调用方可以在 insert 返回后修改原 Vector。
- insert 先准备完整 Vector 副本，再修改 records_ 或 positions_；复制失败不会修改 Collection。
- 插入失败不修改已有记录。
- MVP 不提供线程安全保证。

## 8. 测试设计

Collection 功能测试使用无第三方依赖的自检程序，并通过 CTest 注册。
Phase 4 使用独立参考实现校验结果，再对 Release-like 搜索循环计时；结果写入 benchmark/results/。

后续测试按风险分层：

1. 构造与校验：合法/非法 dimension、非有限值、维度不符。
2. 数据生命周期：插入、覆盖、删除、重复 ID、size。
3. 距离正确性：手工二维/三维样例，检查已知距离。
4. 搜索行为：空集合、k=0、k>N、重复距离、确定性平局。
5. 批量行为：批量插入覆盖、失败原子性、空批次和批量查询顺序。
6. 属性测试：对小数据随机生成，检查结果排序、结果不重复，并与独立的朴素参考实现比较。
7. 回归测试：每次修复 bug 添加一个最小案例。

测试必须检查 ID、距离、数量和排序等可观察结果，而不是只检查程序没有崩溃。浮点距离使用约定的相对/绝对误差。

## 9. Benchmark 设计

Benchmark 使用固定随机种子，覆盖：

- N ∈ {100, 1_000, 10_000}；
- D ∈ {8, 128}；
- k ∈ {1, 10, min(100,N), N/2, N}；
- 一批预生成查询，报告 warm-up 后的平均耗时。

使用 std::chrono::steady_clock，只测量搜索主路径，把数据生成时间分开，并通过结果校验或 checksum 防止无效计算被优化掉。每组记录参数、编译器、构建模式、运行环境、随机种子和提交版本，建议输出 CSV。

Phase 4 的排序对比在同一批向量和查询上测量四个变体：当前 `Collection::search`、独立的完整排序参考、`partial_sort`，以及 `nth_element` 后对前 k 项排序。k 覆盖小值、中间值、N/2 和 N，用于判断大 k 时是否需要阈值。后两个变体只存在于 benchmark 层，不改变默认 API；每个变体先与独立完整排序结果比较，再进入计时。

## 10. 阶段门槛

- Phase 0：Spec、Design、AGENTS、AI_LOG、TODO 完成并通过审查。
- Phase 1：建立最小构建骨架和公共类型，不实现完整搜索。
- Phase 2：实现 Collection 生命周期和输入校验，先让测试通过。
- Phase 3：实现精确搜索和完整功能测试。
- Phase 4：加入 benchmark，比较完整排序与 top-k 优化。
- Phase 5：根据数据决定下一项能力，并更新 Spec 版本。

Phase 5 先完成 MVP 复盘：确认公共 API、异常边界、测试证据、Benchmark 结果、README 和构建入口一致；再从持久化、批量操作或 ANN 等候选能力中选择下一项，并为选择写明目标、非目标和验收标准。

## 11. 工具链

- C++20。
- CMake 3.20+。
- CMake 构建目标包含静态库、smoke test 和 CTest 测试。
- 初期测试不依赖第三方框架。
- 编译器警告应保持开启；具体平台选项由 CMake 条件选择。
