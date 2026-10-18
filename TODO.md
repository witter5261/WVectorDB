# TODO

## Phase 0：规格与设计审查

- [x] 创建项目规范、设计、协作规则、AI 日志和任务清单
- [x] 建立 src/、include/、tests/、benchmark/ 目录骨架
- [x] 写明 MVP 范围、非目标、错误边界和验收标准
- [x] 审查 SPEC.md，列出歧义并完成决策
- [x] 审查 DESIGN.md，确认模块边界、数据结构和复杂度
- [x] 决定 C++ 标准、CMake 版本和测试框架
- [x] 决定错误表达方式、删除策略和 benchmark 输出格式
- [x] 将审查结论写回 SPEC.md、DESIGN.md、AI_LOG.md

## Phase 1：可编译骨架

- [x] 增加 CMakeLists.txt
- [x] 增加公共类型和 Collection 骨架接口
- [x] 增加无第三方依赖的 smoke test 和 CTest 入口
- [x] 增加 build 输出的 .gitignore
- [x] 使用可用 C++20 编译器手动构建并运行 smoke test
- [x] 安装 CMake 3.20+（当前为 4.4.4）
- [x] 将手动构建和 smoke test 结果补充到 AI_LOG.md
- [ ] 安装 Ninja 或 MinGW Make 后运行 CMake configure/build/CTest

## Phase 2：生命周期与校验

- [x] 实现 Collection 创建和 dimension/size
- [x] 实现插入新 ID 和重复 ID 覆盖
- [x] 实现维度和有限值校验
- [x] 为插入边界行为增加测试
- [x] 实现 erase 和 swap-and-pop 索引更新
- [x] 为删除行为增加测试
- [x] 更新 README、AI_LOG 和 TODO

## Phase 3：精确搜索

- [x] 实现平方 L2 距离
- [x] 实现精确 top-k 搜索
- [x] 覆盖空集合、k 边界、重复距离和确定性排序
- [x] 与独立参考实现比较

## Phase 4：Benchmark

- [x] 固定随机种子并生成可复现实验数据
- [x] 测量不同 N、D、k 下的搜索耗时
- [x] 保存原始结果和运行环境
- [x] 比较完整排序与 top-k 优化是否值得
- [x] 将 partial_sort 应用到默认 search，并保留 k >= N 的完整排序分支
- [x] 扩大 k 参数并判断是否需要额外阈值

## Phase 5：MVP 复盘与下一步

- [x] 总结 Collection 数据结构、异常安全和搜索算法
- [x] 总结功能测试、独立参考校验和 Benchmark 结论
- [x] 修正文档中的阶段、目录和构建入口描述
- [ ] 用可用生成器完成一次 CMake Release configure/build/CTest
- [x] 增加一个最小可运行使用示例，作为 README 验收入口
- [x] 选择批量操作作为下一项能力并提升 Spec 版本到 0.2.0
- [ ] 比较持久化和 ANN 作为后续能力的学习收益与复杂度
- [x] 编写批量 API 的 Benchmark 并减少批量插入的复制成本
- [x] 根据 Benchmark 决定暂不优化 search_batch
- [ ] 编写 Phase 5 复盘和下一阶段验收标准

## Phase 6：最小持久化（当前）

- [x] 定义版本化二进制文件格式
- [x] 实现 Collection save/load
- [x] 测试保存加载 round-trip
- [x] 拒绝错误魔数、截断文件和非法文件内容
- [ ] 增加更多文件损坏和大数据量测试
- [ ] 评估临时文件与原子替换策略
