#include "RocketOSGeneral.h"

using namespace RocketOS;

//implementation for error_t

error_t::operator bool() const{
    return m_code !=0;
}

error_t::operator uint_t() const{
    return m_code;
}

bool error_t::operator==(const error_t& other) const{
    return m_code == other.m_code;
}

bool error_t::operator!=(const error_t& other) const{
    return m_code != other.m_code;
}

error_t error_t::GOOD(0);
error_t error_t::ERROR(1);