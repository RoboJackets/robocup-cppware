#ifndef __TEMP_H
#define __TEMP_H

#include <stdint.h>
typedef struct {
  float ambient; // Ambient temperature reading (LM75)
  float motor;   // Motor temperature reading (Thermistor)
  float die;     // Die temperature reading (STSPIN32)
} temp_readings_t;

temp_readings_t temp_get_readings(void);

float temp_get_ambient(void);
float temp_get_motor(void);
float temp_get_die(uint32_t raw_adc);

#endif /* __TEMP_H */