/*
 * soil.c
 *
 *      Author: logan
 */

#include "soil.h"

void SoilMoistureSensor_Init(Keyestudio_SoilSensor_t *sensor)
{
	sensor->Dryness = 0.0f;
	sensor->RawValue = 0x0000U;
}

void SoilMoistureSensor_GetValue(Keyestudio_SoilSensor_t *sensor)
{
	// 2582 -> 2.3V Max analog output
	if( sensor->RawValue <= 2582)
	{
		sensor->Dryness = ((sensor->RawValue * 100) / 2582 );
	}
}
