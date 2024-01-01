#pragma once
#include <stdint.h>

/**
 * @brief The rtctime_t struct is a structure for storing the time.
 * 
 * This structure is used by the ESPMegaPRO library to store the time.
 * 
 * @warning This structure is not compatible with the Arduino Time library.
 */
struct rtctime_t {
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;
    uint8_t day;
    uint8_t month;
    uint16_t year;
};
