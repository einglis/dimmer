
// BOARD: "Arduino UNO"
// PROGRAMMER: "ArduinoISP"

// const int STATUS_LED_PIN = 13;
    // NOOO!  This is output channel six.

const int ZERO_CROSSING_INT_PIN = 2;

enum {
    boot_char = '!',
//    error_char = '?',
    heartbeat_char = '.'
};

enum {
    heatbeat_interval = 10 * 1000, // 10 seconds
};

volatile uint8_t *port_b = (volatile uint8_t *)&PORTB; // outputs
volatile uint8_t *port_d = (volatile uint8_t *)&PORTD; // debug

// ----------------------------------------------------------------------------

const int num_outputs = 6;
volatile int output_level[ num_outputs ] = { 0 };
volatile int output_target[ num_outputs ] = { 255, 255, 255, 255, 255, 255,  };
int output_rate[ num_outputs ] = { 0 };

const int phase_max = 255;
int phase = phase_max;

// ----------------------------------------------------------------------------

void Zero_Crossing_Int()
{
  if (phase > phase_max/2)
  {
    TCNT2 = 0; // reset timer...
    OCR2A = 140; // empirically correct to align with centre of pulse (0.3ms)
    TCCR2B |= (1 << CS21) | (1 << CS20); // ...and enable with /32 prescaler
    phase = 0;
  }
}

#define PROFILE2
ISR(TIMER2_COMPA_vect)
{
    *port_d |= _BV(7);

    if (phase == 0)
    {
        #ifndef PROFILE2
        OCR2A = 10;
        #else
        OCR2A = 20;
        #endif

        uint8_t port_b_highs = 0;
        if (output_level[0] > 0) port_b_highs |= _BV(0);
        if (output_level[1] > 0) port_b_highs |= _BV(1);
        if (output_level[2] > 0) port_b_highs |= _BV(2);
        if (output_level[3] > 0) port_b_highs |= _BV(3);
        if (output_level[4] > 0) port_b_highs |= _BV(4);
        if (output_level[5] > 0) port_b_highs |= _BV(5);
        *port_b |= port_b_highs;
    }
    else if (phase < phase_max)
        // ie. won't turn off if output_level == phase_max
    {
        #ifndef PROFILE2
        if      (phase == 160) OCR2A = 15; // non-linear numbers calculated
        else if (phase == 192) OCR2A = 26; // to fill 90% of the cycle with a
        else if (phase == 224) OCR2A = 42; // bias towards the low end.
        #else
        if      (phase ==  32) OCR2A =  6; // non-linear numbers calculated
        else if (phase == 160) OCR2A = 12; // to fill 90% of the cycle with a
        else if (phase == 192) OCR2A = 25; // bias towards the low end.
        else if (phase == 224) OCR2A = 50;
        #endif

        uint8_t port_b_lows = 0;
        if (output_level[0] <= phase) port_b_lows |= _BV(0);
        if (output_level[1] <= phase) port_b_lows |= _BV(1);
        if (output_level[2] <= phase) port_b_lows |= _BV(2);
        if (output_level[3] <= phase) port_b_lows |= _BV(3);
        if (output_level[4] <= phase) port_b_lows |= _BV(4);
        if (output_level[5] <= phase) port_b_lows |= _BV(5);
        *port_b &= ~port_b_lows;
    }
    else
    {
        TCCR2B = 0; // disable timer
    }

    ++phase;
    *port_d &= ~_BV(7);
}

// ----------------------------------------------------------------------------

static void report_id()
{
    Serial.print( (char)boot_char );
    Serial.println( "dimmer v2.0" );
}

void setup()
{
  Serial.begin(9600);
  const byte reset_reason = MCUSR;

  const uint8_t port_b_outputs = _BV(0) | _BV(1) | _BV(2) | _BV(3) | _BV(4) | _BV(5);
  *port_b &= ~port_b_outputs; // clear output before setting direction.
  *(uint8_t *)&DDRB = port_b_outputs;

  const uint8_t port_d_outputs = _BV(7); // debug
  *port_d &= ~port_d_outputs;
  *(uint8_t *)&DDRD = port_d_outputs;


  TCCR2A = 0; // no compare output, normal mode
  TCCR2B = 0; // normal mode (ctd), no clock source

  TCCR2A |= (1 << WGM21); // CTC mode (Clear Timer on Compare Match)

  TIMSK2 |= (1 << OCIE2A); // compare A match interrupt

  pinMode( ZERO_CROSSING_INT_PIN, INPUT );
  attachInterrupt( digitalPinToInterrupt(ZERO_CROSSING_INT_PIN), Zero_Crossing_Int, RISING );

  report_id();
}

// ----------------------------------------------------------------------------

static void report_levels( void )
{
    static char buf[256];

    char* bp = &buf[0];
    for (int i = 0; i < num_outputs; i++)
        bp += sprintf( bp, "%c%d ", i + 'A', output_level[i] );

    Serial.println( buf );
}

static bool off_target = false;





void loop()
{
    static long int last = millis();
    unsigned long now = millis();

    if (now > last + 10)
    {
        off_target = false;
        for (int i = 0; i < num_outputs; ++i)
        {
            if (output_level[i] < output_target[i])
            {
                output_level[i] = min( output_level[i] + (1 << output_rate[i] ), phase_max );
                off_target = true;
            }
            else if (output_target[i] < output_level[i] )
            {
                output_level[i] = max( output_level[i] - (1 << output_rate[i] ), 0 );
                off_target = true;
            }
        }
        last = now;

        if (off_target)
            report_levels();
    }


    poll_serial();




    static unsigned long last_send_time = 0;
    if ((long)(last_send_time + heatbeat_interval - now) < 0)
    {
        report_levels();
        Serial.print( (char)heartbeat_char );
        last_send_time = now;
    }



}

// ----------------------------------------------------------------------------
