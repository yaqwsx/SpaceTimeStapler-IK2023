#include <Arduino.h>
#include <WiFiUdp.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "fi.hpp"
#include "http.hpp"
#include "quadrature_encoder.hpp"
#include <iostream>
#include <thread>
#include <LiquidCrystal_I2C.h>
#include <esp_timer.h>


#include "tones.hpp"


using namespace std::literals;

static const std::string SERVER = SERVERNAME;
static const std::string CHARSET = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

static const int CAP_PIN = 34;
static const int DIS1_PIN = 16;
static const int DIS2_PIN = 17;

static const int MOTOR_PIN = 33;

static const int HIGH_THR = 1400;
static const int LOW_THR = 500;

static const std::vector< String > VALID_CODES = {
    "AAAAAAAAAAAAAAAA"
};

LiquidCrystal_I2C lcd( 0x27, 20, 4 );
QuadratureEncoder encoder( GPIO_NUM_12, GPIO_NUM_13, GPIO_NUM_14 );


bool isCapPresent() {
    return analogRead( CAP_PIN ) > HIGH_THR;
}

int measureCap() {
    // vTaskSuspendAll();

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
    // xTaskResumeAll();

    return end - start;
}

void doAlert() {
    for ( int i = 0; i != 5; i++ ) {
        lcd.noBacklight();
        delay( 100 );
        lcd.backlight();
        delay( 100 );
        lcd.noBacklight();
        delay( 50 );
        lcd.backlight();
    }
    fuckup();
}


bool isValidCode( const String& code ) {
    for ( const auto& s : VALID_CODES )
        if ( s == code )
            return true;
    return false;
}

struct State {
    bool fuelOverride = false;
    String code = "";

    float fuel = 0.5;
    float capFuel = 0.2;
    float discharge = 0.1;
    int capThreshold = 0;
    int lastCapValue = 0;

    int phase = 0;

    bool isOn() {
        return fuelOverride || (fuel > 1.0 / 16.0);
    }

    bool isComplete() {
        return code.length() == 16;
    }

    void pushChar( char c ) {
        if ( !isComplete() )
            code += c;
    }

    void back() {
        if ( code.length() > 0 ) {
            code.remove(code.length() - 1 );
        }
    }

    void resetCode() {
        code = "";
    }

    void tick() {
        fuel = std::max( 0.0f, fuel - discharge );

        if ( fuel > 0.01 && fuel < 0.3 )
            doAlert();
    }

    void onCap( int value ) {
        lastCapValue = value;
        if ( value < capThreshold )
            return;
        tones.cap();
        fuel = std::min( 1.0f, fuel + capFuel );
    }
};

State STATE{};

char encoderLetter() {
    return CHARSET[ abs( encoder.bigSteps() ) % CHARSET.size() ];
}

struct LcdRenderer {
    void renderLcd( const State& state ) {
        if ( !_lastBacklight )
            lcd.backlight();
        if ( _lastCode != state.code ) {
            lcd.setCursor( 0, 0 );
            lcd.print( "                " );
            lcd.setCursor( 0, 0 );
            lcd.print( state.code.c_str() );
        }
        if ( _lastFuel != state.fuel ) {
            lcd.setCursor( 0, 1 );
            lcd.print( "                " );
            lcd.setCursor( 0, 1 );
            for ( int i = 0; i < 16 * state.fuel; i++ ) {
                lcd.print(char(255));
            }
        }


        lcd.setCursor( state.code.length(), 0 );
        lcd.write( encoderLetter() );
        lcd.setCursor( state.code.length(), 0 );
        _lastBacklight = true;
        _lastCode = state.code;
        _lastFuel = state.fuel;
    }

    void renderDarkLcd() {
        lcd.clear();
        lcd.noBacklight();
        _lastCode = "";
        _lastBacklight = false;
        _lastFuel = -1;
    }

    String _lastCode = "";
    bool _lastBacklight = false;
    float _lastFuel = -1;
};

LcdRenderer LCD_RENDERER;

void motorOn() {
    pinMode( MOTOR_PIN, OUTPUT );
    digitalWrite( MOTOR_PIN, 0 );
}

void motorOff() {
    pinMode( MOTOR_PIN, OUTPUT );
    digitalWrite( MOTOR_PIN, 1 );
}

void prompt( const char *m1, const char *m2 ) {
    lcd.backlight();
    lcd.clear();
    lcd.setCursor( 0, 0 );
    lcd.print(m1);
    lcd.setCursor( 0, 1 );
    lcd.print(m2);
    while (!encoder.wasPressed())
        delay( 100 );

}

void finalStage() {
    prompt("Jsou vsichni", "blizko?");
    prompt("Daleko = ", "nebezpeci!");
    prompt("Jste si", "jisti?");

    lcd.clear();
    lcd.setCursor( 0, 0 );
    lcd.print("OK, sesivam!");
    motorOn();
    famfare();
    lcd.clear();
    lcd.setCursor( 0, 0 );
    lcd.print("Sesivani");
    lcd.setCursor( 0, 1 );
    lcd.print("dokonceno!");

    delay(10000);
    motorOff();
}

void uiRoutine() {
    int lastEncVal = 0;
    int lastTick = millis();
    while ( true ) {
        if ( millis() - lastTick > 10000 ) {
            lastTick = millis();
            STATE.tick();
        }
        bool pressed = encoder.wasPressed();
        if ( STATE.isOn() ) {
            if ( encoder.bigSteps() != lastEncVal ) {
                tones.step();
                lastEncVal = encoder.bigSteps();
            }

            if ( pressed ) {
                STATE.pushChar( encoderLetter() );
                encoder.reset();
                lastEncVal = 0;
                tones.next();
            }

            if ( STATE.isComplete() ) {
                if ( isValidCode( STATE.code ) ) {
                    finalStage();
                    STATE.resetCode();
                }
                else {
                    fuckup();
                    STATE.resetCode();
                    encoder.wasPressed();
                }
            }
            LCD_RENDERER.renderLcd( STATE );
        }
        else {
            STATE.code = "";
            LCD_RENDERER.renderDarkLcd();
        }

        if ( isCapPresent() ) {
            int cap = measureCap();
            STATE.onCap( cap );
        }
        vTaskDelay( 50 / portTICK_PERIOD_MS );
    }
}

void setup() {
    initNvs();
    Serial.begin( 115200 );

    gpio_install_isr_service( 0 );
    encoder.start();

    lcd.init();
    lcd.backlight();
    lcd.blink_on();
    LCD_RENDERER.renderDarkLcd();

    motorOff();

    for ( int pin : {CAP_PIN, DIS1_PIN, DIS2_PIN})
        pinMode(pin, INPUT_PULLDOWN);

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

    std::thread uiThread( uiRoutine );

    while (true) {
        std::string json = "{";
        json += "\"password\": \""s + STATE.code.c_str() + "\", ";
        json += "\"fuel\": "s + std::to_string( STATE.fuel ) + ", ";
        json += "\"phase\": "s + std::to_string( STATE.phase ) + ", ";
        json += "\"lastCap\": "s + std::to_string( STATE.lastCapValue );
        json += "}";
        auto response = postJson( SERVER + "/cpu/register", json );

        if ( response ) {
            cJSON *node = cJSON_GetObjectItem( response.get(), "dischargeRate" );
            if ( node ) {
                STATE.discharge = node->valuedouble;
            }
            node = cJSON_GetObjectItem( response.get(), "capFuel" );
            if ( node ) {
                STATE.capFuel = node->valuedouble;
            }
            node = cJSON_GetObjectItem( response.get(), "overrideFuel" );
            if ( node ) {
                STATE.fuelOverride = node->valueint;
            }
            node = cJSON_GetObjectItem( response.get(), "capThreshold" );
            if ( node ) {
                STATE.capThreshold = node->valueint;
            }
            node = cJSON_GetObjectItem( response.get(), "commands" );
            if ( node ) {
                int size = cJSON_GetArraySize( node );
                for ( int i = 0; i < size; i++ ) {
                    cJSON *cmd = cJSON_GetArrayItem( node, i );
                    std::string c = cmd->valuestring;
                    if ( c == "motoron" ) {
                        motorOn();
                    }
                    if ( c == "motoroff" ) {
                        motorOff();
                    }
                    if ( c == "clear" ) {
                        STATE.resetCode();
                    }
                    if ( c == "back" ) {
                        STATE.back();
                    }
                    if ( c == "fanfare" ) {
                        famfare();
                    }
                    if ( c == "alert" ) {
                        doAlert();
                    }
                    if ( c == "cap" ) {
                        STATE.onCap(99999);
                    }
                }
            }
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void loop() {

}
