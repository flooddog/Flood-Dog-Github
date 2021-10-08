////////////////////////////////
//                            //
//      Cap Water Sensor      //
//                            //
//                            //
//                            //
//                            //
//                            //
////////////////////////////////

SYSTEM_THREAD(ENABLED)

#define BUILT_IN_LED D7       // Built In LED

#define ANALOG_DETECTOR A3

SerialLogHandler logHandler;

int minValue = 0;
int maxValue = 4100;

float voltage;
String voltageStr;

int analogValue = 0;
String analogValueString = "0";
int digitalValue = 0;
String digitalValueString = "0";

int sampleCounter = 0;
int sampleSum = 0;
int sampleTotalCount = 1000;
float sampleAverage = 0;

const unsigned long MAX_TIME_TO_WAIT_FOR_CONNECT_MS = (5 * 60 * 1000); // Only stay awake for [5] minutes trying to connect to the Particle.io
const unsigned long MAX_TIME_TO_SAMPLE_WAIT = (1 * 1000); // Only stay awake for [1] seconds trying to connect to the cloud and publish
const unsigned long MAX_COUNT_TEST_SAMPLING = 3; // 5 tries
const unsigned long MAX_TIME_TO_WAIT_PUBLISH_MS = (1 * 1000); // Only stay awake for [1] seconds trying to connect to the cloud and publish
const unsigned long TIME_AFTER_PUBLISH_MS = (1 * 1000); // After publish, wait [1] seconds for data to go out
const unsigned long TIME_TO_SLEEP_SEC = (15 * 60); // sleep for [15] minutes
const unsigned long TIME_TO_SLEEP_MS = (TIME_TO_SLEEP_SEC * 1000); // sleep for [15] minutes

enum State {
				MESH_CONNECT_WAIT_STATE, 
				PARTICLE_CONNECT_WAIT_STATE, 
				SAMPLING_ACCEL_WAIT_STATE, 
				SAMPLE_ACCEL_STATE, 
				SAMPLING_WAIT_STATE, 
				SAMPLE_STATE, 
				PUBLISH_WAIT_STATE, 
				PUBLISH_STATE, 
				SLEEP_WAIT_STATE, 
				SLEEP_STATE
			};
State state = PARTICLE_CONNECT_WAIT_STATE;
unsigned long stateTime = 0;

String feedName;
String feedID;
String feedData;


void setup(){
	selectExternalMeshAntenna();

	Serial.begin(9600);

	// Particle.function("Read_Voltage", readVoltage);
	// Particle.variable("Voltage", getVoltageValue);

	state = PARTICLE_CONNECT_WAIT_STATE;

	logData("SETUP", "SETUP COMPLETE", true, false);
}

void loop(){
	switch(state){
		// case MESH_CONNECT_WAIT_STATE:			
		// 	if(Mesh.ready()){
		// 		logData("MESH", "MESH CONNECTED", true, false);

		// 		stateTime = millis();

		// 		state = PARTICLE_CONNECT_WAIT_STATE;
		// 	} else {
		// 		Mesh.connect();

		// 		if(millis() - stateTime > MAX_TIME_TO_WAIT_FOR_CONNECT_MS){
		// 			logData("MESH", "MESH FAILED TO CONNECT", true, false);
					
		// 			stateTime = millis();

		// 			state = SLEEP_WAIT_STATE;
		// 		}
		// 	}
		// 	break;

		case PARTICLE_CONNECT_WAIT_STATE:			
			if (Particle.connected()) {
				logData("PARTICLE", "PARTICLE CONNECTED", true, false);

				digitalWrite(BUILT_IN_LED, HIGH);

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
			// voltage = analogRead(BATT) * 0.0011224;
			// voltageStr = String::format("%.2f", voltage);

			analogValue = analogRead(ANALOG_DETECTOR);

			sampleSum = sampleSum + analogValue;

			if(sampleCounter++ > sampleTotalCount){
				digitalValue = map(analogValue, minValue, maxValue, 0, 100);
				
				sampleAverage = sampleSum / sampleTotalCount;

				stateTime = millis();

				state = PUBLISH_WAIT_STATE;
			}
			break;

		case PUBLISH_WAIT_STATE:
			if(millis() - stateTime >= MAX_TIME_TO_WAIT_PUBLISH_MS){
				logData("PUBLISH_WAIT_STATE", "START PUBLISH_STATE", true, false);

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

				// feedName = "Garden-Lady-Bug-04-BATT";
				// // feedID = "n8epcPsCFksb9UWYtgXtoe4Kjhkv";
				// // feedData = "{ \"value\": \"" + String::format("%d", digitalValue) + "\", \"feedID\": \"" + feedID + "\"}";
				// feedData = voltageStr;
				// logData(feedName, feedData, false, true);    
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
			if(millis() - stateTime >= TIME_AFTER_PUBLISH_MS){
				logData("SLEEP_WAIT_STATE", "GOING TO SLEEP", true, false);

				stateTime = millis();

				state = SLEEP_STATE;
			}
			break;

		case SLEEP_STATE:
			if(millis() - stateTime >= TIME_TO_SLEEP_MS){
				logData("SLEEP_STATE", "WAKE UP", true, false);

				stateTime = millis();

				state = PARTICLE_CONNECT_WAIT_STATE;
			}
			break;
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

bool meterParticlePublish(void) {                                       // Enforces Particle's limit on 1 publish a second
  static unsigned long lastPublish=0;                                   // Initialize and store value here
  if(millis() - lastPublish >= 1000) {                                  // Particle rate limits at 1 publish per second
    lastPublish = millis();
    return 1;
  }
  else return 0;
}

// String getVoltageValue(){
// 	float tempVoltageValue = analogRead(BATT) * 0.0011224;
// 	// int tempVoltageValueInt = tempVoltageValue;
// 	// double tempVoltageValueDouble = tempVoltageValueInt / 100;
// 	String tempVoltageString = String::format("%.2f", tempVoltageValue);
	
// 	return tempVoltageString;
// }

String getVoltageString(float tempVoltageValue){
	String tempVoltageString = String::format("%.2f", tempVoltageValue);
	return tempVoltageString;
}

// String readVoltage(String command){
// 	return getVoltageString(getVoltageValue());
// }

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