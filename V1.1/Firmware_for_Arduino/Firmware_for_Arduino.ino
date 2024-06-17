/*
 ESP32 Tetris with Nintendo 64 Controller
 Based on the work of Brian Lough
 Adapted for Nintendo 64 Controller by Maxim Bortnikov
 For more information please visit
 https://github.com/Northstrix/ESP32-Tetris-With-Nintendo-64-Controller
*/
#include <N64Controller.h>
#include <Wire.h>

N64Controller N64_ctrl(2);

#define REPEAT_AFTER_HOLD 100

unsigned long lastLeftConditionMet = 0;
unsigned long lastRightConditionMet = 0;
bool leftButtonHeld = false;
bool rightButtonHeld = false;

uint16_t code;
uint8_t found;
byte output = 0;  //holds the binary output value
byte sample = 0;
byte current_bit = 0;
boolean waiting = false;  //bool true when ISR runs
byte threshold = 111;
bool pressed_a;
bool pressed_b;
bool pressed_start;
bool pressed_z;
bool pressed_up;
bool pressed_down;
bool pressed_left;
bool pressed_right;
bool pressed_left_b;
bool pressed_right_b;

void send_data_over_i2c(byte x) {
  Wire.beginTransmission(13);
  Wire.write(x);
  //Serial.println(x);
  Wire.endTransmission();
  delay(4);
}

void handle_input() {
  N64_ctrl.update();
  if (N64_ctrl.A() == true) {
    if (pressed_a == false) {
      send_data_over_i2c(133);
    }
    pressed_a = true;
  } else {
    pressed_a = false;
  }

  if (N64_ctrl.B() == true) {
    if (pressed_b == false) {
      send_data_over_i2c(8);
    }
    pressed_b = true;
  } else {
    pressed_b = false;
  }

  if (N64_ctrl.Start() == true) {
    if (pressed_start == false) {
      send_data_over_i2c(13);
    }
    pressed_start = true;
  } else {
    pressed_start = false;
  }

  if (N64_ctrl.Z() == true) {
    if (pressed_z == false) {
      send_data_over_i2c(27);
    }
    pressed_z = true;
  } else {
    pressed_z = false;
  }

  if (N64_ctrl.C_up() == true) {
    if (pressed_up == false) {
      send_data_over_i2c(131);
    }
    pressed_up = true;
  } else {
    pressed_up = false;
  }

  if (N64_ctrl.axis_y() < (threshold) * -1 || N64_ctrl.C_down() == true || N64_ctrl.D_down() == true) {
    send_data_over_i2c(132);
  }

  if (N64_ctrl.L() == true || N64_ctrl.C_left() == true) {
    if (pressed_left_b == false) {
      send_data_over_i2c(129);
    }
    pressed_left_b = true;
  } else {
    pressed_left_b = false;
  }

  if (N64_ctrl.R() == true || N64_ctrl.C_right() == true) {
    if (pressed_right_b == false) {
      send_data_over_i2c(130);
    }
    pressed_right_b = true;
  } else {
    pressed_right_b = false;
  }

if (N64_ctrl.D_left() == true || N64_ctrl.axis_x() < ((threshold) * -1)) {
    if (!leftButtonHeld) {
        send_data_over_i2c(129);
        lastLeftConditionMet = millis();
        leftButtonHeld = true;
    }
} else {
    leftButtonHeld = false;
}

if (N64_ctrl.D_right() == true || N64_ctrl.axis_x() > threshold) {
    if (!rightButtonHeld) {
        send_data_over_i2c(130);
        lastRightConditionMet = millis();
        rightButtonHeld = true;
    }
} else {
    rightButtonHeld = false;
}

if (leftButtonHeld && millis() - lastLeftConditionMet >= REPEAT_AFTER_HOLD) {
    send_data_over_i2c(129);
    lastLeftConditionMet = millis();
}

if (rightButtonHeld && millis() - lastRightConditionMet >= REPEAT_AFTER_HOLD) {
    send_data_over_i2c(130);
    lastRightConditionMet = millis();
}

  delayMicroseconds(256);
}

void setup() {
  //Serial.begin(115200);
  Wire.begin();
  bool pressed_a = false;
  bool pressed_b = false;
  bool pressed_start = false;
  bool pressed_z = false;
  bool pressed_up = false;
  bool pressed_down = false;
  bool pressed_left = false;
  bool pressed_right = false;
}

void loop() {
  handle_input();
}
