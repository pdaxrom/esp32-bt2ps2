#pragma once
#include "FreeRTOS.h"
BaseType_t xTaskCreate(TaskFunction_t, const char *, uint32_t, void *, UBaseType_t, TaskHandle_t *);
BaseType_t xTaskCreatePinnedToCore(TaskFunction_t, const char *, uint32_t, void *, UBaseType_t, TaskHandle_t *, BaseType_t);
void vTaskDelay(TickType_t ticks);
void vTaskDelete(TaskHandle_t);
