# Dji_Action2
大疆Action2运动相机外控

用 ESP32-S3 + 面包板 + 按键控制 DJI Action 2（开始/停止录像、拍照）的分步教程。

## 总体方案

Action 2 的蓝牙协议没有公开（DJI 官方 ESP32 蓝牙示例只支持 Action 4 及以后的机型），所以本项目利用 Action 2 **自带的语音控制**：ESP32-S3 通过 I2S 功放和小喇叭播放「开始录像 / 停止录像 / 拍照」口令来控制相机，不拆改相机。

```
 [按键1] ─┐
 [按键2] ─┼─> ESP32-S3 ──I2S──> MAX98357A ──> 喇叭 ))) ──语音──> DJI Action 2
 [Apple Watch 快捷指令] ──Wi-Fi(HTTP)──┘
```

## 路线图

| 步骤 | 内容 | 状态 |
|---|---|---|
| [第一步](docs/Step1_方案选型与按键输入.md) | 方案选型、开发环境、两个按键的读取与逻辑 | ✅ 已完成 |
| 第二步 | I2S 功放 + 喇叭，播放语音口令，真正控制相机 | ⏳ 待写 |
| 第三步 | Apple Watch 无线遥控：ESP32-S3 开 Wi-Fi 网页接口，手表用「快捷指令」触发 | ⏳ 待写 |

### 第三步预览：Apple Watch 遥控

- ESP32-S3 连接 Wi-Fi（如手机热点），运行一个小型 HTTP 服务：
  - `GET /record` → 切换录像（等同按键 1）
  - `GET /photo`  → 拍照（等同按键 2）
  - `GET /status` → 返回当前是否在录像
- Apple Watch 上用「快捷指令」App 新建两个指令，使用「获取 URL 内容」动作请求上面的地址；可放到表盘复杂功能，或绑定到 Apple Watch Ultra 的**操作按钮**。
- 手表请求和面包板按键调用**同一套** `cmdStartRecording()` / `cmdStopRecording()` / `cmdTakePhoto()` 逻辑，二者可同时使用、状态保持一致。
- 前提：手表与 ESP32-S3 在同一网络；建议给 ESP32 设置固定 IP 或使用 mDNS（如 `http://action2.local/record`）。
