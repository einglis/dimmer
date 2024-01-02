
#include "rako.h"
#include "Arduino.h"

namespace Rako {

namespace {

int inputPin = 0;
Rx::callback_fn_t callback_fn = 0;
void *callback_context = 0;

// ------------------------------------

enum {
    t_short_min = 560 - 100, // centre value is measured,
    t_short_max = 560 + 100, // error bounds are estimated but
    t_med_min = 1090 - 100, // very conservative
    t_med_max = 1090 + 100,
    t_long_min = 1200,
    t_long_max = 2400,
};

// NOTE: I did some experiments where I tried to trim the expected
// durations to what we were really seeing, to correct for drift.  The
// result was a system that was much less reliable, since my approach
// was too simple to not be affected by all the noise.  Instead, we'll stick
// with accurate times with large error bounds, then let the higher
// levels pick up the slack.

// Many years later, after lots of trouble with lost controls, I looked again.
// I see the TX sending the final pulse about 2180ms long, but this being
// received around just 1260ms long.  Not quite sure why, but it's clearly
// shorter than the 1540 +/- 150 I was expecting.  So I've tightened the
// short and med bounds (since they are really very consistent in reality),
// and greatly extended the long bounds.

// ------------------------------------

uint32_t popcount( uint32_t x )
{
    x = (x & 0x55555555) + ((x >>  1) & 0x55555555);
    x = (x & 0x33333333) + ((x >>  2) & 0x33333333);
    x = (x & 0x0f0f0f0f) + ((x >>  4) & 0x0f0f0f0f);
    x = (x & 0x00ff00ff) + ((x >>  8) & 0x00ff00ff);
    x = (x & 0x0000ffff) + ((x >> 16) & 0x0000ffff);
    return x;
}

// ------------------------------------

void dispatch( uint32_t raw )
{
    uint32_t pop = popcount( raw );
    if (pop == 0)
        return; // not strictly impossible, but is for our purposes
    if (pop & 1)
        return; // parity error

    uint8_t check = raw & 1;
    raw >>= 1;
    uint8_t command = raw & 0xf;
    raw >>= 4;
    uint8_t channel = raw & 0xf;
    raw >>= 4;
    uint8_t room = raw & 0xff;
    raw >>= 8;
    uint8_t house = raw & 0xff;
    raw >>= 8;
    uint8_t type = raw & 0xf;
    raw >>= 4;

    if (0) // debug
    {
        Serial.println( raw, 2 ); // binary
        Serial.print("   type: "); Serial.println(type);
        Serial.print("  house: "); Serial.println(house);
        Serial.print("   room: "); Serial.println(room);
        Serial.print("channel: "); Serial.println(channel);
        Serial.print("command: "); Serial.println(command);
    }

    // NOTE: used to also check the channel, but I don't know what this is for,
    if (house == House && room == Room)
        if (callback_fn)
            callback_fn( callback_context, (Command)command );
}

void reset( uint32_t &raw )
{
    raw = 0;
}

void newbit( int bit )
{
    static uint32_t raw = 0;

    unsigned long currentTime = micros();
    static unsigned long lastTime = 0;

    int duration = currentTime - lastTime;
    lastTime = currentTime;

    if(bit == 1) // low to high
    {
        if (duration < t_short_min || duration > t_short_max) {
            reset(raw);
        }
    }
    else // high to low
    {
        if (t_short_min <= duration && duration <= t_short_max) {
            raw = (raw << 1) | 0;
        } else if (t_med_min <= duration && duration <= t_med_max) {
            raw = (raw << 1) | 1;
        } else if (t_long_min <= duration && duration <= t_long_max) {
            dispatch(raw);
            reset(raw);
        } else {
            reset(raw);
        }
    }
}

} // anon

namespace Rx {

void setup( int inputPin_, callback_fn_t callback_fn_, void *callback_context_ )
{
    inputPin = inputPin_;
    callback_fn = callback_fn_;
    callback_context = callback_context_;
}

void loop( )
{
    static int last = 0;

    int current = digitalRead(inputPin);
    if (current != last)
    {
        newbit(current);
        last = current;
    }
}

} // namespace Rx

} // namespace Rako
