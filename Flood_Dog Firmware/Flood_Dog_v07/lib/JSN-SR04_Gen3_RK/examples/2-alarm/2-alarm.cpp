#include "JSN-SR04_Gen3_RK.h"


SerialLogHandler logHandler;

SYSTEM_THREAD(ENABLED);
SYSTEM_MODE(SEMI_AUTOMATIC);

// Declare one of these as a global variable to manage the JSN-SR04 sensor
JSN_SR04_Gen3 distanceSensor;

void distanceCallback(JSN_SR04_Gen3::DistanceResult result) {
    switch(result.status) {
        case JSN_SR04_Gen3::DistanceResult::Status::ENTER_ALARM:
            Log.info("entering alarm state");
            break;

        case JSN_SR04_Gen3::DistanceResult::Status::EXIT_ALARM:
            Log.info("exiting alarm state");
            break;

        case JSN_SR04_Gen3::DistanceResult::Status::SUCCESS:
            // Log.info("cm=%lf inch=%lf", result.cm(), result.inch());
            break;

        default:
            break;
    }
}

void setup() {
    // Initialize the sensor configuration from setup()
    distanceSensor
        .withTrigPin(D3)
        .withEchoPin(D4)
        .withUnusedPins(A0, A1)
        .withCallback(distanceCallback)
        .withDistanceAlarm( JSN_SR04_Gen3::DistanceAlarmLessThan( JSN_SR04_Gen3::DistanceInch(2.5) ) )
        .setup();

    Particle.connect();
}

void loop() {
    // You must call this frequently from loop(), preferable on every execution
    distanceSensor.loop();
}
