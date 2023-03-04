#include <Arduino.h>
#include <WiFiUdp.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "fi.hpp"
#include "http.hpp"
#include <iostream>
#include <thread>



using namespace std::literals;

static const std::string SERVER = SERVERNAME;
static const std::string CHARSET = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

static const int MEASURE_PIN = 33;
static const int CHG_PIN = 26;
static const int LED_R = 12;
static const int LED_G = 14;
static const int PHOTO_PIN = 32;

static const int ON_THR = 1000;

volatile bool ENABLED = false;

void simCharging() {
    digitalWrite(LED_G, 0);
    for ( int i = 0; i != 30; i++ ) {
        digitalWrite(LED_G, !digitalRead(LED_G));
        if (digitalRead(LED_G))
            delay(50);
        else
            delay(100);
    }
    digitalWrite(LED_G, 1);
    delay(2000);
    digitalWrite(LED_G, 0);
}

void capRoutine() {
    while ( true ) {
        if (analogRead(PHOTO_PIN) < ON_THR)
            ENABLED = true;

        digitalWrite(CHG_PIN, ENABLED);
        digitalWrite(LED_R, ENABLED);

        if (ENABLED && analogRead(MEASURE_PIN) < 3900 )
            simCharging();
        delay(1);
    }
}

void setup() {
    initNvs();
    Serial.begin( 115200 );

    pinMode(LED_R, OUTPUT);
    digitalWrite(LED_R, LOW);
    pinMode(LED_G, OUTPUT);
    digitalWrite(LED_G, LOW);

    pinMode(CHG_PIN, OUTPUT);
    digitalWrite(CHG_PIN, LOW);

    pinMode(35, OUTPUT);
    digitalWrite(35, HIGH);
    pinMode(PHOTO_PIN, INPUT_PULLUP);

    auto connector = WiFiConnector();
    connector.async(
        [&]() {
            std::cout << "Connected!\n";
        },
        [&](auto) {
            std::cout << "We have IP!" << connector.ipAddrStr() << "\n";
        },
        [&](auto, auto) {
            std::cout << "Disconnected\n";
            return true;
        }
    );
    connector.connect( WIFI_SSID, WIFI_PASS );

    std::thread capThread( capRoutine );

    while (true) {
        std::string json = "{";
        json += "\"enabled\": "s + (ENABLED ? "true" : "false") + "";
        json += "}";
        auto response = postJson( SERVER + "/laser/register", json );

        if ( response ) {
            cJSON *node = cJSON_GetObjectItem( response.get(), "restart" );
            if ( node && node->valueint) {
                ENABLED = false;
            }
        }

        std::cout << "Photo: " << analogRead(PHOTO_PIN) << "\n";

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void loop() {

}
