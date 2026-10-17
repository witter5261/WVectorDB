# WVectorDB Design

- 版本：0.1.0
- 对应规格：SPEC.md 0.1.0
- 状态：Phase 2 Collection 生命周期与输入校验实现中

本文描述可审查的设计和当前骨架边界。MVP 的完整数据操作仍未实现。

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
    └── smoke_test.cpp    Phase 2 Collection 生命周期自检

    benchmark/
    └── （Phase 4 使用）

最终文件名可在后续审查时调整，但模块职责应保持清晰。

## 3. 公共接口约定

目标 MVP 接口如下：

    class Collection {
    public:
        explicit Collection(std::size_t dimension);
        std::size_t dimension() const noexcept;
        std::size_t size() const noexcept;
        void insert(VectorId id, const Vector& vector);
        bool erase(VectorId id) noexcept;
        std::vector<SearchResult> search(
            const Vector& query,
            std::size_t k) const;
    };

Phase 1 提供构造、dimension 和 size 的骨架接口。
Phase 2 已加入 insert；erase 将在本阶段后续小步加入，search 留到 Phase 3。

- VectorId 是 std::uint64_t，0 合法。
- Vector 是 std::vector<float>。
- SearchResult 包含 VectorId id 和 float distance。
- 参数错误使用 std::invalid_argument。
- erase 不存在的 ID 返回 false。
- API 不承诺记录的物理位置稳定。

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
4. Phase 3 初版完整收集后排序；
5. 当 benchmark 证明有必要时，再改用大小为 k 的堆。

完整排序复杂度为 O(ND + N log N)；大小为 k 的堆可以把排序部分降为 O(N log k)。先实现更容易验证的版本，再用 benchmark 决定是否优化。

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

Phase 2 使用无第三方依赖的 Collection 测试，并通过 CTest 注册。

后续测试按风险分层：

1. 构造与校验：合法/非法 dimension、非有限值、维度不符。
2. 数据生命周期：插入、覆盖、删除、重复 ID、size。
3. 距离正确性：手工二维/三维样例，检查已知距离。
4. 搜索行为：空集合、k=0、k>N、重复距离、确定性平局。
5. 属性测试：对小数据随机生成，检查结果排序、结果不重复，并与独立的朴素参考实现比较。
6. 回归测试：每次修复 bug 添加一个最小案例。

测试必须检查 ID、距离、数量和排序等可观察结果，而不是只检查程序没有崩溃。浮点距离使用约定的相对/绝对误差。

## 9. Benchmark 设计

第一版 benchmark 使用固定随机种子，至少覆盖：

- N ∈ {100, 1_000, 10_000}；
- D ∈ {8, 128}；
- k ∈ {1, 10}；
- 一批预生成查询，报告 warm-up 后的平均耗时。

使用 std::chrono::steady_clock，只测量搜索主路径，把数据生成时间分开，并通过结果校验或 checksum 防止无效计算被优化掉。每组记录参数、编译器、构建模式、运行环境、随机种子和提交版本，建议输出 CSV。

## 10. 阶段门槛

- Phase 0：Spec、Design、AGENTS、AI_LOG、TODO 完成并通过审查。
- Phase 1：建立最小构建骨架和公共类型，不实现完整搜索。
- Phase 2：实现 Collection 生命周期和输入校验，先让测试通过。
- Phase 3：实现精确搜索和完整功能测试。
- Phase 4：加入 benchmark，比较完整排序与 top-k 优化。
- Phase 5：根据数据决定下一项能力，并更新 Spec 版本。

## 11. 工具链

- C++20。
- CMake 3.20+。
- CMake 构建目标包含静态库、smoke test 和 CTest 测试。
- 初期测试不依赖第三方框架。
- 编译器警告应保持开启；具体平台选项由 CMake 条件选择。
