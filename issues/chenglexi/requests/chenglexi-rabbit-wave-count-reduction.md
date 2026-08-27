# chenglexi - 战斗 3 至 8 兔子投放调整

## 基本信息

- 类型：需求
- 状态：已解决
- 策划身份：chenglexi
- 创建时间：2026-08-27
- 源分支：`merge/chenglexi/card-audio-and-rabbit-spawn`
- 源提交：`59a5cf030e4daa8290aaa002dfc566bd4f67bbfe`
- 当时的 `origin/main`：`9bf2cb663aa746c4fb96025754efddb1307519a7`
- 登记分支：无（既有 Merge 分支补登记）
- Merge 分支：`merge/chenglexi/card-audio-and-rabbit-spawn`

## 描述

- 当前行为或现象：战斗 3 至 8 的每一波兔子投放量比目标值多 7 只。
- 目标行为：战斗 3 至 8 每一波的兔子投放分别减少 7 只。
- 适用场景：新局进入 Encounter 3 至 Encounter 8 的全部三波战斗。
- 影响范围：18 行 Encounter Wave 的 `RangedCount`。
- 明确排除：不修改战斗 1 至 2、近战怪、精英、Boss、波次时间或生成逻辑。

## 复现与验收

- 运行环境与构建类型：策划本地编辑器数据验证。
- 前置条件：从新局加载最新 Encounter 数据。
- 复现步骤：进入战斗 3 至 8，逐波核对兔子投放量。
- 出现频率：数据加载后每局稳定生效。
- 验收标准：Encounter 3 至 8 共 18 个 `RangedCount` 均在原值基础上减少 7，且所有结果非负；其他怪物和波次字段不变。

## 日志与证据

- 已提交日志：无。
- 原始日志文件名：无。
- 覆盖或截取时间范围：不适用。
- 关键标记与摘要：18 个目标值逐项对比为原值减 7，工作簿与 CSV 同步校验通过。
- 未附日志的原因：该需求是确定性的配表调整，没有运行时报错或异常日志。

## 调查与交接

- 已检查的文件、字段或资产：`Design/Data/ReEchoEncounterData.xlsx` 的 `EncounterWaves` Sheet、Encounter 3 至 8 的 `RangedCount`；`Content/Data/encounter_waves.csv`。
- 已尝试操作及结果：18 个目标单元格分别减 7 并重新生成 CSV，数据校验通过且无负值。
- 已排除方向：无需修改怪物生成代码、Schema 或其他 Encounter 字段。
- 建议程序检查方向：若新局计数与表格不符，检查运行时是否加载了当前 `encounter_waves.csv` 及是否存在额外生成来源。
- 依赖与跨团队影响：关卡数值平衡；不改变生成机制。

## 人工确认

- 策划对描述的确认：已确认“每一波分别减少 7”，不是总投放量减少 7。
- 策划对验收结果的确认：已确认提交 `59a5cf03` 并交由项目秘书集成。
- 产品取舍确认：上述数值取舍已由策划确认。

## 解决记录

- 最终 Merge 提交：`59a5cf030e4daa8290aaa002dfc566bd4f67bbfe`
- 最终 `origin/main` 集成提交：`8caeb42b2c25ae92abc3c4dba915c76b01c8df88`
- 结果与残余事项：已与 `284af2f5` 主线组合；无已知残余事项。
