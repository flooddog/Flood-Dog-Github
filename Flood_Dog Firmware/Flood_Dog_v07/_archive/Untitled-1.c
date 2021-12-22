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
PRODUCT_VERSION(21);
char currentPointRelease[5] ="21";				// With Accelerometer!
// 
///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////
// Particle
//
SYSTEM_THREAD(ENABLED)

SerialLogHandler logHandler();			// Sends everything
///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////
// DEBUG Mode turns on/off the sleep option
//
bool debug = false;

int sensorType = 1;									// Sensor Type: 0 = n/a, 1 = TiltSensor1, 2 = A02YYUW dual sonic, 3 = GY-521 TiltSensor2
///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////
// Particle Includes
//
#include "MCP79410RK.h"
#include "MB85RC256V-FRAM-RK.h"

#include "Adafruit_Sensor.h"
#include "Adafruit_ADXL343.h"

// #include "HC-SR04.h"
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
const int ANALOG_DETECTOR = A3;							// Capacitance Probe
const int tmp36Pin = A4;								// Simple Analog temperature sensor
const int userSwitch = D4;								// User switch with a pull-up resistor
const int donePin = D5;									// Pin the Electron uses to "pet" the watchdog
const int DeepSleepPin = D6;							// Power Cycles the Particle Device and the Carrier Board only RTC Alarm can wake
const int BUILT_IN_LED = D7;     						// Built In LED
const int wakeUpPin = D8;								// This is the Particle Electron WKP pin
///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////
// A02YYUW Sonic
//
// #define TRIGPIN TX
// #define ECHOPIN RX

// // Floats to calculate distance
// float duration, distance;
// const int triggerPin = A0;
// const int echoPin = D0;
// HC_SR04 rangefinder = HC_SR04(triggerPin, echoPin);

// unsigned long start;
// float inch;
// unsigned long calcTime;
///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////
// GY-521 Sonic
//
// const int MPU_ADDR = 0x68; // I2C address of the MPU-6050. If AD0 pin is set to HIGH, the I2C address will be 0x69.

// int16_t accelerometer_x, accelerometer_y, accelerometer_z; // variables for accelerometer raw data
// int16_t gyro_x, gyro_y, gyro_z; // variables for gyro raw data
// int16_t temperature; // variables for temperature data

// char tmp_str[7]; // temporary variable used in convert function

// char* convert_int16_to_str(int16_t i) { // converts int16 to string. Moreover, resulting strings will have the same length in the debug monitor.
//   sprintf(tmp_str, "%6d", i);
//   return tmp_str;
// }
///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////
// ADXL343 Accelerometer
//
#define ADXL343_SCK 13
#define ADXL343_MISO 12
#define ADXL343_MOSI 11
#define ADXL343_CS 10

/* Assign a unique ID to this sensor at the same time */
/* Uncomment following line for default Wire bus      */
Adafruit_ADXL343 accel = Adafruit_ADXL343(12345);

float tiltX;
float tiltY;
float tiltZ;
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
		SAMPLING_WAIT_STATE,
		SAMPLE_STATE,
		PUBLISH_WAIT_STATE,
		PUBLISH_STATE,
		SLEEP_WAIT_STATE,
		SLEEP_STATE
	};
State state = PARTICLE_CONNECT_WAIT_STATE;
unsigned long stateTime = 0;

byte pirState = 0;													// Store the current state for the tests that might cause a reset
byte rtcState = 0;													// Store the current state for the tests that might cause a reset
byte watchdogState = 0;												// Store the current state for the tests that might cause a reset

char resultStr[64];
char SignalString[64];												// Used to communicate Wireless RSSI and Description

const int MAX_COUNT_TEST_SAMPLING = 3; 								// 3 sampling tries
const int MAX_TIME_TO_WAIT_FOR_CONNECT_MS = (5 * 60 * 1000); 		// Wait for [5] minutes trying to connect to the Particle.io
const int MAX_TIME_TO_SAMPLE_WAIT = (1 * 1000); 					// Wait for [1] second before sampling
const int MAX_TIME_TO_WAIT_PUBLISH_MS = (1 * 1000); 				// Wait for [1] seconds before publishing
const int MAX_TIME_TO_WAIT_BEFORE_SLEEP_MS = (1 * 60 * 1000);		// After publish, wait [1] minute before sleeping
const int MAX_TIME_TO_SLEEP_SEC = (15 * 60); 						// sleep for [15] minutes in seconds
const int MAX_TIME_TO_SLEEP_MS = (MAX_TIME_TO_SLEEP_SEC * 1000);	// sleep for [15] minutes in milliseconds


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
  };
};

const char releaseNumber[6] = "0.7";					// Displays the release on the menu ****  this is not a production release ****
const int FRAMversionNumber = 1;

int minValue = 0;
int maxValue = 4100;

float analogValue = 0;
String analogValueString = "0";
float digitalValue = 0;
String digitalValueString = "0";

int sampleCounter = 0;
int sampleSum = 0;
int sampleTotalCount = 10;
float sampleAverage = 0;

float temperatureC = 0;
float temperatureF = 0;
String temperatureStringC = "0";
String temperatureStringF = "0";

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

PMIC pmic;
SystemPowerConfiguration conf;

FuelGauge fuel;
String VCellString;
String SoCString;
///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////
// Program Functions
//
///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
// PMIC Setup
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
// Particle Function to start sampling values in debug for Dev_Dog_001
//
int startSampling(String tempStart) {
	stateTime = millis();
	state = SAMPLING_WAIT_STATE;
	return 1;
}
///////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
// Particle function to handle the trimmed response code back from the webhooks

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
// Boron Setup
//
void setup() {
	Serial.begin(9600);
	// Serial1.begin(9600);

	// pinMode(ECHOPIN, INPUT);
	// pinMode(TRIGPIN, OUTPUT);

	// **********************
	// Apply a custom power configuration
	setupPMIC();
	// **********************

    Particle.function("StartSampling", startSampling);

	pinMode(userSwitch, INPUT);										// Button for user input
	pinMode(wakeUpPin, INPUT_PULLDOWN);								// This pin is active HIGH
	pinMode(BUILT_IN_LED, OUTPUT);									// declare the Blue LED Pin as an output
	pinMode(donePin, OUTPUT);										// Allows us to pet the watchdog
	digitalWrite(donePin, HIGH);
	digitalWrite(donePin, LOW);										// Pet the watchdog
	pinMode(DeepSleepPin , OUTPUT);									// For a hard reset active HIGH

	pinMode(ANALOG_DETECTOR, INPUT_PULLDOWN);						// This pin is active HIGH

	pinMode(PIRPin, INPUT_PULLDOWN);								// PIR Sensor Digital
	digitalWrite(PIRPin, LOW);										// SET PIR Sensor Digital

	// detachInterrupt(LOW_BAT_UC);
	// // Delay for up to two system power manager loop invocations
	// delay(2000);

	Time.zone(-5 + Time.getDSTOffset());

	rtc.setup();

	fram.begin();									  				// Initializes Wire but does not return a boolean on successful initialization

	attachInterrupt(wakeUpPin, watchdogISR, RISING);  				// Need to pet the watchdog when needed
	watchdogISR();

	switch(sensorType){
		case 0:	// No Sensor
			break;
		case 1:	// Tilt Sensor
			/* Initialise the Accelerometer */
			// accel.begin();
			// delay(50);
			// // if(!accel.begin())
			// // {
			// // 	/* There was a problem detecting the ADXL343 ... check your connections */
			// // 	Serial.println("Ooops, no ADXL343 detected ... Check your wiring!");
			// // 	while(1);
			// // }
			// /* Set the range to whatever is appropriate for your project */
			// // accel.setRange(ADXL343_RANGE_16_G);
			// // accel.setRange(ADXL343_RANGE_8_G);
			// // accel.setRange(ADXL343_RANGE_4_G);
			// accel.setRange(ADXL343_RANGE_2_G);
			// /* Display some basic information on this sensor */
			// // accel.printSensorDetails();
			// // displayDataRate();
			// // displayRange();
			// Serial.println("");
			/* Accelerometer Setup end */
			break;
		case 2:	// Sonic Sensor
			break;
		case 3:	// Capacitance Sensor
			Wire.begin();
			Wire.beginTransmission(MPU_ADDR); // Begins a transmission to the I2C slave (GY-521 board)
			Wire.write(0x6B); // PWR_MGMT_1 register
			Wire.write(0); // set to zero (wakes up the MPU-6050)
			Wire.endTransmission(true);
			break;
	}

}
///////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////
// Boron Loop
//
void loop() {
	rtc.loop();									  				// Need to run this in the main loop

	switch(state){
		case PARTICLE_CONNECT_WAIT_STATE:
			if(Particle.connected()){
				logData("PARTICLE", "PARTICLE CONNECTED", true, true);

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

				stateTime = millis();

				state = SAMPLING_WAIT_STATE;
			} else {
				Particle.connect();

				if(millis() - stateTime > MAX_TIME_TO_WAIT_FOR_CONNECT_MS){
					logData("PARTICLE", "PARTICLE FAILED TO CONNECT", true, true);

					stateTime = millis();

					state = SLEEP_WAIT_STATE;
				}
			}
			break;

		case SAMPLING_WAIT_STATE:
			if(millis() - stateTime >= MAX_TIME_TO_SAMPLE_WAIT){
				logData("SAMPLING_WAIT_STATE", "START SAMPLING", true, true);

				sampleCounter = 1;
				sampleSum = 0;
				sampleAverage = 0;

				switch(sensorType){
					case 0:	// No Sensor
						break;
					case 1:	// Tilt Sensor
						tiltX = 0;
						tiltY = 0;
						tiltZ = 0;
						break;
					case 2:	// Sonic Sensor
						break;
					case 3:	// Capacitance Sensor
						break;
				}

				stateTime = millis();

				state = SAMPLE_STATE;
			}
			break;

		case SAMPLE_STATE:
			analogValue = analogRead(ANALOG_DETECTOR);

			sampleSum = sampleSum + analogValue;
			
			switch(sensorType){
				case 1:
					/* Acceleromter */
					/* Get a new sensor event */
					// sensors_event_t event;
					// accel.getEvent(&event);

					// tiltX = tiltX + event.acceleration.x;
					// tiltY = tiltY + event.acceleration.y;
					// tiltZ = tiltZ + event.acceleration.z;
					delay(50);
					break;
			}

			if(sampleCounter++ > sampleTotalCount){
				// digitalValue = map(analogValue, minValue, maxValue, 0, 100);

				sampleAverage = sampleSum / sampleTotalCount;

				switch(sensorType){
					case 0:	// No Sensor
						break;
					case 1:	// Tilt Sensor
						tiltX = tiltX / sampleTotalCount;
						tiltY = tiltY / sampleTotalCount;
						tiltZ = tiltZ / sampleTotalCount;

						/* Display the results (acceleration is measured in m/s^2) */
						// Serial.print("X: "); Serial.print(tiltX); Serial.print("  ");
						// Serial.print("Y: "); Serial.print(tiltY); Serial.print("  ");
						// Serial.print("Z: "); Serial.print(tiltZ); Serial.print("  ");Serial.println("m/s^2 ");
						// delay(50);
						break;
					case 2:	// Sonic Sensor
					    start = micros();
						rangefinder.distInch();
						calcTime = micros() - start;
						Serial.printf("Range finding duration: %lu | Distance in inches: %.2f\n", calcTime, inch);

						// Set the trigger pin LOW for 2uS
						// digitalWrite(TRIGPIN, LOW);
						// delayMicroseconds(2);

						// Set the trigger pin HIGH for 20us to send pulse
						// digitalWrite(TRIGPIN, HIGH);
						// delayMicroseconds(20);

						// // Return the trigger pin to LOW
						// digitalWrite(TRIGPIN, LOW);

						// // Measure the width of the incoming pulse
						// duration = pulseIn(ECHOPIN, HIGH);

						// // Determine distance from duration
						// // Use 343 metres per second as speed of sound
						// // Divide by 1000 as we want millimeters

						// distance = (duration / 2) * 0.343;

						// Print result to serial monitor
						// Serial.print("distance: ");
						// Serial.print(distance);
						// Serial.println(" mm");

						// Delay before repeating measurement
						// delay(100);
  						break;
					case 3:	// Capacitance Sensor
						Wire.beginTransmission(MPU_ADDR);
						Wire.write(0x3B); // starting with register 0x3B (ACCEL_XOUT_H) [MPU-6000 and MPU-6050 Register Map and Descriptions Revision 4.2, p.40]
						Wire.endTransmission(false); // the parameter indicates that the Arduino will send a restart. As a result, the connection is kept active.
						Wire.requestFrom(MPU_ADDR, 7*2, true); // request a total of 7*2=14 registers
						
						// "Wire.read()<<8 | Wire.read();" means two registers are read and stored in the same variable
						accelerometer_x = Wire.read()<<8 | Wire.read(); // reading registers: 0x3B (ACCEL_XOUT_H) and 0x3C (ACCEL_XOUT_L)
						accelerometer_y = Wire.read()<<8 | Wire.read(); // reading registers: 0x3D (ACCEL_YOUT_H) and 0x3E (ACCEL_YOUT_L)
						accelerometer_z = Wire.read()<<8 | Wire.read(); // reading registers: 0x3F (ACCEL_ZOUT_H) and 0x40 (ACCEL_ZOUT_L)
						temperature = Wire.read()<<8 | Wire.read(); // reading registers: 0x41 (TEMP_OUT_H) and 0x42 (TEMP_OUT_L)
						gyro_x = Wire.read()<<8 | Wire.read(); // reading registers: 0x43 (GYRO_XOUT_H) and 0x44 (GYRO_XOUT_L)
						gyro_y = Wire.read()<<8 | Wire.read(); // reading registers: 0x45 (GYRO_YOUT_H) and 0x46 (GYRO_YOUT_L)
						gyro_z = Wire.read()<<8 | Wire.read(); // reading registers: 0x47 (GYRO_ZOUT_H) and 0x48 (GYRO_ZOUT_L)
						
						// print out data
						Serial.print("aX = "); Serial.print(convert_int16_to_str(accelerometer_x));
						Serial.print(" | aY = "); Serial.print(convert_int16_to_str(accelerometer_y));
						Serial.print(" | aZ = "); Serial.print(convert_int16_to_str(accelerometer_z));
						// the following equation was taken from the documentation [MPU-6000/MPU-6050 Register Map and Description, p.30]
						Serial.print(" | tmp = "); Serial.print(temperature/340.00+36.53);
						Serial.print(" | gX = "); Serial.print(convert_int16_to_str(gyro_x));
						Serial.print(" | gY = "); Serial.print(convert_int16_to_str(gyro_y));
						Serial.print(" | gZ = "); Serial.print(convert_int16_to_str(gyro_z));
						Serial.println();
						
						// delay
						delay(1000);
						break;
				}


				stateTime = millis();

				state = PUBLISH_WAIT_STATE;
			}
			break;

		case PUBLISH_WAIT_STATE:
			if(millis() - stateTime >= MAX_TIME_TO_WAIT_PUBLISH_MS){
				getTemperature();

				// **********************
				if(temperatureC > 0){
					logData("POWER", "ENBABLE CHARGING", true, true);
					pmic.enableCharging();
				} else {
					logData("POWER", "DISABLE CHARGING", true, true);
					pmic.disableCharging();
				}
				// **********************

				logData("PUBLISHING", "START PUBLISH_STATE", true, true);

				stateTime = millis();

				state = PUBLISH_STATE;
			}
			break;

		case PUBLISH_STATE:
			char data[256];										// character array to build webhook payload

			// Need to write these so they assign a value to a variable - don't need to assign Strings
			powerSource = System.powerSource();
			batteryState = System.batteryState();
			powerSString = powerSources[std::max(0, powerSource)];
			powerBSString = batteryStates[std::max(0, batteryState)];
			// powerOn = powerSources[std::max(0, powerSource)];
			// batterySocString = String(System.batteryCharge(), 2);
			// VCellString = String(fuel.getVCell(), 2);
			// SoCString = String(fuel.getSoC(), 2);

			// logData("Power sources",  powerSString, true, true);
			// logData("Battery state", powerBSString, true, true);
			// logData("Battery charge", batterySocString, true, true);
			// logData("Battery VCell", VCellString, true, true);
			// logData("Battery SoC", SoCString, true, true);
			// , \"context\":{\"batterystate\":%s}
			//, batteryStates[std::max(0, batteryState)]

			switch(sensorType){
				case 0:	// No Sensor
					snprintf(data, sizeof(data), "{\"capacitance\":%.2f, \"internaltemp\":%.2f, \"battery\":%.2f, \"timestamp\":%lu000, \"batterystate\":\"%s\"}", analogValue, temperatureC, fuel.getVCell(), Time.now(), batteryStates[std::max(0, batteryState)]);
					break;
				case 1:	// Tilt Sensor
					snprintf(data, sizeof(data), "{\"capacitance\":%.2f, \"internaltemp\":%.2f, \"battery\":%.2f, \"tiltX\":%.2f, \"tiltY\":%.2f, \"tiltZ\":%.2f, \"timestamp\":%lu000, \"batterystate\":\"%s\"}", analogValue, temperatureC, fuel.getVCell(), tiltX, tiltY, tiltZ, Time.now(), batteryStates[std::max(0, batteryState)]);
					break;
				case 2:	// Sonic Sensor
						Serial.printf("Range finding duration: %lu | Distance in inches: %.2f\n", calcTime, inch);
					snprintf(data, sizeof(data), "{\"capacitance\":%.2f, \"internaltemp\":%.2f, \"battery\":%.2f, \"timestamp\":%lu000, \"batterystate\":\"%s\", \"duration\":%lu, \"distance\":%.2f}", analogValue, temperatureC, fuel.getVCell(), Time.now(), batteryStates[std::max(0, batteryState)], calcTime, inch);
					break;
				case 3:	// Capacitance Sensor
					snprintf(data, sizeof(data), "{\"capacitance\":%.2f, \"internaltemp\":%.2f, \"battery\":%.2f, \"tiltX\":%d, \"tiltY\":%d, \"tiltZ\":%d, \"timestamp\":%lu000, \"batterystate\":\"%s\"}", analogValue, temperatureC, fuel.getVCell(), accelerometer_x, accelerometer_y, accelerometer_z, Time.now(), batteryStates[std::max(0, batteryState)]);
					break;
			}

			Particle.publish("Ubidots-Hook-v2", data, PRIVATE);			// Note, you can add Publish Queue POSIX later - let's get this going first

			logData("PUBLISH_STATE", "START SLEEP_WAIT_STATE", true, true);

			digitalWrite(BUILT_IN_LED, LOW);

			stateTime = millis();

			state = SLEEP_WAIT_STATE;
			break;		

		case SLEEP_WAIT_STATE:
			if(millis() - stateTime >= MAX_TIME_TO_WAIT_BEFORE_SLEEP_MS){
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

						digitalWrite(BUILT_IN_LED, HIGH);

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

					state = SAMPLING_WAIT_STATE;
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
	temperatureC = (voltage - 0.5) * 100.0;  //converting from 10 mv per degree with 500 mV offset to degrees ((voltage - 500mV) times 100) - 5 degree calibration
	temperatureF = (temperatureC * 9.0 / 5.0) + 32.0;  // now convert to Fahrenheit

	if(temperatureC < -20.0 || temperatureC > 30.0) {             // Reasonable range for garage temperature
		// snprintf(resultStr, sizeof(resultStr),"Temp seems whack: %3.1f", temperatureF);
		return 0;
	}
	else {
		// snprintf(resultStr, sizeof(resultStr),"Temperature is: %3.1f", temperatureF);
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
