
// BOARD: "Arduino Nano"
// PROCESSOR: "ATmega328P (Old Bootloader)"
// PROGRAMMER: "ArduinoISP"

const int STATUS_LED_PIN = 13;

#include "rako.h"

const int RAKO_RX_ENABLE_PIN = A0;
const int RAKO_RX_PIN = A1;
const int RAKO_RSSI_PIN = A2; // not used

enum {
    boot_char = '!',
    error_char = '?',
    heartbeat_char = '.'
};

enum {
    send_led_time = 500, // ms
    repeat_led_time = 100,
    heartbeat_led_time = 10,
};

enum {
    repeat_holdoff_time = 1000, // ms
    repeat_flow_time = 200,
    repeat_max_count = 50, // 50 cycles at 200ms intervals == 10 seconds
      // (stops us repeating forever if the button-up event is lost for any reason)
    heartbeat_interval = 10 * 1000, // 10 seconds
};

// ----------------------------------------------------------------------------

static void report_id()
{
    Serial.print( (char)boot_char );
    Serial.println( "rakoshim v1.1" );
}

void setup()
{
    Serial.begin(9600);

    pinMode( STATUS_LED_PIN, OUTPUT );

    pinMode( RAKO_RX_PIN, INPUT );
    pinMode( RAKO_RX_ENABLE_PIN, OUTPUT );

    Rako::Rx::setup( RAKO_RX_PIN, rako_command, (void *)0 );

    digitalWrite( RAKO_RX_ENABLE_PIN, HIGH ); // enable rx

    report_id();
}

// ----------------------------------------------------------------------------

char char_to_send = error_char;
bool do_send = false;
int do_repeat = 0;

// ------------------------------------

static char command_to_char( Rako::Command command )
{
    switch (command)
    {
        case 0: // off
            return '0';
        case 3: case 4: case 5: case 6: // scenes 1-4
            return command - 3 + '1';

        case 1: // raise / up
            return 'U';
        case 2: // lower / down
            return 'D';

        default:
            return error_char;
    }
}

static void rako_command( void *context, Rako::Command command )
{
    (void)context;

    char_to_send = command_to_char( command );
    if (command != 15/*stop*/)
        do_send = true;

    if (command == 1/*raise*/ || command == 2/*lower*/)
        do_repeat = repeat_max_count;
    else
        do_repeat = 0;
}

// ------------------------------------

static unsigned long last_send_time = 0;
static unsigned long next_repeat_time = 0;
static unsigned long led_off_time = 0;

static void tx_command( const char command, const int led_time, const int repeat_time )
{
    digitalWrite(STATUS_LED_PIN, HIGH);
    Serial.print( command );

    const unsigned long now = millis();
    last_send_time = now;
    led_off_time = now + led_time;
    next_repeat_time = now + repeat_time;
}

// ------------------------------------

void loop()
{
    Rako::Rx::loop( );

    const unsigned long now = millis();

    if (do_send)
    {
        tx_command( char_to_send, send_led_time, repeat_holdoff_time );
        do_send = false;
    }
    else if (do_repeat > 0 && (long)(next_repeat_time - now) < 0)
    {
        tx_command( char_to_send  - 'A' + 'a', repeat_led_time, repeat_flow_time );
        do_repeat--;
    }

    if ((long)(last_send_time + heartbeat_interval - now) < 0)
        tx_command( heartbeat_char, heartbeat_led_time, 0 /*repeat*/ );

    if ((long)(led_off_time - now) < 0)
        digitalWrite(STATUS_LED_PIN, LOW);

    if (Serial.available())
        if (Serial.read() == (char)boot_char)
            report_id();
}

// ----------------------------------------------------------------------------
