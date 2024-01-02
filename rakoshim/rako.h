
#pragma once

// This is intended to provide a common interface to receive
// messages from Rako wireless controllers.  It is based on work
// from: http://hacks.esar.org.uk/rako-wireless-protocol/.
// In my tests, the start of the messages kept getting lost, so
// it's hardwired to only work for me.

namespace Rako {

    enum {
        House = 23,
        Room = 1,
    };

    enum Command {
        Off = 0,
        Raise = 1,
        Lower = 2,
        Scene1 = 3,
        Scene2 = 4,
        Scene3 = 5,
        Scene4 = 6,
        Stop = 15,
    };

    namespace Rx {
        typedef void (*callback_fn_t)( void *, Command );
        void setup( int inputPin, callback_fn_t callback_fn , void *callback_context );
        void loop( );
    }

} // Rako
