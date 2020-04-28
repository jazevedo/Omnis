#pragma once

#include "../../ITimerChannel.h"
#include "PITMap.h"
#include "core_pins.h"

namespace TeensyTimerTool
{
    class PIT_t;

    class PITChannel : public ITimerChannel
    {
     public:
        inline PITChannel(unsigned nr);
        inline virtual ~PITChannel();

        inline errorCode begin(callback_t cb, float tcnt, bool periodic) override;
        inline errorCode begin(callback_t cb, uint32_t tcnt, bool periodic) override;

        inline errorCode trigger(uint32_t) override;
        inline errorCode trigger(float) override;
        inline void setPeriod(uint32_t) {}
        inline float getMaxPeriod() override;

        bool isPeriodic;

     protected:
        //IMXRT_GPT_t* regs;
        //uint32_t reload;

        inline void isr();

        PITChannel() = delete;
        PITChannel(const PITChannel&) = delete;

        const unsigned chNr;
        callback_t callback = nullptr;

        static uint32_t clockFactor;

        friend PIT_t;
    };

    // IMPLEMENTATION ==============================================

    PITChannel::PITChannel(unsigned nr)
        : ITimerChannel(nullptr), chNr(nr)
    {
        callback = nullptr;
        clockFactor = (CCM_CSCMR1 & CCM_CSCMR1_PERCLK_CLK_SEL) ? 24 : (F_BUS_ACTUAL / 1000000);
    }

    errorCode PITChannel::begin(callback_t cb, uint32_t micros, bool periodic)
    {
        return begin(cb, (float)micros, periodic);
    }

    errorCode PITChannel::begin(callback_t cb, float micros, bool periodic)
    {
        isPeriodic = periodic;
        callback = cb;

        if (isPeriodic)
        {
            IMXRT_PIT_CHANNELS[chNr].TCTRL = 0;
            IMXRT_PIT_CHANNELS[chNr].TFLG = 1;

            float tmp = micros * clockFactor;
            if (tmp > 0xFFFF'FFFF)
            {
                postError(errorCode::periodOverflow);
                IMXRT_PIT_CHANNELS[chNr].LDVAL = 0xFFFF'FFFE;
            } else
                IMXRT_PIT_CHANNELS[chNr].LDVAL = (uint32_t)tmp - 1;

            IMXRT_PIT_CHANNELS[chNr].TCTRL = PIT_TCTRL_TEN | PIT_TCTRL_TIE;
        }
        return errorCode::OK;
    }

    void PITChannel::isr()
    {
        if (callback != nullptr)
        {
            callback();
            if (!isPeriodic) IMXRT_PIT_CHANNELS[chNr].TCTRL = 0; // switch off timer
        }
    }

    PITChannel::~PITChannel()
    {
        callback = nullptr;
    }

    errorCode PITChannel::trigger(uint32_t delay)
    {
        return trigger((float)delay);
    }

    errorCode PITChannel::trigger(float delay) //should be optimized somehow
    {
        IMXRT_PIT_CHANNELS[chNr].TCTRL = 0;
        IMXRT_PIT_CHANNELS[chNr].TFLG = 1;

        float tmp = delay * clockFactor;
        if (tmp > 0xFFFF'FFFF)
        {
            postError(errorCode::periodOverflow);
            IMXRT_PIT_CHANNELS[chNr].LDVAL = 0xFFFF'FFFE;
        } else
            IMXRT_PIT_CHANNELS[chNr].LDVAL = (uint32_t)tmp - 1;

        IMXRT_PIT_CHANNELS[chNr].TCTRL = PIT_TCTRL_TEN | PIT_TCTRL_TIE;

        return errorCode::OK;
    }

    float PITChannel::getMaxPeriod()
    {
        return (float)0xFFFF'FFFE / clockFactor;
    }


   

} // namespace TeensyTimerTool