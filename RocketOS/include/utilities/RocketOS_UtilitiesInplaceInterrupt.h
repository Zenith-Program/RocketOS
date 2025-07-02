#pragma once
#include "RocketOS_UtilitiesGeneral.h"
#include <Arduino.h>


namespace RocketOS{
    namespace Utilities{
        using interruptCallback_t = inplaceFunction_t<void(void), RocketOS_Utilities_InterruptCallbackCaptureSize>;

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