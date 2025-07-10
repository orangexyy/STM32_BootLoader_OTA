#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266HTTPClient.h>

// WiFi配置
const char* ssid = "Redmi K50";
const char* password = "cxy030218";

// 巴法云配置
const char* mqtt_server = "bemfa.com";
const int mqtt_port = 9501;
const char* deviceId = "0f40254c8ee0ea74067bb34426aa35f4";
const char* controlTopic = "OTAcontrol";  // 控制主题
const char* statusTopic = "OTAstatus";    // 状态反馈主题

WiFiClient espClient;
PubSubClient client(espClient);

// 缓冲区大小 - 进一步减小以节省内存
#define BUFFER_SIZE 128
uint8_t buffer[BUFFER_SIZE];

// 下载状态
bool downloadInProgress = false;
String pendingFirmwareUrl = "";
uint32_t downloadOffset = 0;
uint32_t totalSize = 0;
bool firstChunk = true;

// 网络监控
unsigned long lastWiFiCheck = 0;
unsigned long lastMqttCheck = 0;
unsigned long lastDownloadActivity = 0;
bool mqttConnected = false;
bool wifiConnected = false;

// 错误计数器
uint8_t downloadErrors = 0;
uint8_t reconnectAttempts = 0;

// 连接WiFi
void setup_wifi() {
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();  // 先断开，确保连接干净
  delay(100);
  
  WiFi.begin(ssid, password);
  
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < 15000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.println("\nWiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("RSSI: ");
    Serial.println(WiFi.RSSI());
  } else {
    wifiConnected = false;
    Serial.println("WiFi connection failed!");
    resetNetworkStack();  // 重置网络栈
  }
}

// 重置网络栈（解决顽固断连问题）
void resetNetworkStack() {
  Serial.println("Resetting network stack...");
  
  // 关闭所有网络连接
  if (client.connected()) {
    client.disconnect();
  }
  
  WiFi.disconnect();
  delay(500);
  
  // 释放内存
  ESP.wdtDisable();
  ESP.wdtEnable(8000);
  
  // 重新初始化WiFi
  setup_wifi();
  
  // 重置计数器
  reconnectAttempts = 0;
}

// MQTT回调函数
void callback(char* topic, byte* payload, unsigned int length) {
  char msg[256] = {0};
  for (unsigned int i = 0; i < length && i < 255; i++) {
    msg[i] = (char)payload[i];
  }
  Serial.printf("MQTT message [%s]: %s\n", topic, msg);

  // 解析下载指令
  if (strncmp(msg, "DOWNLOAD=", 9) == 0) {
    if (downloadInProgress) {
      client.publish(statusTopic, "Error: Download in progress");
      return;
    }
    
    pendingFirmwareUrl = String(msg + 9);
    downloadOffset = 0;
    totalSize = 0;
    firstChunk = true;
    downloadErrors = 0;
    
    Serial.print("Firmware URL: ");
    Serial.println(pendingFirmwareUrl);
    
    client.publish(statusTopic, "Starting firmware download...");
    downloadInProgress = true;
    lastDownloadActivity = millis();
  }
  
  // 重置下载指令
  else if (strcmp(msg, "RESET") == 0) {
    downloadInProgress = false;
    downloadErrors = 0;
    client.publish(statusTopic, "Download reset");
  }
  
  // 手动重置网络
  else if (strcmp(msg, "RESET_NETWORK") == 0) {
    resetNetworkStack();
    client.publish(statusTopic, "Network stack reset");
  }
}

// 连接MQTT服务器
void reconnect() {
  if (!wifiConnected) {
    setup_wifi();
    if (!wifiConnected) return;
  }
  
  client.setKeepAlive(30);  // 30秒保活时间
  
  while (!client.connected() && reconnectAttempts < 5) {
    Serial.print("Connecting to MQTT server...");
    if (client.connect(deviceId)) {
      mqttConnected = true;
      reconnectAttempts = 0;
      Serial.println("Connected");
      client.subscribe(controlTopic);
      client.publish(statusTopic, "OTA relay online");
    } else {
      mqttConnected = false;
      reconnectAttempts++;
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.print(", attempt ");
      Serial.println(reconnectAttempts);
      delay(2000);
      
      if (reconnectAttempts >= 3) {
        Serial.println("Too many failed attempts, resetting network...");
        resetNetworkStack();
      }
    }
  }
  
  if (!client.connected()) {
    Serial.println("MQTT connection failed after retries, will retry later");
  }
}

// 检查WiFi连接
void checkWiFi() {
  if (millis() - lastWiFiCheck > 5000) {
    lastWiFiCheck = millis();
    
    if (WiFi.status() != WL_CONNECTED) {
      if (wifiConnected) {  // 状态变化时才打印
        Serial.println("WiFi disconnected!");
        wifiConnected = false;
      }
      
      // 尝试重连
      setup_wifi();
      
      // 如果重连失败，重置网络栈
      if (!wifiConnected && downloadInProgress) {
        downloadErrors++;
        if (downloadErrors > 3) {
          Serial.println("WiFi reconnect failed, resetting network stack");
          resetNetworkStack();
          downloadErrors = 0;
        }
      }
    } else if (!wifiConnected) {  // 重新连接成功
      wifiConnected = true;
      Serial.println("WiFi reconnected");
    }
  }
}

// 检查MQTT连接
void checkMQTT() {
  if (millis() - lastMqttCheck > 5000) {
    lastMqttCheck = millis();
    
    if (!client.connected()) {
      if (mqttConnected) {  // 状态变化时才打印
        Serial.println("MQTT disconnected!");
        mqttConnected = false;
      }
      
      // 尝试重连
      reconnect();
    } else if (!mqttConnected) {  // 重新连接成功
      mqttConnected = true;
      Serial.println("MQTT reconnected");
    }
  }
}

// 下载并发送一块固件数据（非阻塞）
bool downloadAndSendChunk() {
  // 检查网络状态
  if (!wifiConnected) {
    Serial.println("WiFi not connected, skipping download");
    return false;
  }
  
  // 检查是否长时间无活动（防止卡死）
  if (millis() - lastDownloadActivity > 30000) {
    Serial.println("Download timeout, resetting...");
    downloadErrors++;
    if (downloadErrors > 3) {
      downloadInProgress = false;
      return true;  // 强制结束下载
    }
    return false;
  }
  
  // 先断开MQTT连接
  if (client.connected()) {
    client.disconnect();
    mqttConnected = false;
    delay(100);
  }
  
  // 打印内存状态
  Serial.print("Free heap before download: ");
  Serial.println(ESP.getFreeHeap());
  
  HTTPClient http;
  http.begin(espClient, pendingFirmwareUrl);
  http.addHeader("Range", "bytes=" + String(downloadOffset) + "-");
  
  Serial.printf("Downloading chunk from offset %u...\n", downloadOffset);
  int httpCode = http.GET();
  
  bool downloadComplete = false;
  
  if (httpCode == HTTP_CODE_PARTIAL_CONTENT || httpCode == HTTP_CODE_OK) {
    // 获取文件总大小
    if (totalSize == 0) {
      String contentLength = http.header("Content-Length");
      totalSize = contentLength.toInt();
      if (totalSize == 0) {
        totalSize = http.getSize();
      }
      Serial.printf("Firmware size: %u bytes\n", totalSize);
    }
    
    WiFiClient * stream = http.getStreamPtr();
    uint32_t chunkSize = 0;
    bool wifiLost = false;
    
    // 读取数据块（限制大小，避免长时间阻塞）
    while (http.connected() && chunkSize < 2048 && (http.getSize() > 0 || stream->available())) {
      // 每100ms检查一次WiFi状态（非阻塞）
      if (millis() - lastWiFiCheck > 100) {
        if (WiFi.status() != WL_CONNECTED) {
          wifiLost = true;
          break;
        }
        lastWiFiCheck = millis();
      }
      
      size_t size = stream->available();
      if (size) {
        int c = stream->readBytes(buffer, min(size, (size_t)BUFFER_SIZE));
        
        // 发送数据到串口
        Serial.write(buffer, c);
        
        downloadOffset += c;
        chunkSize += c;
        lastDownloadActivity = millis();
        
        // 每发送一次数据就释放CPU
        yield();
      }
      delay(1);
    }
    
    http.end();
    
    // 检查WiFi状态
    if (wifiLost) {
      Serial.println("WiFi lost during download!");
      downloadErrors++;
      return false;
    }
    
    // 打印下载进度
    if (totalSize > 0) {
      float progress = (float)downloadOffset / totalSize * 100;
      Serial.printf("Progress: %.1f%% (%u/%u bytes)\n", progress, downloadOffset, totalSize);
      
      // 每下载10%就报告一次进度
      if ((downloadOffset % (totalSize / 10)) < BUFFER_SIZE && mqttConnected) {
        String status = "Download progress: " + String((int)progress) + "%";
        client.publish(statusTopic, status.c_str());
      }
    }
    
    // 检查是否下载完成
    if (downloadOffset >= totalSize || chunkSize == 0) {
      downloadComplete = true;
    }
    
  } else {
    Serial.printf("HTTP request failed, code: %d\n", httpCode);
    http.end();
    downloadErrors++;
  }
  
  // 打印内存状态
  Serial.print("Free heap after download: ");
  Serial.println(ESP.getFreeHeap());
  
  return downloadComplete;
}

// 管理固件下载过程
void handleFirmwareDownload() {
  bool downloadComplete = downloadAndSendChunk();
  
  // 下载完成或出错
  if (downloadComplete || downloadErrors > 3) {
    downloadInProgress = false;
    
    if (downloadComplete) {
      Serial.println("Firmware download completed!");
      if (mqttConnected) {
        client.publish(statusTopic, "Firmware download completed!");
      }
      
      // 下载完成后等待10秒再重启，给用户时间查看
      delay(10000);
      ESP.restart();
    } else {
      Serial.println("Download failed after multiple errors");
      if (mqttConnected) {
        client.publish(statusTopic, "Download failed after multiple errors");
      }
    }
  } else {
    // 下载未完成，短暂休眠减轻负担
    delay(500);
    Serial.println("Sleep for a while");
  }
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  
  // 初始化MQTT
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  
  Serial.println("ESP8266 OTA relay initialized");
  Serial.print("Free heap: ");
  Serial.println(ESP.getFreeHeap());
  
  // 首次连接MQTT
  reconnect();
}

void loop() {
  // 常规状态检查
  checkWiFi();
  
  if (downloadInProgress) {
    handleFirmwareDownload();  // 执行分块下载
  } else {
    // 维护MQTT连接
    checkMQTT();
    
    // 处理MQTT消息
    if (client.connected()) {
      client.loop();
    }
  }
  
  // 定期垃圾回收
  if (millis() % 30000 == 0) {
    Serial.print("Free heap: ");
    Serial.println(ESP.getFreeHeap());
  }
}