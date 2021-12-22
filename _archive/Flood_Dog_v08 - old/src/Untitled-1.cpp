/*
  JSN-SR04T-V3.0 Ultrasonic Sensor - Mode 1 Demo
  srt04-mode1.ino
  Uses JSN-SR04T-V3.0 Ultrasonic Sensor
  Displays on Serial Monitor

  Mode 1 is set by bridging "M1" pads on board

  Also works with A02YYUW Ultrasonic Sensor

  DroneBot Workshop 2021
  https://dronebotworkshop.com
*/
SerialLogHandler logHandler;

// Include the Software Serial library
// #include <SoftwareSerial.h>

// Define connections to sensor
// int pinRX = 10;
// int pinTX = 11;

// Array to store incoming serial data
unsigned char data_buffer[4] = {0};
char data[256];										// character array to build webhook payload

// Integer to store distance
int distance = 0;

// Variable to hold checksum
unsigned char CS;

// Object to represent software serial port
// SoftwareSerial Serial1(pinRX, pinTX);

void setup() {
  // Set up serial monitor
  Serial.begin(9600);
  // Set up software serial port
  Serial1.begin(9600);
}

void loop() {
  Log.info("Serial");
  // snprintf(data, sizeof(data), "{\"distance\":%d}", distance);
  // Log.info(data);

  // Run if data available
  if (Serial1.available() > 0) {
    Log.info("Serial1");

    delay(4);

    // Check for packet header character 0xff
    if (Serial1.read() == 0xff) {
      // Insert header into array
      data_buffer[0] = 0xff;
      // Read remaining 3 characters of data and insert into array
      for (int i = 1; i < 4; i++) {
        data_buffer[i] = Serial1.read();
      }

      //Compute checksum
      CS = data_buffer[0] + data_buffer[1] + data_buffer[2];
      // If checksum is valid compose distance from data
      if (data_buffer[3] == CS) {
        distance = (data_buffer[1] << 8) + data_buffer[2];
        // Print to serial monitor
        snprintf(data, sizeof(data), "{\"distance\":%d}", distance);
        Log.info(data);
        // Serial.print("distance: ");
        // Serial.print(distance);
        // Serial.println(" mm");
      }
    }
  }
}