//
// Created by HAIRONG ZHU on 25-2-11.
//
// logic_manager.cpp
#include "logic_manager.h"

using namespace ArduinoJson;

LogicManager::LogicManager(PressureSensor *pressure_sensor, FocDriver *foc_driver) {
    pressure_sensor_ = pressure_sensor;
    foc_driver_ = foc_driver;
    mode_mutex_ = xSemaphoreCreateMutex();
    mode_info_mutex_ = xSemaphoreCreateMutex();
    xTaskCreatePinnedToCore(_logic_manager_task_static, "logic_manager_task", 4096, this, 0, nullptr, 1);
}

void LogicManager::set_mode(LogicMode *mode) {
    xSemaphoreTake(mode_mutex_, portMAX_DELAY);
    if (current_mode_) {
        current_mode_->destroy(); // 销毁当前模式
    }
    current_mode_ = mode;
    current_mode_->init(); // 初始化新模式
    xSemaphoreGive(mode_mutex_);
}

void LogicManager::register_mode(const std::string &name, LogicMode *mode) {
    xSemaphoreTake(mode_mutex_, portMAX_DELAY);
    modes_.emplace_back(name, mode);
    xSemaphoreGive(mode_mutex_);
}

void LogicManager::set_mode_by_name(const std::string &name) {
    xSemaphoreTake(mode_mutex_, portMAX_DELAY);
    for (size_t i = 0; i < modes_.size(); ++i) {
        if (modes_[i].first == name) {
            if (current_mode_) {
                current_mode_->destroy();
            }
            current_mode_ = modes_[i].second;
            current_mode_index_ = i;
            current_mode_->init();
            break;
        }
    }
    xSemaphoreGive(mode_mutex_);
}

void LogicManager::set_next_mode() {
    xSemaphoreTake(mode_mutex_, portMAX_DELAY);
    if (!modes_.empty()) {
        if (current_mode_) {
            current_mode_->destroy();
        }
        current_mode_index_ = (current_mode_index_ + 1) % modes_.size();
        current_mode_ = modes_[current_mode_index_].second;
        current_mode_->init();
    }
    xSemaphoreGive(mode_mutex_);
}

void LogicManager::set_mode_info(const std::string &key, const std::string &value) {
    xSemaphoreTake(mode_info_mutex_, portMAX_DELAY);
    mode_info_[key] = value;
    xSemaphoreGive(mode_info_mutex_);
}

std::string LogicManager::get_mode_info_mqtt_json() {
    // 生成 MQTT JSON 字符串
    // cJSON *root = cJSON_CreateObject();
    // cJSON *params = cJSON_CreateObject();
    //
    // cJSON_AddStringToObject(root, "method", "control");
    // cJSON_AddStringToObject(root, "clientToken", "foc_knob");
    //
    // for (const auto &mode_info: mode_info_) {
    //     cJSON_AddStringToObject(params, mode_info.first.c_str(), mode_info.second.c_str());
    // }
    // cJSON_AddItemToObject(root, "params", params);
    // mode_info_mqtt_json_ = cJSON_Print(root);
    // cJSON_Delete(root);
    //
    auto json = JsonDocument();
    json["method"] = "control";
    json["clientToken"] = "foc_knob";
    for (const auto &mode_info: mode_info_) {
        json["params"][mode_info.first] = mode_info.second;
    }
    serializeJson(json, mode_info_mqtt_json_);
    return mode_info_mqtt_json_;
}

void LogicManager::_on_press() const {
    // 震动反馈
    current_mode_->stop_motor(); // 按下按钮, 停止当前电机旋钮反馈( 模拟按键按下 )
    foc_driver_->set_dq(0, PRESS_SHOCKPROOFNESS);
    vTaskDelay(pdMS_TO_TICKS(1));
    foc_driver_->set_dq(0, -PRESS_SHOCKPROOFNESS);
    vTaskDelay(pdMS_TO_TICKS(1));
    foc_driver_->set_free();
    current_mode_->resume_motor(); // 启动旋钮电机反馈
}

void LogicManager::_on_release() {
    //在此定义松开按钮时的操作
    set_next_mode();
}

void LogicManager::_logic_manager_task_static(void *arg) {
    auto *self = static_cast<LogicManager *>(arg);
    self->_logic_manager_main_loop();
}

void LogicManager::_logic_manager_main_loop() {
    while (true) {
        // 更新当前模式逻辑
        if (current_mode_) {
            xSemaphoreTake(mode_mutex_, portMAX_DELAY);
            current_mode_->update(); // 更新模式逻辑
            xSemaphoreGive(mode_mutex_);
        }

        if (!modes_.empty()) {
            for (auto &mode: modes_) {
                this->set_mode_info(mode.first, std::to_string(mode.second->get_current_step_index()));
            }
        }

        // 检测按钮按下和松开事件
        bool current_pressed = pressure_sensor_->is_pressed();
        if (current_pressed && !previous_pressed_) {
            // 检测到按下事件
            this->_on_press();
        } else if (!current_pressed && previous_pressed_) {
            // 检测到松开事件
            this->_on_release();
        }
        previous_pressed_ = current_pressed;
    }
}
