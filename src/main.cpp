#include "M5Atom.h"
#include <Wire.h>
#include "SparkFun_SCD30_Arduino_Library.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <UNIT_ENV.h>
#include "Secret.h"

#define WAIT_MSEC 60000

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;
const char* mqtt_server = MQTT_BROKER_URL;

WiFiClient wifiClient;
PubSubClient client(wifiClient);
#define MSG_BUFFER_SIZE (50)
char msg[MSG_BUFFER_SIZE];

#ifdef CO2
SCD30 airSensor;
#endif
#ifdef AIR_PRESSURE
SHT3X sht30;
QMP6988 qmp6988;
#endif

int value = 0;

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}


void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      /* // Once connected, publish an announcement... */
      /* client.publish("outTopic", "hello world"); */
      /* // ... and resubscribe */
      /* client.subscribe("inTopic"); */
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

#ifdef CO2
unsigned long lastPublishCo2 = 0;
void publishCo2() {
    if (airSensor.dataAvailable()) {
        uint co2 = airSensor.getCO2();
        float temp = airSensor.getTemperature();
        uint humid = airSensor.getHumidity();
        Serial.print("co2(ppm):");
        Serial.print(co2);
        Serial.print(" temp(C):");
        Serial.print(temp, 1);
        Serial.print(" humidity(%):");
        Serial.print(humid, 1);
        Serial.println();
        char co2Str[10];
        char tempStr[10];
        char humidStr[10];
        sprintf(co2Str, "%d", co2);
        sprintf(tempStr, "%f", temp);
        sprintf(humidStr, "%d", humid);
        client.publish("sensor0/co2", co2Str);
        client.publish("sensor0/temp", tempStr);
        client.publish("sensor0/humid", humidStr);
    } else {
        Serial.println("Waiting for new data");
    }
}
#endif

#ifdef AIR_PRESSURE
uint lastAirPressure = 0;
void publishAirPressure() {
    if (sht30.get() != 0) {
        return;
    }
    char temp[10];
    char humid[10];
    char airPressure[10];
    sprintf(temp, "%2.2f", sht30.cTemp);
    sprintf(humid, "%0.2f", sht30.humidity);
    sprintf(airPressure, "%0.2f", qmp6988.calcPressure() / 100);
    Serial.printf("Temperature: %s, Humidity: %s, Air Pressure: %s\n", temp, humid, airPressure);
    client.publish("sensor1/temp", temp);
    client.publish("sensor1/humid", humid);
    client.publish("sensor1/air_pressure", airPressure);
}
#endif

void setup() {
    Serial.begin(115200);
    Serial.println("starting...");
    Wire.begin();

    #ifdef CO2
    if (airSensor.begin() == false) {
        Serial.println("Air sensor not detected. Please check wiring. Freezing..");
        while(1);
    }
    #endif
    #ifdef AIR_PRESSURE
    qmp6988.init();
    #endif

    setup_wifi();
    client.setServer(mqtt_server, 1883);
}

void loop() {
    #ifdef CO2
    if (millis() - lastPublishCo2 > WAIT_MSEC) {
        if (!client.connected()) {
            reconnect();
        }
        publishCo2();
        lastPublishCo2 = millis();
        client.disconnect();
    }
    #endif
    #ifdef AIR_PRESSURE
    if (millis() - lastAirPressure > WAIT_MSEC) {
        if (!client.connected()) {
            reconnect();
        }
        publishAirPressure();
        lastAirPressure = millis();
        client.disconnect();
    }
    #endif
}
