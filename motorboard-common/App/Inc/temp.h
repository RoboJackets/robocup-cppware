#ifndef __TEMP_H
#define __TEMP_H

typedef struct {
  float ambient; // Ambient temperature reading (LM75)
  float motor;   // Motor temperature reading (Thermistor)
  float die;     // Die temperature reading (STSPIN32)
} temp_readings_t;

temp_readings_t get_temperature_readings(void);

#endif /* __TEMP_H */