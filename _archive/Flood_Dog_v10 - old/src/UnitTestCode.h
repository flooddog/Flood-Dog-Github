
#include "Particle.h"

bool steadyCountTest() {
    const int steadyCountRate = 2000;
    static unsigned long lastCountEvent = millis();
    if (millis() - lastCountEvent > steadyCountRate) {
        lastCountEvent = millis();
        return true;
    }
    else return false;
}