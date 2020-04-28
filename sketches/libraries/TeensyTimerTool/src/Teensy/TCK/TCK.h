#pragma once

#include "TckChannel.h"
#include "core_pins.h"

namespace TeensyTimerTool
{
    extern const unsigned NR_OF_TCK_TIMERS;
    
    class TCK_t
    {
     public:
        static inline ITimerChannel* getTimer();
        static inline void removeTimer(TckChannel*);
        static inline void tick();

     protected:
        static bool isInitialized;
        static TckChannel* channels[NR_OF_TCK_TIMERS];
    };

    // IMPLEMENTATION ==================================================================

    ITimerChannel* TCK_t::getTimer()
    {
        if (!isInitialized)
        {
            // enable the cycle counter
            ARM_DEMCR |= ARM_DEMCR_TRCENA;
            ARM_DWT_CTRL |= ARM_DWT_CTRL_CYCCNTENA;

            for (unsigned chNr = 0; chNr < NR_OF_TCK_TIMERS; chNr++)
            {
                channels[chNr] = nullptr;
            }
            isInitialized = true;
        }

        for (unsigned chNr = 0; chNr < NR_OF_TCK_TIMERS; chNr++)
        {
            if (channels[chNr] == nullptr)
            {
                channels[chNr] = new TckChannel();
                return channels[chNr];
            }
        }

        return nullptr;
    }

    void TCK_t::removeTimer(TckChannel* channel)
    {
        for (unsigned chNr = 0; chNr < NR_OF_TCK_TIMERS; chNr++)
        {
            if (channels[chNr] == channel)
            {
                channels[chNr] = nullptr;
                delete channel;
                break;
            }
        }
    }

    void TCK_t::tick()
    {
        for (unsigned i = 0; i < NR_OF_TCK_TIMERS; i++)
        {
            if (channels[i] != nullptr)
            {
                channels[i]->tick();
            }
        }
    }
}
