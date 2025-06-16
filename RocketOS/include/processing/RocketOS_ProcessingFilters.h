#pragma once
#include "RocketOS_ProcessingGeneral.h"
#include <array>

namespace RocketOS{
    namespace Processing{
        template<std::size_t t_size>
        class FliterMemory{
        private:
            std::array<float_t, t_size> m_data;
            uint_t m_position;
            bool m_filled;

        public:
            FliterMemory() : m_position(0), m_filled(false) {}

            void push(float_t value){
                m_data[m_position] = value;
                m_position++;
                if(m_position >= m_data.size()){
                    m_position = 0;
                    m_filled = true;
                }
            }

            void clear(){
                m_position = 0;
                m_filled = false;
            }

            float_t get(uint_t index) const{
                if(m_filled) return m_data[(m_position + 1 + index) % currentSize()];
                return m_data[index & currentSize()];
            }

            constexpr uint_t maxSize() const{
                return m_data.size();
            }

            uint_t currentSize() const{
                if(m_filled) return m_data.size();
                return m_position;
            }

            bool filled() const{
                return m_filled;
            }

            void initialize(float_t value){
                for(uint_t i=0; i<m_data.size(); i++)
                    m_data[i] = value;
                m_position = 0;
                m_filled = true;
            }

            void initialize(std::array<float_t, t_size> values){
                for(uint_t i=0; i<m_data.size(); i++)
                    m_data[i] = values[i];
                m_position = 0;
                m_filled = true;
            }

        };

        template<std::size_t t_order>
        class FIRFilter{
        private:
            FliterMemory<t_order> m_memory;
            const std::array<float_t, t_order> m_coefficients;
        public:
            FIRFilter(std::array<float_t, t_order>&& coefficients) : m_coefficients(coefficients){
                m_memory.initialize(0);
            }

            FIRFilter(const std::array<float_t, t_order>& coefficients) : m_coefficients(coefficients){
                m_memory.initialize(0);
            }

            void push(float_t value){
                m_memory.push(value);
            }

            float_t output() const{
                float_t value = 0;
                for(uint_t i=0; i<m_coefficients.size(); i++)
                    value += m_memory.get(i) * m_coefficients[i];
                return value;
            }

            bool filled() const{
                return m_memory.filled();
            }

            void reset(){
                m_memory.initialize(0);
            }
            
        };

        enum class AccumulationFilterTypes : uint_t{
            ORDERED, UNORDERED
        };

        template<std::size_t, AccumulationFilterTypes>
        class AccumulationFilter;

        template<std::size_t t_order>
        class AccumulationFilter<t_order, AccumulationFilterTypes::ORDERED>{
        private:
            FliterMemory<t_order> m_memory;
            float_t (* const m_operation)(float_t, uint_t);
        public:
            AccumulationFilter(float_t (* const operation )(float_t, uint_t)) : m_operation(operation) {
                m_memory.initialize(0);
            }

            AccumulationFilter(float_t (* const operation )(float_t, uint_t), float_t initial) : m_operation(operation) {
                m_memory.initialize(initial);
            }

            void push(float_t value){
                m_memory.push(value);
            }

            float_t output() const{
                float_t value = 0;
                for(uint_t i=0; i<m_memory.currentSize(); i++)
                    value += m_operation(m_memory.get(i), i);
                return value;
            }

            bool filled() const{
                return m_memory.filled();
            }

            void reset(float_t initial=0){
                m_memory.initialize(initial);
            }
        };

        template<std::size_t t_order>
        class AccumulationFilter<t_order, AccumulationFilterTypes::UNORDERED>{
        private:
            FliterMemory<2> m_memory;
            float_t (*const m_operation)(float_t);
        public:
            AccumulationFilter(float_t (* const operation)(float_t)) : m_operation(operation) {
                m_memory.clear();
            }
            
            AccumulationFilter(float_t (* const operation)(float_t), float_t initial) : m_operation(operation) {
                m_memory.clear();
            }

            void push(float_t value){
                m_memory.push(value);
            }

            float_t output() const{
                float_t value = 0;
                for(uint_t i=0; i<m_memory.currentSize(); i++)
                    value += m_operation(m_memory.get(i));
                return value;
            }

            bool filled() const{
                return m_memory.filled();
            }

            uint_t currentSize() const{
                return m_memory.currentSize();
            }

            void reset(){
                m_memory.clear();
            }
        };
    }
}