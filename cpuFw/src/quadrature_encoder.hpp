#include <driver/gpio.h>
#include <Arduino.h>
#include <functional>

#include "xtensa/core-macros.h"

struct QuadratureEncoder {
    QuadratureEncoder( gpio_num_t aPin, gpio_num_t bPin, gpio_num_t btnPin = GPIO_NUM_MAX)
        : _value( 0 ), _aPin( aPin ), _bPin( bPin ), _btnPin( btnPin )
    {
        _lastTick = millis();
    }

    void start() {
        gpio_set_direction( _aPin, GPIO_MODE_INPUT );
        gpio_set_pull_mode( _aPin, GPIO_PULLUP_ONLY );
        gpio_set_intr_type( _aPin, GPIO_INTR_ANYEDGE );
        gpio_isr_handler_add( _aPin, _onAEdge, this );

        gpio_set_direction( _bPin, GPIO_MODE_INPUT );
        gpio_set_pull_mode( _bPin, GPIO_PULLUP_ONLY );
        gpio_set_intr_type( _bPin, GPIO_INTR_ANYEDGE );
        gpio_isr_handler_add( _bPin, _onBEdge, this );

        gpio_set_direction( _btnPin, GPIO_MODE_INPUT );
        gpio_set_pull_mode( _btnPin, GPIO_PULLUP_ONLY );
        gpio_set_intr_type( _btnPin, GPIO_INTR_POSEDGE );
        gpio_isr_handler_add( _btnPin, _onBtnEdge, this );
    }

    static void _onAEdge( void *arg ) {
        reinterpret_cast< QuadratureEncoder *>( arg )->_onAEdge();
    }

    static void _onBEdge( void *arg ) {
        // reinterpret_cast< QuadratureEncoder *>( arg )->_onBEdge();
    }

    static void _onBtnEdge( void *arg ) {
        reinterpret_cast< QuadratureEncoder *>( arg )->_onBtnEdge();
    }

    void _onAEdge() {
        // delay( 50 );
        if ( millis() - _lastTick < 10 )
            return;
        _lastTick = millis();
        if ( gpio_get_level( _aPin ) )
            if ( gpio_get_level( _bPin ) )
                _value--;
            else
                _value++;
        else
            if ( gpio_get_level( _bPin ) )

                _value++;
            else
                _value--;
    }

    void _onBEdge() {
        if ( !gpio_get_level( _bPin ) )
            if ( gpio_get_level( _aPin ) )
                _value--;
            else
                _value++;
        else
            if ( gpio_get_level( _aPin ) )
                _value++;
            else
                _value--;
    }

    void _onBtnEdge() {
        _pressed = true;
    }

    int bigSteps() {
        return _value / 2;
    }

    void reset() {
        _value = 0;
    }

    bool wasPressed() {
        bool v = _pressed;
        _pressed = false;
        return v;
    }

    int _value;
    gpio_num_t _aPin, _bPin, _btnPin;
    uint32_t _lastTick;
    uint32_t _lastBtnTick = 0;
    bool _pressed = false;
};
