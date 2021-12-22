
# class JSN_SR04_Gen3::Distance 

Utility class for holding a distance.

The storage value is in meters, but there are accessors for cm, mm, and inches. This is used for both getting distances (the sensor value) as well as setting distances (for distance alarm mode).

## Members

---

### double distanceM 

The value of the distance in meters.

```
double distanceM
```

---

###  JSN_SR04_Gen3::Distance::Distance() 

Construct a new Distance object with a distance of 0.

```
 Distance()
```

---

###  JSN_SR04_Gen3::Distance::Distance(double valueM) 

Construct a new Distance object with a distance in meters.

```
 Distance(double valueM)
```

#### Parameters
* `valueM` distance in meters (double floating point)

---

###  JSN_SR04_Gen3::Distance::Distance(const Distance & value) 

Construct a new Distance object from another Distance object.

```
 Distance(const Distance & value)
```

#### Parameters
* `value` The object to copy the distance from

---

### Distance & JSN_SR04_Gen3::Distance::operator=(const Distance & value) 

Copy the distance value from another Distance object.

```
Distance & operator=(const Distance & value)
```

#### Parameters
* `value` The object to copy the distance from

#### Returns
Distance& Return this object, so you can chain multiple assignments

---

### void JSN_SR04_Gen3::Distance::setDistanceM(double distanceM) 

Set the Distance in meters.

```
void setDistanceM(double distanceM)
```

#### Parameters
* `distanceM` the distance in meters to set

---

### double JSN_SR04_Gen3::Distance::getDistanceM() const 

Get the Distance in meters.

```
double getDistanceM() const
```

#### Returns
double Distance in meters

---

### void JSN_SR04_Gen3::Distance::cm(double cm) 

Set the distance in centimeters.

```
void cm(double cm)
```

#### Parameters
* `cm` Distance in centimeters (double floating point)

Internally, the distance is stored in meters, but this sets the value in centimeters. You can mix-and-match, for example you can get the distance in inches after setting it in centimeters.

---

### double JSN_SR04_Gen3::Distance::cm() const 

Get the value of the Distance in centimeters.

```
double cm() const
```

#### Returns
double Distance in centimeters

---

### void JSN_SR04_Gen3::Distance::mm(double mm) 

Set the distance in millimeter.

```
void mm(double mm)
```

#### Parameters
* `mm` Distance in millimeters (double floating point)

Internally, the distance is stored in meters, but this sets the value in millimeters. You can mix-and-match, for example you can get the distance in inches after setting it in millimeters.

---

### double JSN_SR04_Gen3::Distance::mm() const 

Get the value of the Distance in millimeters.

```
double mm() const
```

#### Returns
double Distance in millimeters

---

### void JSN_SR04_Gen3::Distance::inch(double inch) 

Set the distance in inches.

```
void inch(double inch)
```

#### Parameters
* `inch` Distance in inches (double floating point)

Internally, the distance is stored in meters, but this sets the value in inches. You can mix-and-match, for example you can get the distance in centimeters after setting it in inches.

---

### double JSN_SR04_Gen3::Distance::inch() const 

Get the value of the Distance in inches.

```
double inch() const
```

#### Returns
double Distance in inches

# class JSN_SR04_Gen3::DistanceAlarm 

```
class JSN_SR04_Gen3::DistanceAlarm
  : public JSN_SR04_Gen3::Distance
```  

Settings for use with distance alarm mode.

If you need fine control over the alarm mode, use this class directly. For simple use cases, you can use DistanceAlarmLessThan or DistanceAlarmGreaterThan which are easier to set up.

## Members

---

### Direction direction 

Stores the direction of the test. Default: LESS_THAN.

```
Direction direction
```

---

### Distance hysteresis 

Stores the distance for hysteresis. Default: 0.5 cm.

```
Distance hysteresis
```

---

### unsigned long periodMs 

How often check the sensor for alarm condition.

```
unsigned long periodMs
```

This probably should be longer than safetyTimeoutMs but can be shorter.

---

### DistanceAlarm & JSN_SR04_Gen3::DistanceAlarm::withDistance(Distance distance) 

Sets the distance for the alarm.

```
DistanceAlarm & withDistance(Distance distance)
```

#### Parameters
* `distance` The distance value to set 

#### Returns
DistanceAlarm& This object, for chaining options, fluent-style

The Distance object contains a distance in meters, but for convenience, the classes DistanceCm and DistanceInch can make setting the value easy in other units.

---

### DistanceAlarm & JSN_SR04_Gen3::DistanceAlarm::withDirection(Direction direction) 

Direction LESS_THAN or GREATER_THAN.

```
DistanceAlarm & withDirection(Direction direction)
```

#### Parameters
* `direction` the direction enum to test 

#### Returns
DistanceAlarm& This object, for chaining options, fluent-style

---

### DistanceAlarm & JSN_SR04_Gen3::DistanceAlarm::withHysteresis(Distance hysteresis) 

Hystersis value (default: 0.5 cm)

```
DistanceAlarm & withHysteresis(Distance hysteresis)
```

#### Parameters
* `hysteresis` The distance value to set 

#### Returns
DistanceAlarm& This object, for chaining options, fluent-style

The Distance object contains a distance in meters, but for convenience, the classes DistanceCm and DistanceInch can make setting the value easy in other units.

When in LESS_THAN mode, you enter alarm mode when the distance is less than the alarm distance. You exit alarm mode when the distance is greater than the alarm distance + hysteresis.

When in GREATER_THAN mode, you enter alarm mode when the distance is greater than the alarm distance. You exit alarm mode when the distance is less than the alarm distance - hysteresis.

---

### DistanceAlarm & JSN_SR04_Gen3::DistanceAlarm::withPeriod(std::chrono::milliseconds period) 

The period to test (default: 500 milliseconds, twice per second)

```
DistanceAlarm & withPeriod(std::chrono::milliseconds period)
```

#### Parameters
* `period` The period as a chrono literal

#### Returns
DistanceAlarm& This object, for chaining options, fluent-style

The default is 500ms, but you can pass constants like 10s for 10 seconds (10000 ms).

---

### bool JSN_SR04_Gen3::DistanceAlarm::isValid() const 

Returns true if a distance has been configured.

```
bool isValid() const
```

#### Returns
true 

#### Returns
false

---

### enum Direction 

This enum specifies whether being in alarm is when the distance is less than or greater than the current distance.

```
enum Direction
```

 Values                         | Descriptions                                
--------------------------------|---------------------------------------------
LESS_THAN            | Alarm when less than distance (you are too close)
GREATER_THAN            | Alarm when greater than distance (you are too far away)

# class JSN_SR04_Gen3::DistanceAlarmGreaterThan 

```
class JSN_SR04_Gen3::DistanceAlarmGreaterThan
  : public JSN_SR04_Gen3::DistanceAlarm
```  

Helper class to configure a GREATER_THAN DistanceAlarm with the specified distance and default hysteresis.

## Members

---

###  JSN_SR04_Gen3::DistanceAlarmGreaterThan::DistanceAlarmGreaterThan(Distance distance) 

This helper class is used to set a DistanceAlarm in GREATER_THAN mode with the specified distance and default settings.

```
 DistanceAlarmGreaterThan(Distance distance)
```

#### Parameters
* `distance` The distance to alarm at, as a Distance object. You can pass a DistanceCm or DistanceInch object as the value if desired

Example:

.withDistanceAlarm( JSN_SR04_Gen3::DistanceAlarmGreaterThan( JSN_SR04_Gen3::DistanceCm(5.0) ) )

# class JSN_SR04_Gen3::DistanceAlarmLessThan 

```
class JSN_SR04_Gen3::DistanceAlarmLessThan
  : public JSN_SR04_Gen3::DistanceAlarm
```  

Helper class to configure a LESS_THAN DistanceAlarm with the specified distance and default hysteresis.

## Members

---

###  JSN_SR04_Gen3::DistanceAlarmLessThan::DistanceAlarmLessThan(Distance distance) 

This helper class is used to set a DistanceAlarm in LESS_THAN mode with the specified distance and default settings.

```
 DistanceAlarmLessThan(Distance distance)
```

#### Parameters
* `distance` The distance to alarm at, as a Distance object. You can pass a DistanceCm or DistanceInch object as the value if desired

Example:

.withDistanceAlarm( JSN_SR04_Gen3::DistanceAlarmLessThan( JSN_SR04_Gen3::DistanceCm(5.0) ) )

# class JSN_SR04_Gen3::DistanceCm 

```
class JSN_SR04_Gen3::DistanceCm
  : public JSN_SR04_Gen3::Distance
```  

Helper class for specifying a Distance in centimeters.

This helper is used when you want to pass centimeters as a Distance value, for example:

.withDistanceAlarm( JSN_SR04_Gen3::DistanceAlarmLessThan( JSN_SR04_Gen3::DistanceCm(5.0) ) )

## Members

---

###  JSN_SR04_Gen3::DistanceCm::DistanceCm(double value) 

Construct a new DistanceCm object with a value in centimeters.

```
 DistanceCm(double value)
```

#### Parameters
* `value`

# class JSN_SR04_Gen3::DistanceInch 

```
class JSN_SR04_Gen3::DistanceInch
  : public JSN_SR04_Gen3::Distance
```  

Helper class for specifying a Distance in inches.

This helper is used when you want to pass inches as a Distance value, for example:

.withDistanceAlarm( JSN_SR04_Gen3::DistanceAlarmLessThan( JSN_SR04_Gen3::DistanceInch(2.5) ) )

## Members

---

###  JSN_SR04_Gen3::DistanceInch::DistanceInch(double value) 

Construct a new DistanceInch object with a value in inches.

```
 DistanceInch(double value)
```

#### Parameters
* `value`

# class JSN_SR04_Gen3::DistanceResult 

```
class JSN_SR04_Gen3::DistanceResult
  : public JSN_SR04_Gen3::Distance
```  

Structure passed to the callback when the distance has been retrieved.

This includes a Status enum for the result status, and optionally a distance as this class is derived from class Distance. Thus you can use the inherited methods such as cm(), mm(), and inch() to get the distance in centimeters, millimeters, or inches, for example.

## Members

---

### Status status 

Current status value.

```
Status status
```

---

### Status JSN_SR04_Gen3::DistanceResult::getStatus() const 

Get the Status value for this result.

```
Status getStatus() const
```

#### Returns
Status

---

### bool JSN_SR04_Gen3::DistanceResult::success() const 

Helper function to return true if the Status is SUCCESS.

```
bool success() const
```

#### Returns
true 

#### Returns
false

---

### enum Status 

Status of the call.

```
enum Status
```

 Values                         | Descriptions                                
--------------------------------|---------------------------------------------
SUCCESS            | Success, got a valid looking measurement.
ERROR            | An internal error (problem with the I2S peripheral, etc.)
RANGE_ERROR            | Too close or too far away to detect.
BUSY            | Called before the previous call completed.
IN_PROGRESS            | Call is in progress (getting sample from sensor)
ENTER_ALARM            | When using distance alarm, entering alarm state.
EXIT_ALARM            | When using distance alarm, exiting alarm state.

# class JSN_SR04_Gen3 

Class for a JSN-SR04 ultrasonic distance sensor.

Note: You can effectively only have one instance of this class per device, because there is only one I2S peripheral, which is what is used to implement the device driver.

## Members

---

###  JSN_SR04_Gen3::JSN_SR04_Gen3() 

Construct the object. You typically instantiate one of these as a global variable.

```
 JSN_SR04_Gen3()
```

Do not construct one of these objects on the stack in a function that returns. It needs to have a persistent lifetime to function correctly.

---

###  JSN_SR04_Gen3::~JSN_SR04_Gen3() 

Destroy the object.

```
virtual  ~JSN_SR04_Gen3()
```

You typically instantiate one of these as a global variable, so it will never be deleted.

---

### JSN_SR04_Gen3 & JSN_SR04_Gen3::withTrigPin(pin_t trigPin) 

Specifies the TRIG pin (OUTPUT)

```
JSN_SR04_Gen3 & withTrigPin(pin_t trigPin)
```

#### Parameters
* `trigPin` A pin, such as D2 or A0, or another port that is not being used, such as TX, RX, DAC, etc.

#### Returns
JSN_SR04_Gen3& This object, for chaining options, fluent-style

---

### JSN_SR04_Gen3 & JSN_SR04_Gen3::withEchoPin(pin_t echoPin) 

Specifies the ECHO pin (INPUT)

```
JSN_SR04_Gen3 & withEchoPin(pin_t echoPin)
```

#### Parameters
* `echoPin` A pin, such as D2 or A0, or another port that is not being used, such as TX, RX, DAC, etc.

#### Returns
JSN_SR04_Gen3& This object, for chaining options, fluent-style

The ECHO pin is typically a 5V logic level. You MUST add a level shifter before connecting it to a Particle Gen 3 device GPIO because GPIO are not 5V tolerant on the nRF52!

The ECHO pin on a JSN-SR04 is driven by a push-pull driver so you can only connect a single sensor to a single GPIO.

---

### JSN_SR04_Gen3 & JSN_SR04_Gen3::withUnusedPins(pin_t unusedPin1, pin_t unusedPin2) 

You must specify two GPIO that are not otherwise used that will be used as outputs by this library.

```
JSN_SR04_Gen3 & withUnusedPins(pin_t unusedPin1, pin_t unusedPin2)
```

#### Parameters
* `unusedPin1` A pin, such as D2 or A0, or another port that is not being used, such as TX, RX, DAC, etc. 

* `unusedPin2` A pin, such as D2 or A0, or another port that is not being used, such as TX, RX, DAC, etc. Must be different than unusedPin1. 

#### Returns
JSN_SR04_Gen3& This object, for chaining options, fluent-style

You need to dedicate two other pins, unusedPin1 and unusedPin2. These must not be the same pin, and can't be used for anything else for all practical purposes. This is due to how the I2S peripheral works. You have to assign the I2S LRCK and SCK to pins or the nRF52 I2S peripheral won't initialize. You won't need these outputs for anything, but they do need to be assigned GPIOs. The signals happen to be 32 kHz for the LRCK and 1 MHz for SCK while getting a distance sample.

---

### JSN_SR04_Gen3 & JSN_SR04_Gen3::withMaxLengthMeters(float maxLengthM) 

The maximum length that can be measured in meters. Default: 1 meter.

```
JSN_SR04_Gen3 & withMaxLengthMeters(float maxLengthM)
```

#### Parameters
* `maxLengthM` Distance in meters 

#### Returns
JSN_SR04_Gen3& This object, for chaining options, fluent-style

This affects the amount of memory that is used, and also the amount of time a sampling takes. See the README for more information.

At the default is 1 meter, the memory is 2,080 bytes and the time is 9 milliseconds.

---

### JSN_SR04_Gen3 & JSN_SR04_Gen3::withCallback(std::function< void(DistanceResult)> callback) 

Specify a callback function to be called on samples, errors, and alarm conditions.

```
JSN_SR04_Gen3 & withCallback(std::function< void(DistanceResult)> callback)
```

#### Parameters
* `callback` The callback function 

#### Returns
JSN_SR04_Gen3& This object, for chaining options, fluent-style

The callback function has the prototype;

void callback(DistanceResult distanceResult)

It can be a C++11 lambda, if desired, to call a class member function.

---

### JSN_SR04_Gen3 & JSN_SR04_Gen3::withSamplePeriodic(std::chrono::milliseconds period) 

Enabling periodic sampling mode.

```
JSN_SR04_Gen3 & withSamplePeriodic(std::chrono::milliseconds period)
```

#### Parameters
* `period` The sampling period as a chrono literal, such as 500ms, 10s, etc. 

#### Returns
JSN_SR04_Gen3& This object, for chaining options, fluent-style

It's recommended to specify a sampling period greater than safetyTimeoutMs milliseconds (currently 300). However, in practice you can specify a much faster sampling period, as low as getSampleTimeMs() milliseconds. The latter varies depending on the max length meters. At the default value of 1 meter, you can use a periodic sample rate at low as 10 milliseconds, however you might not get every sample. The sensor may not always reset in time an the BUSY error will be called on the callback.

---

### JSN_SR04_Gen3 & JSN_SR04_Gen3::withSamplePeriodicMs(unsigned long periodMs) 

Enabling periodic sampling mode.

```
JSN_SR04_Gen3 & withSamplePeriodicMs(unsigned long periodMs)
```

#### Parameters
* `periodMs` The sampling period in milliseconds 

#### Returns
JSN_SR04_Gen3& This object, for chaining options, fluent-style

It's recommended to specify a sampling period greater than safetyTimeoutMs milliseconds (currently 300). However, in practice you can specify a much faster sampling period, as low as getSampleTimeMs() milliseconds. The latter varies depending on the max length meters. At the default value of 1 meter, you can use a periodic sample rate at low as 10 milliseconds, however you might not get every sample. The sensor may not always reset in time an the BUSY error will be called on the callback.

---

### JSN_SR04_Gen3 & JSN_SR04_Gen3::withDistanceAlarm(DistanceAlarm distanceAlarm) 

Enable distance alarm mode.

```
JSN_SR04_Gen3 & withDistanceAlarm(DistanceAlarm distanceAlarm)
```

#### Parameters
* `distanceAlarm` The distance alarm configuration 

#### Returns
JSN_SR04_Gen3& This object, for chaining options, fluent-style

---

### bool JSN_SR04_Gen3::setup() 

You typically call this from setup() after setting all of the configuration parameters using the withXXX() methods.

```
bool setup()
```

You must call setup() before any samples can be taken.

---

### void JSN_SR04_Gen3::loop() 

You must call this from loop()

```
void loop()
```

---

### bool JSN_SR04_Gen3::sampleOnce() 

Get a sample from the sensor.

```
bool sampleOnce()
```

#### Returns
true 

#### Returns
false

You still must configure the pins and call the setup() method. You must also call the loop() method from loop().

---

### DistanceResult JSN_SR04_Gen3::sampleOnceSync() 

Synchronous version of sampleOnce - not recommended.

```
DistanceResult sampleOnceSync()
```

#### Returns
DistanceResult

Using the asynchronous callback is preferable, but this call is provided to make it somewhat easier to convert code from other libraries.

---

### DistanceResult JSN_SR04_Gen3::getLastResult() const 

Gets the last result code.

```
DistanceResult getLastResult() const
```

#### Returns
DistanceResult

Normally you should use the callback, but if you want to poll instead of using a callback you can use this function for periodic, sample once, and alarm modes.

---

### unsigned long JSN_SR04_Gen3::getSampleTimeMs() const 

Returns the number of milliseconds it takes to process a sample.

```
unsigned long getSampleTimeMs() const
```

#### Returns
unsigned long number of milliseconds

In practice it might take a few milliseconds longer because of the delays in dispatching loop(). The value is calculated from the maxLengthM and leadingOverhead values.

With the default value of 1 meter maximum and 152 leadingOverhead, this returns 9 milliseconds.

In theory you could sample at around every 9 milliseconds, maybe 10, but it's probably best to limit it to 100 milliseconds, or even 500 milliseconds, to be safe. If you sample frequently, be sure to handle the case where BUSY status is returned. This means that that sensor has not yet reset the ECHO output low and a sample cannot be taken yet.

Note that the callback will not be called for at least this long, regardless of distance! The reason is that the sample buffer is not processed until the DMA engine stops writing to the entire buffer, and then it waits until in loop context again.

Generated by [Moxygen](https://sourcey.com/moxygen)