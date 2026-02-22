#ifndef SENSORS_DATA_H
#define SENSORS_DATA_H
#include "stm32f10x.h"                  // Device header

typedef struct {
    uint16_t        temp;
    uint16_t        humi;
	// ...
} SensorsData_t;

void SensorsData_Update_Temp_Humi(const u16* temp,const u16* humi);
const SensorsData_t* SensorsData_Get(void);

extern SensorsData_t g_sensor_data;
//调用函数更新各个数据，


#endif
