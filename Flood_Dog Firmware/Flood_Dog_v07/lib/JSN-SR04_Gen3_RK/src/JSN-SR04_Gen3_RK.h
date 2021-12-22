#ifndef __JSN_SR04_GEN3_RK_H
#define __JSN_SR04_GEN3_RK_H

#include "Particle.h"

// Repository: https://github.com/rickkas7/JSN-SR04_Gen3_RK
// License: MIT

/**
 * @brief Class for a JSN-SR04 ultrasonic distance sensor
 * 
 * Note: You can effectively only have one instance of this class per device, because
 * there is only one I2S peripheral, which is what is used to implement the device
 * driver.
 */
class JSN_SR04_Gen3 {
public:
    /**
     * @brief Utility class for holding a distance
     * 
     * The storage value is in meters, but there are accessors for cm, mm, and inches. This
     * is used for both getting distances (the sensor value) as well as setting distances
     * (for distance alarm mode).
     */
    class Distance {
    public:
        /**
         * @brief Construct a new Distance object with a distance of 0
         */
        Distance() {};

        /**
         * @brief Construct a new Distance object with a distance in meters
         * 
         * @param valueM distance in meters (double floating point)
         */
        Distance(double valueM) { distanceM = valueM; };

        /**
         * @brief Construct a new Distance object from another Distance object
         * 
         * @param value The object to copy the distance from
         */
        Distance(const Distance &value) { *this = value; };

        /**
         * @brief Copy the distance value from another Distance object
         * 
         * @param value The object to copy the distance from
         * 
         * @return Distance& Return this object, so you can chain multiple assignments
         */
        Distance &operator=(const Distance &value) { distanceM = value.distanceM; return *this; };

        /**
         * @brief Set the Distance in meters
         * 
         * @param distanceM the distance in meters to set
         */
        void setDistanceM(double distanceM) { this->distanceM = distanceM; };

        /**
         * @brief Get the Distance in meters
         * 
         * @return double Distance in meters
         */
        double getDistanceM() const { return distanceM; };

        /**
         * @brief Set the distance in centimeters
         * 
         * @param cm Distance in centimeters (double floating point)
         * 
         * Internally, the distance is stored in meters, but this sets the value in centimeters.
         * You can mix-and-match, for example you can get the distance in inches after setting it
         * in centimeters.
         */
        void cm(double cm) { distanceM = cm / 100.0; };

        /**
         * @brief Get the value of the Distance in centimeters
         * 
         * @return double Distance in centimeters
         */
        double cm() const { return distanceM * 100.0; };

        /**
         * @brief Set the distance in millimeter
         * 
         * @param mm Distance in millimeters (double floating point)
         * 
         * Internally, the distance is stored in meters, but this sets the value in millimeters.
         * You can mix-and-match, for example you can get the distance in inches after setting it
         * in millimeters.
         */
        void mm(double mm) { distanceM = mm / 1000.0; };

        /**
         * @brief Get the value of the Distance in millimeters
         * 
         * @return double Distance in millimeters
         */
        double mm() const { return distanceM * 1000.0; };

        /**
         * @brief Set the distance in inches
         * 
         * @param inch Distance in inches (double floating point)
         * 
         * Internally, the distance is stored in meters, but this sets the value in inches.
         * You can mix-and-match, for example you can get the distance in centimeters after setting it
         * in inches.
         */
        void inch(double inch) { distanceM = inch / 39.3701; };

        /**
         * @brief Get the value of the Distance in inches
         * 
         * @return double Distance in inches
         */
        double inch() const { return distanceM * 39.3701; };

        /**
         * @brief The value of the distance in meters
         */
        double distanceM = 0.0;
    };

    /**
     * @brief Helper class for specifying a Distance in centimeters
     * 
     * This helper is used when you want to pass centimeters as a Distance value, for example:
     *  
     * .withDistanceAlarm( JSN_SR04_Gen3::DistanceAlarmLessThan( JSN_SR04_Gen3::DistanceCm(5.0) ) )
     */
    class DistanceCm : public Distance {
    public:
        /**
         * @brief Construct a new DistanceCm object with a value in centimeters
         * 
         * @param value 
         */
        DistanceCm(double value) {
            cm(value);
        }
    };

    /**
     * @brief Helper class for specifying a Distance in inches
     * 
     * This helper is used when you want to pass inches as a Distance value, for example:
     *  
     * .withDistanceAlarm( JSN_SR04_Gen3::DistanceAlarmLessThan( JSN_SR04_Gen3::DistanceInch(2.5) ) )
     */
    class DistanceInch : public Distance {
    public:
        /**
         * @brief Construct a new DistanceInch object with a value in inches
         * 
         * @param value 
         */
        DistanceInch(double value) {
            inch(value);
        }
    };

    /**
     * @brief Structure passed to the callback when the distance has been retrieved
     * 
     * This includes a Status enum for the result status, and optionally a distance
     * as this class is derived from class Distance. Thus you can use the inherited
     * methods such as cm(), mm(), and inch() to get the distance in centimeters,
     * millimeters, or inches, for example.
     */
    class DistanceResult : public Distance {
    public:
        /**
         * @brief Status of the call
         */
        enum class Status : int {
            SUCCESS = 0,		//!< Success, got a valid looking measurement
            ERROR,				//!< An internal error (problem with the I2S peripheral, etc.)
            RANGE_ERROR,	    //!< Too close or too far away to detect
            BUSY,				//!< Called before the previous call completed
            IN_PROGRESS,        //!< Call is in progress (getting sample from sensor)
            ENTER_ALARM,        //!< When using distance alarm, entering alarm state
            EXIT_ALARM          //!< When using distance alarm, exiting alarm state
        };

        /**
         * @brief Get the Status value for this result
         * 
         * @return Status 
         */
        Status getStatus() const { return status; };

        /**
         * @brief Helper function to return true if the Status is SUCCESS
         * 
         * @return true 
         * @return false 
         */
        bool success() const { return status == Status::SUCCESS; };

        /**
         * @brief Current status value
         */
        Status status = Status::ERROR;
    };

    /**
     * @brief Settings for use with distance alarm mode
     * 
     * If you need fine control over the alarm mode, use this class directly. For simple use cases,
     * you can use DistanceAlarmLessThan or DistanceAlarmGreaterThan which are easier to set up.
     */
    class DistanceAlarm : public Distance {
    public:
        /**
         * @brief This enum specifies whether being in alarm is when the distance is less than
         * or greater than the current distance.
         */
        enum class Direction : int {
            LESS_THAN = -1,     //!< Alarm when less than distance (you are too close)
            GREATER_THAN = +1   //!< Alarm when greater than distance (you are too far away)
        };

        /**
         * @brief Sets the distance for the alarm
         * 
         * @param distance The distance value to set
         * @return DistanceAlarm& This object, for chaining options, fluent-style
         * 
         * The Distance object contains a distance in meters, but for convenience, the classes
         * DistanceCm and DistanceInch can make setting the value easy in other units.
         */
        DistanceAlarm &withDistance(Distance distance) { Distance::operator=(distance); return *this; };

        /**
         * @brief Direction LESS_THAN or GREATER_THAN
         * 
         * @param direction the direction enum to test
         * @return DistanceAlarm& This object, for chaining options, fluent-style
         */
        DistanceAlarm &withDirection(Direction direction) { this->direction = direction; return *this; };

        /**
         * @brief Hystersis value (default: 0.5 cm)
         * 
         * @param hysteresis The distance value to set
         * @return DistanceAlarm& This object, for chaining options, fluent-style
         * 
         * The Distance object contains a distance in meters, but for convenience, the classes
         * DistanceCm and DistanceInch can make setting the value easy in other units.
         * 
         * When in LESS_THAN mode, you enter alarm mode when the distance is less than the
         * alarm distance. You exit alarm mode when the distance is greater than the alarm
         * distance + hysteresis.
         * 
         * When in GREATER_THAN mode, you enter alarm mode when the distance is greater than the
         * alarm distance. You exit alarm mode when the distance is less than the alarm
         * distance - hysteresis.
         */
        DistanceAlarm &withHysteresis(Distance hysteresis) { this->hysteresis = hysteresis; return *this; };

        /**
         * @brief The period to test (default: 500 milliseconds, twice per second)
         * 
         * @param period The period as a chrono literal
         * 
         * @return DistanceAlarm& This object, for chaining options, fluent-style
         * 
         * The default is 500ms, but you can pass constants like 10s for 10 seconds (10000 ms).
         */
        DistanceAlarm &withPeriod(std::chrono::milliseconds period) { this->periodMs = period.count(); return *this; };

        /**
         * @brief Returns true if a distance has been configured
         * 
         * @return true 
         * @return false 
         */
        bool isValid() const { return getDistanceM() != 0.0; };

        /**
         * @brief Stores the direction of the test. Default: LESS_THAN
         */
        Direction direction = Direction::LESS_THAN;

        /**
         * @brief Stores the distance for hysteresis. Default: 0.5 cm.
         */
        Distance hysteresis = 0.0005;

        /**
         * @brief How often check the sensor for alarm condition
         * 
         * This probably should be longer than safetyTimeoutMs but can be shorter. 
         */
        unsigned long periodMs = 500; 
    };

    /**
     * @brief Helper class to configure a LESS_THAN DistanceAlarm with the specified distance and default hysteresis.
     */
    class DistanceAlarmLessThan : public DistanceAlarm {
    public:
        /**
         * @brief This helper class is used to set a DistanceAlarm in LESS_THAN mode with the specified distance and default settings
         * 
         * @param distance The distance to alarm at, as a Distance object. You can pass a DistanceCm or DistanceInch object as 
         * the value if desired
         * 
         * Example:
         * 
         *  .withDistanceAlarm( JSN_SR04_Gen3::DistanceAlarmLessThan( JSN_SR04_Gen3::DistanceCm(5.0) ) )
         */
        DistanceAlarmLessThan(Distance distance) {
            withDistance(distance);
            withDirection(DistanceAlarm::Direction::LESS_THAN);
        }
    };

    /**
     * @brief Helper class to configure a GREATER_THAN DistanceAlarm with the specified distance and default hysteresis.
     */
    class DistanceAlarmGreaterThan : public DistanceAlarm {
    public:
        /**
         * @brief This helper class is used to set a DistanceAlarm in GREATER_THAN mode with the specified distance and default settings
         * 
         * @param distance The distance to alarm at, as a Distance object. You can pass a DistanceCm or DistanceInch object as 
         * the value if desired
         * 
         * Example:
         * 
         *  .withDistanceAlarm( JSN_SR04_Gen3::DistanceAlarmGreaterThan( JSN_SR04_Gen3::DistanceCm(5.0) ) )
         */
        DistanceAlarmGreaterThan(Distance distance) {
            withDistance(distance);
            withDirection(DistanceAlarm::Direction::GREATER_THAN);
        }
    };

    /**
     * @brief Construct the object. You typically instantiate one of these as a global variable.
     * 
     * Do not construct one of these objects on the stack in a function that returns. It needs
     * to have a persistent lifetime to function correctly.
     */
    JSN_SR04_Gen3();

    /**
     * @brief Destroy the object
     * 
     * You typically instantiate one of these as a global variable, so it will never be deleted.
     */
    virtual ~JSN_SR04_Gen3();

    /**
     * @brief Specifies the TRIG pin (OUTPUT)
     * 
     * @param trigPin A pin, such as D2 or A0, or another port that is not being used, such as TX, RX, DAC, etc.
     * 
     * @return JSN_SR04_Gen3& This object, for chaining options, fluent-style
     */
    JSN_SR04_Gen3 &withTrigPin(pin_t trigPin) { this->trigPin = trigPin; return *this; };

    /**
     * @brief Specifies the ECHO pin (INPUT)
     * 
     * @param echoPin A pin, such as D2 or A0, or another port that is not being used, such as TX, RX, DAC, etc.
     * 
     * @return JSN_SR04_Gen3& This object, for chaining options, fluent-style
     * 
     * The ECHO pin is typically a 5V logic level. You MUST add a level shifter before connecting it 
     * to a Particle Gen 3 device GPIO because GPIO are not 5V tolerant on the nRF52!
     * 
     * The ECHO pin on a JSN-SR04 is driven by a push-pull driver so you can only connect a single sensor to
     * a single GPIO.
     */
    JSN_SR04_Gen3 &withEchoPin(pin_t echoPin) { this->echoPin = echoPin; return *this; };

    /**
     * @brief You must specify two GPIO that are not otherwise used that will be used as outputs
     * by this library.
     * 
     * @param unusedPin1 A pin, such as D2 or A0, or another port that is not being used, such as TX, RX, DAC, etc.
     * @param unusedPin2 A pin, such as D2 or A0, or another port that is not being used, such as TX, RX, DAC, etc. Must be different than unusedPin1.
     * @return JSN_SR04_Gen3& This object, for chaining options, fluent-style
     * 
     * You need to dedicate two other pins, unusedPin1 and unusedPin2. These must not be the same pin, and can't be 
     * used for anything else for all practical purposes. This is due to how the I2S peripheral works. You have to 
     * assign the I2S LRCK and SCK to pins or the nRF52 I2S peripheral won't initialize. You won't need these outputs 
     * for anything, but they do need to be assigned GPIOs. The signals happen to be 32 kHz for the LRCK and 1 MHz 
     * for SCK while getting a distance sample. 
     */
    JSN_SR04_Gen3 &withUnusedPins(pin_t unusedPin1, pin_t unusedPin2) { this->unusedPin1 = unusedPin1; this->unusedPin2 = unusedPin2; return *this; };

    /**
     * @brief The maximum length that can be measured in meters. Default: 1 meter
     * 
     * @param maxLengthM Distance in meters
     * @return JSN_SR04_Gen3& This object, for chaining options, fluent-style
     * 
     * This affects the amount of memory that is used, and also the amount of time a sampling takes.
     * See the README for more information.
     * 
     * At the default is 1 meter, the memory is 2,080 bytes and the time is 9 milliseconds.
     */
    JSN_SR04_Gen3 &withMaxLengthMeters(float maxLengthM) { this->maxLengthM = maxLengthM; return *this; };

    /**
     * @brief Specify a callback function to be called on samples, errors, and alarm conditions
     * 
     * @param callback The callback function
     * @return JSN_SR04_Gen3& This object, for chaining options, fluent-style
     * 
     * The callback function has the prototype;
     * 
     *   void callback(DistanceResult distanceResult)
     * 
     * It can be a C++11 lambda, if desired, to call a class member function.
     */
    JSN_SR04_Gen3 &withCallback(std::function<void(DistanceResult)> callback) { this->callback = callback; return *this; };

    /**
     * @brief Enabling periodic sampling mode
     * 
     * @param period The sampling period as a chrono literal, such as 500ms, 10s, etc.
     * @return JSN_SR04_Gen3& This object, for chaining options, fluent-style
     * 
     * It's recommended to specify a sampling period greater than safetyTimeoutMs milliseconds (currently 300).
     * However, in practice you can specify a much faster sampling period, as low as getSampleTimeMs() milliseconds.
     * The latter varies depending on the max length meters. At the default value of 1 meter, you can use a 
     * periodic sample rate at low as 10 milliseconds, however you might not get every sample. The sensor may
     * not always reset in time an the BUSY error will be called on the callback.
     */
    JSN_SR04_Gen3 &withSamplePeriodic(std::chrono::milliseconds period) { samplePeriodic = period.count(); return *this; };

    /**
     * @brief Enabling periodic sampling mode
     * 
     * @param periodMs The sampling period in milliseconds
     * @return JSN_SR04_Gen3& This object, for chaining options, fluent-style
     * 
     * It's recommended to specify a sampling period greater than safetyTimeoutMs milliseconds (currently 300).
     * However, in practice you can specify a much faster sampling period, as low as getSampleTimeMs() milliseconds.
     * The latter varies depending on the max length meters. At the default value of 1 meter, you can use a 
     * periodic sample rate at low as 10 milliseconds, however you might not get every sample. The sensor may
     * not always reset in time an the BUSY error will be called on the callback.
     */
    JSN_SR04_Gen3 &withSamplePeriodicMs(unsigned long periodMs) { samplePeriodic = periodMs; return *this; };

    /**
     * @brief Enable distance alarm mode
     * 
     * @param distanceAlarm The distance alarm configuration
     * @return JSN_SR04_Gen3& This object, for chaining options, fluent-style
     */
    JSN_SR04_Gen3 &withDistanceAlarm(DistanceAlarm distanceAlarm); 

    /**
     * @brief You typically call this from setup() after setting all of the configuration parameters using the withXXX() methods
     * 
     * You must call setup() before any samples can be taken.
     */
    bool setup();

    /**
     * @brief You must call this from loop()
     * 
     */
    void loop();

    /**
     * @brief Get a sample from the sensor
     * 
     * @return true 
     * @return false 
     * 
     * You still must configure the pins and call the setup() method. You must also call the loop() method from loop().
     */
    bool sampleOnce();


    /**
     * @brief Synchronous version of sampleOnce - not recommended
     * 
     * @return DistanceResult 
     * 
     * Using the asynchronous callback is preferable, but this call is provided to make it somewhat
     * easier to convert code from other libraries.
     */
    DistanceResult sampleOnceSync();

    /**
     * @brief Gets the last result code
     * 
     * @return DistanceResult 
     * 
     * Normally you should use the callback, but if you want to poll instead of using a callback
     * you can use this function for periodic, sample once, and alarm modes.
     */
    DistanceResult getLastResult() const { return lastResult; };


    /**
     * @brief Returns the number of milliseconds it takes to process a sample. 
     * 
     * @return unsigned long number of milliseconds
     * 
     * In practice it might take a few milliseconds longer because of the delays in dispatching loop().
     * The value is calculated from the maxLengthM and leadingOverhead values.
     * 
     * With the default value of 1 meter maximum and 152 leadingOverhead, this returns 9 milliseconds.
     * 
     * In theory you could sample at around every 9 milliseconds, maybe 10, but it's probably 
     * best to limit it to 100 milliseconds, or even 500 milliseconds, to be safe. If you sample 
     * frequently, be sure to handle the case where BUSY status is returned. This means that that 
     * sensor has not yet reset the ECHO output low and a sample cannot be taken yet.
     * 
     * Note that the callback will not be called for at least this long, regardless of distance!
     * The reason is that the sample buffer is not processed until the DMA engine stops writing 
     * to the entire buffer, and then it waits until in loop context again.
     */
    unsigned long getSampleTimeMs() const;

    /**
     * @brief Utility function to count the number of one bits in a 16-bit unsigned integer
     * 
     * @param sample The uint16_t value to test
     * 
     * @return int A value from 0 to 16
     * 
     * This is optimized for the common cases of 0x0000 (0) and 0xffff (16).
     */
    static int countOneBits(uint16_t sample);

protected:
    /**
     * @brief State handler for idle, waiting to take a sample
     * 
     * Next state: startState, trigged by calling sampleOnce(). Note that periodic sampling
     * calls sampleOnce() internally. Alarm mode automatically enables periodic sampling.
     */
    void idleState();

    /**
     * @brief State handler for when ready to start taking a sample
     * 
     * Once the I2S peripheral has been initialized, goes into sampleState to wait for samples.
     * 
     * Previous state: idleState
     * Next state: sampleState
     */
    void startState();

    /**
     * @brief State handle to wait for samples, then act on them
     * 
     * Previous state: startState
     * Next state: idleState
     */
    void sampleState();

    /**
     * @brief Used internally to set the result of the last measurement
     * 
     * @param status The status, such as SUCCESS, BUSY, ERROR, etc.
     * @param distanceM Distance in meters, optional. Usually only set for SUCCESS, ENTER_ALARM, and EXIT_ALARM.
     */
    void setResult(DistanceResult::Status status, double distanceM = 0.0);

    /**
     * @brief Function to handle the distance alarm. Used internally, called from sampleState() if distanceAlarm is configured.
     * 
     * @param distanceResult 
     */
    void distanceAlarmCallback(DistanceResult distanceResult);

    pin_t trigPin = PIN_INVALID; //!< TRIG pin (OUTPUT)
    pin_t echoPin = PIN_INVALID; //!< ECHO pin (input)
    pin_t unusedPin1 = PIN_INVALID; //!< SCK output (1 mHz)
    pin_t unusedPin2 = PIN_INVALID; //!< LRCK output (32 kHz)
    float maxLengthM = 1.0; //!< Maximum distance that can be read (affects memory and sample period)
    std::function<void(DistanceResult)> callback = nullptr; //!< Callback after sample or error
    size_t numSamples = 0; //!< Number of samples, depends on maxLengthM and leadingOverhead
    uint16_t *rxBuffer = nullptr; //!< ECHO pin buffer
    uint16_t *txBuffer = nullptr; //!< TRIG pin buffer
    bool isIdle = true; //!< True if in idleState
    DistanceResult lastResult; //!< The last sample read (single, periodic, or alarm)
    unsigned long samplePeriodic = 0; //!< If in periodic sample mode, the number of milliseconds in period (0 = disabled)
    DistanceAlarm distanceAlarm; //!< Distance alarm mode settings, if enabled. If distance == 0, then disabled.
    bool inAlarm = false; //!< True if the distance alarm has been notified
    size_t leadingOverhead = 152; //!< How long in 16-bit samples from falling TRIG to rising ECHO (maximum, typical). Must be a multiple of 4. 
    unsigned long safetyTimeoutMs = 300;  //!< Maximum amount of time to wait if sensor does not respond.
    unsigned long sampleTime = 0; //!< millis value when last sample was taken, used for periodic sampling
    std::function<void(JSN_SR04_Gen3&)> stateHandler = &JSN_SR04_Gen3::idleState; //!< state handler function
};

#endif /* __JSR_SR04_GEN3_RK_H */
