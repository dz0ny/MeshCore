#pragma once

#include <Arduino.h>
#include <NonBlockingRtttl.h>

/* class abstracts underlying RTTTL library 

    Just a simple imlementation to start.  At the moment use same
    melody for message and discovery
    Suggest enum type for different sounds
    - on message
    - on discovery

    TODO
    - make message ring tone configurable

*/

class genericBuzzer
{
    public:
        void begin();  // set up buzzer port
        void play(const char *melody); // Generic play function
        void loop();  // loop driven-nonblocking
        void startup();  // play startup sound
        void shutdown();  // play shutdown sound
        bool isPlaying();  // returns true if a sound is still playing else false
        void quiet(bool buzzer_state);  // enables or disables the buzzer
        bool isQuiet();  // get buzzer state on/off

        // Button feedback sounds
        void playBeep();  // Short beep
        void playChirp();  // Quick chirp for button clicks
        void playBoop();  // Button press boop
        void playComboTune();  // For double/triple clicks
        void playLongPressLeadUp();  // Ascending sequence for long press

    private:
        // Enhanced startup/shutdown melodies
        const char *startup_song = "Startup:d=4,o=5,b=160:16f3,16a#3,8c#4";  // FS3→AS3→CS4
        const char *shutdown_song = "Shutdown:d=4,o=5,b=100:8c#4,16a#3,16f3";  // CS4→AS3→FS3

        // Button feedback sounds (RTTTL format)
        const char *beep_song = "Beep:d=8,o=5,b=120:b3";  // NOTE_B3, 125ms
        const char *chirp_song = "Chirp:d=64,o=5,b=600:a#3";  // NOTE_AS3, 20ms - very short
        const char *boop_song = "Boop:d=32,o=5,b=300:a3";  // NOTE_A3, 50ms
        const char *combo_song = "Combo:d=16,o=5,b=450:8g3,16b3,8c#4,16g3,16c#4,8b3";  // Quick trills
        const char *leadup_song = "LeadUp:d=16,o=5,b=150:c3,e3,g3,8b3";  // C3→E3→G3→B3 ascending

        bool _is_quiet = true;
};
