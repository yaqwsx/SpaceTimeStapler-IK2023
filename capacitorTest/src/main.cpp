#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_timer.h>

static const int CAP_PIN = 25;
static const int DIS1_PIN = 26;
static const int DIS2_PIN = 27;

static const int HIGH_THR = 3000;
static const int LOW_THR = 500;

void setup() {
    Serial.begin(921600);

    for ( int pin : {CAP_PIN, DIS1_PIN, DIS2_PIN})
        pinMode(pin, INPUT_PULLDOWN);
}

void loop() {
    if ( analogRead( CAP_PIN ) < HIGH_THR )
        return;
    Serial.print("Start measurement: ");
    vTaskSuspendAll();
    auto start = esp_timer_get_time();
    for ( int pin : {DIS1_PIN, DIS2_PIN}) {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, 0);
    }
    while ( analogRead( CAP_PIN ) > LOW_THR );
    auto end = esp_timer_get_time();
    for ( int pin : {DIS1_PIN, DIS2_PIN}) {
        pinMode(pin, INPUT_PULLDOWN);
    }
    xTaskResumeAll();

    Serial.println(end - start);
}
