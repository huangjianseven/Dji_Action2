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
