# WVectorDB Specification

- 版本：0.1.0
- 状态：已通过 Phase 0 审查，正在进行 Phase 1 可编译骨架
- 项目类型：用于学习 AI 工程的最小向量数据库
- MVP 目标：精确最近邻检索基线

## 1. 目标

WVectorDB 提供一个可读、可测试、可基准测试的本地向量检索基线。用户可以：

1. 创建一个固定维度的向量集合；
2. 插入、覆盖、删除带整数 ID 的向量；
3. 用查询向量执行精确的 top-k 最近邻搜索；
4. 从结果中得到向量 ID 和距离；
5. 通过测试和 benchmark 理解数据结构、距离计算、排序和复杂度。

项目首先服务于学习和实验，不以生产级吞吐量或完整数据库功能为目标。

当前工作阶段是 Phase 1：只建立可编译骨架和公共类型，不实现 MVP 的完整生命周期或搜索功能。

## 2. MVP 用户场景

    创建 Collection(dimension)
      → 插入 N 个同维向量
      → 给出 query vector 和 k
      → 线性扫描全部向量
      → 计算平方 L2 距离
      → 返回最相近的 k 个结果

首个完整 MVP 示例必须能展示：插入 3 个二维向量，查询一个二维向量，并按距离从小到大输出结果。

## 3. MVP 范围内功能

### 3.1 Collection

- 创建时指定 dimension，且 dimension 必须大于 0。
- 一个 Collection 内所有向量维度必须相同。
- 使用 uint64_t 作为用户提供的唯一 VectorId；ID 0 也合法。
- insert(id, vector)：新 ID 插入；已存在 ID 覆盖原向量。
- 插入前先完成全部校验；失败时保留原有记录。
- erase(id)：删除存在的向量并返回 true；删除不存在的 ID 返回 false。
- size()：返回当前向量数量。
- dimension()：返回固定维度。
- MVP 不提供 Database 对象、集合命名注册或多个 Collection 的持久化管理。

### 3.2 Exact top-k search

- search(query, k) 对当前全部记录进行精确线性扫描。
- k 使用 std::size_t，因此不会出现负数；k == 0 返回空结果。
- k 大于现有记录数时返回全部记录。
- 空 Collection 搜索返回空结果。
- 查询向量维度错误必须被拒绝。
- 返回结果至少包含 VectorId id 和 float distance。
- 结果按 distance 升序排列；距离相同按 id 升序排列，以保证可复现。

MVP 距离为平方欧氏距离：

    d(a,b) = Σ (a[i] - b[i])²

输入向量使用 float，距离累加使用 double；输出距离为 float。排序只依赖距离，因此不需要开平方。

### 3.3 输入约束与错误行为

以下情况必须在 API 边界被检测：

- 创建时 dimension 小于或等于 0；
- 插入向量或查询向量维度不匹配；
- 向量包含 NaN 或无穷大；
- 其他违反接口前置条件的参数。

MVP 采用以下错误约定：

- 参数错误抛出 std::invalid_argument；
- erase 对不存在的 ID 返回 false；
- 不吞掉内存分配等系统异常；
- 测试检查错误类别和状态，不依赖完整错误文字。

## 4. 非目标

MVP 不实现：

- 持久化、文件格式、崩溃恢复；
- HNSW、IVF、PQ 等 ANN 索引；
- 多线程并发安全；
- 网络服务、RPC、鉴权；
- 分布式分片和复制；
- 元数据过滤、范围查询；
- GPU、SIMD 或平台专用优化；
- cosine、inner-product 等其他距离；
- 数据库级 Collection 注册和命名管理。

这些内容可以在 MVP 有测试和 benchmark 后单独立项。

## 5. MVP 可验收标准

### 功能验收

- 能创建合法 Collection，并拒绝非法 dimension。
- 能插入新 ID、覆盖旧 ID、删除 ID，并正确报告 size。
- 能拒绝维度不符和非有限值；失败插入不改变旧记录。
- 对手工构造的小数据集，search 的 ID、距离和排序完全正确。
- 能处理 k=0、k>N、空 Collection、重复距离。
- 结果排序满足距离升序、ID 升序。

### 工程验收

- 公共接口位于 include/，实现位于 src/，测试位于 tests/。
- 有一条可重复的构建和测试命令。
- 关键行为有自动化测试，而不是只靠手工输出。
- benchmark 至少记录不同数据规模和维度下的搜索耗时，并说明机器、编译模式和参数。
- README 能让新读者在不读实现的情况下理解当前状态和运行入口。

## 6. 可观测指标

MVP 的 benchmark 记录：

- 数据量 N；
- 向量维度 D；
- 查询数量 Q；
- k；
- 总耗时、平均单查询耗时和查询吞吐；
- 编译器、构建模式、运行环境、随机种子和提交版本。

由于 MVP 是精确搜索，暂不使用 ANN recall 作为指标；后续加入近似索引时再定义与精确结果的 recall@k。

## 7. 工具链与阶段门槛

- C++ 标准：C++20。
- 构建系统：CMake 3.20 或更高版本。
- Phase 1：建立 CMake 骨架、公共类型、空 Collection 接口和 smoke test，不实现完整数据操作。
- Phase 2：实现 Collection 生命周期和输入校验。
- Phase 3：实现平方 L2 距离和精确 top-k 搜索。
- Phase 4：加入 benchmark，比较完整排序与 top-k 优化。
- Phase 5：复盘并决定下一项能力。

测试初期不依赖第三方框架，使用自检可执行文件并接入 CTest；当项目规模证明有必要时再评估 GoogleTest 等框架。

进入下一阶段前必须在 AI_LOG.md 写明变更、依据、测试证据和未解决风险。
