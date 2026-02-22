#include "sensors_data.h"
#include "stddef.h"
SensorsData_t g_sensor_data = {        
    .temp = 0,
    .humi = 0
	// ...
};



void SensorsData_Update_Temp_Humi(const u16* temp,const u16* humi) {
	
    if (temp == NULL || humi == NULL) return;
    
    g_sensor_data.temp = *temp; 
	g_sensor_data.humi = *humi;
}

// 安全获取只读指针（外部只能读，不能改）

const SensorsData_t* SensorsData_Get(void) {
    return &g_sensor_data;  // ← 这是一个“读取”操作！
}


