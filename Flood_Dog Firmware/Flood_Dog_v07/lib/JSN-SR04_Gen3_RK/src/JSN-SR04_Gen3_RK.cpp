

#include "JSN-SR04_Gen3_RK.h"

// Repository: https://github.com/rickkas7/JSN-SR04_Gen3_RK
// License: MIT

// Must undefine this or the direct nRF52 libraries won't compile as there is a struct member SCK
#undef SCK

#include "nrf_gpio.h"
#include "nrfx_i2s.h"
#include <math.h>

// These are defined here because they require nrfx_i2s.h to be included, but this requires that
// SCK be undefined, so we don't want to do it from JSN-SR04_Gen3_RK.h, so these can't be 
// class members.
static volatile int _buffersRequested = 0;
static nrfx_i2s_buffers_t _i2sBuffers;

// Interrupt handler function
static void _dataHandler(nrfx_i2s_buffers_t const *p_released, uint32_t status) {
	if (status == NRFX_I2S_STATUS_NEXT_BUFFERS_NEEDED) {
		_buffersRequested++;
		if (_buffersRequested >= 2) {
			nrfx_i2s_stop();
		}
	}
}

JSN_SR04_Gen3::JSN_SR04_Gen3() {

}

JSN_SR04_Gen3::~JSN_SR04_Gen3() {
    delete[] rxBuffer;
    delete[] txBuffer;
}

JSN_SR04_Gen3 &JSN_SR04_Gen3::withDistanceAlarm(DistanceAlarm distanceAlarm) { 
    this->distanceAlarm = distanceAlarm; 

    withSamplePeriodicMs(distanceAlarm.periodMs);
    
    return *this; 
};


bool JSN_SR04_Gen3::setup() {
    if (trigPin == PIN_INVALID || echoPin == PIN_INVALID || unusedPin1 == PIN_INVALID || unusedPin2 == PIN_INVALID) {
        return false;
    }    
    if (!isIdle) {
        return false;
    }

    // leadingOverhead (currently 152) is the number of samples that include the 16 us pulse
    // on TRIG and the time from falling TRIG to rising ECHO.
    // This was determined experimentally to include the length of the TRIG pulse and the amount of
    // time the sensor takes from TRIG falling to ECHO rising. It was around 150, so 152 gives
    // a little extra margin. It must be a multiple of 4.
    numSamples = leadingOverhead;

    // We have 192 samples per meter. See the README for the detailed explanation of where that
    // number comes from. It must also be multiple of 4.
    numSamples += floor(maxLengthM * 48.0) * 4;

    // Allocate the tx and rx buffers
    if (!rxBuffer) {
        rxBuffer = new uint16_t[numSamples];
    }
    if (!txBuffer) {
        txBuffer = new uint16_t[numSamples];
    }
    if (!rxBuffer || !txBuffer) {
        return false;
    }

    pinMode(trigPin, OUTPUT); 
    digitalWrite(trigPin, LOW);
    
    pinMode(echoPin, INPUT); 
    pinMode(unusedPin1, OUTPUT); // SCK
    pinMode(unusedPin2, OUTPUT); // LRCK

    return true;
}

bool JSN_SR04_Gen3::sampleOnce() {

    if (isIdle) {
        if (digitalRead(echoPin) == LOW) {
            stateHandler = &JSN_SR04_Gen3::startState;
            isIdle = false;

            return true;
        }
    }

    setResult(DistanceResult::Status::BUSY);
    return false;
}

JSN_SR04_Gen3::DistanceResult JSN_SR04_Gen3::sampleOnceSync() {

    if (sampleOnce()) {
        while(lastResult.status == DistanceResult::Status::IN_PROGRESS) {
            delay(1);
        }
    }

    return getLastResult(); 
}


void JSN_SR04_Gen3::loop() {

    stateHandler(*this);
}

void JSN_SR04_Gen3::setResult(DistanceResult::Status status, double distanceM) { 
    lastResult.status = status; 
    lastResult.distanceM = distanceM; 

    if (callback) {
        callback(lastResult);
    }
}

unsigned long JSN_SR04_Gen3::getSampleTimeMs() const {

    /*
    The getSampleTimeMs() method returns the number of milliseconds it takes to process a sample. 
    In practice it might take a few milliseconds longer because of the delays in dispatching loop().

    The formula is:

    - T = (2 × D) / C
    - leadingOverheadMs = leadingOverhead * 16 / 1000 = 152 * 16 / 1000 = 2.432 ms
    - Cms = 0.340 meters/millisecond
    - Tms = (2 × D) / Cms + leadingOverheadMs

    For the default of D = 1 meter:

    - D = 1 meter
    - Tms = (2 × D) / Cms + leadingOverheadMs
    - Tms = 2 / 0.340 + 2.432
    - Tms = 8.31 ms (rounded up to 9)

    Thus in theory you could sample at around every 9 milliseconds, maybe 10, but it's probably 
    best to limit it to 100 milliseconds, or even 500 milliseconds, to be safe. If you sample 
    frequently, be sure to handle the case where BUSY status is returned. This means that that 
    sensor has not yet reset the ECHO output low and a sample cannot be taken yet.
    */    
    double leadingOverheadMs = ((double)leadingOverhead) * 16.0 / 1000.0;

    double Tms = (2 * maxLengthM) / 0.340 + leadingOverheadMs;

    return (unsigned long) ceil(Tms);
}



void JSN_SR04_Gen3::idleState() {
    if (samplePeriodic) {
        if (millis() - sampleTime >= samplePeriodic) {
            sampleTime = millis();
            sampleOnce();
        }
    }
}

void JSN_SR04_Gen3::startState() {

    _i2sBuffers.p_rx_buffer = (uint32_t *)rxBuffer;
    _i2sBuffers.p_tx_buffer = (uint32_t *)txBuffer;

	attachInterruptDirect(I2S_IRQn, nrfx_i2s_irq_handler, false);


    Hal_Pin_Info *pinMap = HAL_Pin_Map();

    nrfx_i2s_config_t config = NRFX_I2S_DEFAULT_CONFIG;

    // Log.info("Got %02x Expected %02x", NRF_GPIO_PIN_MAP(pinMap[dhtPin].gpio_port, pinMap[dhtPin].gpio_pin), NRF_GPIO_PIN_MAP(0, 29));

    config.sdout_pin = (uint8_t)NRF_GPIO_PIN_MAP(pinMap[trigPin].gpio_port, pinMap[trigPin].gpio_pin);
    config.sdin_pin = (uint8_t)NRF_GPIO_PIN_MAP(pinMap[echoPin].gpio_port, pinMap[echoPin].gpio_pin);
    config.sck_pin = (uint8_t)NRF_GPIO_PIN_MAP(pinMap[unusedPin1].gpio_port, pinMap[unusedPin1].gpio_pin);
    config.lrck_pin = (uint8_t)NRF_GPIO_PIN_MAP(pinMap[unusedPin2].gpio_port, pinMap[unusedPin2].gpio_pin);
    config.mck_pin = NRFX_I2S_PIN_NOT_USED;

    config.mode = NRF_I2S_MODE_MASTER;
    config.format = NRF_I2S_FORMAT_I2S;
    config.alignment = NRF_I2S_ALIGN_LEFT;

    // 16-bit stereo 32000 Hz sample rate = 1,024,000 sample bit rate
    // 32MDIV31 at 32X ratio = 32000 Hz LRCK, 16-bit samples 
    config.sample_width = NRF_I2S_SWIDTH_16BIT;
    config.channels = NRF_I2S_CHANNELS_STEREO;
    config.mck_setup = NRF_I2S_MCK_32MDIV31;
    config.ratio = NRF_I2S_RATIO_32X;

    nrfx_err_t err = nrfx_i2s_init(&config, _dataHandler);
    if (err != NRFX_SUCCESS) {
        Log.info("nrfx_i2s_init error=%lu", err);
        setResult(DistanceResult::Status::ERROR);
        return;
    }

    _buffersRequested = 0;

    // Prepare the output (tx) buffer which is the SDOUT pin for I2S and TRIG for the sensor
    for(size_t ii = 0; ii < numSamples; ii++) {
        rxBuffer[ii] = txBuffer[ii] = 0;
    }
    // 16 bits of 1 is 16 us, which is just a little more than the 10 us minimum TRIG pulse
    txBuffer[0] = 0xffff;
    
    // Sample data. The / 2 factor because the parameter is the number of 32-bit words, not number of 16-bit samples!
    err = nrfx_i2s_start(&_i2sBuffers, numSamples / 2, 0);
    if (err != NRFX_SUCCESS) {
        Log.info("nrfx_i2s_start error=%lu", err);
        setResult(DistanceResult::Status::ERROR);
        return;
    }

    stateHandler = &JSN_SR04_Gen3::sampleState;
    sampleTime = millis();
}

void JSN_SR04_Gen3::sampleState() {
    // safetyTimeoutMs = 30 ms. If this time is reached, something bad happened with the I2S
    // peripheral buffers were not received successfully
    if (_buffersRequested < 2 && millis() - sampleTime < safetyTimeoutMs) {
        // Wait for samples to complete
        return;
    }

    // uninitialize the I2S peripheral
    nrfx_i2s_uninit();

    // TRIG should already be low, but just to be safe
    digitalWrite(trigPin, LOW);

    if (_buffersRequested < 2) {
        // This means the I2S peripheral is in a weird and unknown state (not related to the sensor)
        setResult(DistanceResult::Status::ERROR);
        return;
    }    

    // Calculate distance

    /*
    int count = 0;
    for(size_t ii = 0; ii < numSamples; ii++) {
        if (rxBuffer[ii]) {
            count++;
        }
    }
    for(size_t ii = 0; ii < numSamples; ii += 4) {
        Log.info("%04x: %04x %04x %04x %04x", ii, rxBuffer[ii], rxBuffer[ii + 1], rxBuffer[ii + 2], rxBuffer[ii + 3]);
    }
    Log.info("count=%d rx[0]=%04x rx[%u]=%04x", count, rxBuffer[0], numSamples - 1, rxBuffer[numSamples - 1]);
    */

    int pulseUs = 0;
    for(size_t ii = 0; ii < numSamples; ii++) {
        pulseUs += countOneBits(rxBuffer[ii]);
    }

    if (rxBuffer[numSamples - 1] == 0) {
        double distanceM = ((double)pulseUs) / 1000000 * 170.0;

        setResult(DistanceResult::Status::SUCCESS, distanceM);

        if (distanceAlarm.isValid()) {
            distanceAlarmCallback(lastResult);
        }
    }
    else {
        setResult(DistanceResult::Status::RANGE_ERROR);
    }

    stateHandler = &JSN_SR04_Gen3::idleState;
    isIdle = true;

}

void JSN_SR04_Gen3::distanceAlarmCallback(DistanceResult distanceResult) {

    if (inAlarm) {
        if (distanceAlarm.direction == DistanceAlarm::Direction::GREATER_THAN) {
            if (distanceResult.getDistanceM() < (distanceAlarm.getDistanceM() + distanceAlarm.hysteresis.getDistanceM()) ) {
                // Exiting alarm
                inAlarm = false;
            }
        }
        else {
            if (distanceResult.getDistanceM() > (distanceAlarm.getDistanceM() - distanceAlarm.hysteresis.getDistanceM()) ) {
                // Exiting alarm
                inAlarm = false;
            }

        }
        if (!inAlarm && callback) {
            // Exiting alarm
            DistanceResult tempDistanceResult;
            tempDistanceResult.status = DistanceResult::Status::EXIT_ALARM;
            tempDistanceResult.distanceM = distanceResult.getDistanceM();
            callback(tempDistanceResult);
        }
    }
    else {
        if (distanceAlarm.direction == DistanceAlarm::Direction::GREATER_THAN) {
            if (distanceResult.getDistanceM() > distanceAlarm.getDistanceM()) {
                // Entering alarm
                inAlarm = true;
            }
        }
        else {
            if (distanceResult.getDistanceM() < distanceAlarm.getDistanceM()) {
                // Entering alarm
                inAlarm = true;
            }            
        }
        if (inAlarm && callback) {
            // Entering alarm
            DistanceResult tempDistanceResult;
            tempDistanceResult.status = DistanceResult::Status::ENTER_ALARM;
            tempDistanceResult.distanceM = distanceResult.getDistanceM();
            callback(tempDistanceResult);
        }
    }
}

// [static] 
int JSN_SR04_Gen3::countOneBits(uint16_t sample) {
    if (sample == 0xffff) {
        return 16;
    }
    int count = 0;
    for(int ii = 0; sample != 0; ii++) {
        if (sample & 1) {
            count++;
        }
        sample >>= 1;
    }
    return count;
}





