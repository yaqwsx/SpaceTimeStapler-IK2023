#include <Arduino.h>
#include <WiFiUdp.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "fi.hpp"
#include "http.hpp"
#include <iostream>
#include <vector>
#include <algorithm>
#include <numeric>
#include <thread>



using namespace std::literals;

static const std::string SERVER = SERVERNAME;

static const int CHAIN_PIN = 32;
static const int SAMPLES_SIZE = 100;


volatile int ON_THR = 3000;
volatile unsigned long LAST_ON = 0;
volatile int lastAvg = 0;
std::vector< int > SAMPLES;

bool isOn() {
    return millis() - LAST_ON < 3000;
}

void chainRoutine() {
    while ( true ) {
        SAMPLES.push_back(analogRead(CHAIN_PIN));
        while (SAMPLES.size() > SAMPLES_SIZE)
            SAMPLES.erase(SAMPLES.begin()); // Meh, I am ashamed for what I am doing here.

        int avg = std::accumulate(SAMPLES.begin(), SAMPLES.end(), 0) / SAMPLES.size();
        if ( avg < ON_THR )
            LAST_ON = millis();
        lastAvg = avg;

        delay(10);
    }
}

void setup() {
    initNvs();
    Serial.begin( 115200 );

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

    std::thread capThread( chainRoutine );

    while (true) {
        std::string json = "{";
        json += "\"enabled\": "s + (isOn() ? "true" : "false") + ",";
        json += "\"lastValue\": "s + std::to_string(lastAvg) + "";
        json += "}";
        auto response = postJson( SERVER + "/chain/register", json );

        if ( response ) {
            // cJSON *node = cJSON_GetObjectItem( response.get(), "restart" );
            // if ( node && node->valueint) {
            //     ENABLED = false;
            // }
        }

        std::cout << "ON: " << isOn() << "\n";
        std::cout << lastAvg << "\n";
        std::cout << millis() - LAST_ON << "\n";

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void loop() {

}
