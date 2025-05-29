#pragma once
#include "RocketOS_TelemetryGeneral.h"
#include "RocketOS_TelemetrySD.h"
#include <cstring>

template<>
RocketOS::result_t<char*> RocketOS::Telemetry::printToBuffer<char*>(char* buffer, uint_t size, char* const& value){
    uint_t targetSize = snprintf(buffer, size, "%s", value);
    if(targetSize>size) return {buffer + size, error_t::ERROR};
    return {buffer + targetSize, error_t::GOOD};
}

template<>
RocketOS::result_t<char*> RocketOS::Telemetry::printToBuffer<const char*>(char* buffer, uint_t size, const char* const& value){
    uint_t targetSize = snprintf(buffer, size, "%s", value);
    if(targetSize>size) return {buffer + size, error_t::ERROR};
    return {buffer + targetSize, error_t::GOOD};
}

template<>
RocketOS::result_t<char*> RocketOS::Telemetry::printToBuffer<RocketOS::int_t>(char* buffer, uint_t size, const RocketOS::int_t& value){
    uint_t targetSize = snprintf(buffer, size, "%d", value);
    if(targetSize>size) return {buffer + size, error_t::ERROR};
    return {buffer + targetSize, error_t::GOOD};
}

template<>
RocketOS::result_t<char*> RocketOS::Telemetry::printToBuffer<RocketOS::uint_t>(char* buffer, uint_t size, const RocketOS::uint_t& value){
    uint_t targetSize = snprintf(buffer, size, "%d", value);
    if(targetSize>size) return {buffer + size, error_t::ERROR};
    return {buffer + targetSize, error_t::GOOD};
}

template<>
RocketOS::result_t<char*> RocketOS::Telemetry::printToBuffer<RocketOS::float_t>(char* buffer, uint_t size, const RocketOS::float_t& value){
    uint_t targetSize = snprintf(buffer, size, "%f", value);
    if(targetSize>size) return {buffer + size, error_t::ERROR};
    return {buffer + targetSize, error_t::GOOD};
}