///////////////////////////////////////////////////////////////////////////
//    											                         //
//    					  		   Flood Dog     					     //
//   						  	  Water Sensor							 //
//    											                         //
///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
// Particle Product Info edit ID for each firmware update
//
PRODUCT_ID(15526);
PRODUCT_VERSION(29);
char currentPointRelease[5] = "29.0";

const int FRAMversionNumber = 4;						// FRAM Version - RESETS ALL DEFAULTS
int tempFRAMversionNumber;

const char releaseNumber[6] = "2.0";					// Displays the release on the menu
///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////
// Particle
//
SYSTEM_THREAD(ENABLED)

SerialLogHandler logHandler;			// Sends everything
///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////
// DEBUG Mode turns on/off the sleep option
//
bool debug = false;										

/* Sensor Types: 
	0 = n/a
	1 = Carrier Board Only
	2 = Adafruit_ADXL343
	3 = Adafruit_ADT7410 Temperature
	4 = GY-521 6050 Accelerometer
	5 = A02YYUW
	6 = JSN-SR04T
	7 = Capacitance
	8 = Resistance
*/

int sensorType = 1;
int sensorTypeTest = 0;
///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////
// Particle Includes
//
// #include "sys_status.h"

#include "MCP79410RK.h"
#include "MB85RC256V-FRAM-RK.h"

#include "OneWire.h"
#include "Adafruit_Sensor.h"
#include "Adafruit_ADXL343.h"
#include "Adafruit_ADT7410.h"

#include "MPU6050.h"
///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////
// System Status
//
// struct systemStatus_structure sysStatus;
///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////
// Carrier Board Setup
//
MCP79410 rtc;											// Rickkas MCP79410 libarary
MB85RC64 fram(Wire, 0);
///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////
// Pin Constants for Boron
//
const int PIRPin = A0;									// PIR Sensor Digital
const int CAPACITANCE_SIGNAL_1 = A2;					// Capacitance Probe
const int CAPACITANCE_EN = A3;							// Capacitance Enable Power
const int tmp36Pin = A4;								// Simple Analog temperature sensor
const int PWR_6050 = D2;							// Capacitance Enable Power
const int userSwitch = D4;								// User switch with a pull-up resistor
const int donePin = D5;									// Pin the Electron uses to "pet" the watchdog
const int DeepSleepPin = D6;							// Power Cycles the Particle Device and the Carrier Board only RTC Alarm can wake
const int BUILT_IN_LED = D7;     						// Built In LED
const int wakeUpPin = D8;								// This is the Particle Electron WKP pin
///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////
// Sensortype 2, 3
//
// ADXL343 Accelerometer / Tilt Deluxe
//
// Create the ADXL343 accelerometer sensor object
Adafruit_ADXL343 accel = Adafruit_ADXL343(12345);
float ADXL343_accelX;
float ADXL343_accelY;
float ADXL343_accelZ;

// Create the ADT7410 temperature sensor object
Adafruit_ADT7410 deluxeTemperatureSensor = Adafruit_ADT7410();
float ADT7410_TemperatureReading;
float ADT7410_TemperatureReadingSum;
float ADT7410_TemperatureReadingAverage;
///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////
// Sensortype 4
//
// Tilt Magnetometer
//
MPU6050 accelgyro;
int16_t accelX_6050, accelY_6050, accelZ_6050;
int16_t accelX_6050_Sum, accelY_6050_Sum, accelZ_6050_Sum;
int16_t gx_6050, gy_6050, gz_6050;
int16_t gx_6050_Sum, gy_6050_Sum, gz_6050_Sum;
float accel_6050_Temp;
float accelAngleX_6050, accelAngleY_6050;
//
///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////
// Sensortype 5
//
// A02YYUW Sonic
//
//
///////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////
// Program Variables
//
///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
// STATE Machine
namespace FRAM {										// Moved to namespace instead of #define to limit scope
  enum Addresses {
    versionAddr           = 0x00,                   	// Where we store the memory map version number
    controlRegisterAddr   = 0x01,
    currentTestAddr       = 0x02,                   	// What test are we on?  Some perform reset
    timeZoneAddr          = 0x03,                   	// Store the local time zone data
    tomeZoneOffsetAddr    = 0x04,                   	// Store the DST offset value 0 - 2
    deepSleepStartAddr    = 0x05,                   	// Time we started the deep sleep test
    rtcAddr               = 0x09,						// address 9 If restarted by RTC
    watchdogAddr          = 0x10,						// address 16 if restarted by watchDog
    pirAddr               = 0x11,						// address 17 if restarted by watchDog
    sensorTypeAddr        = 0x12						// address 18 sensorType
  };
};

enum State {
		BATTERY_TEST_STATE,
		MESH_CONNECT_WAIT_STATE,
		PARTICLE_CONNECT_WAIT_STATE,
		SAMPLE_SETUP_STATE,
		SAMPLE_STATE,
		SAMPLE_PROCESS_STATE,
		PUBLISH_WAIT_STATE,
		PUBLISH_STATE,
		SLEEP_WAIT_STATE,
		SLEEP_STATE
	};
State state = PARTICLE_CONNECT_WAIT_STATE;
unsigned long stateTime = 0;

const float BATTERY_SAVER_VOLTAGE = 3.75; 								// Below this the sleep time increases

const int MAX_COUNT_TEST_SAMPLING = 3; 									// 3 sampling tries
const int MAX_TIME_TO_WAIT_FOR_CONNECT_MS = (5 * 60 * 1000); 			// Wait for [5] minutes trying to connect to the Particle.io
const int MAX_TIME_TO_SAMPLE_SETUP_WAIT = (1 * 1000); 					// Wait for [1] second before sampling
const int MAX_TIME_TO_WAIT_PUBLISH_MS = (1 * 1000); 					// Wait for [1] seconds before publishing
unsigned long MAX_TIME_TO_WAIT_BEFORE_SLEEP_MS = (15 * 1000);		// After publish, wait [1] minute before sleeping
const int MAX_TIME_TO_SLEEP_SEC = (15); 							// sleep for [15] minutes in seconds
const int BATTERY_SAVER_MODE_SEC = (15); 							// sleep for [60] minutes in seconds
unsigned long MAX_TIME_TO_SLEEP_MS = (MAX_TIME_TO_SLEEP_SEC * 1000);			// sleep for [15] minutes in milliseconds

byte pirState = 0;														// Store the current state for the tests that might cause a reset
byte rtcState = 0;														// Store the current state for the tests that might cause a reset
byte watchdogState = 0;													// Store the current state for the tests that might cause a reset

char resultStr[64];
char SignalString[64];													// Used to communicate Wireless RSSI and Description

int sampleCounter = 0;
int sampleTotalCount = 10;

int capacitanceSampleValue = 0;
int capacitanceSampleSum = 0;
int capacitanceSampleAverage = 0;

int minValue = 0;
int maxValue = 4100;

float carrierBoardTempSensor = 0;
float carrierBoardTempSensorSum = 0;
float carrierBoardTempSensorAverage = 0;
float carrierBoardTempSensorFahrenheit = 0;

int powerSource;
int batteryState;
String powerSString;
String powerBSString;
String batterySocString;

constexpr char const* batteryStates[] = {
											"unknown",
											"not charging",
											"charging",
											"charged",
											"discharging",
											"fault",
											"disconnected"
										};

constexpr char const* powerSources[] = 	{	"unknown",
											"vin",
											"usb host",
											"usb adapter",
											"usb otg",
											"battery"
										};
///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////
// Program Functions
//
///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
// PMIC Setup
//
PMIC pmic;
SystemPowerConfiguration conf;

FuelGauge fuel;
String VCellString;
String SoCString;

void setupPMIC(){
	pmic.begin();						// Don't need PMIC for this - only to prevent charging - would put in a function where you test temp and determine whether to charge

    conf.powerSourceMaxCurrent(900)   	// Don't limit this - let the power flow
        .powerSourceMinVoltage(4300)
        .batteryChargeCurrent(900)		// Don't limit this - let the power flow
        .batteryChargeVoltage(4210);

    int res = System.setPowerConfiguration(conf);
    Log.info("setPowerConfiguration=%d", res);
}
///////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////
// Particle Function to start sampling values in debug TRUE mode
//
int startSampling(String command) {
	stateTime = millis();
	state = SAMPLE_SETUP_STATE;
	return 1;
}
///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////
// Set The SleepWait For 5 minutes
//
int setTempHold(String command) {
	if(command == "hold"){
		MAX_TIME_TO_WAIT_BEFORE_SLEEP_MS = (5 * 60 * 1000);		// After publish, wait [5] minutes before sleeping

		stateTime = millis();

		state = SLEEP_WAIT_STATE;

		return 1;
	} else {
		return 0;
	}
}
///////////////////////////////////////////////////////////////////////////
//   Uses Flood_Dog_Test_A02YYUW Ultrasonic Sensor
//
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
///////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////
// Set The Sensor Type
//
int setSensorType(String command) {
	int tempSetSensorFlag = 1;											// By default tempSetSensorFlag is 1 = OK

	// if(command == "0"){
	// 	sysStatus.sensorType = 0;
	// 	sensorType = 0;
	// } else if(command == "1"){
	// 	sysStatus.sensorType = 1;
	// 	sensorType = 1;
	// } else if(command == "2"){
	// 	sysStatus.sensorType = 2;
	// 	sensorType = 2;
	// } else if(command == "3"){
	// 	sysStatus.sensorType = 3;
	// 	sensorType = 3;
	// } else if(command == "4"){
	// 	sysStatus.sensorType = 4;
	// 	sensorType = 4;
	// } else if(command == "5"){
	// 	sysStatus.sensorType = 5;
	// 	sensorType = 5;
	// } else if(command == "6"){
	// 	sysStatus.sensorType = 6;
	// 	sensorType = 6;
	// } else if(command == "7"){
	// 	sysStatus.sensorType = 7;
	// 	sensorType = 7;
	// } else if(command == "8"){
	// 	sysStatus.sensorType = 8;
	// 	sensorType = 8;
	// } else {
	// 	tempSetSensorFlag = 0;
	// }

	// if(tempSetSensorFlag == 1){
	// 	// fram.put(FRAM::systemStatusAddr,sysStatus);                         		// Write it now since this is a big deal and I don't want values over written

	// 	char data[256]; // character array to build webhook payload

	// 	struct systemStatus_structure tempSysStatus;
	// 	// fram.get(FRAM::systemStatusAddr, tempSysStatus);                          	// See if this worked
	// 	if(tempSysStatus.sensorType == sysStatus.sensorType){
	// 		snprintf(data, sizeof(data), "{\"Sensor Type updated: \":%d}", sensorType);

	// 		logData("OK", data, true, true);
	// 		tempSetSensorFlag = 1;

	// 		startSampling("OK");
	// 	} else {
	// 		snprintf(data, sizeof(data), "{\"Sensor Type NOT updated: \":%d}", sensorType);

	// 		logData("ERROR_STATE", data, true, true);
	// 		tempSetSensorFlag = 0;
	// 	}	
	// }

	return tempSetSensorFlag;
}
///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////
// Load System Defaults
//
void saveSystemDefaults() {                                           // Default settings for the device - connected, not-low power and always on
	// logData("MODE", "Saving system defaults", true, true);
	// sysStatus.structuresVersion = 1;
	// sysStatus.verboseMode = false;
	// sysStatus.verboseCounts = false;
	// sysStatus.lowBatteryMode = false;
	// sysStatus.timezone = -5;                                            // Default is East Coast Time
	// sysStatus.dstOffset = 1;
	// sysStatus.solarPowerMode = true;
	// sysStatus.lastConnectionDuration = 0;                               // New measure
	// sysStatus.sensorType = 1;											// By default - no sensor
	// fram.put(FRAM::systemStatusAddr,sysStatus);                         // Write it now since this is a big deal and I don't want values over written
}
///////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////
// Boron Setup
//
void setup() {
	Serial.begin(9600);
	Serial1.begin(9600);

	// **********************
	// Apply a custom power configuration
	setupPMIC();
	// **********************

    Particle.function("StartSampling", startSampling);
    Particle.function("SetSensorType", setSensorType);
	Particle.variable("SensorType", sensorType);

	pinMode(userSwitch, INPUT);										// Button for user input
	pinMode(wakeUpPin, INPUT_PULLDOWN);								// This pin is active HIGH
	pinMode(BUILT_IN_LED, OUTPUT);									// declare the Blue LED Pin as an output
	pinMode(donePin, OUTPUT);										// Allows us to pet the watchdog
	digitalWrite(donePin, HIGH);
	digitalWrite(donePin, LOW);										// Pet the watchdog
	pinMode(DeepSleepPin , OUTPUT);									// For a hard reset active HIGH

	pinMode(CAPACITANCE_SIGNAL_1, INPUT_PULLDOWN);					// This pin is active HIGH
	pinMode(CAPACITANCE_EN, OUTPUT);					// This pin is active HIGH
	digitalWrite(CAPACITANCE_EN, LOW);								// Turns on cap sensor transistor

	pinMode(PWR_6050, OUTPUT);										// Power for 6050
	digitalWrite(PWR_6050, LOW);									// Power for 6050 OFF

	pinMode(PIRPin, INPUT_PULLDOWN);								// PIR Sensor Digital
	digitalWrite(PIRPin, LOW);										// SET PIR Sensor Digital

	// detachInterrupt(LOW_BAT_UC);
	// // Delay for up to two system power manager loop invocations
	// delay(2000);

	Time.zone(-5 + Time.getDSTOffset());

	rtc.setup();
	delay(100);

	fram.begin();									  				// Initializes Wire but does not return a boolean on successful initialization
	delay(100);

	attachInterrupt(wakeUpPin, watchdogISR, RISING);  				// Need to pet the watchdog when needed
	watchdogISR();
}
///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////
// Boron Loop
//
void loop() {
	rtc.loop();									  							// Need to run this in the main loop

	switch(sensorType){
		case 5:	// A02YYUW
		case 6:	// JSN-SR04T
			if(Serial1.available() > 0){
				distance = 0;

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

						// char data[256]; // character array to build webhook payload
						// Print to serial monitor
						// snprintf(data, sizeof(data), "Distance: %6d mm", distance);
						// logData("Distance", data, true, true);
					}
				}
			}
			break;
	}


	switch(state){
		case PARTICLE_CONNECT_WAIT_STATE:
			if(Particle.connected()){
				logData("PARTICLE", "PARTICLE CONNECTED", true, true);

				char data[256]; // character array to build webhook payload
				fram.put(FRAM::sensorTypeAddr, sensorType);
				fram.get(FRAM::sensorTypeAddr, sensorTypeTest);
				snprintf(data, sizeof(data), "SetSensorType = %d, GetSensorType = %d", sensorType, sensorTypeTest);
				logData("FRAM RESULT", data, true, true);

				if(watchdogState){
					logData("SLEEP RESULT", "Woke By Watcdhdog", true, true);
				}else{
					fram.get(FRAM::rtcAddr, rtcState);
					fram.get(FRAM::pirAddr, pirState);

					if(rtcState){
						logData("SLEEP RESULT", "Woke By RTC", true, true);
					} else if(pirState){
						logData("SLEEP RESULT", "Woke By PIR", true, true);
					}
				}

				fram.get(FRAM::versionAddr, tempFRAMversionNumber);
				// if(tempFRAMversionNumber != FRAMversionNumber){                         // Check to see if the memory map in the sketch matches the data on the chip
				// 	fram.erase();                                                      	// Reset the FRAM to correct the issue
				// 	fram.put(FRAM::versionAddr, FRAMversionNumber);                    	// Put the right value in
				// 	fram.get(FRAM::versionAddr, tempFRAMversionNumber);                          	// See if this worked
				// 	if(tempFRAMversionNumber != FRAMversionNumber){
				// 		logData("ERROR_STATE", "Device will not work without FRAM", true, true);
				// 	} else {
				// 		logData("OK", "Device will load defaults to FRAM", true, true);
				// 		saveSystemDefaults();                                         	// Out of the box, we need the device to be awake and connected
				// 	}
				// } else {
				// 	logData("VERSION OK", "Device will get from FRAM", true, true);
				// 	fram.get(FRAM::systemStatusAddr,sysStatus);                        	// Loads the System Status array from FRAM
				// 	sensorType = sysStatus.sensorType;									// Set sensorType from sysStatus.sensorType
				// }

				stateTime = millis();

				state = SAMPLE_SETUP_STATE;
			} else {
				Particle.connect();

				if(millis() - stateTime > MAX_TIME_TO_WAIT_FOR_CONNECT_MS){
					logData("PARTICLE", "PARTICLE FAILED TO CONNECT", true, true);

					stateTime = millis();

					state = SLEEP_WAIT_STATE;
				}
			}
			break;

		case SAMPLE_SETUP_STATE:
			if(millis() - stateTime >= MAX_TIME_TO_SAMPLE_SETUP_WAIT){
				logData("SAMPLE_SETUP_STATE", "START SAMPLING", true, true);

				debug = false;
				digitalWrite(BUILT_IN_LED,LOW);                                    	// Turns off the LED

				sampleCounter = 1;

				carrierBoardTempSensorSum = 0;

				ADXL343_accelX = 0;
				ADXL343_accelY = 0;
				ADXL343_accelZ = 0;

				ADT7410_TemperatureReading = 0;
				ADT7410_TemperatureReadingSum = 0;
				ADT7410_TemperatureReadingAverage = 0;

				accelX_6050 = 0;
				accelY_6050 = 0;
				accelZ_6050_Sum = 0;
				accelX_6050_Sum = 0;
				accelY_6050 = 0;
				accelZ_6050_Sum = 0;
				accel_6050_Temp = 0;
				accelAngleX_6050 = 0;
				accelAngleY_6050 = 0;

				capacitanceSampleSum = 0;
				capacitanceSampleAverage = 0;

				distance = 0;

				switch(sensorType){
					case 0:	// Debug
						debug = true;
						digitalWrite(BUILT_IN_LED,HIGH);								// If sensorType is N/A then turn on BLUELED
						break;
					case 1:	// CB
						break;
					case 2:	// Tilt Sensor
					case 3:	// Tilt Deluxe
						accel.begin();
						delay(50);
						accel.setRange(ADXL343_RANGE_2_G);
						if(sensorType == 3) deluxeTemperatureSensor.begin();
						break;
					case 4:	// Tilt/Magnetometer
						digitalWrite(PWR_6050, HIGH);									// Power for 6050 ON
						delay(500);
						accelgyro.initialize();
						break;
					case 5:	// A02YYUW
					case 6:	// JSN-SR04T
						break;
					case 7:	// Capacitance
						break;
					case 8:	// Resistance
						break;
				}

				stateTime = millis();

				state = SAMPLE_STATE;
			}
			break;

		case SAMPLE_STATE:
			// logData("SAMPLE_STATE", "SAMPLE_STATE", true, true);

			if(getTemperature()){
				carrierBoardTempSensorSum = carrierBoardTempSensorSum + carrierBoardTempSensor;
			}

			switch(sensorType){
				case 0:	// Debug
					break;
				case 1:	// CB
					break;
				case 2:	// Tilt Sensor
				case 3:	// Tilt Deluxe
					// Get a new Acceleromter event
					sensors_event_t event;
					accel.getEvent(&event);

					ADXL343_accelX = ADXL343_accelX + event.acceleration.x;
					ADXL343_accelY = ADXL343_accelY + event.acceleration.y;
					ADXL343_accelZ = ADXL343_accelZ + event.acceleration.z;
					delay(50);

					if(sensorType == 3){
						ADT7410_TemperatureReading = deluxeTemperatureSensor.readTempC();               			// Read Tilt Deluxe Temp Sensor
						
						if(ADT7410_TemperatureReading > -20 && ADT7410_TemperatureReading < 40){
							ADT7410_TemperatureReadingSum = ADT7410_TemperatureReadingSum + ADT7410_TemperatureReading;	// Tilt Deluxe Temp						
						}
					}
					break;
				case 4:	// Tilt/Magnetometer
					if(accelgyro.testConnection()){
						// read raw accel/gyro measurements from device
						accelgyro.getMotion6(&accelX_6050, &accelY_6050, &accelZ_6050, &gx_6050, &gy_6050, &gz_6050);
						accel_6050_Temp = (accelgyro.getTemperature() / 340.00) + 36.53;
					}
					break;
				case 5:	// A02YYUW
				case 6:	// JSN-SR04T

					break;
				case 7:	// Capacitance
					char data[256]; // character array to build webhook payload

					capacitanceSampleValue = analogRead(CAPACITANCE_SIGNAL_1);

					snprintf(data, sizeof(data), "{\"capacitance\":%d}", capacitanceSampleValue);
					logData("Cap", data, true, true);

					capacitanceSampleSum = capacitanceSampleSum + capacitanceSampleValue;
					break;
				case 8:	// Resistance
					break;
			}

			if(sampleCounter++ > sampleTotalCount){
				stateTime = millis();

				state = SAMPLE_PROCESS_STATE;
			}
			break;

		case SAMPLE_PROCESS_STATE:
				batteryState = System.batteryState();

				carrierBoardTempSensorAverage = carrierBoardTempSensorSum / sampleTotalCount;

				switch(sensorType){
					case 0:	// Debug
						break;
					case 1:	// No Sensor
						break;
					case 2:	// Tilt Sensor
					case 3: // Tilt Deluxe
						ADXL343_accelX = ADXL343_accelX / sampleTotalCount;
						ADXL343_accelY = ADXL343_accelY / sampleTotalCount;
						ADXL343_accelZ = ADXL343_accelZ / sampleTotalCount;

						ADT7410_TemperatureReadingAverage = ADT7410_TemperatureReadingSum / sampleTotalCount;
						break;
					case 4:	// Tilt/Magnetometer
						if(!(accelX_6050 == 0 && accelY_6050 == 0 && accelZ_6050 == 0)){
							accelAngleX_6050 = (atan(accelY_6050 / sqrt(pow(accelX_6050, 2) + pow(accelZ_6050, 2))) * 180 / PI) - 0.58; // AccErrorX ~(0.58) See the calculate_IMU_error()custom function for more details
							accelAngleY_6050 = (atan(-1 * accelX_6050 / sqrt(pow(accelY_6050, 2) + pow(accelZ_6050, 2))) * 180 / PI) + 1.58; // AccErrorY ~(-1.58)
						}
						break;
					case 5:	// A02YYUW
						break;
					case 6:	// JSN-SR04T
						break;
					case 7:	// Capacitance
						capacitanceSampleAverage = capacitanceSampleSum / sampleTotalCount;
						break;
					case 8:	// Resistance
						break;
				}

				stateTime = millis();

				state = PUBLISH_WAIT_STATE;
			break;

		case PUBLISH_WAIT_STATE:
			if(millis() - stateTime >= MAX_TIME_TO_WAIT_PUBLISH_MS){
				logData("PUBLISHING", "START PUBLISH_STATE", true, true);

				stateTime = millis();

				state = PUBLISH_STATE;
			}
			break;

		case PUBLISH_STATE:
			char data[256]; // character array to build webhook payload

			switch(sensorType){
				case 0:	// Debug
					snprintf(data, sizeof(data), "{\"CB_Temp\":%.2f, \"battery\":%.2f, \"batterystate\":\"%s\", \"timestamp\":%lu000}", carrierBoardTempSensorAverage, fuel.getVCell(), batteryStates[std::max(0, batteryState)], Time.now());
					break;
				case 1:	// No Sensor
					snprintf(data, sizeof(data), "{\"CB_Temp\":%.2f, \"battery\":%.2f, \"batterystate\":\"%s\", \"timestamp\":%lu000}", carrierBoardTempSensorAverage, fuel.getVCell(), batteryStates[std::max(0, batteryState)], Time.now());
					break;
				case 2:	// ADXL343 Accelerometer
					snprintf(data, sizeof(data), "{\"CB_Temp\":%.2f, \"ADXL343_AccelX\":%.2f, \"ADXL343_AccelY\":%.2f, \"ADXL343_AccelZ\":%.2f, \"battery\":%.2f, \"batterystate\":\"%s\", \"timestamp\":%lu000}", carrierBoardTempSensorAverage, ADXL343_accelX, ADXL343_accelY, ADXL343_accelZ, fuel.getVCell(), batteryStates[std::max(0, batteryState)], Time.now());
					break;
				case 3:	// ADXL343 Accelerometer with ADT7410 Temp Sensor
					snprintf(data, sizeof(data), "{\"CB_Temp\":%.2f, \"ADXL343_AccelX\":%.2f, \"ADXL343_AccelY\":%.2f, \"ADXL343_AccelZ\":%.2f, \"ADT7410_Temp\":%.2f, \"battery\":%.2f, \"batterystate\":\"%s\", \"timestamp\":%lu000}", carrierBoardTempSensorAverage, ADXL343_accelX, ADXL343_accelY, ADXL343_accelZ, ADT7410_TemperatureReadingAverage, fuel.getVCell(), batteryStates[std::max(0, batteryState)], Time.now());
					break;
				case 4:	// 6050 Accelerometer
					snprintf(data, sizeof(data), "{\"CB_Temp\":%.2f, \"AccelX_6050\":%6d, \"AccelY_6050\":%6d, \"AccelZ_6050\":%6d, \"Temp_6050\":%.2f, \"AccAngleX_6050\":%.2f, \"AccAngleY_6050\":%.2f, \"battery\":%.2f, \"batterystate\":\"%s\", \"timestamp\":%lu000}", carrierBoardTempSensorAverage, accelX_6050, accelY_6050, accelZ_6050, accel_6050_Temp, accelAngleX_6050, accelAngleY_6050, fuel.getVCell(), batteryStates[std::max(0, batteryState)], Time.now());
					break;
				case 5:	// A02YYUW Sonic Sensor
					snprintf(data, sizeof(data), "{\"CB_Temp\":%.2f, \"distance\":%d, \"battery\":%.2f, \"batterystate\":\"%s\", \"timestamp\":%lu000}", carrierBoardTempSensorAverage, distance, fuel.getVCell(), batteryStates[std::max(0, batteryState)], Time.now());
					break;
				case 6:	// JSN-SR04T Sonic Sensor
					snprintf(data, sizeof(data), "{\"CB_Temp\":%.2f, \"battery\":%.2f, \"batterystate\":\"%s\", \"timestamp\":%lu000}", carrierBoardTempSensorAverage, fuel.getVCell(), batteryStates[std::max(0, batteryState)], Time.now());
					break;
				case 7:	// Capacitance Sensor
					snprintf(data, sizeof(data), "{\"CB_Temp\":%.2f, \"capacitance\":%d, \"battery\":%.2f, \"batterystate\":\"%s\", \"timestamp\":%lu000}", carrierBoardTempSensorAverage, capacitanceSampleAverage, fuel.getVCell(), batteryStates[std::max(0, batteryState)], Time.now());
					break;
				case 8:	// Resistance Sensor
					snprintf(data, sizeof(data), "{\"CB_Temp\":%.2f, \"battery\":%.2f, \"batterystate\":\"%s\", \"timestamp\":%lu000}", carrierBoardTempSensorAverage, fuel.getVCell(), batteryStates[std::max(0, batteryState)], Time.now());
					break;
			}

			Particle.publish("Ubidots-v4", data, PRIVATE);			// Note, you can add Publish Queue POSIX later - let's get this going first

			logData("PUBLISH_STATE", "START SLEEP_WAIT_STATE", true, true);

			// digitalWrite(BUILT_IN_LED, LOW);

			sensorType = 2;
			fram.put(FRAM::sensorTypeAddr, sensorType);

			stateTime = millis();

			state = SLEEP_WAIT_STATE;
			break;		

		case SLEEP_WAIT_STATE:
			if(millis() - stateTime >= MAX_TIME_TO_WAIT_BEFORE_SLEEP_MS){
				char data[256]; // character array to build webhook payload
				fram.get(FRAM::sensorTypeAddr, sensorTypeTest);
				snprintf(data, sizeof(data), "SetSensorType = %d, GetSensorType = %d", sensorType, sensorTypeTest);
				logData("FRAM RESULT", data, true, true);
				
				if(debug){
					logData("SLEEP", "DEBUG NO SLEEP", true, true);
				} else {
					logData("SLEEP", "GOING TO SLEEP", true, true);
				}

				stateTime = millis();

				state = SLEEP_STATE;
			}
			break;

		case SLEEP_STATE:
			if(!debug){
				// NORMAL mode
				logData("SLEEP", "SLEEP STOP SLEEP", true, true);

				digitalWrite(PWR_6050, LOW);											// Power for 6050 OFF

				pirState = 0;
				fram.put(FRAM::pirAddr, pirState);
				rtcState = 0;
				fram.put(FRAM::rtcAddr, rtcState);
				watchdogState = 0;
				fram.put(FRAM::watchdogAddr, watchdogState);

				if(fuel.getVCell() < BATTERY_SAVER_VOLTAGE){
					MAX_TIME_TO_SLEEP_MS = (BATTERY_SAVER_MODE_SEC * 1000);				// sleep for [60] minutes in milliseconds
				}
				SystemSleepConfiguration config;
					config.mode(SystemSleepMode::STOP)
						.gpio(PIRPin, RISING)
						.duration(MAX_TIME_TO_SLEEP_MS);
				SystemSleepResult result = System.sleep(config);


				digitalWrite(PIRPin, LOW);												// Pet the watchdog

				if(result.wakeupReason() == SystemSleepWakeupReason::BY_GPIO){
					// Waken by pin
					pin_t whichPin = result.wakeupPin();

					if(whichPin == A0){
						logData("SLEEP RESULT", "Woke By BY_GPIO PIR", true, true);

						// digitalWrite(BUILT_IN_LED, HIGH);

						pirState = 1;
						fram.put(FRAM::pirAddr, pirState);
					} else {
						logData("SLEEP RESULT", "Woke By BY_GPIO RTC", true, true);

						rtcState = 1;
						fram.put(FRAM::rtcAddr, rtcState);
					}
				}

				if(result.wakeupReason() == SystemSleepWakeupReason::BY_RTC){
					// Waken by RTC
					logData("SLEEP RESULT", "SLEEP RESULT - Woke BY_RTC", true, true);
				}

				if(result.wakeupReason() == SystemSleepWakeupReason::BY_NETWORK){
					// Waken by NETWORK
					logData("SLEEP RESULT", "SLEEP RESULT - Woke BY_NETWORK", true, true);
				}

				if(result.wakeupReason() == SystemSleepWakeupReason::UNKNOWN) {
					// Waken by RTC
					logData("SLEEP RESULT", "SLEEP RESULT - Woke By UNKNOWN", true, true);
				}


				stateTime = millis();

				state = PARTICLE_CONNECT_WAIT_STATE;
			} else {
				// DEBUG mode
				if(millis() - stateTime >= MAX_TIME_TO_SLEEP_MS){

					stateTime = millis();

					state = SAMPLE_SETUP_STATE;
				}
			}
			break;
	}
}
///////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////
// WatchDog setup
//
void watchdogISR(){
	pirState = 0;
	fram.put(FRAM::pirAddr, pirState);
	rtcState = 0;
	fram.put(FRAM::rtcAddr, rtcState);
	watchdogState = 1;
	fram.put(FRAM::watchdogAddr, watchdogState);

	digitalWrite(donePin, HIGH);                           // Pet the watchdog
	digitalWrite(donePin, LOW);
}
///////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
// Particle function to handle the trimmed response code back from the webhooks
//
void UbidotsHandler(const char *event, const char *data) {            // Looks at the response from Ubidots - Will reset Photon if no successful response
//   char responseString[64];
//     // Response is only a single number thanks to Template
//   if (!strlen(data)) {                                                // No data in response - Error
//     snprintf(responseString, sizeof(responseString),"No Data");
//   }
//   else if (atoi(data) == 200 || atoi(data) == 201) {
//     snprintf(responseString, sizeof(responseString),"Response Received");
//     sysStatus.lastHookResponse = Time.now();                          // Record the last successful Webhook Response
//     systemStatusWriteNeeded = true;
//     dataInFlight = false;                                             // Data has been received
//   }
//   else {
//     snprintf(responseString, sizeof(responseString), "Unknown response recevied %i",atoi(data));
//   }
//   if (sysStatus.verboseMode && sysStatus.connectedStatus) {
//     waitUntil(meterParticlePublish);
//     Particle.publish("Ubidots Hook", responseString, PRIVATE);
//   }
}
////////////////////////////////////////////////////////////////////////////




///////////////////////////////////////////////////////////////////////////
// Accelerometer Functions
//
void displayDataRate(void)
{
  Serial.print("Data Rate:");

  switch(accel.getDataRate())
  {
    case ADXL343_DATARATE_3200_HZ:
      Serial.print("3200 ");
      break;
    case ADXL343_DATARATE_1600_HZ:
      Serial.print("1600 ");
      break;
    case ADXL343_DATARATE_800_HZ:
      Serial.print("800 ");
      break;
    case ADXL343_DATARATE_400_HZ:
      Serial.print("400 ");
      break;
    case ADXL343_DATARATE_200_HZ:
      Serial.print("200 ");
      break;
    case ADXL343_DATARATE_100_HZ:
      Serial.print("100 ");
      break;
    case ADXL343_DATARATE_50_HZ:
      Serial.print("50 ");
      break;
    case ADXL343_DATARATE_25_HZ:
      Serial.print("25 ");
      break;
    case ADXL343_DATARATE_12_5_HZ:
      Serial.print("12.5 ");
      break;
    case ADXL343_DATARATE_6_25HZ:
      Serial.print("6.25 ");
      break;
    case ADXL343_DATARATE_3_13_HZ:
      Serial.print("3.13 ");
      break;
    case ADXL343_DATARATE_1_56_HZ:
      Serial.print("1.56 ");
      break;
    case ADXL343_DATARATE_0_78_HZ:
      Serial.print("0.78 ");
      break;
    case ADXL343_DATARATE_0_39_HZ:
      Serial.print("0.39 ");
      break;
    case ADXL343_DATARATE_0_20_HZ:
      Serial.print("0.20 ");
      break;
    case ADXL343_DATARATE_0_10_HZ:
      Serial.print("0.10 ");
      break;
    default:
      Serial.print("???? ");
      break;
  }
  Serial.println(" Hz");
}

void displayRange(void)
{
  Serial.print("Range: +/- ");

  switch(accel.getRange())
  {
    case ADXL343_RANGE_16_G:
      Serial.print("16 ");
      break;
    case ADXL343_RANGE_8_G:
      Serial.print("8 ");
      break;
    case ADXL343_RANGE_4_G:
      Serial.print("4 ");
      break;
    case ADXL343_RANGE_2_G:
      Serial.print("2 ");
      break;
    default:
      Serial.print("?? ");
      break;
  }
  Serial.println(" g");
}
///////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////
// Carrier Board Temp Value
//
bool getTemperature() {
	int reading = analogRead(tmp36Pin);   //getting the voltage reading from the temperature sensor
	delay(1000);
	reading = analogRead(tmp36Pin);   //getting the voltage reading from the temperature sensor
	delay(1000);
	float voltage = reading * 3.3;        // converting that reading to voltage, for 3.3v arduino use 3.3
	voltage /= 4096.0;                    // Electron is different than the Arduino where there are only 1024 steps
	carrierBoardTempSensor = (voltage - 0.5) * 100.0;  //converting from 10 mv per degree with 500 mV offset to degrees ((voltage - 500mV) times 100) - 5 degree calibration
	carrierBoardTempSensorFahrenheit = (carrierBoardTempSensor * 9.0 / 5.0) + 32.0;  // now convert to Fahrenheit

	if(carrierBoardTempSensor < -20.0 || carrierBoardTempSensor > 30.0) {             // Reasonable range for garage temperature
		// snprintf(resultStr, sizeof(resultStr),"Temp seems whack: %3.1f", carrierBoardTempSensorFahrenheit);
		return 0;
	}
	else {
		// snprintf(resultStr, sizeof(resultStr),"Temperature is: %3.1f", carrierBoardTempSensorFahrenheit);
		return 1;
	}
}
///////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////
// Log Data to Serial and Particle
//
bool logData(String tempEvent, String tempData, bool tempLogSerial, bool tempLogParticle){
	if(tempLogSerial){
		Log.info(tempEvent + ": " + tempData);
	}
	if(tempLogParticle){
		waitUntil(meterParticlePublish);
		Particle.publish(tempEvent, tempData, 60, PRIVATE);
	}
	return true;
}
///////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////
// Meter the data to Particle
//
bool meterParticlePublish(void){                           // Enforces Particle's limit on 1 publish a second
  static unsigned long lastPublish = 0;                    // Initialize and store value here
  if(millis() - lastPublish >= 1000) {                     // Particle rate limits at 1 publish per second
    lastPublish = millis();
    return 1;
  }
  else return 0;
}
///////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////
// Select External Mesh Antena for BlueTooth and Xenon devices
//
void selectExternalMeshAntenna(){
	#if(PLATFORM_ID == PLATFORM_ARGON)
		digitalWrite(ANTSW1, 1);
		digitalWrite(ANTSW2, 0);
	#elif(PLATFORM_ID == PLATFORM_BORON)
		digitalWrite(ANTSW1, 0);
	#else
		digitalWrite(ANTSW1, 0);
		digitalWrite(ANTSW2, 1);
	#endif
}
///////////////////////////////////////////////////////////////////////////
