/*  Accesspoint - station communication without router
 *  see: https://github.com/esp8266/Arduino/blob/master/doc/esp8266wifi/soft-access-point-class.rst
 *       https://github.com/esp8266/Arduino/blob/master/doc/esp8266wifi/soft-access-point-examples.rst
 *       https://github.com/esp8266/Arduino/issues/504
 *  Works with: station_bare_01.ino
 */


#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <Wire.h>
WiFiServer server(5191);
IPAddress IP(192, 168, 4, 15);
IPAddress mask = (255, 255, 255, 0);
WiFiUDP udp;
byte ledPin = 2;

//SlowSoftWire Wire2 = SlowSoftWire(D3, D2);
void setup() {
    Serial.begin(115200);
    Wire.begin();
    //Wire2.begin();
    //Wire.onReceive(rec);
    WiFi.mode(WIFI_STA);
    //WiFi.softAP("Wemos_AP", "");
    //WiFi.softAPConfig(IP, IP, mask);
    //server.begin(5191, 10);
    pinMode(ledPin, OUTPUT);
    Serial.println();
    Serial.println("accesspoint_bare_01.ino");
    Serial.println("Server started.");
    Serial.print("IP: ");     Serial.println(WiFi.softAPIP());
    Serial.print("MAC:");     Serial.println(WiFi.softAPmacAddress());
}

int lastLED = 0;
long lastUdp = 0;
long lastTcp = 0;
WiFiClient clientObj;
bool clientConnected = 0;
long lastIncoming = 0;
float steer = 0;
float steerOffset = 0;
float drive = 0;
long lastDataFetch = 0;
void loop() {

    if (millis() - lastDataFetch > 20)
    {
        lastDataFetch = millis();
        Wire.requestFrom(1, 4);
        long start = millis();
        while (Wire.available() < 4 && millis() - start < 5)
            delay(1);
        if (Wire.available() >= 4)
        {
            uint8_t buf[10];
            Wire.readBytes(buf, 4);
            while (Wire.available())
            {
                Wire.read();
            }
            /*int16_t A0 = (buf[0] + buf[1] * 256);
            int16_t A1 = (buf[2] + buf[3] * 256);
            int16_t A2 = (buf[4] + buf[5] * 256);
            int16_t A3 = (buf[6] + buf[7] * 256);
            uint16_t buttons = buf[8] + buf[9] * 256;*/
            drive = (int8_t)buf[1];
            steer = (int8_t)buf[2];
            Serial.print(drive);
            Serial.print(", ");
            Serial.println(steer);
            /*Serial.print(", A1: "); Serial.print((int8_t)buf[1]);
            Serial.print(", A2: "); Serial.print((int8_t)buf[2]);
            Serial.print(", A3: "); Serial.println((int8_t)buf[3]);*/
            //Serial.print(", bs: "); Serial.println(buttons, 2);
        }
    }
    if (Serial.available())
    {
        String com = Serial.readStringUntil('=');
        String value = Serial.readStringUntil('\n');
        if (com == "steer")
            steer = value.toInt();
        else if (com == "steeroffset")
            steerOffset = value.toInt();
        else if (com == "drive")
            drive = value.toInt();
    }
    if (WiFi.status() != WL_CONNECTED) {
        WiFi.begin("RCSlave");
        while (WiFi.status() != WL_CONNECTED) {

            Serial.print(".");
            delay(500);
            digitalWrite(ledPin, lastLED % 2 ? HIGH : LOW); lastLED++;
        }

        Serial.println();
        Serial.println("Connected");
        Serial.println("station_bare_01.ino");
        Serial.print("LocalIP:"); Serial.println(WiFi.localIP());
        Serial.println("MAC:" + WiFi.macAddress());
        Serial.print("Gateway:"); Serial.println(WiFi.gatewayIP());
        Serial.print("AP MAC:"); Serial.println(WiFi.BSSIDstr());
    }
    else if (millis() - lastUdp > 20)
    {
        lastUdp = millis();
        udp.beginPacket("192.168.4.15", 5292);
        udp.println(steer + steerOffset);
        udp.println(drive);
        udp.endPacket();
        Serial.print("x");
        digitalWrite(ledPin, lastLED % 2); lastLED++;
    }
    if (!clientConnected)
    {
        clientObj = server.available();
        if (clientObj)
            clientConnected = true;
    }
    if (clientConnected)
    {
        if (clientObj.available())
        {
            String incoming = clientObj.readStringUntil('\n');
            if (incoming == "")
            {
                Serial.println("client disconnected 1");
                clientConnected = false;
                return;
            }
            else
                lastIncoming = millis();
            clientObj.print("ack\n");
            clientObj.flush();
            Serial.println(incoming);
        }
    }
    if (clientConnected)
    {
        if (millis() - lastIncoming > 5000)
        {
            Serial.println("client disconnected 2");
            clientConnected = false;
        }
    }
}
