#include "driver/ledc.h"

// --- pins for 4 motors ---
const int IN1A = 14;   // Motor 1 direction A
const int IN2A = 12;   // Motor 1 direction B
const int ENA  = 25;   // Motor 1 PWM

const int IN1B = 27;   // Motor 2 direction A
const int IN2B = 26;   // Motor 2 direction B
const int ENB  = 33;   // Motor 2 PWM

const int IN1C = 32;   // Motor 3 direction A
const int IN2C = 23;   // Motor 3 direction B
const int ENC  = 22;   // Motor 3 PWM

const int IN1D = 21;   // Motor 4 direction A
const int IN2D = 19;   // Motor 4 direction B
const int END  = 18;   // Motor 4 PWM

const int STATUS_LED = 2;   // optional status LED

// PWM configuration
const int PWM_FREQ = 5000;          // 5 kHz
const int PWM_RES_BITS = 8;         // 8-bit resolution (0..255)
const int PWM_MAX = (1 << PWM_RES_BITS) - 1;

// Define 4 PWM channels for 4 motors
const ledc_channel_t CH_M1 = (ledc_channel_t)0;
const ledc_channel_t CH_M2 = (ledc_channel_t)1;
const ledc_channel_t CH_M3 = (ledc_channel_t)2;
const ledc_channel_t CH_M4 = (ledc_channel_t)3;

const ledc_mode_t LEDC_MODE = LEDC_HIGH_SPEED_MODE;
const ledc_timer_t LEDC_TIMER = LEDC_TIMER_0;

bool ledcReady = false;

// ---------------------- LEDC SETUP ----------------------
bool setupLEDCChannel(ledc_channel_t ch, int pin) {
  ledc_channel_config_t c = {};
  c.speed_mode = LEDC_MODE;
  c.channel = ch;
  c.timer_sel = LEDC_TIMER;
  c.intr_type = LEDC_INTR_DISABLE;
  c.gpio_num = pin;
  c.duty = 0;
  c.hpoint = 0;
  return (ledc_channel_config(&c) == ESP_OK);
}

bool setupLEDC() {
  ledc_timer_config_t t = {};
  t.speed_mode = LEDC_MODE;
  t.timer_num = LEDC_TIMER;
  t.duty_resolution = (ledc_timer_bit_t)PWM_RES_BITS;
  t.freq_hz = PWM_FREQ;
  t.clk_cfg = LEDC_AUTO_CLK;
  if (ledc_timer_config(&t) != ESP_OK) {
    Serial.println("❌ LEDC timer config failed");
    return false;
  }

  bool ok = true;
  ok &= setupLEDCChannel(CH_M1, ENA);
  ok &= setupLEDCChannel(CH_M2, ENB);
  ok &= setupLEDCChannel(CH_M3, ENC);
  ok &= setupLEDCChannel(CH_M4, END);

  if (ok) Serial.println("✅ LEDC initialized OK for 4 motors");
  else Serial.println("⚠️ LEDC setup failed for one or more motors");
  return ok;
}

// ---------------------- PWM SETTER ----------------------
void setAllMotorsPWM(uint32_t duty) {
  duty = constrain(duty, 0u, (uint32_t)PWM_MAX);
  ledc_set_duty(LEDC_MODE, CH_M1, duty);
  ledc_update_duty(LEDC_MODE, CH_M1);
  ledc_set_duty(LEDC_MODE, CH_M2, duty);
  ledc_update_duty(LEDC_MODE, CH_M2);
  ledc_set_duty(LEDC_MODE, CH_M3, duty);
  ledc_update_duty(LEDC_MODE, CH_M3);
  ledc_set_duty(LEDC_MODE, CH_M4, duty);
  ledc_update_duty(LEDC_MODE, CH_M4);
}

// ---------------------- SMOOTH STOP ----------------------
void smoothSlowdown() {
  int steps[] = {220, 160, 100, 40, 0};
  for (size_t i = 0; i < sizeof(steps) / sizeof(steps[0]); ++i) {
    setAllMotorsPWM((uint32_t)steps[i]);
    delay(1200);
  }
}

// ---------------------- SERIAL READER ----------------------
String readSerialLine() {
  static String line = "";
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      String out = line;
      line = "";
      out.trim();
      return out;
    } else if (c != '\r') {
      line += c;
      if (line.length() > 200) line = line.substring(line.length() - 200);
    }
  }
  return String();
}

// ---------------------- SETUP ----------------------
void setup() {
  Serial.begin(115200);
  delay(200);

  // Direction pins
  pinMode(IN1A, OUTPUT); pinMode(IN2A, OUTPUT);
  pinMode(IN1B, OUTPUT); pinMode(IN2B, OUTPUT);
  pinMode(IN1C, OUTPUT); pinMode(IN2C, OUTPUT);
  pinMode(IN1D, OUTPUT); pinMode(IN2D, OUTPUT);
  pinMode(STATUS_LED, OUTPUT);

  digitalWrite(STATUS_LED, LOW);

  // Setup LEDC PWM
  ledcReady = setupLEDC();
  if (!ledcReady) {
    Serial.println("⚠️ LEDC setup failed. Motor PWM not available.");
  }

  // Default direction: forward for all motors
  digitalWrite(IN1A, HIGH); digitalWrite(IN2A, LOW);
  digitalWrite(IN1B, HIGH); digitalWrite(IN2B, LOW);
  digitalWrite(IN1C, HIGH); digitalWrite(IN2C, LOW);
  digitalWrite(IN1D, HIGH); digitalWrite(IN2D, LOW);

  // Safety: start stopped
  setAllMotorsPWM(0);

  Serial.println("✅ ESP32 ready for 4 motors. Commands: SLOW / RESTORE / STOP");
}

// ---------------------- LOOP ----------------------
void loop() {
  String cmd = readSerialLine();
  if (cmd.length()) {
    Serial.println("Cmd: " + cmd);
    if (cmd == "SLOW") {
      smoothSlowdown();
      Serial.println("Action: SLOW done");
    } 
    else if (cmd == "RESTORE" || cmd == "NORMAL") {
      setAllMotorsPWM(PWM_MAX);
      digitalWrite(STATUS_LED, HIGH);
      Serial.println("Action: RESTORE -> full speed");
    } 
    else if (cmd == "STOP") {
      setAllMotorsPWM(0);
      digitalWrite(STATUS_LED, LOW);
      Serial.println("Action: STOP -> motors off");
    } 
    else {
      Serial.println("Unknown command");
    }
  }
  delay(10);
}
