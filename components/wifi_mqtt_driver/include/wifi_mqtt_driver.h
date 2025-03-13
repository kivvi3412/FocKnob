//
// Created by HAIRONG ZHU on 25-3-12.
//

#ifndef WIFI_MQTT_DRIVER_H
#define WIFI_MQTT_DRIVER_H

#include <cJSON.h>
#include <esp_bit_defs.h>
#include <esp_event_base.h>
#include <functional>
#include <mqtt_client.h>
#include <string>
#include <freertos/event_groups.h>

/**
 * @brief MQTT 收到消息的回调函数类型
 * @param[in] topic   订阅主题
 * @param[in] json    已解析的 cJSON 对象指针（使用完后外部可自行 cJSON_Delete(json)）
 */
using MqttMessageCallback = std::function<void(const std::string &topic, cJSON *json)>;


class WifiMqttDriver {
public:
    /**
     * @brief 构造函数
     * @param ssid        Wi-Fi SSID
     * @param password    Wi-Fi 密码
     * @param mqttUri     MQTT Broker 的 URI，例如 "mqtt://xxx.xxx.xxx:1883"
     * @param mqttUser    MQTT 用户名（如有需要，可为空）
     * @param mqttPass    MQTT 密码（如有需要，可为空）
     * @param clientId    MQTT Client ID（可自定义，不填则默认为"esp-client"）
     */
    WifiMqttDriver(std::string ssid,
                   std::string password,
                   std::string mqttUri,
                   std::string mqttUser = "",
                   std::string mqttPass = "",
                   std::string clientId = "esp-client");

    /**
     * @brief 启动 Wi-Fi 并连接，然后初始化并连接 MQTT
     * @note 调用此函数会阻塞直到 Wi-Fi 连接成功（或失败超时），然后再启动 MQTT 连接
     */
    void connect();

    /**
     * @brief 设置收到 MQTT 消息时的回调函数
     * @param cb  回调函数
     */
    void setOnMessageCallback(MqttMessageCallback cb);

    /**
     * @brief 用户「注册」(缓存)一个主题，等待 MQTT 连接成功后再批量订阅
     *
     * @param topic  要订阅的主题
     * @param qos    QOS 等级(0~2)
     *
     * @note 若 MQTT 目前已处于连接状态，则会立即订阅；若尚未连接，则记录下来，等 MQTT
     *       连接成功后在内部批量订阅。
     */
    void registerTopicToSubscribe(const std::string &topic, int qos = 0);

    /**
     * @brief 订阅某个主题
     * @param topic  主题名称
     * @param qos    QOS 等级(0~2)
     */
    void subscribeTopic(const std::string &topic, int qos = 0);

    /**
     * @brief 发布 JSON 格式的消息
     * @param topic     主题名称
     * @param jsonData  要发送的 JSON 字符串
     * @param qos       QOS 等级(0~2)
     * @param retain    是否保留消息
     */
    void publishJson(const std::string &topic, const std::string &jsonData, int qos = 0, bool retain = false);

private:
    // Wi-Fi
    std::string wifiSsid_;
    std::string wifiPassword_;

    static EventGroupHandle_t wifiEventGroup_; // FreeRTOS 事件组，用来等待 Wi-Fi 连接
    static constexpr int WIFI_CONNECTED_BIT = BIT0;
    static constexpr int WIFI_FAIL_BIT = BIT1;
    static constexpr int MAXIMUM_RETRY = 5; // Wi-Fi 连接重试次数
    int retryCount_ = 0;


    void initWifi();

    static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                                   int32_t event_id, void *event_data);

    // MQTT
    std::string mqttUri_;
    std::string mqttUser_;
    std::string mqttPass_;
    std::string mqttClientId_;
    esp_mqtt_client_handle_t mqttClient_{};
    MqttMessageCallback onMessageCb_;
    std::vector<std::pair<std::string, int> > topicsToSubscribe_;
    bool mqttConnected_ = false;

    void initMqtt();

    static void mqtt5_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);

    void subscribeAllRegisteredTopics(); // 连上后订阅所有注册的主题
};


#endif //WIFI_MQTT_DRIVER_H
