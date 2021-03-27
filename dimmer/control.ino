
const size_t num_channels = num_outputs; // airgap for now
uint8_t current[num_channels] = { 0 };

// ----------------------------------------------------------------------------

static bool is_hex( int ch )
{
    switch (ch)
    {
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            return true;
        case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
            return true;
        case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
            return true;
        default:
            return false;
    }
}

static uint8_t atoh( int ch )
{
    switch (ch)
    {
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            return ch - '0';
        case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
            return ch - 'a' + 10;
        case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
            return ch - 'A' + 10;
        default:
            return 0;
    }
}

static void handle_buf( const uint8_t *buf, const size_t buf_len )
{
    for (size_t i = 0; i + 1 < buf_len; i += 2)
    {
        uint8_t val = atoh(buf[i+0]) * 16 + atoh(buf[i+1]);
        output_target[i/2] = val;
    }
}

static uint8_t buf[num_outputs * 2];
static const size_t buf_invalid = ~0;
static size_t buf_used = buf_invalid;

enum parse_state {
    ps_idle,
    ps_collecting_level,
    ps_collecting_rate,
};
static parse_state ps = ps_idle;
static int ps_channel = 0;
static int ps_level = 0;
static int ps_rate = 0;

void handle_update( int channel, int level, int rate = -1 )
{
    if (channel < 0 || channel >= num_channels)
        return;

    if (level > phase_max)
        level = phase_max;
    if (rate > 8)
        rate = 8;

    Serial.print("update channel ");
    Serial.print((char)(channel + 'A'));
    Serial.print(": level ");
    Serial.print(level);
    Serial.print(", rate ");
    if (rate >= 0)
        Serial.print(rate);
    else
        Serial.print("unchanged");
    Serial.println(".");

    output_target[channel] = level;
    if (rate >= 0)
        output_rate[channel] = rate;
}

void poll_serial()
{
    while (Serial.available())
    {
        const int ch = Serial.read();
        if (ch == (char)boot_char)
        {
            report_id();
            ps = ps_idle;
        }
        else if (ch >= 'A' && ch < 'A' + num_channels)
        {
            if (ps == ps_idle)
            {
                ps_channel = ch - 'A';
                ps_level = 0;
                ps_rate = 0;

                //Serial.println("got channel");
                ps = ps_collecting_level;
            }
            else
            {
                Serial.println("error 1");
                ps = ps_idle;
            }
        }
        else if (ch >= '0' && ch <= '9')
        {
            if (ps == ps_collecting_level)
            {
                ps_level = ps_level * 10 + (ch - '0');
            }
            else if (ps == ps_collecting_rate)
            {
                ps_rate = ps_rate * 10 + (ch - '0');
            }
            else
            {
                //Serial.println("error 2");
                ps = ps_idle;
            }
        }
        else if (ch == ':')
        {
            if (ps == ps_collecting_level)
            {
                //Serial.print("got level ");
                //Serial.println(ps_level);
                ps = ps_collecting_rate;
            }
            else
            {
                Serial.println("error 3");
                ps = ps_idle;
            }
        }
        else if (ch == ' ' || ch == ';' || ch == '\n' || ch == '\r')
        {
            if (ps == ps_collecting_level)
            {
                //Serial.print("got level ");
                //Serial.println(ps_level);
                handle_update( ps_channel, ps_level );
            }
            else if (ps == ps_collecting_rate)
            {
                //Serial.print("got rate ");
                //Serial.println(ps_rate);
                handle_update( ps_channel, ps_level, ps_rate );
            }
            // it might be an error, it might not; it makes no odds.
            ps = ps_idle;
        }
        else
        {
            // siliently ignore
            ps = ps_idle;
        }


    //     if (ch == '{')
    //     {
    //         buf_used = 0;
    //     }
    //     else if (is_hex( ch ))
    //     {
    //         if (buf_used < sizeof(buf))
    //             buf[buf_used++] = (uint8_t)(ch & 0xff);
    //         else
    //             buf_used = buf_invalid;
    //     }
    //     else if (ch == '}')
    //     {
    //         //if (buf_used == sizeof(buf))
    //             handle_buf(buf, buf_used);
    //         buf_used = buf_invalid;
    //     }
    //     {
    //         buf_used = buf_invalid;
    //     }
    }
}
