//#include <s.h>
#include <WiFi.h>
#include "PubSubClient.h"

#define TX_GPIO_NUM 5
#define RX_GPIO_NUM 4

const char* ssid = "Saurabh";                          // SSID
const char* password = "12345678";                  // Password
const char* mqttBroker = "mqtt3.thingspeak.com";     // Broker address
const int mqttPort = 1883;                           // Broker port number
const char* clientID = "FA0MLSgZOSwSBDMOKhgbLjU";    // Client ID
const char* mqtt_topic_1 = "channels/2591627/publish/fields/field1"; // Topic names
const char* mqtt_topic_2 = "channels/2591627/publish/fields/field2";
const char* mqtt_topic_3 = "channels/2591627/publish/fields/field3";
const char* mqtt_topic_4 = "channels/2591627/publish/fields/field4";
//const char* mqtt_topic_5 = "channels/2591627/publish/fields/field5";



WiFiClient MQTTclient;
PubSubClient client(MQTTclient);
long lastReconnectAttempt = 0;

boolean reconnect() {
  if (client.connect(clientID, "FA0MLSgZOSwSBDMOKhgbLjU", "BWagG7HwvuGRKNzzWVaPnWzY")) {
    Serial.println("Connected to MQTT Broker");
  } else {
    Serial.print("Failed to connect to MQTT Broker, rc=");
    Serial.println(client.state());
  }
  return client.connected();
}

void setup() {
  Serial.begin(115200);
  while (!Serial);
  delay(1000);

  Serial.println("Attempting to connect to WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Couldn't connect to WiFi.");
    while (1);
  }
  Serial.print("ESP32 IP Address: ");
  Serial.println(WiFi.localIP());

  client.setServer(mqttBroker, mqttPort);
  lastReconnectAttempt = 0;

  CAN.setPins(RX_GPIO_NUM, TX_GPIO_NUM);
  Serial.println("CAN Receiver");

  if (!CAN.begin(500E3)) {
    Serial.println("Starting CAN failed!");
    while (1);
  }
  Serial.println("CAN Initialized successfully");
}

void loop() {
  if (!client.connected()) {
    long now = millis();
    if (now - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = now;
      if (reconnect()) {
        lastReconnectAttempt = 0;
      }
    }
  } else {
    client.loop();

    int packetSize = CAN.parsePacket();
    if (packetSize) {
      Serial.print("CAN Packet Received: ");
      Serial.print("ID: ");
      Serial.print(CAN.packetId(), HEX);
      Serial.print(", Size: ");
      Serial.println(packetSize);

      if (CAN.packetId() == 0x65D && packetSize == 5) {
        uint8_t temperature = CAN.read();  // Read temperature
        uint8_t motion = CAN.read();       // Read motion
        uint8_t heat = CAN.read();         // Read heat
        uint8_t gas = CAN.read();         // Read gas
       // uint8_t voltage = CAN.read();         // Read voltage

        // Skip reading further bytes if you don't need them

        // Print the received data to Serial Monitor
        Serial.print("Temperature: ");
        Serial.println(temperature);

        Serial.print("Motion: ");
        Serial.println(motion);

        Serial.print("Heat: ");
        Serial.println(heat);

        Serial.print("Gas: ");
        Serial.println(gas);
        
//        Serial.print("Voltage: ");
//        Serial.println(voltage);

        // Publish the received data to the MQTT broker
        if (client.publish(mqtt_topic_1, String(temperature).c_str())) {
          Serial.println("Temperature message published");
        } else {
          Serial.println("Failed to publish temperature message");
        }
        delay(200);

        if (client.publish(mqtt_topic_2, String(motion).c_str())) {
          Serial.println("Motion message published");
        } else {
          Serial.println("Failed to publish motion message");
        }
        delay(200);

        if (client.publish(mqtt_topic_3, String(heat).c_str())) {
          Serial.println("Heat message published");
        } else {
          Serial.println("Failed to publish heat message");
        }
        delay(200);
        
        if (client.publish(mqtt_topic_4, String(gas).c_str())) {
          Serial.println("Gas message published");
        } else {
          Serial.println("Failed to publish gas message");
        }
        delay(200);
      } else {
        Serial.print("Unexpected CAN packet received: ");
        Serial.print("ID: ");
        Serial.print(CAN.packetId(), HEX);
        Serial.print(", Size: ");
        Serial.println(packetSize);
      }
    }
  }
}
