#pragma once
#include "RocketOS_UtilitiesGeneral.h"
#include <array>

namespace RocketOS{
    namespace Utilities{
        template<class T, std::size_t t_size>
        class Queue{
        private:
            std::array<T, t_size> m_data;
            uint_t m_currentFront;
            uint_t m_currentBack;
        public:
            Queue() : m_currentFront(0), m_currentBack(0) {}

            error_t push(T newElement){
                uint_t newBack = (m_currentBack == m_data.size()-1)? 0 : m_currentBack + 1;
                if(newBack == m_currentFront) return error_t::ERROR;
                m_data[m_currentBack] = newElement;
                m_currentBack = newBack;
                return error_t::GOOD;
            }

            result_t<T> pop(){
                if(m_currentFront == m_currentBack) return error_t::ERROR;
                T returnData = m_data[m_currentFront];
                m_currentFront = (m_currentFront == m_data.size()-1)? 0 : m_currentFront + 1;
                return returnData;
            }

            void clear(){
                m_currentBack = 0;
                m_currentFront = 0;
            }

            uint_t size() const{
                return (m_currentBack - m_currentFront + m_data.size()) % m_data.size();
            }

            uint_t capacity() const{
                return m_data.size();
            }

            bool empty() const{
                return size() == 0;
            }
        };
    }
}