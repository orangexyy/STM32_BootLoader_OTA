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

// Xmodem协议定义
#define SOH 0x01  // 数据块开始(128字节)
#define STX 0x02  // 数据块开始(1024字节)
#define EOT 0x04  // 传输结束
#define ACK 0x06  // 确认
#define NAK 0x15  // 否定确认(请求重发)
#define CAN 0x18  // 取消传输
#define CRC 0x43  // 'C' - 请求CRC校验模式

// Xmodem配置
#define XMODEM_BLOCK_SIZE 128  // 使用128字节块
#define XMODEM_MAX_RETRIES 10  // 最大重试次数
#define XMODEM_TIMEOUT 1000    // 超时时间(毫秒)

// 缓冲区大小
#define BUFFER_SIZE (XMODEM_BLOCK_SIZE + 4)  // 数据块大小 + 头部 + 校验
uint8_t buffer[BUFFER_SIZE];

// 下载状态
bool downloadInProgress = false;
String pendingFirmwareUrl = "";
uint32_t downloadOffset = 0;
uint32_t totalSize = 0;
uint32_t total_Size[4];
bool firstChunk = true;

// 网络监控
#define WIFI_CHECK_INTERVAL 15000
#define MQTT_CHECK_INTERVAL 15000
unsigned long lastWiFiCheck = 0;
unsigned long lastMqttCheck = 0;
unsigned long lastDownloadActivity = 0;
bool mqttConnected = false;
bool wifiConnected = false;

// 错误计数器
uint8_t downloadErrors = 0;
uint8_t reconnectAttempts = 0;

// Xmodem状态
uint8_t xmodemBlockNumber = 1;  // 块编号从1开始
uint8_t xmodemRetries = 0;      // 当前块重试次数
bool xmodemInProgress = false;  // Xmodem传输进行中
uint32_t xmodemLastByteTime = 0; // 上次接收字节的时间

// 新增：版本号和进度信息
String firmwareVersion = "Unknown";  // 存储版本号
uint32_t totalSentBytes = 0;         // 已发送字节数

// 全局缓冲区
static char urlBuffer[256];

// 操作模式
#define MODE_AUTO 0     // 自动模式（上电自动连接）
#define MODE_MANUAL 1   // 手动模式（等待指令连接）
uint8_t operationMode = MODE_MANUAL;  // 默认手动模式

// 查询命令定义（0x01将用于触发连接，原查询状态调整为0x08避免冲突）
#define CMD_CHECK_STATUS 0x08  // 原0x01，修改为0x08避免冲突
#define CMD_START_WIFI 0x02    // 连接WiFi命令
#define CMD_START_MQTT 0x03    // 连接MQTT命令
#define CMD_STOP_ALL 0x04      // 断开所有连接命令
#define CMD_SET_MODE_AUTO 0x05 // 设置为自动模式
#define CMD_SET_MODE_MANUAL 0x06 // 设置为手动模式
#define CMD_START_DOWNLOAD 0x07 // 开始下载命令

// 状态码定义
#define STATUS_INIT 0         // 初始化状态
#define STATUS_WIFI_DISCONN 1 // WiFi未连接
#define STATUS_WIFI_CONN 2    // WiFi已连接
#define STATUS_MQTT_DISCONN 3 // MQTT未连接
#define STATUS_MQTT_CONN 4    // MQTT已连接
#define STATUS_DOWNLOADING 5  // 正在下载

// 计算16位CRC校验值
uint16_t crc16(const uint8_t *data, int length) {
    uint16_t crc = 0;
    for (int i = 0; i < length; i++) {
        crc = (crc << 8) ^ data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x8000)
                crc ^= 0x1021;
            crc <<= 1;
        }
    }
    return crc;
}

// 等待接收特定字符，带超时
bool waitForChar(char expected, unsigned long timeout) {
    unsigned long startTime = millis();
    while (millis() - startTime < timeout) {
        if (Serial.available()) {
            char c = Serial.read();
            if (c == expected) {
                return true;
            }
        }
        if (millis() % 100 == 0) yield();  // 减少yield调用
    }
    return false;
}

// 发送Xmodem数据块
bool sendXmodemBlock(const uint8_t *data, size_t length) {
    // 构建Xmodem数据帧
    buffer[0] = SOH;                 // 帧头
    buffer[1] = xmodemBlockNumber;   // 块编号
    buffer[2] = ~xmodemBlockNumber;  // 块编号取反
    
    // 复制数据
    memcpy(&buffer[3], data, length);
    if (length < XMODEM_BLOCK_SIZE) {
        memset(&buffer[3 + length], 0x1A, XMODEM_BLOCK_SIZE - length);  // 不足补0x1A
    }
    
    // 计算CRC
    uint16_t crc = crc16(&buffer[3], XMODEM_BLOCK_SIZE);
    buffer[3 + XMODEM_BLOCK_SIZE] = (crc >> 8) & 0xFF;  // CRC高字节
    buffer[4 + XMODEM_BLOCK_SIZE] = crc & 0xFF;          // CRC低字节
    
    // 添加扩展信息
    uint8_t* ext_ptr = &buffer[3 + XMODEM_BLOCK_SIZE + 2];  // 扩展信息起始位置
    
    // 版本号（固定16字节，不足补0）
    memset(ext_ptr, 0, 16);
    memcpy(ext_ptr, firmwareVersion.c_str(), firmwareVersion.length() > 16 ? 16 : firmwareVersion.length());
    ext_ptr += 16;
    
    // 固件总大小（4字节，大端序）
    *ext_ptr++ = (totalSize >> 24) & 0xFF;
    *ext_ptr++ = (totalSize >> 16) & 0xFF;
    *ext_ptr++ = (totalSize >> 8) & 0xFF;
    *ext_ptr++ = totalSize & 0xFF;
    
    // 已发送字节大小（4字节，大端序）
    totalSentBytes = (xmodemBlockNumber - 1) * XMODEM_BLOCK_SIZE + length;
    *ext_ptr++ = (totalSentBytes >> 24) & 0xFF;
    *ext_ptr++ = (totalSentBytes >> 16) & 0xFF;
    *ext_ptr++ = (totalSentBytes >> 8) & 0xFF;
    *ext_ptr++ = totalSentBytes & 0xFF;
    
    // 发送数据块
    Serial.write(buffer, XMODEM_BLOCK_SIZE + 5 + 24);
    
    // 等待ACK（优化超时和重试逻辑）
    xmodemLastByteTime = millis();
    while (millis() - xmodemLastByteTime < (XMODEM_TIMEOUT * (1 << xmodemRetries))) {
        if (Serial.available()) {
            char c = Serial.read();
            xmodemLastByteTime = millis();
            
            if (c == ACK) {
                xmodemBlockNumber++;
                xmodemRetries = 0;
                return true;
            } else if (c == NAK || c == CAN) {
                xmodemRetries++;
                return false;
            }
        }
        if (millis() % 100 == 0) yield();  // 减少yield调用
    }
    
    // 超时后指数退避
    xmodemRetries++;
    delay(100 * xmodemRetries);  // 退避延时
    return false;
}

// 初始化Xmodem传输
bool startXmodemTransfer() {
    // Serial.println("Starting Xmodem transfer...");
    xmodemBlockNumber = 1;
    xmodemRetries = 0;
    xmodemInProgress = true;
    
    // 等待接收方发送'C'请求CRC模式
    if (!waitForChar(CRC, 5000)) {
        // Serial.println("No CRC request received, aborting");
        xmodemInProgress = false;
        return false;
    }
    
    // Serial.println("CRC mode enabled");
    return true;
}

// 完成Xmodem传输
void finishXmodemTransfer() {
    // 发送EOT
    for (int i = 0; i < 3; i++) {
        Serial.write(EOT);
        delay(100);
        
        // 等待ACK
        if (waitForChar(ACK, 1000)) {
            // Serial.println("Xmodem transfer completed successfully");
            return;
        }
    }
    
    // Serial.println("Failed to confirm transfer completion");
}

// 连接WiFi
void setup_wifi() {
//   Serial.println();
//   Serial.print("Connecting to ");
//   Serial.println(ssid);

  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(false);
  
  WiFi.begin(ssid, password);
  
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < 10000) {
    delay(200);
    yield();
  }

  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    // Serial.println("\nWiFi connected");
    // Serial.print("IP address: ");
    // Serial.println(WiFi.localIP());
  } else {
    wifiConnected = false;
    // Serial.println("WiFi connection failed!");
  }
}

// 重置网络栈
void resetNetworkStack() {
//   Serial.println("Resetting network stack...");
  
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
  memset(urlBuffer, 0, 256);
  memcpy(urlBuffer, payload, length > 255 ? 255 : length);
  
  // 解析带版本号的下载指令
  if (strncmp(urlBuffer, "DOWNLOAD=", 9) == 0) {
    if (downloadInProgress) {
      client.publish(statusTopic, "Error: Download in progress");
      return;
    }
    
    char* urlAndVersion = urlBuffer + 9;
    char* versionSep = strstr(urlAndVersion, ",version=");
    
    if (versionSep) {
      *versionSep = '\0';  // 截断URL部分
      pendingFirmwareUrl = String(urlAndVersion);
      firmwareVersion = String(versionSep + 9);  // 提取版本号
    } else {
      pendingFirmwareUrl = String(urlAndVersion);
      firmwareVersion = "Unknown";
    }
    
    // 初始化下载参数
    downloadOffset = 0;
    totalSize = 0;
    totalSentBytes = 0;
    firstChunk = true;
    downloadErrors = 0;
    
    // Serial.print("Firmware URL: ");
    // Serial.println(pendingFirmwareUrl);
    // Serial.print("Firmware version: ");
    // Serial.println(firmwareVersion);
    
    client.publish(statusTopic, "Starting firmware download...");
    downloadInProgress = true;
    lastDownloadActivity = millis();
  }
  
  // 重置下载指令
  else if (strcmp(urlBuffer, "RESET") == 0) {
    downloadInProgress = false;
    downloadErrors = 0;
    client.publish(statusTopic, "Download reset");
  }
  
  // 手动重置网络
  else if (strcmp(urlBuffer, "RESET_NETWORK") == 0) {
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
    // Serial.print("Connecting to MQTT server...");
    if (client.connect(deviceId)) {
      mqttConnected = true;
      reconnectAttempts = 0;
      client.subscribe(controlTopic);
      client.publish(statusTopic, "OTA relay online");
    } else {
      mqttConnected = false;
      reconnectAttempts++;
    //   Serial.print("Failed, rc=");
    //   Serial.print(client.state());
    //   Serial.print(", attempt ");
    //   Serial.println(reconnectAttempts);
      delay(2000);
      
      if (reconnectAttempts >= 3) {
        // Serial.println("Too many failed attempts, resetting network...");
        resetNetworkStack();
      }
    }
  }
  
  if (!client.connected()) {
    // Serial.println("MQTT connection failed after retries, will retry later");
  }
}

// 检查WiFi连接
void checkWiFi() {
  static unsigned long lastCheck = 0;
  if (millis() - lastCheck < WIFI_CHECK_INTERVAL) return;
  lastCheck = millis();
  
  if (WiFi.status() != WL_CONNECTED) {
    // Serial.println("WiFi disconnected!");
    wifiConnected = false;
    setup_wifi();
  } else if (!wifiConnected) {
    wifiConnected = true;
    // Serial.println("WiFi reconnected");
  }
}

// 检查MQTT连接
void checkMQTT() {
  static unsigned long lastCheck = 0;
  if (millis() - lastCheck < MQTT_CHECK_INTERVAL) return;
  lastCheck = millis();
  
  if (!client.connected()) {
    // Serial.println("MQTT disconnected!");
    mqttConnected = false;
    reconnect();
  } else if (!mqttConnected) {
    mqttConnected = true;
    // Serial.println("MQTT reconnected");
  }
}

// 发送状态信息到STM32
void sendStatus() {
    uint8_t status = getCurrentStatus();
    
    // 构建状态响应帧
    uint8_t response[6] = {0xAA, 0x55, 0x04, 0x01, status, 0};
    response[5] = calculateChecksum(&response[2], 3);  // 计算校验和
    
    // 发送响应
    Serial.write(response, 6);
}

// 获取当前状态
uint8_t getCurrentStatus() {
    if (downloadInProgress) return STATUS_DOWNLOADING;
    if (!wifiConnected) return STATUS_WIFI_DISCONN;
    if (!mqttConnected) return STATUS_MQTT_DISCONN;
    return STATUS_MQTT_CONN;
}

// 计算校验和
uint8_t calculateChecksum(uint8_t* data, uint8_t length) {
    uint8_t checksum = 0;
    for (uint8_t i = 0; i < length; i++) {
        checksum ^= data[i];
    }
    return checksum;
}

// 处理串口命令（核心修改：接收到0x01时连接WiFi和服务器）
void handleSerialCommand() {
    if (Serial.available()) {
        uint8_t cmd = Serial.read();
        
        // 核心修改：接收到0x01时，强制连接WiFi和MQTT
        if (cmd == 0x01) {
            // Serial.println("Received 0x01, connecting WiFi and server...");
            
            // 1. 连接WiFi（未连接时）
            if (!wifiConnected) {
                setup_wifi();
                if (!wifiConnected) {
                    sendStatus();  // 返回WiFi未连接状态
                    return;
                }
            }
            
            // 2. 连接MQTT（WiFi已连但MQTT未连时）
            if (!mqttConnected) {
                reconnect();
                if (!mqttConnected) {
                    sendStatus();  // 返回MQTT未连接状态
                    return;
                }
            }
            
            // 3. 连接成功，返回状态
            sendStatus();
            return;  // 处理完毕，退出
        }
        
        // 以下为原命令处理（保持不变）
        switch (cmd) {
            case CMD_CHECK_STATUS:
                // 查询状态
                sendStatus();
                break;
                
            case CMD_START_WIFI:
                // 连接WiFi
                if (!wifiConnected) {
                    setup_wifi();
                    sendStatus();  // 返回状态
                }
                break;
                
            case CMD_START_MQTT:
                // 连接MQTT
                if (wifiConnected && !mqttConnected) {
                    reconnect();
                    sendStatus();  // 返回状态
                } else if (!wifiConnected) {
                    // WiFi未连接，先连接WiFi
                    setup_wifi();
                    if (wifiConnected) {
                        reconnect();
                    }
                    sendStatus();  // 返回状态
                }
                break;
                
            case CMD_STOP_ALL:
                // 断开所有连接
                if (client.connected()) {
                    client.disconnect();
                    mqttConnected = false;
                }
                if (wifiConnected) {
                    WiFi.disconnect();
                    wifiConnected = false;
                }
                sendStatus();  // 返回状态
                break;
                
            case CMD_SET_MODE_AUTO:
                // 设置为自动模式
                operationMode = MODE_AUTO;
                // Serial.println("Mode set to AUTO");
                // 如果当前未连接，则开始连接
                if (!wifiConnected) {
                    setup_wifi();
                }
                if (wifiConnected && !mqttConnected) {
                    reconnect();
                }
                sendStatus();  // 返回状态
                break;
                
            case CMD_SET_MODE_MANUAL:
                // 设置为手动模式
                operationMode = MODE_MANUAL;
                // Serial.println("Mode set to MANUAL");
                sendStatus();  // 返回状态
                break;
                
            case CMD_START_DOWNLOAD:
                // 从MQTT接收的最后一个URL开始下载
                if (!downloadInProgress && mqttConnected && !pendingFirmwareUrl.isEmpty()) {
                    downloadOffset = 0;
                    totalSize = 0;
                    totalSentBytes = 0;
                    firstChunk = true;
                    downloadErrors = 0;
                    
                    // Serial.println("Starting firmware download from stored URL...");
                    client.publish(statusTopic, "Starting firmware download...");
                    downloadInProgress = true;
                    lastDownloadActivity = millis();
                } else {
                    // 发送错误状态
                    uint8_t errorStatus = downloadInProgress ? STATUS_DOWNLOADING : 
                                         (!mqttConnected ? STATUS_MQTT_DISCONN : 0xFF);
                    uint8_t response[6] = {0xAA, 0x55, 0x04, 0x02, errorStatus, 0};
                    response[5] = calculateChecksum(&response[2], 3);
                    Serial.write(response, 6);
                }
                break;
                
            default:
                // 未知命令
                uint8_t response[6] = {0xAA, 0x55, 0x04, 0xFF, 0x00, 0};
                response[5] = calculateChecksum(&response[2], 3);
                Serial.write(response, 6);
                break;
        }
    }
}

// 下载并通过Xmodem协议发送一块固件数据
bool downloadAndSendChunk() {
    // 检查网络状态
    if (!wifiConnected) {
        // Serial.println("WiFi not connected, skipping download");
        return false;
    }
    
    // 检查是否长时间无活动
    if (millis() - lastDownloadActivity > 30000) {
        // Serial.println("Download timeout, resetting...");
        downloadErrors++;
        if (downloadErrors > 3) {
            downloadInProgress = false;
            xmodemInProgress = false;
            return true;  // 强制结束下载
        }
        return false;
    }
    
    // 首次下载，初始化Xmodem
    if (!xmodemInProgress && downloadInProgress) {
        if (!startXmodemTransfer()) {
            downloadErrors++;
            return false;
        }
    }
    
    HTTPClient http;
    http.begin(espClient, pendingFirmwareUrl);
    http.addHeader("Range", "bytes=" + String(downloadOffset) + "-");
    
    // Serial.printf("Downloading chunk from offset %u...\n", downloadOffset);
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
            // Serial.printf("Firmware size: ");
            // Serial.write(0x23);
            // Serial.write(0x53);
            // Serial.printf("%u", totalSize);
            // Serial.write(0x23);
        }
        
        WiFiClient * stream = http.getStreamPtr();
        
        // 读取数据块
        uint8_t xmodemData[XMODEM_BLOCK_SIZE];
        int bytesRead = stream->readBytes(xmodemData, XMODEM_BLOCK_SIZE);
        
        if (bytesRead > 0) {
            // 发送数据块（带指数退避重试）
            bool blockSent = false;
            while (!blockSent && xmodemRetries < XMODEM_MAX_RETRIES) {
                blockSent = sendXmodemBlock(xmodemData, bytesRead);
                if (!blockSent) {
                    // Serial.printf("Block %d send failed, retry %d\n", xmodemBlockNumber, xmodemRetries);
                }
            }
            
            if (blockSent) {
                downloadOffset += bytesRead;
                lastDownloadActivity = millis();
                
                // 打印下载进度
                if (totalSize > 0) {
                    float progress = (float)downloadOffset / totalSize * 100;
                    Serial.write(0x23);
                    Serial.write(0x50);
                    // Serial.printf("Progress: %.1f%% (%u/%u bytes)", progress, downloadOffset, totalSize);
                    Serial.write(0x23);
                    
                    // 每下载10%报告一次进度
                    if ((downloadOffset % (totalSize / 10)) < XMODEM_BLOCK_SIZE && mqttConnected) {
                        String status = "Download progress: " + String((int)progress) + "%";
                        client.publish(statusTopic, status.c_str());
                    }
                }
            } else {
                // Serial.printf("Failed to send block %d after retries\n", xmodemBlockNumber);
                downloadErrors++;
            }
        }
        
        http.end();
        
        // 检查WiFi状态
        if (WiFi.status() != WL_CONNECTED) {
            // Serial.println("WiFi lost during download!");
            downloadErrors++;
            return false;
        }
        
        // 检查是否下载完成
        if (downloadOffset >= totalSize) {
            downloadComplete = true;
        }
        
    } else {
        // Serial.printf("HTTP request failed, code: %d\n", httpCode);
        http.end();
        downloadErrors++;
    }
    
    return downloadComplete;
}

// 管理固件下载过程
void handleFirmwareDownload() {
    bool downloadComplete = downloadAndSendChunk();
    
    // 下载完成或出错
    if (downloadComplete || downloadErrors > 3) {
        downloadInProgress = false;
        
        if (downloadComplete) {
            // 完成Xmodem传输
            finishXmodemTransfer();
            
            // Serial.println("Firmware download completed!");
            if (mqttConnected) {
                client.publish(statusTopic, "Firmware download completed!");
            }
            
            // 下载完成后等待10秒再重启
            delay(10000);
            ESP.restart();
        } else {
            // Serial.println("Download failed after multiple errors");
            if (mqttConnected) {
                client.publish(statusTopic, "Download failed after multiple errors");
            }
        }
    } else {
        // 下载未完成，短暂休眠减轻负担
        delay(500);
    }
}

void setup() {
  Serial.begin(115200);
//   Serial.println("ESP8266 OTA relay initialized, waiting for commands...");
  
  // 初始化MQTT
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  
  // 根据模式决定是否立即连接
  if (operationMode == MODE_AUTO) {
    setup_wifi();
    reconnect();
  }
  
  // 启用看门狗
  ESP.wdtEnable(WDTO_8S);
}

void loop() {
  ESP.wdtFeed();  // 喂狗
  
  // 处理串口命令
  handleSerialCommand();
  
  // 常规状态检查（仅在连接后执行）
  if (wifiConnected) {
    checkWiFi();
  }
  
  if (downloadInProgress) {
    handleFirmwareDownload();  // 执行分块下载
  } else if (wifiConnected && mqttConnected) {
    // 维护MQTT连接
    checkMQTT();
    
    // 处理MQTT消息
    if (client.connected()) {
      client.loop();
    }
  }
  
  // 定期垃圾回收
  if (millis() % 60000 == 0) {
    // Serial.print("Free heap: ");
    // Serial.println(ESP.getFreeHeap());
  }
}