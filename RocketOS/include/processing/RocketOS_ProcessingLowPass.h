#pragma once
#include "RocketOS_ProcessingGeneral.h"
#include "RocketOS_ProcessingFilters.h"

namespace RocketOS{
    namespace Processing{
        template<std::size_t t_order>
        class LowPass{
            FIRFilter<t_order> m_filter;
        public:
            LowPass() : m_filter(makeCoefficients()) {}

            void push(float_t value){
                m_filter.push(value);
            }

            float_t output() const{
                return m_filter.output() / pow2(t_order);
            }

            void reset(){
                m_filter.reset();
            }

            void printCoefTest(){//debug
                auto arr = makeCoefficients();
                for(uint_t i=0; i<arr.size(); i++)
                    Serial.println(arr[i]);
            }

            constexpr uint_t size() const{
                return t_order;
            }

            private:
            static constexpr float_t binomial(uint_t n, uint_t k){
                if (k > n) return 0;
                if (k == 0 || k == n) return 1;

                float_t res = 1;
                for (uint_t i = 1; i <= k; ++i) {
                    res = res * (n - i + 1) / i;
                }
                return res;
            }

            static constexpr std::array<float_t,t_order> makeCoefficients(){
                std::array<float_t, t_order> array{};
                for(uint_t i=0; i<t_order; i++){
                    array[i] = binomial(t_order, i);
                }
                return array;
            }

            static constexpr float_t pow2(uint_t power){
                float_t val = 1;
                for(uint_t i=0; i<power; i++)
                    val *= 2;
                return val;
            }
        };

        

    }
}