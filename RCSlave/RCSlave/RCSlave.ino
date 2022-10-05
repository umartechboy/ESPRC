/*  Accesspoint - station communication without router
 *  see: https://github.com/esp8266/Arduino/blob/master/doc/esp8266wifi/station-class.rst
 *  Works with: accesspoint_bare_01.ino
 */

#include "ESP32Servo/Servo.h"
#include <WiFi.h>
#include <WiFiUdp.h>
char ssid[] = "Wemos_AP";           // SSID of your AP
char pass[] = "";         // password of your AP

IPAddress server(192, 168, 4, 15);     // IP address of the AP
WiFiUDP udp;
const int ledPin = 2;  // 16 corresponds to GPIO16
#define motorSpeedPin 4
#define motorDirPin1 5
#define motorDirPin2 15
#define steerPin    18
#define steerRange 45
#define steerOffset 0
int currentSteer = 0;
// setting PWM properties
Servo steerServo;  // create servo object to control a servo
void setup() {
    Serial.begin(115200);
    //Serial1.begin(9600);
    WiFi.mode(WIFI_AP);        // connects to the WiFi AP
    WiFi.softAP("RCSlave");
    IPAddress IP(192, 168, 4, 15);
    IPAddress mask = (255, 255, 255, 0);
    WiFi.softAPConfig(IP, IP, mask);

    Serial.println();
    Serial.println("Made AP");
    Serial.println("station_bare_01.ino");
    Serial.print("LocalIP:"); Serial.println(WiFi.softAPIP());
    Serial.println("MAC:" + WiFi.macAddress());
    Serial.print("Gateway:"); Serial.println(WiFi.gatewayIP());
    Serial.print("AP MAC:"); Serial.println(WiFi.BSSIDstr());
    Serial.println();
    pinMode(ledPin, OUTPUT);
    udp.begin(5292);
    // attach the channel to the GPIO to be controlled
    
    steerServo.attach(steerPin, 0);
    ledcSetup(1, 50, 16);
    ledcAttachPin(motorSpeedPin, 1);
    pinMode(motorDirPin1, OUTPUT);
    pinMode(motorDirPin2, OUTPUT);
    ledcWrite(1, 0);
}


int lastLED = 0;
long lastTcp = 0;
bool lastServer = false;
long lastUdp = 0;
WiFiClient* client = 0;
float mapfloat(float v, float vmin, float vmax, float outmin, float outmax)
{
    return outmin + (v - vmin) / (vmax - vmin) * (outmax - outmin);
}
void applySteer(int angle)
{
    angle -= 25;
    angle = -angle;
    steerServo.write((int)constrain(mapfloat(angle, -100, 100, 55, 125), 55, 125));
}
void applyDrive(int power)
{
    bool isNeg = power < 0;
    power = abs(power);
    if (power < 10)
        power = 0;
    if (power > 0)
        power = mapfloat(power, 0, 100, 500, 65535);

    ledcWrite(1, power);
    digitalWrite(motorDirPin1, !isNeg);
    digitalWrite(motorDirPin2, isNeg);
    
}
void loop() {
//    int pos = 0;
//    for (pos = 40; pos <= 125; pos += 1) { // goes from 0 degrees to 180 degrees
//// in steps of 1 degree
//        steerServo.write(pos);              // tell servo to go to position in variable 'pos'
//        delay(15);                       // waits 15ms for the servo to reach the position
//    }
//    for (pos = 130; pos >= 45; pos -= 1) { // goes from 180 degrees to 0 degrees
//        steerServo.write(pos);              // tell servo to go to position in variable 'pos'
//        delay(15);                       // waits 15ms for the servo to reach the position
//    }
//    return;
//    digitalWrite(motorDirPin1, HIGH);
//    digitalWrite(motorDirPin2, LOW);
//    // increase the LED brightness
//    for (int dutyCycle = 0; dutyCycle <= 255; dutyCycle++) {
//        // changing the LED brightness with PWM
//        ledcWrite(1, dutyCycle);
//        delay(10);
//    }
//    // decrease the LED brightness
//    for (int dutyCycle = 255; dutyCycle >= 0; dutyCycle--) {
//        // changing the LED brightness with PWM
//        ledcWrite(1, dutyCycle);
//        delay(15);
//    }
//    digitalWrite(motorDirPin1, LOW);
//    digitalWrite(motorDirPin2, HIGH);
//    // increase the LED brightness
//    for (int dutyCycle = 0; dutyCycle <= 255; dutyCycle++) {
//        // changing the LED brightness with PWM
//        ledcWrite(1, dutyCycle);
//        delay(10);
//    }
//    // decrease the LED brightness
//    for (int dutyCycle = 255; dutyCycle >= 0; dutyCycle--) {
//        // changing the LED brightness with PWM
//        ledcWrite(1, dutyCycle);
//        delay(15);
//    }
//    return;
   /* if (WiFi.status() != WL_CONNECTED) {
        WiFi.mode(WIFI_STA);
        WiFi.enableSTA(true);
        WiFi.begin("Wemos_AP", "");
        while (WiFi.status() != WL_CONNECTED) delay(1);


        Serial.println();
        Serial.println("Connected");
        Serial.println("station_bare_01.ino");
        Serial.print("LocalIP:"); Serial.println(WiFi.localIP());
        Serial.println("MAC:" + WiFi.macAddress());
        Serial.print("Gateway:"); Serial.println(WiFi.gatewayIP());
        Serial.print("AP MAC:"); Serial.println(WiFi.BSSIDstr());
    }*/
    if (udp.parsePacket())
    {
        int steer = udp.readStringUntil('\n').toInt();
        int drive = udp.readStringUntil('\n').toInt();

        digitalWrite(ledPin, lastLED % 2); lastLED++;
        applySteer(steer);
        applyDrive(drive);
        Serial.printf("drive: %d, steer: %d\r\n", drive, steer);
        lastUdp = millis();
        lastServer = true;
    }
    if (millis() - lastUdp > 1000) // connection broken
    {
        if (lastServer)
        {
            digitalWrite(ledPin, LOW); lastLED++;

            applySteer(0);
            applyDrive(0);

            lastServer = false;
            lastUdp = millis();
            Serial.println("Server not sending data");
        }
    }
    return;
    if (millis() - lastTcp > 1000)
    {
        if (client == 0)
        {
            Serial.println("Connecting");
            client = new WiFiClient();
            if (client->connect(server, 5191))
                client->setTimeout(1000);
            else
            {
                delete client;
                client = 0;
                lastTcp = millis();
                return;
            }
        }
        if (client)
        {
            client->println("clientdata");
            if (client->readStringUntil('\n') != "ack")
            {
                lastTcp = millis();
                Serial.println("Connection assumed broken");
                delete client;
                client = 0;
                return;
            }
            client->flush();
            lastTcp = millis();
        }
    }
}

