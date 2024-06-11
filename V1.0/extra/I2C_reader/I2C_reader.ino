/*
 ESP32 Tetris with Nintendo 64 Controller
 Based on the work of Brian Lough
 Adapted for Nintendo 64 Controller by Maxim Bortnikov
 For more information please visit
 https://github.com/Northstrix/ESP32-Tetris-With-Nintendo-64-Controller
*/
#include <Wire.h>

void setup() {
  Wire.setPins(22, 27);  // Set custom pins for I2C SDA and SCL
  Wire.begin(13);        // Initialize I2C slave mode with address 13
  Serial.begin(115200);  // Start serial for output
  Serial.println("Listening...");
  Wire.onReceive(receiveEvent);  // Register receiveEvent function for I2C data reception
}

void loop() {
  delay(100);
}

// function that executes whenever data is received from master
void receiveEvent(int howMany) {
  howMany = howMany;              // clear warning msg
  int x = Wire.read();            // receive byte as an integer
  Serial.println(x);              // print the integer
}
