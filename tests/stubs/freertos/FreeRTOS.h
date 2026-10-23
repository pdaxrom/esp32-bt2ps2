#pragma once
#include <cassert>
#include <cstdint>
using BaseType_t = int;
using UBaseType_t = unsigned;
using TickType_t = uint32_t;
struct FakeSemaphore;
struct FakeQueue;
using SemaphoreHandle_t = FakeSemaphore *;
using QueueHandle_t = FakeQueue *;
using TaskHandle_t = void *;
using TaskFunction_t = void (*)(void *);
constexpr BaseType_t pdTRUE = 1, pdPASS = 1, pdFALSE = 0;
constexpr TickType_t portMAX_DELAY = UINT32_MAX;
constexpr unsigned configMAX_PRIORITIES = 25, portTICK_PERIOD_MS = 1;
#define configASSERT(value) assert(value)
#define IRAM_ATTR
using portMUX_TYPE = int;
#define portMUX_INITIALIZER_UNLOCKED 0
void fake_enter_critical();
void fake_exit_critical();
#define taskENTER_CRITICAL(mux) do { (void)(mux); fake_enter_critical(); } while (0)
#define taskEXIT_CRITICAL(mux) do { (void)(mux); fake_exit_critical(); } while (0)
QueueHandle_t xQueueCreate(unsigned count, unsigned size);
BaseType_t xQueueSend(QueueHandle_t queue, const void *data, TickType_t timeout);
BaseType_t xQueueReceive(QueueHandle_t queue, void *data, TickType_t timeout);
BaseType_t xQueueReset(QueueHandle_t queue);
