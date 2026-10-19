# RTC 闹钟心跳与低功耗分析总结

> 平台：STM32F103C8T6（HAL 库，LSE 32.768kHz）
> 主题：RTC 闹钟作为唯一 1 秒心跳，兼容 STOP 低功耗唤醒，核定走时精度与运行期耗时分布
> 目标：无调试器、仅靠 OLED 显示与硬件引脚完成根因定位与验证

---

## 一、原始问题

主循环进入 STOP 模式后（距上次按键 >5 秒），RTC 心跳由 1 秒变成约 2 秒一次：
运行期正常走字，STOP 期计数减慢。要求找到根因并修复。

---

## 二、排查结论（按时间线）

> 本轮最大教训：**不能靠估算填补差值**。用户坚持实测交叉验证，最终以 `HAL_GetTick()` 快照拆分实测收尾。

结论：**37.4ms 的运行期里，37ms（≈97%）都是 OLED 显示（27ms I2C 传屏 + 10ms 帧绘制）**；万年历 ~0ms、STOP 唤醒 PLL 恢复 1ms，均可忽略。早期判断"万年历<1ms、STOP<1ms"的方向成立，但**数值必须实测确认**，不能口头拍板。

---

## 三、最终稳定架构（核心工作流）

### 1. 心跳由「闹钟回调」唯一驱动

```c
void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc)
{
  rtc_total_seconds++;              // 累计秒数（volatile，主循环差值驱动日历）
  next_alarm_target += 1U;          // 下一次目标软件自增
  RTC_WriteAlarm(hrtc, next_alarm_target);  // 直接写 ALR
}
```

- **只在回调（ISR）里操纵 RTC**，主循环完全不读 CNT、不重装 ALR。
- **核心不变量：全代码只在一处读真实 CNT**（启动时 `RTC_Alarm_Config` 设初始基准），其余全部用软件变量 `next_alarm_target` 推进，天然免疫 STOP 唤醒后影子寄存器锁存旧值的问题。

### 2. 最小写入函数（F1 写保护时序不可省）

```c
static void RTC_WriteAlarm(RTC_HandleTypeDef *hrtc, uint32_t target)
{
  RTC_TypeDef *rtc = hrtc->Instance;
  while ((rtc->CRL & RTC_CRL_RTOFF) == 0U) {}   // 等上次写完成
  rtc->CRL |= RTC_CRL_CNF;                       // 配置模式：可写（关键）
  rtc->ALRH = (uint16_t)(target >> 16U);
  rtc->ALRL = (uint16_t)(target & 0xFFFFU);
  rtc->CRL &= ~(uint32_t)RTC_CRL_CNF;            // 退出配置模式
  while ((rtc->CRL & RTC_CRL_RTOFF) == 0U) {}    // 等本次写完成
}
```

**FAIL-FAST 关键教训**：F1 的 RTC 寄存器受写保护。不置 `CRL.CNF` 直接写 `ALR`，写入会被**静默忽略**，闹钟永不触发——当时表现为"时间死锁/不自增"。这是社区写法常踩的坑。

### 3. 一次性配置（仅启动时执行）

```c
void RTC_Alarm_Config(RTC_HandleTypeDef *hrtc)
{
  __HAL_RCC_RTC_ENABLE();
  __HAL_RTC_ALARM_ENABLE_IT(hrtc, RTC_IT_ALRA);

  // 手动使能 EXTI Line 17（HAL/CubeMX 不会自动配，STOP 唤醒的关键）
  EXTI->IMR  |= (1UL << 17);
  EXTI->RTSR |= (1UL << 17);
  EXTI->PR    = (1UL << 17);

  HAL_NVIC_SetPriority(RTC_Alarm_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(RTC_Alarm_IRQn);

  next_alarm_target = 当前读到的 CNT + 1;  // 唯一一次读 CNT
  RTC_WriteAlarm(hrtc, next_alarm_target);
}
```

### 4. STOP 进入/唤醒

```c
// 进 STOP
__HAL_TIM_DISABLE_IT(&htim4, TIM_IT_UPDATE);   // 挂起系统滴答
__HAL_TIM_CLEAR_IT(&htim4, TIM_IT_UPDATE);
HAL_NVIC_ClearPendingIRQ(TIM4_IRQn);
HAL_PWR_EnterSTOPMode(PWR_MAINREGULATOR_ON, PWR_STOPENTRY_WFI);  // 唤醒后默认 HSI

// 唤醒后
SystemClock_ResetPLL();   // 仅恢复 HSE+PLL 72MHz，绝不重配 RTC
__HAL_TIM_CLEAR_IT(&htim4, TIM_IT_UPDATE);
HAL_NVIC_ClearPendingIRQ(TIM4_IRQn);
__HAL_TIM_ENABLE_IT(&htim4, TIM_IT_UPDATE);
```

**为什么唤醒只用 `SystemClock_ResetPLL` 而不用完整 `SystemClock_Config`**：
完整配置会重配 `RTCClockSelection`（RTC 时钟源），扰动闹钟对齐 → 1 秒心跳掉拍成约 2 秒。这是历史踩过的坑，必须隔离。

---

## 四、内存/时序要点

- 回调里被主循环使用的变量必须 `volatile`（`rtc_total_seconds`）。
- 回调不清 `ALRF`，由 HAL 中断服务在回调后统一清。**主循环不得抢先清 ALRF**——否则 HAL 判定无闹钟事件、不调回调 → 时间冻结。早期"主循环每 10ms 重装闹钟"版本就死于这个竞态。
- 到达 STOP 前的唤醒运行时长用 `HAL_GetTick()`：STOP 时 TIM4 冻结 → tick 停摆；唤醒后可继续真实 ms 推进。故 `进 STOP 前 tick − 唤醒时 tick` 即对该次唤醒的真实运行毫秒数。

---

## 五、实测性能分解（OLED 实读，无调试器）

> 方法：主循环在关键节点各取一次 `HAL_GetTick()` 快照，输出到 OLED 右下角覆盖层。

| 段 | 实测值 | 占比 | 说明 |
|---|---|---|---|
| I2C 传屏（`OLED_ShowFrame` 全量） | **27 ms** | ~72% | 全帧 1024 字节经 I2C 400kHz 推屏，含全部字节提交 |
| 帧缓冲绘制（`REWS` 逐字写 RAM） | **10 ms** | ~27% | 数十次 PrintString/DrawRectangle 逐像素写 1KB 帧缓冲 |
| PLL 恢复（STOP 唤醒后） | **1 ms** | ~3% | `SystemClock_ResetPLL` |
| 万年历秒推进 | **0 ms** | <1% | 非整分秒提前返回，开销可忽略 |
| 主循环 / 回调 / 其他 | ~0.6 ms | 余量 | |

**总计 ≈ 37.4 ms / 秒** → 睡眠占比 ≈ 99%，低功耗效果达成。

**两个被实测敲定的疑问：**
1. 帧绘制是否 >10ms？→ **是，正好 10ms**（不是免费的）。
2. 万年历 + STOP 是否 <1ms？→ **万年历 ~0ms**；**STOP 的 PLL 恢复正好 1ms**，在边界上。

---

## 六、效率瓶颈与后续优化方向

- **瓶颈集中在 OLED 全屏刷新**（27+10=37ms），占运行期 97%。
- 早就规划的三级阶梯局部刷新（秒段/分秒段/整点全量）可将平均每秒 I2C 传输从 ~27ms 降到 ~2.4ms，运行期占比再降一个数量级——这是下一步最大收益点。
- 结果与"低功耗方案"侧推演一致：**显示刷新是唯一值得优化的对象，万年历与 STOP 切换无需动**。

---

## 七、经验教训清单（踩坑记录）

1. **空谈估算危险**：早期"帧绘制 10ms、万年历+STOP<1ms"虽方向对，但未经实测。"确定"一词必须有数据背书。
2. **RTC 写保护**：不置 `CRL.CNF` 写 ALR 被静默忽略 → 闹钟死锁。
3. **影子寄存器陈旧**：STOP 唤醒后立刻读 CNT/ALR 可能是旧值（APB 与 RTC 核心失步）。解决：运行期不读，只在启动时读一次。
4. **主循环抢清 ALRF**：会让 HAL 不调回调 → 时间冻结。
5. **唤醒重配 RTC 时钟源**：扰动闹钟对齐 → 心跳掉拍成 2 秒。
6. **RSF 等待陷阱**：不要在每个秒心跳回调里"清 RSF → 等 RSF"，RSF 依赖 RTC 1Hz 更新点才重新置位，会阻塞近 1 秒。
7. **同步代码要隔离**：闹钟相关处理尽量收敛在单一模块/单一回调，避免多处写入竞争。

---

## 八、附:相关文件/位置

- `Core/Src/rtc.c`：`RTC_WriteAlarm`、`RTC_Alarm_Config`、`HAL_RTC_AlarmAEventCallback`（均在 USER CODE 区，防重生成覆盖）
- `Core/Inc/rtc.h`：`RTC_Alarm_Config` 声明
- `Core/Src/main.c`：STOP 进入/唤醒、`SystemClock_ResetPLL`、显示刷新与实测埋点