/******************************************************/
//       THIS IS A GENERATED FILE - DO NOT EDIT       //
/******************************************************/

#include "Particle.h"
#line 1 "/Users/philipchatterton/IoCP/Particle/Flood_Dog/Flood_Dog_v01/src/Flood_Dog_v01.ino"
////////////////////////////////
//                            //
//      Lady Bug 2 v002       //
//                            //
//      Adafruit BMP280       //
//    Moisture Sensor v1.2    //
//         TPL5111            //
//          BATT              //
//                            //
//        Greenhouse          //
//                            //
////////////////////////////////

void setup();
void loop();
bool logData(String tempEvent, String tempData, bool tempLogSerial, bool tempLogParticle);
bool meterParticlePublish(void);
void selectExternalMeshAntenna();
#line 14 "/Users/philipchatterton/IoCP/Particle/Flood_Dog/Flood_Dog_v01/src/Flood_Dog_v01.ino"
SYSTEM_THREAD(ENABLED)

SerialLogHandler logHandler;

#include "captouch.h"

SYSTEM_THREAD(ENABLED)

#define BUILT_IN_LED D7
#define ANALOG_DETECTOR A0
#define DIGITAL_DETECTOR D3
#define CAP_SEND A1
#define CAP_RECIEVE A2

CapTouch Touch(CAP_SEND, CAP_RECIEVE);

int analogValue = 0;
String analogValueString = "0";
int digitalValue = 0;
String digitalValueString = "0";

String feedName;
String feedID;
String feedData;

float smoothed;

void setup() {
	// selectExternalMeshAntenna();

  Serial.begin(9600);

	pinMode(BUILT_IN_LED, OUTPUT);
	pinMode(ANALOG_DETECTOR, INPUT);
	pinMode(DIGITAL_DETECTOR, INPUT);
  Touch.setup();
}

void loop() {
  /*
	analogValue = analogRead(ANALOG_DETECTOR);
  // analogValueString = String(analogValue);
	digitalValue = digitalRead(DIGITAL_DETECTOR);
  // digitalValueString = String(digitalValue);

  // Serial.println("Sample:");
  Serial.println(analogValue);
  // Serial.println(digitalValue);
  // Serial.println(" ");
  */

  CapTouch::Event touchEvent = Touch.getEvent();
  if (touchEvent == CapTouch::TouchEvent) {
    digitalWrite(BUILT_IN_LED, HIGH);
  } else if (touchEvent == CapTouch::ReleaseEvent){
    digitalWrite(BUILT_IN_LED, LOW);
  }
  //m_tReading

  feedName = "Garden-Lady-Bug-04-Cap";
  feedID = "6cUonqXsLchfn8t5F6UsUrxF58qE";
  feedData = "{ \"value\": \"" + String::format("%.2f", smoothed) + "\", \"feedID\": \"" + feedID + "\"}";
  logData(feedName, feedData, true, true);

  delay(2000);
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