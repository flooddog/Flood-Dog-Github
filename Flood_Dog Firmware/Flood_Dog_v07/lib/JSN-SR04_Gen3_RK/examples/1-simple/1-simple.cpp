#include "JSN-SR04_Gen3_RK.h"


SerialLogHandler logHandler;

SYSTEM_THREAD(ENABLED);
SYSTEM_MODE(SEMI_AUTOMATIC);

// Declare one of these as a global variable to manage the JSN-SR04 sensor
JSN_SR04_Gen3 distanceSensor;

void distanceCallback(JSN_SR04_Gen3::DistanceResult result) {
    switch(result.status) {
        case JSN_SR04_Gen3::DistanceResult::Status::SUCCESS:
            Log.info("cm=%lf inch=%lf", result.cm(), result.inch());
            break;

        case JSN_SR04_Gen3::DistanceResult::Status::RANGE_ERROR:
            Log.info("distance range error");
            break;

        default:
            Log.info("distance error %d", result.status);
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
        .withSamplePeriodic(500ms)
        .setup();

    Particle.connect();
}

void loop() {
    // You must call this frequently from loop(), preferable on every execution
    distanceSensor.loop();
}
