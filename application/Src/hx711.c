#include "hx711.h"
#include "stdint.h"
#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stdbool.h"
#include "periphery.h"
#include "math.h"

typedef struct {
	int32_t value;
	float scale;
	float offset;
	int8_t hx_dout;
	int8_t hx_sck;
} hx_config_t;

hx_config_t hx_sensors[MAX_AXIS_NUM];
uint8_t hx711_count;

void HX711_InitAll(dev_config_t * p_dev_config, sensor_t * sensors, uint8_t * sensors_cnt) {
	// find SCK pins
	int hx711_sck_count=0;
	for (int i=0; i<USED_PINS_NUM; i++) {
	  if (p_dev_config->pins[i] == HX711_SCK)
		{
			hx_sensors[hx711_sck_count].hx_sck = i;
			hx711_sck_count++;
			if (hx711_sck_count >= MAX_AXIS_NUM) {
				break;
			}
		}
	}
	
	// find DOUT pins and match them with SCK pins
	hx711_count = 0;
	for (int i=0; i<USED_PINS_NUM; i++) {
	  if (p_dev_config->pins[i] == HX711_DOUT && hx711_count < hx711_sck_count)
		{
			hx_sensors[hx711_count].hx_dout = i;
			for (uint8_t k=0; k<MAX_AXIS_NUM; k++)
			{
				if (p_dev_config->axis_config[k].source_main == i && *sensors_cnt < MAX_AXIS_NUM)
				{
					hx_sensors[hx711_count].scale = p_dev_config->axis_config[k].hx711_scale;
					hx_sensors[hx711_count].offset = p_dev_config->axis_config[k].hx711_offset;
					sensors[*sensors_cnt].type = HX711;			
					sensors[*sensors_cnt].source = hx711_count;
					(*sensors_cnt)++;
					break;
				}
			}
			hx711_count++;
		}
	}
}

bool read_dout(hx_config_t hx_sensor) {
	return pin_config[hx_sensor.hx_dout].port->IDR & pin_config[hx_sensor.hx_dout].pin;
}

void pulse_clock(hx_config_t hx_sensor) {
	Delay_us(1);
	pin_config[hx_sensor.hx_sck].port->BSRR |= pin_config[hx_sensor.hx_sck].pin;
	Delay_us(1);
	pin_config[hx_sensor.hx_sck].port->BRR |= pin_config[hx_sensor.hx_sck].pin;
}

void HX711_GetForce(sensor_t * sensors, int8_t source, float * force) {
	int pos = 0;
	for (; pos<hx711_count; ++pos) {
		if (hx_sensors[pos].hx_dout == source) {
			break;
		}
	}
	*force = hx_sensors[pos].scale * ((float)hx_sensors[pos].value - hx_sensors[pos].offset);
}

void HX711_Read(sensor_t * sensor) {
	__disable_irq();
	bool ready = read_dout(hx_sensors[sensor->source]) == false;
	if (!ready) {
		__enable_irq();
		return;
	}
	
	int32_t value = 0;
	
	for (int i=0; i<24; ++i) {
		value = value << 1;
		pulse_clock(hx_sensors[sensor->source]);
		value |= read_dout(hx_sensors[sensor->source]) == true ? 1 : 0;
	}
	pulse_clock(hx_sensors[sensor->source]);
	__enable_irq();
	
	value = (value ^ (1u<<23)) - (1u<<23); //sign extend 24to32
	hx_sensors[sensor->source].value = value;
}
