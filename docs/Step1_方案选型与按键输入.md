# 第一步：方案选型 + 开发环境 + 两个按键的读取

> 项目目标：用 **ESP32-S3 开发板** + 面包板 + 杜邦线 + 按钮，控制 **DJI Action 2**：
>
> | 操作 | 相机动作 |
> |---|---|
> | 按键 1 按一下 | 开始录像 |
> | 按键 1 再按一下 | 停止录像 |
> | 按键 2 按一下 | 拍一张照片 |
>
> 整个教程分为几步，每步都能单独验证。本文是 **第一步**。

---

## 1. 先弄清楚：ESP32 能用什么方式"跟 Action 2 说话"？

开工前必须先知道一个关键事实，它决定了整个方案的方向：

- DJI 官方开源了一个 ESP32 蓝牙遥控示例 **[dji-sdk/Osmo-GPS-Controller-Demo](https://github.com/dji-sdk/Osmo-GPS-Controller-Demo)**（基于 DJI R SDK 协议），但它**只支持 Osmo Action 4 / 5 Pro / 6 和 Osmo 360**，**不支持 Action 2**。
- Action 2 使用的是 Action 3 之前的**另一套私有蓝牙协议**，没有公开文档。第三方遥控器厂商也把 Action 2 列为不支持。
- Action 2 自己的蓝牙遥控配件（"遥控延长杆"里的 BTC01 遥控器）协议同样没公开。

所以"直接照抄官方代码走蓝牙"这条路对 Action 2 **行不通**。可选方案对比如下：

| 方案 | 原理 | 难度 | 能否区分 录像/拍照 | 是否改动相机 | 结论 |
|---|---|---|---|---|---|
| **A. 语音控制（推荐）** | Action 2 自带语音控制，支持"开始录像 / 停止录像 / 拍照 / 关机"（中文普通话与英文）。ESP32-S3 通过 I2S 功放 + 小喇叭**播放预先录好的语音指令** | ★★☆ | ✅ 能 | ❌ 不改动 | **主方案** |
| B. 舵机按快门键 | 舵机/电磁铁物理按压相机快门键 | ★☆☆ | ⚠️ 难（拍照/录像要先在屏幕上切模式） | 需要支架 | 备选，只适合"只录像" |
| C. 破解蓝牙协议 | 用 BLE 抓包工具抓 BTC01 遥控器与相机的通讯，再让 ESP32-S3 模拟遥控器 | ★★★★★ | ✅ 能 | ❌ | 进阶，可作为后期挑战 |

**我们选方案 A**：完全不拆不改相机，用的都是便宜常见的元件，而且三个指令正好对应你的三个需求。
（方案 C 是"最优雅"的，但需要专门的蓝牙抓包硬件和大量逆向工作，等主线跑通后可以作为进阶篇再挑战。）

整体框图：

```
 [按键1] ─┐
          ├─> ESP32-S3 ──I2S──> MAX98357A 功放 ──> 小喇叭 ))) 🔊 ──语音──> DJI Action 2
 [按键2] ─┘     │                                                     （已开启语音控制）
                └─ 串口日志（调试用）
```

---

## 2. 采购清单（BOM）

本步（第一步）只需要标 ✅ 的；其余是后面几步要用的，可以一起买。

| # | 元件 | 数量 | 用途 | 本步需要 |
|---|---|---|---|---|
| 1 | ESP32-S3 开发板（如 ESP32-S3-DevKitC-1，建议 N16R8 或 N8R2 带 PSRAM 的版本） | 1 | 主控 | ✅ |
| 2 | USB-C 数据线（**能传数据**的，不是只能充电的） | 1 | 下载程序 / 串口 | ✅ |
| 3 | 面包板（830 孔） | 1 | 搭电路 | ✅ |
| 4 | 轻触按键（6×6mm 四脚） | 2 | 按键 1 / 按键 2 | ✅ |
| 5 | 公对公杜邦线 | 若干 | 连线 | ✅ |
| 6 | MAX98357A I2S 功放模块 | 1 | 播放语音 | 第二步 |
| 7 | 小喇叭 4Ω/8Ω，2–3W（直径 28–40mm） | 1 | 发声 | 第二步 |
| 8 | LED + 220Ω 电阻（可选） | 1 套 | 状态指示（录像中常亮） | 第三步（可选） |

> 💡 按键不需要外接电阻：我们会用 ESP32 内部的上拉电阻（`INPUT_PULLUP`）。

---

## 3. 准备开发环境（Arduino IDE）

本教程用 **Arduino IDE 2.x**，对新手最友好。

1. 下载安装 Arduino IDE 2.x：<https://www.arduino.cc/en/software>
2. 打开 **文件 → 首选项（Preferences）**，在"其他开发板管理器地址"中填入：
   ```
   https://espressif.github.io/arduino-esp32/package_esp32_index.json
   ```
3. 打开 **工具 → 开发板 → 开发板管理器**，搜索 `esp32`，安装 **esp32 by Espressif Systems**（3.x 版本）。
4. 用 USB 线连接开发板。
   - DevKitC-1 上一般有**两个 USB-C 口**：标 `USB` 的是芯片原生 USB，标 `UART`/`COM` 的是串口芯片。**建议插 `UART` 口**，最省事。
   - 如果电脑识别不到串口，Windows 可能需要安装 CP210x 或 CH343 驱动（看板子上串口芯片型号）。
5. 在 **工具** 菜单中设置：
   - 开发板：`ESP32S3 Dev Module`
   - 端口：选择新出现的那个 COM 口（Windows）或 `/dev/cu.usbserial-xxx`（macOS）
   - 如果你插的是 `USB` 原生口：把 **USB CDC On Boot** 设为 `Enabled`，否则串口监视器看不到输出。

**自检**：打开 **文件 → 示例 → 01.Basics → Blink**，点上传，若能成功编译并上传（底部显示 `Hard resetting via RTS pin...`），环境就 OK 了。
如果上传卡在 `Connecting....`：按住板子上的 **BOOT** 键，再按一下 **RST/EN** 键，松开 BOOT，重新上传。

---

## 4. 接线：两个按键

### 4.1 引脚选择

| 功能 | ESP32-S3 引脚 | 说明 |
|---|---|---|
| 按键 1（录像 开始/停止） | **GPIO 4** | 安全的普通 IO |
| 按键 2（拍照） | **GPIO 5** | 安全的普通 IO |
| 公共地 | **GND** | |

> ⚠️ ESP32-S3 有些引脚**不要用**：GPIO 0 / 3 / 45 / 46（启动配置脚）、GPIO 19 / 20（USB）、GPIO 26–37（很多模组被 Flash/PSRAM 占用）。GPIO 4、5 是安全的。

### 4.2 接法

四脚轻触按键：**同一侧（相邻且平行于面包板中槽方向）的两脚是导通的**。最稳妥的做法是让按键**跨在面包板中间凹槽上**，然后使用**对角线**两只脚——对角线上的两脚一定是"按下才导通"的。

```
            面包板中间凹槽
                 ║
   GPIO4 ──── [脚A]║[脚B]
               按键1          （按下时 脚A 与 脚D 导通）
          [脚C]║[脚D] ──── GND
                 ║

   GPIO5 ──── [脚A]║[脚B]
               按键2
          [脚C]║[脚D] ──── GND
                 ║
```

文字版：
1. 把 ESP32-S3 开发板插在面包板上（跨中槽），两排针脚各占一边。
2. 按键 1 跨中槽插好；左上脚用杜邦线连 **GPIO 4**，右下脚连 **GND**。
3. 按键 2 同理；左上脚连 **GPIO 5**，右下脚连 **GND**。
4. 建议把开发板的 GND 引到面包板的蓝色（−）电源轨，按键的 GND 都接到这条轨上，走线更整洁。

原理：引脚开启内部上拉后，**没按时读到 HIGH（1），按下时被拉到 GND，读到 LOW（0）**。

---

## 5. 第一步的程序：读取按键 + 录像状态机

本步还**不发声音**，先把"按键逻辑"做对：
- 按键要**消抖**（机械按键按一下会抖动产生多次跳变）；
- 按键 1 是**切换**逻辑：第一次按 = 开始录像，第二次按 = 停止录像；
- 录像过程中按按键 2 拍照应**忽略**（Action 2 录像中无法拍照，避免发出无效指令）。

程序文件：[`firmware/step1_buttons/step1_buttons.ino`](../firmware/step1_buttons/step1_buttons.ino)

```cpp
// Step 1: 读取两个按键，实现"录像开始/停止 + 拍照"的逻辑（暂时只打印到串口）
// 硬件：按键1 -> GPIO4 与 GND；按键2 -> GPIO5 与 GND（使用内部上拉）

const int PIN_BTN_RECORD = 4;   // 按键1：录像 开始/停止
const int PIN_BTN_PHOTO  = 5;   // 按键2：拍照
const unsigned long DEBOUNCE_MS = 30;    // 消抖时间
const unsigned long COOLDOWN_MS = 1500;  // 两条指令之间的最短间隔（给相机反应时间）

// ---------- 一个简单的消抖按键结构 ----------
struct Button {
  int pin;
  int stableState;          // 消抖后的稳定状态
  int lastReading;          // 上一次原始读数
  unsigned long lastChange; // 原始读数最后一次变化的时间
};

Button btnRecord = {PIN_BTN_RECORD, HIGH, HIGH, 0};
Button btnPhoto  = {PIN_BTN_PHOTO,  HIGH, HIGH, 0};

// 返回 true 表示检测到一次"按下"（从松开 -> 按下的瞬间）
bool pressed(Button &b) {
  int reading = digitalRead(b.pin);
  if (reading != b.lastReading) {
    b.lastChange = millis();
    b.lastReading = reading;
  }
  if (millis() - b.lastChange > DEBOUNCE_MS && reading != b.stableState) {
    b.stableState = reading;
    if (b.stableState == LOW) return true;   // 按下（低电平有效）
  }
  return false;
}

// ---------- 相机状态 ----------
bool isRecording = false;
unsigned long lastCommandAt = 0;

bool cooldownOver() {
  return millis() - lastCommandAt > COOLDOWN_MS;
}

// 第二步会把这三个函数改成"播放对应语音"
void cmdStartRecording() { Serial.println(">>> 指令：开始录像"); }
void cmdStopRecording()  { Serial.println(">>> 指令：停止录像"); }
void cmdTakePhoto()      { Serial.println(">>> 指令：拍照"); }

void setup() {
  Serial.begin(115200);
  delay(500);
  pinMode(PIN_BTN_RECORD, INPUT_PULLUP);
  pinMode(PIN_BTN_PHOTO,  INPUT_PULLUP);
  Serial.println();
  Serial.println("=== DJI Action 2 控制器 - Step 1：按键测试 ===");
  Serial.println("按键1(GPIO4)：录像 开始/停止；按键2(GPIO5)：拍照");
}

void loop() {
  if (pressed(btnRecord)) {
    if (!cooldownOver()) {
      Serial.println("（太快了，忽略。请等相机反应）");
    } else {
      if (!isRecording) cmdStartRecording();
      else              cmdStopRecording();
      isRecording = !isRecording;
      lastCommandAt = millis();
      Serial.printf("当前状态：%s\n", isRecording ? "录像中" : "待机");
    }
  }

  if (pressed(btnPhoto)) {
    if (isRecording) {
      Serial.println("（录像中，不能拍照，已忽略）");
    } else if (!cooldownOver()) {
      Serial.println("（太快了，忽略。请等相机反应）");
    } else {
      cmdTakePhoto();
      lastCommandAt = millis();
    }
  }
}
```

---

## 6. 验证（本步的"通关标准"）

1. 上传程序后，打开 **工具 → 串口监视器**，波特率选 **115200**。
2. 按一下 RST 键，应看到：
   ```
   === DJI Action 2 控制器 - Step 1：按键测试 ===
   ```
3. 依次操作，对照输出：

| 操作 | 期望输出 |
|---|---|
| 按按键 1 | `>>> 指令：开始录像`、`当前状态：录像中` |
| 按按键 2 | `（录像中，不能拍照，已忽略）` |
| 等 2 秒，再按按键 1 | `>>> 指令：停止录像`、`当前状态：待机` |
| 等 2 秒，按按键 2 | `>>> 指令：拍照` |
| 快速连按按键 1 | 第二次显示 `（太快了，忽略…）` |

✅ 每按一次**只出现一条**指令，说明消抖正常，第一步完成！

### 常见问题

| 现象 | 原因 / 解决 |
|---|---|
| 串口监视器什么都没有 | 波特率不是 115200；或插的是原生 `USB` 口但没开 **USB CDC On Boot** |
| 不按按键也一直触发 | 按键插法不对，用的两只脚本来就是导通的 → 换成**对角线**两脚 |
| 按了没反应 | 杜邦线没插紧；或接到了别的 GPIO；用万用表通断档检查 |
| 按一下出现多条指令 | 把 `DEBOUNCE_MS` 调大到 50 |

---

## 7. 顺便在相机上做的准备（为第二步铺路）

在 DJI Action 2 上：
1. 把相机升级到最新固件（用 DJI Mimo App）。
2. 在设置中找到 **语音控制**，打开它，语言选择 **简体中文**（或 English）。
3. 先**用你自己的声音**试一下：对着相机清晰地说"开始录像""停止录像""拍照"，确认相机有响应，并记下相机实际识别的**准确口令措辞**（以你相机说明书 / 屏幕提示为准）。

> 这一步非常重要：如果你的嗓音说口令相机都能识别，那么第二步用喇叭播放录音基本就能成功。

---

## 8. 下一步预告

**第二步：让 ESP32-S3 "开口说话"**
- 接入 MAX98357A I2S 功放和小喇叭；
- 录制/生成"开始录像 / 停止录像 / 拍照"三段语音，转换成 ESP32 能播放的格式；
- 把本步的 `cmdStartRecording()` 等三个函数换成真正的语音播放，第一次真正控制相机！

完成第一步后告诉我结果（或者遇到的报错/现象），我们继续。
