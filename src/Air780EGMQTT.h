#ifndef AIR780EG_MQTT_H
#define AIR780EG_MQTT_H

#include <Arduino.h>
#include "Air780EGCore.h"
#include "Air780EGDebug.h"
#include "Air780EGGNSS.h"

// MQTT连接状态
enum Air780EGMQTTState {
    MQTT_DISCONNECTED = 0,
    MQTT_CONNECTING = 1,
    MQTT_CONNECTED = 2,
    MQTT_DISCONNECTING = 3,
    MQTT_ERROR = 4
};

// MQTT消息结构
struct Air780EGMQTTMessage {
    String topic;
    String payload;
    int qos;
    bool retain;
    unsigned long timestamp;
};

// MQTT连接参数
struct Air780EGMQTTConfig {
    String server;
    int port = 1883;
    String client_id;
    String username;
    String password;
    int keepalive = 60;
    bool clean_session = true;
    bool use_ssl = false;
    String will_topic;
    String will_message;
    int will_qos = 0;
    bool will_retain = false;
};

// MQTT回调函数类型
typedef void (*MQTTMessageCallback)(const String& topic, const String& payload);
typedef void (*MQTTConnectionCallback)(bool connected);

// 定时任务回调函数类型 - 返回要发布的JSON数据
typedef String (*ScheduledTaskCallback)(void);

// 定时任务结构
struct ScheduledTask {
    String topic;                    // 发布主题
    ScheduledTaskCallback callback;  // 数据生成回调函数
    unsigned long interval_ms;       // 执行间隔（毫秒）
    unsigned long last_execution;    // 上次执行时间
    int qos;                        // QoS等级
    bool retain;                    // 保留消息标志
    bool enabled;                   // 任务是否启用
    String task_name;               // 任务名称（用于调试）
};

class Air780EGMQTT {
private:
    static const char* TAG;
    Air780EGGNSS* gnss;
    Air780EGCore* core;
    Air780EGMQTTConfig config;
    Air780EGMQTTState state;
    
    // 回调函数
    MQTTMessageCallback message_callback;
    MQTTConnectionCallback connection_callback;
    
    // 状态管理
    unsigned long last_reconnect_attempt = 0;
    unsigned long reconnect_interval = 5000; // 重连间隔
    
    // 消息缓存
    static const int MAX_CACHED_MESSAGES = 10;
    Air780EGMQTTMessage message_cache[MAX_CACHED_MESSAGES];
    int cache_head = 0;
    int cache_tail = 0;
    int cached_message_count = 0;
    
    // 订阅主题管理
    static const int MAX_SUBSCRIPTIONS = 20;
    String subscriptions[MAX_SUBSCRIPTIONS];
    int subscription_qos[MAX_SUBSCRIPTIONS];
    int subscription_count = 0;
    
    // 定时任务管理
    static const int MAX_SCHEDULED_TASKS = 10;
    ScheduledTask scheduled_tasks[MAX_SCHEDULED_TASKS];
    int scheduled_task_count = 0;
    
    // 内部方法
    bool waitForURC(const String& urc_prefix, String& response, unsigned long timeout = 10000);
    void handleMQTTURC(const String& urc);
    void parseMQTTMessage(const String& message);  // 解析MQTT消息
    String fromHexString(const String& hex);       // HEX转字符串
    bool isHexString(const String& str);           // 检查是否为HEX字符串
    void processMessageCache();
    void processScheduledTasks();  // 处理定时任务
    bool reconnect();
    String toHexString(const String& input);
    
public:
    Air780EGMQTT(Air780EGCore* core_instance, Air780EGGNSS* gnss_instance);
    ~Air780EGMQTT();
    
    // 初始化和配置
    bool begin(const Air780EGMQTTConfig& cfg);
    bool init();
    Air780EGMQTTConfig getConfig() const;
    
    // 连接管理
    bool connect();
    bool connect(const String& server, int port, const String& client_id);
    bool connect(const String& server, int port, const String& client_id, 
                const String& username, const String& password);
    bool disconnect();
    bool isConnected() const;
    Air780EGMQTTState getState() const;
    
    // SSL/TLS支持
    bool enableSSL(bool enable = true);
    bool setSSLConfig(const String& ca_cert = "", const String& client_cert = "", const String& client_key = "");
    
    // 发布消息
    bool publish(const String& topic, const String& payload, int qos = 0, bool retain = false);
    bool publishJSON(const String& topic, const String& json, int qos = 0);
    
    // 定时任务管理
    bool addScheduledTask(const String& task_name, const String& topic, 
                         ScheduledTaskCallback callback, unsigned long interval_ms, 
                         int qos = 0, bool retain = false);
    bool removeScheduledTask(const String& task_name);
    bool enableScheduledTask(const String& task_name, bool enabled = true);
    bool disableScheduledTask(const String& task_name);
    int getScheduledTaskCount() const;
    String getScheduledTaskInfo(int index) const;
    void clearAllScheduledTasks();
    
    // 订阅管理
    bool subscribe(const String& topic, int qos = 0);
    bool unsubscribe(const String& topic);
    bool isSubscribed(const String& topic) const;
    int getSubscriptionCount() const;
    String getSubscription(int index) const;
    
    // 回调函数设置
    void setMessageCallback(MQTTMessageCallback callback);
    void setConnectionCallback(MQTTConnectionCallback callback);
    
    // 主循环处理
    void loop();
    
    // 状态查询
    String getConnectionInfo() const;
    void printStatus() const;
    
    // 消息缓存管理
    bool hasMessages() const;
    Air780EGMQTTMessage getNextMessage();
    void clearMessageCache();
    
    // 配置方法
    void setKeepAlive(int seconds);
    void setReconnectInterval(unsigned long interval_ms);
    void setMaxReconnectAttempts(int attempts);
    
    // 调试方法
    void enableDebug(bool enable = true);
    void printConfig() const;
    
    // URC处理器注册（由主类调用）
    void registerURCHandlers();
};

#endif // AIR780EG_MQTT_H
