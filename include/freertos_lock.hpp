#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

class RecursiveLock {
public:
    explicit RecursiveLock(SemaphoreHandle_t mutex) : mutex_(mutex) {
        configASSERT(mutex_); xSemaphoreTakeRecursive(mutex_, portMAX_DELAY);
    }
    ~RecursiveLock() { xSemaphoreGiveRecursive(mutex_); }
    RecursiveLock(const RecursiveLock &) = delete;
    RecursiveLock &operator=(const RecursiveLock &) = delete;
private:
    SemaphoreHandle_t mutex_;
};
