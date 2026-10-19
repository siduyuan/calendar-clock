# OLED 局部刷新 + STOP 低功耗 优化方案

> 状态：**拟定稿（待实施）**
> 适用范围：STM32F103C8T6 @72MHz，SSD1306 0.96"，I2C1 @400kHz
> 目标：将每秒 I2C 全量刷新（约 24ms）降至 **约 2.2–2.4ms/秒**，并叠加万年历/STOP 后工作周期可控在 7ms 内。

---

## 1. 背景与现状

当前 [main.c](file:///d:/CodeForge/Project/STM32/testcount/Core/Src/main.c) 每整秒做一次 **全帧刷新**（[OLED_ShowFrame](file:///d:/CodeForge/Project/STM32/testcount/Core/Src/oled.c#L173)）：

- 每帧传输 ≈ **1072 B**（8页 × 129数据 + 8×3命令×2）
- 400kHz 下 ≈ **24–25 ms/秒**，期间主循环被 `HAL_I2C_Master_Transmit(…,HAL_MAX_DELAY)` 阻塞。
- 实际上每秒只有**秒这个字段在变**，整帧刷新属于明显浪费。

时间显示为 `HH:MM:SS`，画于 `(20,14)`，用 `font16x16`（其 ASCII 子字体为 24×12 等宽，每字符 12px 宽 × 24px 高）。

### 关键测量/推导结论

- 全帧刷新传输：**≈ 24 ms**（400kHz）
- 两级局部刷新平均：**≈ 2.2–2.4 ms/秒**（省一个数量级）
- 万年历平时刷新：**0.02–0.04 ms**；整分秒（含农历大表）**0.15–0.9 ms**
- **flash 大表查表（农历）只在整分秒发生**，性能影响可忽略，无需迁移 SRAM
- 数字上方的 2px 留白（y=14~15）可跳过 → 时间区锁定 **P2/P3/P4**，P1 整页跳过

---

## 2. 方案设计

### 2.1 三级刷新模式（分层）

| 模式 | 触发时机 | 区域(列×页) | 字节 | 单次传输 |
|---|---|---|---|---|
| **秒型** | 每秒（秒变更） | 秒位 列92–115 × P2-P4 | ≈96 | ≈2.16 ms |
| **分型** | 整分首秒（分秒变更） | 分位+秒位 列56–115 × P2-P4 | ≈192 | ≈4.32 ms |
| **全量** | 整点首秒 或 按键改动/初始化 | 全帧 | ≈1072 | ≈24 ms |

- 整点做全量：因为"时"变更同时牵动干支时柱等，做全量最简单，且每天仅 24 次，对总平均影响 <0.3%。
- 分型包含秒位 → 覆盖当次秒刷新，无需再单独发秒。

### 2.2 区域坐标（依据选择框逻辑坐标 + 肉眼观测得出）

时间字符串 8 字符（`HH:MM:SS`），x 起点 20，每字符 12px：

| 字符 | 内容 | 列区间 |
|---|---|---|
| 6,7 | SS | 92–115 |
| 3,4 | MM | 56–79 |
| 3–7 | MM:SS | 56–115 |

**页**：起始 y=14，但数字实际像素约 **行16–34**（顶部2px留白 + 底部冗余），故：
- **跳过 P1（行 8–15）**
- 实际发送 **P2（16–23）/ P3（24–31）/ P4（32–39）**，且 P4 内仅行 32–34 有数据。

> ⚠️ **待实测确认**：顶部 2px 是否为纯空白（依赖字模取模方向/取模助手设置）。若确认空白，P1 可安全跳过；否则不可省。
> **取模助手特征**：24×12、列式纵向取模，每列 3 字节，bit7=该片最上行。数字 `'0'` 顶部字节含 `0x80/0xC0` 置位 → 若 bit7=顶则数字顶格、可能无 2px 留白，需 GUI 复核 0–9 全部字符。

### 2.3 便携 I2C 发送函数（新增，不动原库）

仿照 `OLED_ShowFrame`，新增一个可指定区域的发送器，复用 `OLED_GRAM`：

```
OLED_SendRegion(col_start, col_end, page_start, page_end):
  for page = page_start..page_end:
    OLED_SendCmd(0xB0 + page)                 // 页地址
    OLED_SendCmd(col_start & 0x0F)            // 列低4位
    OLED_SendCmd((col_start >> 4) | 0x10)     // 列高4位
    sendBuffer[0] = 0x40
    for col = col_start..col_end:
        sendBuffer[1 + (col-col_start)] = OLED_GRAM[page][col]
    OLED_Send(sendBuffer, 1 + (col_end-col_start+1))
```

- 放在 [oled.c](file:///d:/CodeForge/Project/STM32/testcount/Core/Src/oled.c) 自定义区（确保不被 CubeMX 覆盖）。
- 全量 = 调用 `OLED_SendRegion(0,127,0,7)`，可取代原 `OLED_ShowFrame`。
- **不额外调用 `OLED_NewFrame` 全帧清零**，改为只清/重画变更区块到 GRAM，避免全帧 memset。

### 2.4 万年历刷新时机

- `Calendar_Increment` → `Calendar_Refresh` 已自带"整分秒才算、否则提前 return"（见 [calendar.c](file:///d:/CodeForge/Project/STM32/testcount/Core/Src/calendar.c#L390)）。无需改动。
- 整分首秒的农历大表计算与"分型刷新"叠加 → 该秒预算取上限。

### 2.5 STOP 低功耗（后续阶段）

- 用 RTC 闹钟（EXTI17）作 1 秒心跳唤醒（现有 `RTC_ArmNextAlarm` 已兼容）。
- 显示任务完成后 `HAL_PWR_EnterSTOPMode(...)`。
- 唤醒后时钟策略二选一（决定 STOP 开销）：
  - **A（推荐、低开销）**：唤醒回 HSI 8MHz，不重配 PLL → STOP 恢复 ≈0.1ms
  - **B（全速）**：唤醒后 `SystemClock_Config()` 重配 72MHz → 需 HSE+PLL 起振，≈1–3ms
- 唤醒后须重使能 **TIM4**（`uwTick` 来源），否则 `HAL_Delay` 计时错乱。

---

## 3. 时间预算汇总

### 每秒正常秒型（非整分/整点）

| 项 | 耗时 |
|---|---|
| 万年历（非整分） | 0.02–0.04 ms |
| 文字绘制 + 收尾（非传输） | 0.4–0.7 ms |
| IIC 秒型传输 | ≈2.16 ms |
| **合计（无 STOP）** | **≈2.6–2.9 ms** ✓ <4ms |
| 唤醒恢复（STOP 方案A，HSI） | ≈0.1 ms |
| **合计（STOP 方案A）** | **≈3 ms** ✓ <7ms |

### 边界秒型

- **整分秒**（分型 + 万年历大表）：无 STOP ≈5–6 ms（一秒一次，平均仍 <4ms）；STOP 方案B ≤7ms 临界，方案A 稳 <7ms。
- **整点秒**（全量 ≈24ms）：一秒一次的卡顿，可接受。

---

## 4. 实施步骤清单

- [ ] ① 复核 `ascii_24x12` 顶部留白（取模助手 GUI 数 0–9），确认 P1 可跳过
- [ ] ② 在 [oled.c](file:///d:/CodeForge/Project/STM32/testcount/Core/Src/oled.c) 实现 `OLED_SendRegion()`
- [ ] ③ 在 [oled.h](file:///d:/CodeForge/Project/STM32/testcount/Core/Inc/oled.h) 声明
- [ ] ④ 在 [main.c](file:///d:/CodeForge/Project/STM32/testcount/Core/Src/main.c) 主循环改造刷新分发：
      - 秒型 / 分型 / 全量 三级判定
      - 变更区块只重画 GRAM，不整帧 memset
- [ ] ⑤（可选）打 GPIO/读 DWT 实测各段耗时，校正本文预算
- [ ] ⑥（后续）接入 STOP：`HAL_PWR_EnterSTOPMode` + 唤醒恢复（HSI 或 72MHz）+ TIM4 重使能
- [ ] ⑦ 全量刷新兼容性验证：确认 `OLED_SendRegion(0,127,0,7)` 可替代 `OLED_ShowFrame`

---

## 5. 待验证 / 风险

- **P1 是否可跳过**取决于字模顶部 2px 是否纯空白——最大不确定性，先实测。
- 分/秒形局部若用 `OLED_PrintString` 重画，需确保新字模顶部字节为 0，避免残留。
- SSD1306 页寻址下"页内部分行不可裁"，局部最小单位是"整页 × 列区间"。
- STOP 方案B（72MHz）的启动耗时受 HSE/PLL 起振制约，是否 <7ms 需硬件实测。