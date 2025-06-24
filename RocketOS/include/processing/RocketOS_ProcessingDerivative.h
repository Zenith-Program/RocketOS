#pragma once
#include "RocketOS_ProcessingGeneral.h"
#include "RocketOS_ProcessingFilters.h"

namespace RocketOS{
    namespace Processing{

        template<std::size_t t_order>
        class Differentiator{
        private:
            static constexpr std::size_t c_filterSize = 2*(t_order/2 + 1);
            FIRFilter<c_filterSize> m_filter;
        public:
            Differentiator() : m_filter(makeCoefficients()) {}

            void push(float_t value){
                m_filter.push(value);
            }

            float_t output() const{
                return m_filter.output() / pow2(t_order);
            }

            bool filled() const{
                return m_filter.filled();
            }

            void reset(){
                m_filter.reset();
            }

            void printCoefTest(){//debug
                auto arr = makeCoefficients();
                for(uint_t i=0; i<arr.size(); i++)
                    Serial.println(arr[i]);
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

            static constexpr float_t binomialDifference(uint_t n, uint_t k){
                if(k == 0) return 1;
                if(k > n/2) return 0;
                return binomial(n,k) - binomial(n,k-1);
            }

            static constexpr std::array<float_t, c_filterSize> makeCoefficients(){
                std::array<float_t, c_filterSize> array{};
                for(uint_t i=0; i<c_filterSize/2; i++){
                    array[i] = binomialDifference(t_order, i);
                    array[array.size()-1-i] = -binomialDifference(t_order, i);
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