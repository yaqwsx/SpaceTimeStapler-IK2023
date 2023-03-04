#include <library/BlackBox_Manager.hpp>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "fi.hpp"
#include "http.hpp"
#include <iostream>
#include <thread>

using namespace BlackBox;
using namespace std::literals;

static const int BRIGHTNESS = 50;
static const int SHUTDOWN_SECS = 5;
static const std::string SERVER = SERVERNAME;

static const Rgb RED( 255, 0, 0 );
static const Rgb GREEN( 0, 255, 0 );
static const Rgb BLUE( 0, 0, 255 );
static const Rgb YELLOW( 128, 128, 0 );

void showWiFiStatus( bool successfull, bool hasIp ) {
    Manager& man = Manager::singleton();
    auto& beacon = man.beacon();
    auto top = beacon.top();
    if (successfull && hasIp)
        top.fill( Rgb( 0, 50, 0 ) );
    else if (successfull && !hasIp)
        top.fill( Rgb( 0, 0, 50 ) );
    else
        top.fill( Rgb(50, 0, 0 ) );
    beacon.show(BRIGHTNESS);
}

void runRestartMode() {
    Manager& man = Manager::singleton();

    auto& beacon = man.beacon();
    auto perimeter = beacon.perimeter();

    perimeter.drawArc(RED, 0, 12);
    perimeter.drawArc(GREEN, 13, 25);
    perimeter.drawArc(BLUE, 26, 38);
    perimeter.drawArc(YELLOW, 39, 51);

    beacon.show(BRIGHTNESS);
}

void runNormalMode() {
    Manager& man = Manager::singleton();

    auto& beacon = man.beacon();
    auto perimeter = beacon.perimeter();

    perimeter.clear();

    beacon.show(BRIGHTNESS);
}

int getPressure(BlackBox::Touchpad& touch, int samples = 50) {
    int pressure = 0;
    for ( int i = 0; i != samples; i++) {
        pressure += touch.calculate().pressure;
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    pressure /= samples;
    return pressure;
}

void shutdownRoutine() {
    Manager& man = Manager::singleton();

    auto& power = man.power();
    auto& beacon = man.beacon();
    auto top = beacon.top();
    auto& ldc = man.ldc();
    auto& touch = man.touchpad();
    ldc.init();
    touch.init(&ldc);

    vTaskDelay(1000 / portTICK_PERIOD_MS);
    const int INITIAL_SAMPLES = 1000;
    int initialPressure = 0;
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    for ( int i = 0; i != INITIAL_SAMPLES; i++) {
        initialPressure += touch.calculate().pressure;
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    initialPressure /= INITIAL_SAMPLES;

    int pressedDown = 0;
    while ( true ) {
        if (getPressure(touch) - initialPressure > 20) {
            pressedDown++;
            top[0] = Rgb(0, 0, 255);
            beacon.show( BRIGHTNESS );
        }
        else
            pressedDown = 0;
        if (pressedDown >= SHUTDOWN_SECS) {
            std::cout << "Goodbye\n";
            power.turnOff();
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

}

extern "C" void app_main() {
    initNvs();
    Manager& man = Manager::singleton();
    man.init();
    man.power().turnOn5V();

    bool hasId = false;
    int id = 0;

    auto connector = WiFiConnector();
    connector.async(
        [&]() {
            showWiFiStatus( true, false );
            std::cout << "Connected!\n";
        },
        [&](auto) {
            showWiFiStatus( true, true );
            std::cout << "We have IP!" << connector.ipAddrStr() << "\n";
        },
        [&](auto, auto) {
            hasId = false;
            showWiFiStatus( false, false );
            std::cout << "Disconnected\n";
            return true;
        }
    );
    connector.connect( SSID, PASS );

    std::thread shutdownThread( shutdownRoutine );

    auto& doors = man.doors();
    for ( auto& d : doors ) {
        d.init();
    }

    auto handleDoors = [&]( cJSON* j, int i ) {
        static const std::array< const char *, 4 > DOOR_NAMES = {
            "doorA", "doorB", "doorC", "doorD"
        };
        cJSON *node = cJSON_GetObjectItem(j, DOOR_NAMES[ i ]);
        if ( !node )
            return;
        if ( node->valueint )
            doors[ i ].open();
        else
            doors[ i ].close();
    };

    while (true) {
        if ( !hasId ) {
            auto idJson = getJson( SERVER + "/lanterns/register" );
            if ( idJson ) {
                cJSON *node = cJSON_GetObjectItem( idJson.get(), "id" );
                if ( node ) {
                    id = node->valueint;
                    hasId = true;
                }
            }
        }

        if ( hasId ) {
            postJson( SERVER + "/lanterns/" + std::to_string( id ) + "/register",
                "{\"battery\": "s + std::to_string(man.power().batteryPercentage( true )) + "}" );
        }

        auto doorJson = getJson(SERVER + "/lanterns/doors");
        if ( doorJson ) {
            cJSON *node = cJSON_GetObjectItem( doorJson.get(), "restarting" );
            if ( node && node->valueint )
                runRestartMode();
            else
                runNormalMode();

            for ( int i = 0; i != 4; i++ )
                handleDoors( doorJson.get(), i );
        }
        std::cout << "Batt: " << man.power().batteryPercentage( true ) << "\n";
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

