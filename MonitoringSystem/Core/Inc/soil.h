/*
 * soil.h
 *
 *      Author: logan
 */

#ifndef SOIL_H_
#define SOIL_H_

#include <stdint.h>

// Soil Moisture Library
typedef struct Keyestudio_SoilSensor_tag
{
	uint16_t Dryness;
	uint16_t RawValue;
}Keyestudio_SoilSensor_t;

void SoilMoistureSensor_Init(Keyestudio_SoilSensor_t *sensor);
void SoilMoistureSensor_GetValue(Keyestudio_SoilSensor_t *sensor);

#endif /* SOIL_H_ */
