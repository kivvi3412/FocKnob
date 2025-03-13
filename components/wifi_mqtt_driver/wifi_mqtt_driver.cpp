//
// Created by HAIRONG ZHU on 25-3-12.
//

#include "wifi_mqtt_driver.h"

#include <esp_log.h>
#include <nvs_flash.h>
#include <esp_wifi.h>

#include <utility>

const char *TAG_WIFI = "WifiMqttDriver";

EventGroupHandle_t WifiMqttDriver::wifiEventGroup_ = nullptr;

WifiMqttDriver::WifiMqttDriver(std::string ssid,
                               std::string password,
                               std::string mqttUri,
                               std::string mqttUser,
                               std::string mqttPass,
                               std::string clientId)
    : wifiSsid_(std::move(ssid)),
      wifiPassword_(std::move(password)),
      mqttUri_(std::move(mqttUri)),
      mqttUser_(std::move(mqttUser)),
      mqttPass_(std::move(mqttPass)),
      mqttClientId_(std::move(clientId)) {
    // 初始化 NVS, 为 Wi-Fi 做准备
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    // 创建 Wi-Fi STA netif
    esp_netif_create_default_wifi_sta();
}

void WifiMqttDriver::connect() {
    this->initWifi();
    this->initMqtt();
}

void WifiMqttDriver::setOnMessageCallback(MqttMessageCallback cb) {
    onMessageCb_ = std::move(cb);
}

void WifiMqttDriver::registerTopicToSubscribe(const std::string &topic, int qos) {
    if (mqttConnected_ && mqttClient_) {
        // 如果 MQTT 处于连接状态，立刻执行订阅
        esp_mqtt_client_subscribe(mqttClient_, topic.c_str(), qos);
        ESP_LOGI(TAG_WIFI, "Subscribe topic immediately: %s, qos: %d", topic.c_str(), qos);
    } else {
        // 否则存到列表里，等到 MQTT_EVENT_CONNECTED 后再批量执行
        topicsToSubscribe_.emplace_back(topic, qos);
        ESP_LOGI(TAG_WIFI, "Register topic to subscribe later: %s, qos: %d", topic.c_str(), qos);
    }
}

void WifiMqttDriver::subscribeTopic(const std::string &topic, int qos) {
    if (mqttConnected_ && mqttClient_) {
        esp_mqtt_client_subscribe(mqttClient_, topic.c_str(), qos);
        ESP_LOGI(TAG_WIFI, "Subscribe topic: %s, qos: %d", topic.c_str(), qos);
    }
}

void WifiMqttDriver::publishJson(const std::string &topic, const std::string &jsonData, int qos, bool retain) {
    if (mqttConnected_ && mqttClient_) {
        int msg_id = esp_mqtt_client_publish(
            mqttClient_,
            topic.c_str(),
            jsonData.c_str(),
            static_cast<int>(jsonData.size()),
            qos,
            (retain ? 1 : 0)
        );
        ESP_LOGI(TAG_WIFI, "Publish to %s, msg_id=%d, data=%s", topic.c_str(), msg_id, jsonData.c_str());
    }
}

void WifiMqttDriver::initWifi() {
    // 创建事件组
    wifiEventGroup_ = xEventGroupCreate();
    retryCount_ = 0;

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // 注册事件回调
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID,
        &WifiMqttDriver::wifi_event_handler,
        this,
        NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP,
        &WifiMqttDriver::wifi_event_handler,
        this,
        NULL));

    wifi_config_t wifi_config = {};
    strncpy((char *) wifi_config.sta.ssid, wifiSsid_.c_str(), sizeof(wifi_config.sta.ssid));
    strncpy((char *) wifi_config.sta.password, wifiPassword_.c_str(), sizeof(wifi_config.sta.password));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    // 等待连接成功或失败
    EventBits_t bits = xEventGroupWaitBits(
        wifiEventGroup_,
        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
        pdFALSE,
        pdFALSE,
        portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG_WIFI, "Wi-Fi connected successfully!");
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGE(TAG_WIFI, "Failed to connect Wi-Fi.");
    } else {
        ESP_LOGE(TAG_WIFI, "UNEXPECTED EVENT");
    }
}

void WifiMqttDriver::wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    auto *driver = static_cast<WifiMqttDriver *>(arg);

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
        ESP_LOGI(TAG_WIFI, "Station started");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        auto *event = static_cast<ip_event_got_ip_t *>(event_data);
        ESP_LOGI(TAG_WIFI, "Got IP:" IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(wifiEventGroup_, WIFI_CONNECTED_BIT);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (driver->retryCount_ < MAXIMUM_RETRY) {
            esp_wifi_connect();
            driver->retryCount_++;
            ESP_LOGW(TAG_WIFI, "Retry to connect to the AP, count=%d", driver->retryCount_);
        } else {
            xEventGroupSetBits(driver->wifiEventGroup_, WIFI_FAIL_BIT);
            ESP_LOGI(TAG_WIFI, "wifiEventHandler: Connection to AP failed");
        }
    }
}

void WifiMqttDriver::initMqtt() {
    esp_mqtt_client_config_t mqtt_cfg = {};
    mqtt_cfg.broker.address.uri = mqttUri_.c_str();
    mqtt_cfg.credentials.client_id = mqttClientId_.c_str();
    if (!mqttUser_.empty()) {
        mqtt_cfg.credentials.username = mqttUser_.c_str();
    }
    if (!mqttPass_.empty()) {
        mqtt_cfg.credentials.authentication.password = mqttPass_.c_str();
    }

    mqttClient_ = esp_mqtt_client_init(&mqtt_cfg);
    ESP_ERROR_CHECK(esp_mqtt_client_register_event(
        mqttClient_,
        (esp_mqtt_event_id_t)ESP_EVENT_ANY_ID,
        &WifiMqttDriver::mqtt5_event_handler,
        this));

    ESP_ERROR_CHECK(esp_mqtt_client_start(mqttClient_));
    ESP_LOGI(TAG_WIFI, "MQTT client started");
}

void WifiMqttDriver::mqtt5_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id,
                                         void *event_data) {
    WifiMqttDriver *driver = static_cast<WifiMqttDriver *>(handler_args);
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t) event_data;
    esp_mqtt_client_handle_t client = event->client;

    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG_WIFI, "MQTT_EVENT_CONNECTED");
            driver->mqttConnected_ = true;
        // 在这里执行一次批量订阅
            driver->subscribeAllRegisteredTopics();
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG_WIFI, "MQTT_EVENT_DISCONNECTED");
            break;
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG_WIFI, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG_WIFI, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG_WIFI, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA: {
            ESP_LOGI(TAG_WIFI, "MQTT_EVENT_DATA");
            // 取出 topic 和 data
            std::string topic(event->topic, event->topic_len);
            std::string data(event->data, event->data_len);

            // 使用 cJSON 解析
            cJSON *json = cJSON_Parse(data.c_str());
            if (json == nullptr) {
                ESP_LOGE(TAG_WIFI, "Failed to parse JSON from received data");
                return;
            }
            // 如果用户有设置消息回调，则调用
            if (driver->onMessageCb_) {
                driver->onMessageCb_(topic, json);
            } else {
                // 如果不需要回调，记得删除 cJSON
                cJSON_Delete(json);
            }
            break;
        }
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG_WIFI, "MQTT_EVENT_ERROR");
            break;
        default:
            break;
    }
}

void WifiMqttDriver::subscribeAllRegisteredTopics() {
    if (!mqttClient_) return;

    for (auto &sub: topicsToSubscribe_) {
        esp_mqtt_client_subscribe(mqttClient_, sub.first.c_str(), sub.second);
        ESP_LOGI(TAG_WIFI, "Batch subscribe topic: %s, qos: %d", sub.first.c_str(), sub.second);
    }
}
