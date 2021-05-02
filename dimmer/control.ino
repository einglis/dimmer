
static void handle_buf( const uint8_t *buf, const size_t buf_len )
{
    for (size_t i = 0; i + 1 < buf_len; i += 2)
    {
        uint8_t val = buf[i+0] * 16 + buf[i+1];
        output_target[i/2] = val;
    }
}

static uint8_t buf[num_outputs * 2];
static size_t buf_used = 0;
static uint8_t rate_flag = 0;
    // rate_flag == 0 doubles as "invalid"

void poll_serial()
{
    while (Serial.available())
    {
        const int ch = Serial.read();
        switch (ch)
        {
            case '(': case '{': case '[':
                rate_flag = ch;
                buf_used = 0;
                break;

            case '0': case '1': case '2': case '3': case '4':
            case '5': case '6': case '7': case '8': case '9':
                if (rate_flag)
                    buf[buf_used++] = ch - '0';
                break;
            case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
                if (rate_flag)
                    buf[buf_used++] = ch - 'a' + 10; // do the conversion to decimal here and...
                break;
            case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
                if (rate_flag)
                    buf[buf_used++] = ch - 'A' + 10; // ...here, rather than checking again later
                break;

            case (char)boot_char:
                report_id();
                // fall through
            default:
                rate_flag = 0;
                break;
        }

        if (rate_flag && buf_used == sizeof(buf)) // rate_flag check technically redundant
        {
            handle_buf(buf, buf_used);
            rate_flag = 0;
        }
    }
}
