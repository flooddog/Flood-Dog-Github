//Particle Functions
#include "particle_fn.h"
#include "sys_status.h"
#include "Particle.h"



/**
 * @brief Metering function for Particle Publish - 1 second rule
 * 
 * @details Forces a wait so we don't get rate limited
 * 
 * @returns Returns true if it has been more than a second since last publish
 * 
 */
bool meterParticlePublish() {
  static unsigned long lastPublish = 0;  
  
  if (millis() - lastPublish >= 1000) return 1;
  
  return 0;
}