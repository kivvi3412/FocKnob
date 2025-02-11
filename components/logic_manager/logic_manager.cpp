//
// Created by HAIRONG ZHU on 25-2-11.
//

#include "logic_manager.h"

LogicManager::LogicManager() {
    /*
     * @brief 如果不加入信号量会产生的问题: 简单来说就是set_mode的时候把老的current_mode_给删了，然后 while 循环里访问update指针暴毙了
     */
    mode_mutex_ = xSemaphoreCreateMutex();
    xTaskCreatePinnedToCore(_logic_manager_task_static, "logic_manager_task", 4096, this, 0, NULL, 1);
}


void LogicManager::set_mode(LogicMode *mode) {
    xSemaphoreTake(mode_mutex_, portMAX_DELAY);
    if (current_mode_) {
        delete current_mode_;  // 释放原有的模式对象
    }
    current_mode_ = mode;
    current_mode_->init();  // 初始化新模式
    xSemaphoreGive(mode_mutex_);
}


void LogicManager::_logic_manager_task_static(void *arg) {
    auto *self = static_cast<LogicManager *>(arg);
    self->_logic_manager_main_loop();
}

void LogicManager::_logic_manager_main_loop() {
    while (true) {
        if (current_mode_) {
            xSemaphoreTake(mode_mutex_, portMAX_DELAY);
            current_mode_->update();  // 更新模式逻辑
            xSemaphoreGive(mode_mutex_);
        }
    }
}



