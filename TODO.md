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

## Phase 1：可编译骨架（当前）

- [x] 增加 CMakeLists.txt
- [x] 增加公共类型和 Collection 骨架接口
- [x] 增加无第三方依赖的 smoke test 和 CTest 入口
- [x] 增加 build 输出的 .gitignore
- [x] 使用可用 C++20 编译器手动构建并运行 smoke test
- [x] 安装 CMake 3.20+（当前为 4.4.4）
- [x] 将手动构建和 smoke test 结果补充到 AI_LOG.md
- [ ] 安装 Ninja 或 MinGW Make 后运行 CMake configure/build/CTest

## Phase 2：生命周期与校验

- [ ] 实现创建、插入、覆盖、删除和 size
- [ ] 实现维度、有限值和 ID 校验
- [ ] 为每个边界行为增加测试
- [ ] 更新 README、AI_LOG 和 TODO

## Phase 3：精确搜索

- [ ] 实现平方 L2 距离
- [ ] 实现精确 top-k 搜索
- [ ] 覆盖空集合、k 边界、重复距离和确定性排序
- [ ] 与独立参考实现比较

## Phase 4：Benchmark

- [ ] 固定随机种子并生成可复现实验数据
- [ ] 测量不同 N、D、k 下的搜索耗时
- [ ] 保存原始结果和运行环境
- [ ] 比较完整排序与 top-k 优化是否值得

## Phase 5：复盘与下一步

- [ ] 根据测试和 benchmark 选择下一项能力
- [ ] 需要时提升 Spec 版本
- [ ] 编写 README 使用示例和项目复盘
