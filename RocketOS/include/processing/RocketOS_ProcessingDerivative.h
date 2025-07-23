#pragma once
#include "RocketOS_ProcessingGeneral.h"
#include "RocketOS_ProcessingFilters.h"

/* Robust Differentiator Filter
 * This filter allows you to take derivatives in real time that are less suceptible to noise or sample rate missmatches. 
 * You absolutley NEED this if you are trying to do controls of any kind or bad things will happen.
 * Look in the teams in 2024-2025/Airbrakes/Subsystems/Controls for info on the theory.
 *
 * This is a linear FIR filter (google this if u don't know its not as complex as the name suggests) whose coefficients are constructed with binomial coeffiecients (entries of pascals triangle). 
 * The rank of the filter describes which row of pascals triangle is used to generate the coefficients.
 * The order of the filter (template parameter for the filter class) is the number of coefficients in the filter and how many memory values there are. 
 * For every differentiator filter of this kind: Rank + 2 = Order.
 * 
 * The coefficient of the kth filter coefficient for rank n is the difference between adjacent binomial coeffieicents in row n. 
 * A second triange can be constructed to show this visually:
 * 
 *       Pascal's Triangle                  Forward Difference of Pascal's Triangle
 * 
 *               1                   |0|                   1  -1
 *             1   1                 |1|                 1   0  -1
 *           1   2   1               |2|               1   1  -1  -1
 *         1   3   3   1             |3|             1   2   0  -2  -1  
 *       1   4   6   4   1           |4|           1   3   2  -2  -3  -1
 *     1   5   10  10  5   1         |5|         1   4   5   0  -5  -4  -1
 *   1   6   15  20  15  6   1       |6|       1   5   9   5  -5  -9  -5  -1
 * 1   7   21  35  35  21  7   1     |7|     1   6   14  14  0  -14 -14 -6  -1 
 * 
 * The coefficients on the right are used for differentiators. The ones on the left can be used for a low pass filter (see RocketOS_ProcessingLowPass.h).
 * A scale factor of 1/2^order prefixes the summation. This is handled internaly by the Differentiator class. 
 * The output also needs to manually be divided by the sample period the user is using the differentator at.
 * 
 * Example for order 6 (RocketOS::Processing::Differentiator<6>):
 *      Rank = 4 (use row 4). Let Y(t) be the output of the filter, X(t) be the input, and dt be the sample period:
 *      Y(x) = 1/(2^4) * (1 * X(t) + 3 * X(t - dt) + 2 * X(t - 2*dt) - 2 * X(t - 3*dt) - 3 * X(t - 4*dt) - 1 * X(t - 5*dt))
 *      dX/dt ~= Y(x)/dt
 * 
 * 
 *                                                                  === USAGE ===
 * To use this differentiator, you need to create an instance of it somewhere in your program (probably global in some capacity since it has memory).
 * At your desired sample period, use the push function to push the next value into the filter object. You can then call the output function to get the output value of the filter.
 * Make sure to divide the output by your sample period to get the correct scale for your derivative value.
 * Be careful when initially starting the filter. All memories initialize to zero so for the first few samples (the value of filter order), the filter output may not be reliable.
 * There is a tradeoff when scaling the filter order. Larger orders have better performance, but introduce more delay to the output than lower orders do. 
 * The delay is porportional to the sample period, so fast sample periods can allow for larger filters that maintain a reasonable delay.
 * 
 * ### main.cpp ###
 * #include "RocketOS.h"
 * 
 * RocketOS::Processing::Differentiator<6> g_filter;
 * float_t g_samplePeriod_s = 0.01;
 * 
 * void startup(){}
 * 
 * void loop(){
 *      float_t newValue = someSensor.read(); //read from your sensor
 *      g_filter.push(newValue); //give next value to filter
 *      float_t derivative = g_filter.output() / g_samplePeriod_s; //computed derivative value
 *      delay(g_samplePeriod_s * 1000); // delay to next sample
 * }
*/

namespace RocketOS{
    namespace Processing{

        template<std::size_t t_order>
        class Differentiator{
        private:
            static_assert(t_order > 2, "Order of a differentiator must be greater than 2");
            static constexpr std::size_t c_rank = t_order - 2;
            FIRFilter<t_order> m_filter;
        public:
            Differentiator() : m_filter(makeCoefficients()) {}

            void push(float_t value){
                m_filter.push(value);
            }

            float_t output() const{
                return m_filter.output() / pow2(c_rank);
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

            constexpr uint_t rank() const{
                return c_rank;
            }

        private:
            static constexpr float_t binomial(uint_t n, uint_t k){
                if (k > n) return 0;
                if (k == 0 || k == n) return 1;
                //let compiler use larger floating point numbers
                if constexpr{
                    double res = 1;
                    for (uint_t i = 1; i <= k; ++i) {
                        res = res * (n - i + 1) / i;
                    } 
                    return res;
                }
                //use word length floating point numbers for run time
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

            static constexpr std::array<float_t, t_order> makeCoefficients(){
                std::array<float_t, t_order> array{};
                array[array.size()/2] = 0; //handle odd rank case
                for(uint_t i=0; i<c_rank/2; i++){
                    array[i] = binomialDifference(c_rank, i);
                    array[array.size()-1-i] = -binomialDifference(c_rank, i);
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