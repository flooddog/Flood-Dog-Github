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
PRODUCT_VERSION(38);
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
const int FRAMversionNumber = 1;						// FRAM Version - RESETS ALL DEFAULTS
int tempFRAMversionNumber = 0;

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
    systemStatusAddr      = 0x12						// address 18 systemStatusAddr
  };
};

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

int sensorType = 1;										// Default sensorType
bool sensorSleepState = false;

char data[256]; 										// character array to build webhook payload

bool debug = false;										// TRUE blocks sleep mode
///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////
// Particle Includes
//
#include "sys_status.h"

#include "MCP79410RK.h"
#include "MB85RC256V-FRAM-RK.h"

#include "MPU6050.h"

#include "OneWire.h"
#include "Adafruit_Sensor.h"
#include "Adafruit_ADXL343.h"
#include "Adafruit_ADT7410.h"
///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////
// System Status
//
struct systemStatus_structure sysStatus;
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
const int PIRPin = A3;									// PIR Sensor Digital
const int tmp36Pin = A4;								// Simple Analog temperature sensor
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
Adafruit_ADXL343 ADXL343 = Adafruit_ADXL343(12345);
float ax_ADXL343;
float ay_ADXL343;
float az_ADXL343;

// Create the ADT7410 temperature sensor object
Adafruit_ADT7410 ADT7410 = Adafruit_ADT7410();
///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////
// Sensortype 4
//
// Tilt Magnetometer
//
MPU6050 mpu6050;
int16_t ax_MPU6050_Reading, ay_MPU6050_Reading, az_MPU6050_Reading;
float ax_MPU6050_Average, ay_MPU6050_Average, az_MPU6050_Average;

int16_t gx_MPU6050_Reading, gy_MPU6050_Reading, gz_MPU6050_Reading;
float gx_MPU6050_Average, gy_MPU6050_Average, gz_MPU6050_Average;

float axDegrees_MPU6050_Average, ayDegrees_MPU6050_Average;

float c_Reading;
float c_Average;
//
///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////
// Sensortype 5 & 6
// A02YYUW Sonic
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

// Integer to store distance in mm
int distance;
int distanceSum;
int distanceFinal;

// Variable to hold checksum
unsigned char CS;
//
///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////
// Sensortype 7 Capcitance
//
const int CAPACITANCE_SIGNAL_1 = A0;
//
///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////
// Sensortype 8 Resistance
//
const int RESISTANCE_SIGNAL_1 = A1;
//
///////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////
// Program Variables
//
///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
// STATE Machine
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

const int MIN_COUNT_TEST_SAMPLING = 3; 									// Min 1 sampling tries
const int MAX_COUNT_TEST_SAMPLING = 10; 								// Max 10 sampling tries
const int MAX_TIME_TO_WAIT_FOR_CONNECT_MS = (5 * 60 * 1000); 			// Wait for [5] minutes trying to connect to the Particle.io
const int MAX_TIME_TO_SAMPLE_SETUP_WAIT = (1 * 1000); 					// Wait for [1] second before sampling
const int MAX_TIME_TO_WAIT_PUBLISH_MS = (1 * 1000); 					// Wait for [1] seconds before publishing
unsigned long MAX_TIME_TO_WAIT_BEFORE_SLEEP_MS = (60 * 1000);			// After publish, wait [1] minute before sleeping
const int MAX_TIME_TO_SLEEP_SEC = (15 * 60); 							// sleep for [15] minutes in seconds
const int BATTERY_SAVER_MODE_SEC = (45 * 60); 							// sleep for [60] minutes in seconds
unsigned long MAX_TIME_TO_SLEEP_MS = (MAX_TIME_TO_SLEEP_SEC * 1000);	// sleep for [15] minutes in milliseconds

byte pirState = 0;														// Store the current state for the tests that might cause a reset
byte rtcState = 0;														// Store the current state for the tests that might cause a reset
byte watchdogState = 0;													// Store the current state for the tests that might cause a reset

char resultStr[64];
char SignalString[64];													// Used to communicate Wireless RSSI and Description

int sampleCounter;

int capacitanceSampleValue = 0;
int capacitanceSampleSum = 0;
int capacitanceSampleAverage = 0;
int capacitanceSamplePercentage = 0;

int minValue = 0;
int maxValue = 4100;

float carrierBoardTempSensor = 0;
float carrierBoardTempSensorSum = 0;
float carrierBoardTempSensorC = 0;
float carrierBoardTempSensorF = 0;

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


///////////////////////////////////////////////////////////////////////////
// Hard Reset
//
int hardResetNow(String command)                                      // Will perform a hard reset on the device
{
  if (command == "1")
  {
    Particle.publish("Reset","Hard Reset in 2 seconds",PRIVATE);
    delay(2000);
    ab1805.deepPowerDown(10);                                         // Power cycles the Boron and carrier board
    return 1;                                                         // Unfortunately, this will never be sent
  }
  else return 0;
}
///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////
// Set The Sensor Type
//
int setSensorState(String command) {
	int tempSetSensorStateFlag = 0;											// By default tempSetSensorFlag is 1 = OK

	if(command == "0"){
		sensorSleepState = false;
		tempSetSensorStateFlag = 1;
	} else if(command == "1"){
		sensorSleepState = true;
		tempSetSensorStateFlag = 1;
	}

	return tempSetSensorStateFlag;
}

int setSensorType(String command) {
	int tempSetSensorFlag = 1;											// By default tempSetSensorFlag is 1 = OK

	if(command == "0"){
		sysStatus.sensorType = 0;
	} else if(command == "1"){
		sysStatus.sensorType = 1;
	} else if(command == "2"){
		sysStatus.sensorType = 2;
	} else if(command == "3"){
		sysStatus.sensorType = 3;
	} else if(command == "4"){
		sysStatus.sensorType = 4;
	} else if(command == "5"){
		sysStatus.sensorType = 5;
	} else if(command == "6"){
		sysStatus.sensorType = 6;
	} else if(command == "7"){
		sysStatus.sensorType = 7;
	} else if(command == "8"){
		sysStatus.sensorType = 8;
	} else {
		tempSetSensorFlag = 0;
	}

	if(tempSetSensorFlag == 1){
		fram.put(FRAM::systemStatusAddr, sysStatus);                         		// Write it now since this is a big deal and I don't want values over written

		struct systemStatus_structure tempSysStatus;
		fram.get(FRAM::systemStatusAddr, tempSysStatus);                          	// See if this worked
		
		if(tempSysStatus.sensorType == sysStatus.sensorType){
			sensorType = sysStatus.sensorType;										// Set sensorType from sysStatus.sensorType

			snprintf(data, sizeof(data), "{\"Sensor Type updated: \":%d}", sensorType);
			logData("OK", data, true, true);

			tempSetSensorFlag = 1;

			startSampling("OK");
		} else {
			snprintf(data, sizeof(data), "{\"Sensor Type NOT updated: \":%d}", sensorType);

			logData("ERROR_STATE", data, true, true);
			tempSetSensorFlag = 0;
		}	
	}

	return tempSetSensorFlag;
}
///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////
// Load System Defaults
//
int saveSystemDefaults() {                                           // Default settings for the device - connected, not-low power and always on
	logData("MODE", "Saving system defaults", true, true);

	fram.erase();                                                      	// Reset the FRAM to correct the issue
	fram.put(FRAM::versionAddr, FRAMversionNumber);                    	// Put the right value in
	fram.get(FRAM::versionAddr, tempFRAMversionNumber);

	sysStatus.structuresVersion = 1;
	sysStatus.verboseMode = false;
	sysStatus.verboseCounts = false;
	sysStatus.lowBatteryMode = false;
	sysStatus.timezone = -5;                                            // Default is East Coast Time
	sysStatus.dstOffset = 1;
	sysStatus.solarPowerMode = true;
	sysStatus.lastConnectionDuration = 0;                               // New measure
	sysStatus.sensorType = 1;											// By default - no sensor
	fram.put(FRAM::systemStatusAddr,sysStatus);                         // Write it now since this is a big deal and I don't want values over written

	return tempFRAMversionNumber;
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
    Particle.function("SetSensorState", setSensorState);
	Particle.variable("SensorType", sensorType);
	Particle.variable("Hard Reset", hardResetNow);

	pinMode(userSwitch, INPUT);										// Button for user input
	pinMode(wakeUpPin, INPUT_PULLDOWN);								// This pin is active HIGH
	pinMode(BUILT_IN_LED, OUTPUT);									// declare the Blue LED Pin as an output
	pinMode(donePin, OUTPUT);										// Allows us to pet the watchdog
	digitalWrite(donePin, HIGH);
	digitalWrite(donePin, LOW);										// Pet the watchdog
	pinMode(DeepSleepPin , OUTPUT);									// For a hard reset active HIGH

	pinMode(PIRPin, INPUT_PULLDOWN);								// PIR Sensor Digital

	pinMode(CAPACITANCE_SIGNAL_1, INPUT_PULLDOWN);					// Cap PCB
	pinMode(RESISTANCE_SIGNAL_1, INPUT_PULLDOWN);					// Cap PCB

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

						// Print to serial monitor
						snprintf(data, sizeof(data), "Distance: %d mm", distance);
						logData("Distance", data, true, false);
					}
				}
			}
			break;
	}

	switch(state){
		case PARTICLE_CONNECT_WAIT_STATE:
			if(Particle.connected()){
				logData("PARTICLE", "PARTICLE CONNECTED", true, true);

				fram.get(FRAM::versionAddr, tempFRAMversionNumber);
				if(tempFRAMversionNumber == FRAMversionNumber){
					snprintf(data, sizeof(data), "FRAMversionNumber = %d, tempFRAMversionNumber = %d", FRAMversionNumber, tempFRAMversionNumber);
					logData("tempFRAMversionNumber RESULT", data, true, false);
				} else {
					logData("OK", "Device will load defaults to FRAM", true, false);

					tempFRAMversionNumber = saveSystemDefaults();
					if(tempFRAMversionNumber == FRAMversionNumber){
						snprintf(data, sizeof(data), "FRAMversionNumber = %d, tempFRAMversionNumber = %d", FRAMversionNumber, tempFRAMversionNumber);
						logData("tempFRAMversionNumber RESULT", data, true, false);
					} else {
						logData("ERROR_STATE", "Device will not work without FRAM", true, true);
					}
				}

				if(tempFRAMversionNumber == FRAMversionNumber){
					fram.get(FRAM::systemStatusAddr, sysStatus);                        	// Loads the System Status array from FRAM
					if(sysStatus.sensorType > -1 && sysStatus.sensorType < 9){
						sensorType = sysStatus.sensorType;									// Set sensorType from sysStatus.sensorType
					}
					snprintf(data, sizeof(data), "SensorType = %d", sensorType);
					logData("FRAM SensorType RESULT", data, true, true);

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
				}

				if(sensorType == 4){
					mpu6050.initialize();
					// mpu6050.setClockSource(MPU6050_CLOCK_PLL_XGYRO);
					// mpu6050.setFullScaleGyroRange(MPU6050_GYRO_FS_250);
					// mpu6050.setFullScaleAccelRange(MPU6050_ACCEL_FS_2);
					// mpu6050.setSleepEnabled(sensorSleepState);

					delay(500);

					if(mpu6050.getSleepEnabled()){
						logData("mpu6050.getSleepEnabled", "True", true, false);
					} else {
						logData("mpu6050.getSleepEnabled", "False", true, false);
					}
				}

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

				ax_ADXL343 = 0;
				ay_ADXL343 = 0;
				az_ADXL343 = 0;

				c_Reading = 0;
				c_Average = 0;

				ax_MPU6050_Reading = 0;
				ay_MPU6050_Reading = 0;
				az_MPU6050_Reading = 0;
				ax_MPU6050_Average = 0;
				ay_MPU6050_Average = 0;
				az_MPU6050_Average = 0;
				axDegrees_MPU6050_Average = 0;
				ayDegrees_MPU6050_Average = 0;

				capacitanceSampleSum = 0;
				capacitanceSampleAverage = 0;
				capacitanceSamplePercentage = 0;

				switch(sensorType){
					case 0:	// Debug
						debug = true;
						digitalWrite(BUILT_IN_LED,HIGH);									// If sensorType is N/A then turn on BLUELED
						break;
					case 1:	// CB
						break;
					case 2:	// Tilt Sensor
					case 3:	// Tilt Deluxe
						ADXL343.begin();
						delay(50);
						ADXL343.setRange(ADXL343_RANGE_2_G);
						if(sensorType == 3) ADT7410.begin();
						break;
					case 4:	// Tilt/Magnetometer
						break;
					case 5:	// A02YYUW
					case 6:	// JSN-SR04T
						distanceSum = 0;
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

			switch(sensorType){
				case 0:	// Debug
					break;
				case 1:	// CB
					break;
				case 2:	// Tilt Sensor
				case 3:	// Tilt Deluxe
					// Get a new Acceleromter event
					sensors_event_t event;
					ADXL343.getEvent(&event);

					ax_ADXL343 = ax_ADXL343 + event.acceleration.x;
					ay_ADXL343 = ay_ADXL343 + event.acceleration.y;
					az_ADXL343 = az_ADXL343 + event.acceleration.z;
					delay(50);

					if(sensorType == 3){
						c_Reading = ADT7410.readTempC();               			// Read ADT7410 temp
						
						if(c_Reading > -20 && c_Reading < 40){
							c_Average = c_Average + c_Reading;
						}
					}
					break;
				case 4:	// Tilt/Magnetometer
					if(mpu6050.testConnection()){
						// read raw accel/gyro measurements from device
						mpu6050.getMotion6(&ax_MPU6050_Reading, &ay_MPU6050_Reading, &az_MPU6050_Reading, &gx_MPU6050_Reading, &gy_MPU6050_Reading, &gz_MPU6050_Reading);
						c_Reading = (mpu6050.getTemperature() / 340.00) + 36.53;

						// snprintf(data, sizeof(data), "x: %d, y: %d, z: %d, gx: %d, gy: %d, gz: %d, c_Reading: %.2f", ax_MPU6050_Reading, ay_MPU6050_Reading, az_MPU6050_Reading, gx_MPU6050_Reading, gy_MPU6050_Reading, gz_MPU6050_Reading, c_Reading);
						// logData("MPU6050 Readings", data, true, true);

						ax_MPU6050_Average = ax_MPU6050_Average + ax_MPU6050_Reading;
						ay_MPU6050_Average = ay_MPU6050_Average + ay_MPU6050_Reading;
						az_MPU6050_Average = az_MPU6050_Average + az_MPU6050_Reading;

						gx_MPU6050_Average = gx_MPU6050_Average + gx_MPU6050_Reading;
						gy_MPU6050_Average = gy_MPU6050_Average + gy_MPU6050_Reading;
						gz_MPU6050_Average = gz_MPU6050_Average + gz_MPU6050_Reading;

						c_Average = c_Average + c_Reading;
					}
					break;
				case 5:	// A02YYUW
				case 6:	// JSN-SR04T
						distanceSum = distanceSum + distance;
						// Print to serial monitor
						// snprintf(data, sizeof(data), "Distance: %d mm", distance);
						// logData("Distance", data, true, true);
					break;
				case 7:	// Capacitance
					capacitanceSampleValue = analogRead(CAPACITANCE_SIGNAL_1);

					// snprintf(data, sizeof(data), "{\"capacitance\":%d}", capacitanceSampleValue);
					// logData("Cap", data, true, true);

					capacitanceSampleSum = capacitanceSampleSum + capacitanceSampleValue;
					break;
				case 8:	// Resistance
					break;
			}

			if(sampleCounter++ > MAX_COUNT_TEST_SAMPLING){
				stateTime = millis();

				state = SAMPLE_PROCESS_STATE;
			}
			break;

		case SAMPLE_PROCESS_STATE:
				batteryState = System.batteryState();

				if(getTemperature()){
					carrierBoardTempSensorC = carrierBoardTempSensor;
					// carrierBoardTempSensorC = carrierBoardTempSensorSum / MAX_COUNT_TEST_SAMPLING;
				} else {
					carrierBoardTempSensorC = 0;
				}

				switch(sensorType){
					case 0:	// Debug
						break;
					case 1:	// No Sensor
						break;
					case 2:	// Tilt Sensor
					case 3: // Tilt Deluxe
						ax_ADXL343 = ax_ADXL343 / MAX_COUNT_TEST_SAMPLING;
						ay_ADXL343 = ay_ADXL343 / MAX_COUNT_TEST_SAMPLING;
						az_ADXL343 = az_ADXL343 / MAX_COUNT_TEST_SAMPLING;

						c_Average = c_Average / MAX_COUNT_TEST_SAMPLING;
						break;
					case 4:	// Tilt/Magnetometer
						// snprintf(data, sizeof(data), "x: %d, y: %d, z: %d, gx: %d, gy: %d, gz: %d, c_Average: %.2f", ax_MPU6050_Average, ay_MPU6050_Average, az_MPU6050_Average, gx_MPU6050_Average, gy_MPU6050_Average, gz_MPU6050_Average, c_Average);
						// logData("MPU6050 Readings", data, true, true);

						ax_MPU6050_Average = ax_MPU6050_Average / MAX_COUNT_TEST_SAMPLING;
						ay_MPU6050_Average = ay_MPU6050_Average / MAX_COUNT_TEST_SAMPLING;
						az_MPU6050_Average = az_MPU6050_Average / MAX_COUNT_TEST_SAMPLING;

						gx_MPU6050_Average = gx_MPU6050_Average / MAX_COUNT_TEST_SAMPLING;
						gy_MPU6050_Average = gy_MPU6050_Average / MAX_COUNT_TEST_SAMPLING;
						gz_MPU6050_Average = gz_MPU6050_Average / MAX_COUNT_TEST_SAMPLING;

						if(!(ax_MPU6050_Average == 0 && ay_MPU6050_Average == 0 && az_MPU6050_Average == 0)){
							axDegrees_MPU6050_Average = (atan(ay_MPU6050_Average / sqrt(pow(ax_MPU6050_Average, 2) + pow(az_MPU6050_Average, 2))) * 180 / PI) - 0.58; // AccErrorX ~(0.58) See the calculate_IMU_error()custom function for more details
							ayDegrees_MPU6050_Average = (atan(-1 * ax_MPU6050_Average / sqrt(pow(ay_MPU6050_Average, 2) + pow(az_MPU6050_Average, 2))) * 180 / PI) + 1.58; // AccErrorY ~(-1.58)
						}

						c_Average = c_Average / MAX_COUNT_TEST_SAMPLING;

						// snprintf(data, sizeof(data), "x: %d, y: %d, z: %d, gx: %d, gy: %d, gz: %d, c_Average: %.2f", ax_MPU6050_Average, ay_MPU6050_Average, az_MPU6050_Average, gx_MPU6050_Average, gy_MPU6050_Average, gz_MPU6050_Average, c_Average);
						// logData("MPU6050 Final Readings", data, true, true);
						break;
					case 5:	// A02YYUW
					case 6:	// JSN-SR04T
						distanceFinal = distanceSum / MAX_COUNT_TEST_SAMPLING;
						break;
					case 7:	// Capacitance
						capacitanceSampleAverage = capacitanceSampleSum / MAX_COUNT_TEST_SAMPLING;
						capacitanceSamplePercentage = capacitanceSampleAverage / 4096;
						break;
					case 8:	// Resistance
						break;
				}

				stateTime = millis();

				state = PUBLISH_WAIT_STATE;
			break;

		case PUBLISH_WAIT_STATE:
			if(millis() - stateTime >= MAX_TIME_TO_WAIT_PUBLISH_MS){
				logData("PUBLISHING", "START PUBLISH_STATE", true, false);

				stateTime = millis();

				state = PUBLISH_STATE;
			}
			break;

		case PUBLISH_STATE:
			switch(sensorType){
				case 0:	// Debug
					snprintf(data, sizeof(data), "{\"cb_temp\":%.2f, \"battery\":%.2f, \"batterystate\":\"%s\", \"timestamp\":%lu000}", carrierBoardTempSensorC, fuel.getVCell(), batteryStates[std::max(0, batteryState)], Time.now());
					break;
				case 1:	// No Sensor
					snprintf(data, sizeof(data), "{\"cb_temp\":%.2f, \"battery\":%.2f, \"batterystate\":\"%s\", \"timestamp\":%lu000}", carrierBoardTempSensorC, fuel.getVCell(), batteryStates[std::max(0, batteryState)], Time.now());
					break;
				case 2:	// ADXL343 Accelerometer
					snprintf(data, sizeof(data), "{\"cb_temp\":%.2f, \"accel_x\":%.2f, \"accel_y\":%.2f, \"accel_z\":%.2f, \"battery\":%.2f, \"batterystate\":\"%s\", \"timestamp\":%lu000}", carrierBoardTempSensorC, ax_ADXL343, ay_ADXL343, az_ADXL343, fuel.getVCell(), batteryStates[std::max(0, batteryState)], Time.now());
					break;
				case 3:	// ADXL343 Accelerometer with ADT7410 Temp Sensor
					snprintf(data, sizeof(data), "{\"cb_temp\":%.2f, \"sensor_temp\":%.2f, \"accel_x\":%.2f, \"accel_y\":%.2f, \"accel_z\":%.2f, \"battery\":%.2f, \"batterystate\":\"%s\", \"timestamp\":%lu000}", carrierBoardTempSensorC, c_Average, ax_ADXL343, ay_ADXL343, az_ADXL343, fuel.getVCell(), batteryStates[std::max(0, batteryState)], Time.now());
					break;
				case 4:	// 6050 Accelerometer
					snprintf(data, sizeof(data), "{\"cb_temp\":%.2f, \"sensor_temp\":%.2f, \"accel_x\":%.2f, \"accel_y\":%.2f, \"accel_z\":%.2f, \"g_x\":%.2f, \"g_y\":%.2f, \"g_z\":%.2f, \"degrees_x\":%.2f, \"degrees_y\":%.2f, \"battery\":%.2f, \"batterystate\":\"%s\", \"timestamp\":%lu000}", carrierBoardTempSensorC, c_Average, ax_MPU6050_Average, ay_MPU6050_Average, az_MPU6050_Average, gx_MPU6050_Average, gy_MPU6050_Average, gz_MPU6050_Average, axDegrees_MPU6050_Average, ayDegrees_MPU6050_Average, fuel.getVCell(), batteryStates[std::max(0, batteryState)], Time.now());
					break;
				case 5:	// A02YYUW Sonic Sensor
					snprintf(data, sizeof(data), "{\"cb_temp\":%.2f, \"distance\":%d, \"battery\":%.2f, \"batterystate\":\"%s\", \"timestamp\":%lu000}", carrierBoardTempSensorC, distance, fuel.getVCell(), batteryStates[std::max(0, batteryState)], Time.now());
					break;
				case 6:	// JSN-SR04T Sonic Sensor
					snprintf(data, sizeof(data), "{\"cb_temp\":%.2f, \"distance\":%d, \"battery\":%.2f, \"batterystate\":\"%s\", \"timestamp\":%lu000}", carrierBoardTempSensorC, distance, fuel.getVCell(), batteryStates[std::max(0, batteryState)], Time.now());
					break;
				case 7:	// Capacitance Sensor
					snprintf(data, sizeof(data), "{\"cb_temp\":%.2f, \"capacitance_raw\":%d, \"capacitance_percent\":%d, \"battery\":%.2f, \"batterystate\":\"%s\", \"timestamp\":%lu000}", carrierBoardTempSensorC, capacitanceSampleAverage, capacitanceSamplePercentage, fuel.getVCell(), batteryStates[std::max(0, batteryState)], Time.now());
					break;
				case 8:	// Resistance Sensor
					snprintf(data, sizeof(data), "{\"cb_temp\":%.2f, \"capacitance_raw\":%d, \"capacitance_percent\":%d, \"battery\":%.2f, \"batterystate\":\"%s\", \"timestamp\":%lu000}", carrierBoardTempSensorC, capacitanceSampleAverage, capacitanceSamplePercentage, fuel.getVCell(), batteryStates[std::max(0, batteryState)], Time.now());
					break;
			}

			switch(sensorType){
				case 3:
				case 4:
					Particle.publish("Ubidots-MPU6050", data, PRIVATE);			// Note, you can add Publish Queue POSIX later - let's get this going first
					break;
				case 5:
				case 6:
					Particle.publish("Ubidots-Level-Sonic", data, PRIVATE);			// Note, you can add Publish Queue POSIX later - let's get this going first
					break;
				default:
					Particle.publish("Ubidots-Minimum", data, PRIVATE);			// Note, you can add Publish Queue POSIX later - let's get this going first
					break;
			}

			logData("PUBLISH_STATE", "START SLEEP_WAIT_STATE", true, true);

			// digitalWrite(BUILT_IN_LED, LOW);

			stateTime = millis();

			state = SLEEP_WAIT_STATE;
			break;		

		case SLEEP_WAIT_STATE:
			if(millis() - stateTime >= MAX_TIME_TO_WAIT_BEFORE_SLEEP_MS){
				switch(sensorType){
					case 4:	// Accel 6050
						sensorSleepState = true;
						mpu6050.setSleepEnabled(sensorSleepState);
						if(mpu6050.getSleepEnabled()){
							logData("mpu6050.getSleepEnabled", "True", true, false);
						} else {
							logData("mpu6050.getSleepEnabled", "False", true, false);
						}
						break;
				}				

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

  switch(ADXL343.getDataRate())
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

  switch(ADXL343.getRange())
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
	carrierBoardTempSensorF = (carrierBoardTempSensor * 9.0 / 5.0) + 32.0;  // now convert to Fahrenheit

	if(carrierBoardTempSensor < -20.0 || carrierBoardTempSensor > 30.0) {             // Reasonable range for garage temperature
		// snprintf(resultStr, sizeof(resultStr),"Temp seems whack: %3.1f", carrierBoardTempSensorF);
		return 0;
	}
	else {
		// snprintf(resultStr, sizeof(resultStr),"Temperature is: %3.1f", carrierBoardTempSensorF);
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