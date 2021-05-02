
const size_t num_channels = num_outputs; // airgap for now
uint8_t current[num_channels] = { 255, 255, 255, 255, 255, 255,  };

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

void poll_serial()
{
    while (Serial.available())
    {
        const int ch = Serial.read();
        if (ch == (char)boot_char)
        {
            report_id();
        }
         else if (ch == '{')
         {
             buf_used = 0;
         }
         else if (is_hex( ch ))
         {
             if (buf_used < sizeof(buf))
                 buf[buf_used++] = (uint8_t)(ch & 0xff);
             else
                 buf_used = buf_invalid;
         }
         else if (ch == '}')
         {
             //if (buf_used == sizeof(buf))
                 handle_buf(buf, buf_used);
             buf_used = buf_invalid;
         }
         else
         {
             buf_used = buf_invalid;
         }
    }
}
