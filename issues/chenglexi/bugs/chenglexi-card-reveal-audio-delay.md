# chenglexi - 卡牌刷新音效延迟

## 基本信息

- 类型：Bug
- 状态：已解决
- 策划身份：chenglexi
- 创建时间：2026-08-27
- 源分支：`merge/chenglexi/card-audio-and-rabbit-spawn`
- 源提交：`59a5cf030e4daa8290aaa002dfc566bd4f67bbfe`
- 当时的 `origin/main`：`9bf2cb663aa746c4fb96025754efddb1307519a7`
- 登记分支：无（既有 Merge 分支补登记）
- Merge 分支：`merge/chenglexi/card-audio-and-rabbit-spawn`

## 描述

- 当前行为或现象：三选一卡牌界面刷新单张卡牌时，音效开始播放前存在明显延迟。
- 目标行为：刷新音效触发后立即进入有效声音段。
- 适用场景：三选一卡牌界面刷新单张卡牌。
- 影响范围：`UI.CardReveal` 音频事件的播放入点。
- 明确排除：不修改音频资产、其他 UI 音频事件或卡牌刷新逻辑。

## 复现与验收

- 运行环境与构建类型：策划本地编辑器数据验证。
- 前置条件：使用包含 `UI_CardReveal` 音频资产的项目内容并进入三选一卡牌界面。
- 复现步骤：刷新单张卡牌，观察视觉刷新与音效有效声音段的相对时序。
- 出现频率：修改前按事件触发稳定出现。
- 验收标准：`UI.CardReveal.StartTimeSeconds` 为 `1.28`，刷新时跳过源音频前置静音，且不影响其他 UI 音效。

## 日志与证据

- 已提交日志：无。
- 原始日志文件名：无。
- 覆盖或截取时间范围：不适用。
- 关键标记与摘要：源 WAV 的前置静音检测约为 `1.28027s`；工作簿与 CSV 同步校验通过。
- 未附日志的原因：该问题为可直接听辨的音频入点数据问题，没有运行时报错或异常日志。

## 调查与交接

- 已检查的文件、字段或资产：`Design/Data/ReEchoAudioEvents.xlsx` 的 `AudioEvents` Sheet、`EventId=UI.CardReveal`、`StartTimeSeconds`；`Content/Data/audio_events.csv`。
- 已尝试操作及结果：将入点从 `0` 调整为 `1.28` 并重新生成 CSV，数据校验通过。
- 已排除方向：无需修改 C++ 触发点和音频资产。
- 建议程序检查方向：若实机仍有延迟，检查音频解码/加载延迟及 `PostUiEvent(UiCardReveal)` 的调用时序。
- 依赖与跨团队影响：音频表现；不改变玩法逻辑。

## 人工确认

- 策划对描述的确认：已确认。
- 策划对验收结果的确认：已确认提交 `59a5cf03` 并交由项目秘书集成。
- 产品取舍确认：无。

## 解决记录

- 最终 Merge 提交：`59a5cf030e4daa8290aaa002dfc566bd4f67bbfe`
- 最终 `origin/main` 集成提交：`8caeb42b2c25ae92abc3c4dba915c76b01c8df88`
- 结果与残余事项：已与 `284af2f5` 主线组合；无已知残余事项。
