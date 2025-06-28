#pragma once
#include "RocketOS_UtilitiesGeneral.h"
#include <Arduino.h>

#ifdef RocketOS_CFG_UsingTeensyTimerTool
#include <TeensyTimerTool.h>
namespace RocketOS{
    namespace Utilities{
        using interruptCallback_t = TeensyTimerTool::stdext::inplace_function<void(void), RocketOS_Utilities_InterruptCallbackCaptureSize>;
    }
}
#else
#include <inplace_function.h>
namespace RocketOS{
    namespace Utilities{
        using interruptCallback_t = teensy::inplace_function<void(void), RocketOS_Utilities_InterruptCallbackCaptureSize>;
    }
}
#endif

namespace RocketOS{
    namespace Utilities{
        template<uint8_t t_pin>
        class inplaceInterrupt{
            static interruptCallback_t s_callback;
        public:
            static void configureInterrupt(interruptCallback_t callback, int mode){
                s_callback = callback;
                attachInterrupt(digitalPinToInterrupt(t_pin), ISR, mode);
            }
            static void removeInterrupt(){
                s_callback = nullptr;
                detachInterrupt(t_pin);
            }
            static void ISR(void){
                s_callback();
            }
        };
        
        template<uint8_t t_pin>
        interruptCallback_t inplaceInterrupt<t_pin>::s_callback = nullptr;
    }
}