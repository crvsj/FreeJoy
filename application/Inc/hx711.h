#ifndef HX_711_H
#define HX_711_H

#include "stdint.h"
#include "common_types.h"

void HX711_InitAll(dev_config_t * p_dev_config, sensor_t * sensors, uint8_t * sensors_cnt);
void HX711_GetForce(sensor_t * sensors, int8_t source, float * force);
void HX711_Read(sensor_t * sensor);

#endif
