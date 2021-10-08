/******************************************************/
//       THIS IS A GENERATED FILE - DO NOT EDIT       //
/******************************************************/

#include "Particle.h"
#line 1 "/Users/philipchatterton/IoCP/Particle/Flood_Dog/Flood_Dog_v05/src/Flood_Dog_v05.ino"
////////////////////////////////
//                            //
//       Water Sensor         //
//                            //
//         Flood Dog          //
//                            //
//       Dog_Dev  V1.0        //
//                            //
////////////////////////////////

void setupPMIC();
void setup();
void loop();
void watchdogISR();
bool getTemperature();
bool logData(String tempEvent, String tempData, bool tempLogSerial, bool tempLogParticle);
bool meterParticlePublish(void);
void selectExternalMeshAntenna();
#line 11 "/Users/philipchatterton/IoCP/Particle/Flood_Dog/Flood_Dog_v05/src/Flood_Dog_v05.ino"
SYSTEM_THREAD(ENABLED)

#include "3rdGenDevicePinoutdoc.h"						// Documents pinout
#include "MCP79410RK.h"
#include "MB85RC256V-FRAM-RK.h"

PRODUCT_ID(15526);
PRODUCT_VERSION(3);

MCP79410 rtc;											// Rickkas MCP79410 libarary
MB85RC64 fram(Wire, 0);

// Pin Constants for Boron
const int PIRPin = A0;									// PIR Sensor Digital
const int ANALOG_DETECTOR = A3;							// 
const int tmp36Pin = A4;								// Simple Analog temperature sensor
const int userSwitch = D4;								// User switch with a pull-up resistor
const int donePin = D5;									// Pin the Electron uses to "pet" the watchdog
const int DeepSleepPin = D6;							// Power Cycles the Particle Device and the Carrier Board only RTC Alarm can wake
const int BUILT_IN_LED = D7;     							// Built In LED
const int wakeUpPin = D8;								// This is the Particle Electron WKP pin

bool debug = false;

// Program Variables
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

const int MAX_TIME_TO_WAIT_FOR_CONNECT_MS = (5 * 60 * 1000); 		// Wait for [5] minutes trying to connect to the Particle.io
const int MAX_TIME_TO_SAMPLE_WAIT = (1 * 1000); 					// Wait for [1] second before sampling
const int MAX_COUNT_TEST_SAMPLING = 3; 								// 3 sampling tries
const int MAX_TIME_TO_WAIT_PUBLISH_MS = (1 * 1000); 				// Wait for [1] seconds before publishing
const int MAX_TIME_TO_WAIT_BEFORE_SLEEP_MS = (2 * 60 * 1000); 		// After publish, wait [2] minutes for data to go out
const int MAX_TIME_TO_SLEEP_SEC = (45 * 60); 						// sleep for [15] minutes in seconds -- [1] second for debug
const int MAX_TIME_TO_SLEEP_MS = (MAX_TIME_TO_SLEEP_SEC * 1000);	// sleep for [15] minutes

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

int analogValue = 0;
String analogValueString = "0";
int digitalValue = 0;
String digitalValueString = "0";

int sampleCounter = 0;
int sampleSum = 0;
int sampleTotalCount = 1000;
float sampleAverage = 0;

float temperatureC = 0;
float temperatureF = 0;
String temperatureStringC = "0";
String temperatureStringF = "0";

// **********************
String getInputVoltageLimit;
String getInputCurrentLimit;
String getChargeCurrentValue;
String getChargeVoltageValue;

int powerSource;
int batteryState;

String feedName;
String feedID;
String feedData;



constexpr char const* batteryStates[] = {"unknown", "not charging", "charging", "charged", "discharging", "fault", "disconnected"};

constexpr char const* powerSources[] = {"unknown", "vin", "usb host", "usb adapter", "usb otg", "battery"};

String powerSString;
String powerBSString;
String batterySocString;

PMIC pmic;
SystemPowerConfiguration conf;

FuelGauge fuel;
String VCellString;
String SoCString;

void setupPMIC(){
	pmic.begin();
	// pmic.setInputVoltageLimit(5080);  // CAUSES ISSUE

    conf.powerSourceMaxCurrent(550) 
        .powerSourceMinVoltage(4300) 
        .batteryChargeCurrent(850) 
        .batteryChargeVoltage(4210);

    int res = System.setPowerConfiguration(conf);
    Log.info("setPowerConfiguration=%d", res);
    // returns SYSTEM_ERROR_NONE (0) in case of success
}
// **********************


void setup() {
	Serial.begin(115200);
	
	// **********************
	// Apply a custom power configuration
	setupPMIC();
	// **********************

	pinMode(userSwitch, INPUT);										// Button for user input
	pinMode(wakeUpPin, INPUT);										// This pin is active HIGH
	pinMode(BUILT_IN_LED, OUTPUT);										// declare the Blue LED Pin as an output
	pinMode(donePin, OUTPUT);										// Allows us to pet the watchdog
	digitalWrite(donePin, HIGH);
	digitalWrite(donePin, LOW);										// Pet the watchdog
	pinMode(DeepSleepPin , OUTPUT);									// For a hard reset active HIGH

	pinMode(PIRPin, INPUT);											// PIR Sensor Digital
	digitalWrite(PIRPin, LOW);										// SET PIR Sensor Digital

	// detachInterrupt(LOW_BAT_UC);
	// // Delay for up to two system power manager loop invocations
	// delay(2000);

	Time.zone(-5 + Time.getDSTOffset());

	rtc.setup();

	fram.begin();									  				// Initializes Wire but does not return a boolean on successful initialization

	attachInterrupt(wakeUpPin, watchdogISR, RISING);  				// Need to pet the watchdog when needed
	watchdogISR();
}

// loop() runs over and over again, as quickly as it can execute.
void loop() {
	rtc.loop();									  				// Need to run this in the main loop

	switch(state){
		case PARTICLE_CONNECT_WAIT_STATE:			
			if (Particle.connected()) {
				logData("PARTICLE", "PARTICLE CONNECTED", true, false);

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
					logData("PARTICLE", "PARTICLE FAILED TO CONNECT", true, false);
					
					stateTime = millis();

					state = SLEEP_WAIT_STATE;
				}
			}
			break;

		case SAMPLING_WAIT_STATE:
			if(millis() - stateTime >= MAX_TIME_TO_SAMPLE_WAIT){
				logData("SAMPLING_WAIT_STATE", "START SAMPLING", true, false);

				sampleCounter = 1;
				sampleSum = 0;
				sampleAverage = 0;

				stateTime = millis();

				state = SAMPLE_STATE;
			}
			break;

		case SAMPLE_STATE:
			analogValue = analogRead(ANALOG_DETECTOR);

			sampleSum = sampleSum + analogValue;

			if(sampleCounter++ > sampleTotalCount){
				digitalValue = map(analogValue, minValue, maxValue, 0, 100);
				
				sampleAverage = sampleSum / sampleTotalCount;

				stateTime = millis();

				state = PUBLISH_WAIT_STATE;
			}

		case PUBLISH_WAIT_STATE:
			if(millis() - stateTime >= MAX_TIME_TO_WAIT_PUBLISH_MS){
				getTemperature() ? temperatureStringC = String(temperatureC, 2) : temperatureStringC = "0";

				// **********************
				if(temperatureC != 0){
					logData("POWER", "ENBABLE CHARGING", true, true);
					pmic.enableCharging();

					// if(temperatureC < 0){
					// 	logData("POWER", "DISABLE CHARGING", true, true);
					// 	pmic.disableCharging();
					// } else {
					// 	logData("POWER", "ENBABLE CHARGING", true, true);
					// 	pmic.enableCharging();
					// }
				} else {
					logData("POWER", "ENBABLE CHARGING", true, true);
					pmic.enableCharging();
				}
				// **********************


				logData("PUBLISHING", "START PUBLISH_STATE", true, false);

				stateTime = millis();

				state = PUBLISH_STATE;
			}
			break;

		case PUBLISH_STATE:
			Serial.println(analogValue);
			Serial.println(digitalValue);

			//GET https://api.thingspeak.com/update?api_key=GU5AZ7DOWIVLK5BG&field1=0


			// if(analogValue > 0){
				feedName = "Garden-Lady-Bug-04-Cap";
				feedID = "6cUonqXsLchfn8t5F6UsUrxF58qE";
				feedData = "{ \"value\": \"" + String::format("%.2f", sampleAverage) + "\", \"feedID\": \"" + feedID + "\"}";
				logData(feedName, feedData, true, true);

				feedName = "Garden-Lady-Bug-04-Cap-Percentage";
				feedID = "n8epcPsCFksb9UWYtgXtoe4Kjhkv";
				feedData = "{ \"value\": \"" + String::format("%d", digitalValue) + "\", \"feedID\": \"" + feedID + "\"}";
				logData(feedName, feedData, false, true);    

				powerSource = System.powerSource();
				batteryState = System.batteryState();
				powerSString = powerSources[std::max(0, powerSource)];
				powerBSString = batteryStates[std::max(0, batteryState)];
				batterySocString = String(System.batteryCharge(), 2);
				VCellString = String(fuel.getVCell(), 2);
				SoCString = String(fuel.getSoC(), 2);

				logData("Power sources", powerSString, true, true);
				logData("Battery state", powerBSString, true, true);
				logData("Battery charge", batterySocString, true, true);
				logData("Battery VCell", VCellString, true, true);
				logData("Battery SoC", SoCString, true, true);

				feedName = "Garden-Lady-Bug-04-BATT";
				feedID = "UjkrHg96u4xRvT4uECcsENvwTqhu";
				feedData = "{ \"value\": \"" + VCellString + "\", \"feedID\": \"" + feedID + "\"}";
				logData(feedName, feedData, false, true);    
			// }

			// feedName = "Capacitance_Sensor_01";
			// feedData = String::format("%.2f", sampleAverage);
			// logData(feedName, feedData, true, true);

			// feedName = "Capacitance_Sensor_01_Percentage";
			// feedData = String::format("%d", digitalValue);
			// logData(feedName, feedData, true, true);


			logData("PUBLISH_STATE", "START SLEEP_WAIT_STATE", true, false);

			digitalWrite(BUILT_IN_LED, LOW);
			
			stateTime = millis();

			state = SLEEP_WAIT_STATE;
			break;

		case SLEEP_WAIT_STATE:
			if(millis() - stateTime >= MAX_TIME_TO_WAIT_BEFORE_SLEEP_MS){
				logData("SLEEP", "GOING TO SLEEP", true, false);

				stateTime = millis();
				
				state = SLEEP_STATE;
			}
			break;

		case SLEEP_STATE:
			if(!debug){
				// NORMAL mode
				logData("SLEEP", "SLEEP STOP SLEEP", true, false);

				pirState = 0;
				fram.put(FRAM::pirAddr, pirState);
				rtcState = 0;
				fram.put(FRAM::rtcAddr, rtcState);
				watchdogState = 0;
				fram.put(FRAM::watchdogAddr, watchdogState);

				SystemSleepConfiguration config;
				config.mode(SystemSleepMode::STOP)
					.gpio(PIRPin, RISING)
					.duration(30min);
				SystemSleepResult result = System.sleep(config);

				digitalWrite(PIRPin, LOW);												// Pet the watchdog

				if (result.wakeupReason() == SystemSleepWakeupReason::BY_GPIO) {
					// Waken by pin 
					pin_t whichPin = result.wakeupPin();

					if(whichPin == A0){
						logData("SLEEP RESULT", "Woke By BY_GPIO PIR", true, false);

						digitalWrite(BUILT_IN_LED, HIGH);

						pirState = 1;
						fram.put(FRAM::pirAddr, pirState);
					} else {
						logData("SLEEP RESULT", "Woke By BY_GPIO RTC", true, false);

						rtcState = 1;
						fram.put(FRAM::rtcAddr, rtcState);
					}
				}

				if (result.wakeupReason() == SystemSleepWakeupReason::BY_RTC) {
					// Waken by RTC 
					logData("SLEEP RESULT", "SLEEP RESULT - Woke BY_RTC", true, false);
				}

				if (result.wakeupReason() == SystemSleepWakeupReason::BY_NETWORK) {
					// Waken by NETWORK 
					logData("SLEEP RESULT", "SLEEP RESULT - Woke BY_NETWORK", true, false);
				}

				if (result.wakeupReason() == SystemSleepWakeupReason::UNKNOWN) {
					// Waken by RTC 
					logData("SLEEP RESULT", "SLEEP RESULT - Woke By UNKNOWN", true, false);
				}


				stateTime = millis();

				state = PARTICLE_CONNECT_WAIT_STATE;
			} else {
				// DEBUG mode
				if(millis() - stateTime >= MAX_TIME_TO_SLEEP_MS){
					logData("SLEEP", "SLEEP TIMED SLEEP", true, false);

					stateTime = millis();

					state = SAMPLING_WAIT_STATE;
				}
			}
			break;
	}
}

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

bool getTemperature() {
	int reading = analogRead(tmp36Pin);   //getting the voltage reading from the temperature sensor
	delay(1000);
	reading = analogRead(tmp36Pin);   //getting the voltage reading from the temperature sensor
	delay(1000);
	float voltage = reading * 3.3;        // converting that reading to voltage, for 3.3v arduino use 3.3
	voltage /= 4096.0;                    // Electron is different than the Arduino where there are only 1024 steps
	temperatureC = (voltage - 0.5) * 100.0;  //converting from 10 mv per degree with 500 mV offset to degrees ((voltage - 500mV) times 100) - 5 degree calibration
	temperatureF = (temperatureC * 9.0 / 5.0) + 32.0;  // now convert to Fahrenheit
	
	if (temperatureC < -20.0 || temperatureC > 30.0) {             // Reasonable range for garage temperature
		// snprintf(resultStr, sizeof(resultStr),"Temp seems whack: %3.1f", temperatureF);
		return 0;
	}
	else {
		// snprintf(resultStr, sizeof(resultStr),"Temperature is: %3.1f", temperatureF);
		return 1;
	}
}


bool logData(String tempEvent, String tempData, bool tempLogSerial, bool tempLogParticle){
	if (tempLogSerial){
		Log.info(tempEvent + ": " + tempData);
	}
	if (tempLogParticle){
		waitUntil(meterParticlePublish);
		Particle.publish(tempEvent, tempData, 60, PRIVATE);
	}
	return true;
}

bool meterParticlePublish(void){                           // Enforces Particle's limit on 1 publish a second
  static unsigned long lastPublish = 0;                    // Initialize and store value here
  if(millis() - lastPublish >= 1000) {                     // Particle rate limits at 1 publish per second
    lastPublish = millis();
    return 1;
  }
  else return 0;
}

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