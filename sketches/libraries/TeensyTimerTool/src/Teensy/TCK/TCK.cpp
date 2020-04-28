#include "../../config.h"

#if defined(TEENSYDUINO)

    #include "TCK.h"
    namespace TeensyTimerTool
    {
        bool TCK_t::isInitialized = false;
        TckChannel* TCK_t::channels[NR_OF_TCK_TIMERS];
    }

    //----------------------------------------------------------------------
    #if YIELD_TYPE == YIELD_OPTIMIZED

        void yield()
        {
            TeensyTimerTool::TCK_t::tick();
        }

    //----------------------------------------------------------------------
    #elif YIELD_TYPE == YIELD_STANDARD

        #include "EventResponder.h"

        //----------------------------------------------------------------------
        #if defined(KINETISK) || defined(KINETISL)

            void yield(void)
            {
                static uint8_t running=0;

                if (running) return; // TODO: does this need to be atomic?
                running = 1;

                if (Serial.available()) serialEvent();
                if (Serial1.available()) serialEvent1();
                if (Serial2.available()) serialEvent2();
                if (Serial3.available()) serialEvent3();

                #ifdef HAS_KINETISK_UART3
                    if (Serial4.available()) serialEvent4();
                #endif
                #ifdef HAS_KINETISK_UART4
                    if (Serial5.available()) serialEvent5();
                #endif
                #if defined(HAS_KINETISK_UART5) || defined (HAS_KINETISK_LPUART0)
                    if (Serial6.available()) serialEvent6();
                #endif

                running = 0;
                EventResponder::runFromYield();

                TeensyTimerTool::TCK_t::tick();
            }

        //----------------------------------------------------------------------
        #elif defined(ARDUINO_TEENSY40) || defined(ARDUINO_TEENSY41)

            extern uint8_t usb_enable_serial_event_processing;

            void yield(void)
            {
                static uint8_t running=0;

                if (running) return; // TODO: does this need to be atomic?
                running = 1;

                // USB Serail - Add hack to minimize impact...
                if (usb_enable_serial_event_processing && Serial.available()) serialEvent();

                // Current workaround until integrate with EventResponder.
                if (HardwareSerial::serial_event_handlers_active) HardwareSerial::processSerialEvents();

                running = 0;
                EventResponder::runFromYield();

                TeensyTimerTool::TCK_t::tick();
            };

        #endif
    #endif
#endif