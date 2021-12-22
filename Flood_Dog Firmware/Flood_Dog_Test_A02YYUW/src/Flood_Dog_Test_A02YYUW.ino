/*
  Flood_Dog_Test_A02YYUW Ultrasonic Sensor - Mode 2 Demo
  Flood_Dog_Test_A02YYUW.ino
  Uses Flood_Dog_Test_A02YYUW Ultrasonic Sensor
  Displays on Serial Monitor

  ORIGINAL: DroneBot Workshop 2021
  https://dronebotworkshop.com
*/

SerialLogHandler logHandler;			// Sends everything

// A02YYUW:
//  Red = VCC
//  Black = GRD
//  Blue = RX
//  Green = TX
//

// CARRIER BOARD:
// Black = GRN
// Red = VCC
// Blue = TX => BLUE RX
// Yellow = RX => GREEN TX

// Array to store incoming serial data
unsigned char data_buffer[4] = {0};

// Integer to store distance
int distance = 0;

// Variable to hold checksum
unsigned char CS;

// VCC Pin
// const int vccPIN = TX;									// PIR Sensor Digital

void setup() {
  // Set up serial monitor
  Serial.begin(9600);
  // Set up software serial port
  Serial1.begin(9600);

  // pinMode(vccPIN, OUTPUT);										// Allows us to pet the watchdog
  // digitalWrite(vccPIN, HIGH);
}


void loop() {
  // Serial.println("Hello");

  // Run if data available
  if(Serial1.available() > 0){

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
        // Print to logHandler
        Log.info("Distance: %d", distance);
      }
    }
  }
}