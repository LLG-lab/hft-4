#ifndef __INTERVAL_TYPE_HPP__
#define __INTERVAL_TYPE_HPP__

#include <string>

enum class interval_t
{
    I_M1,
    I_M2,
    I_M5,
    I_M10,
    I_M15,
    I_M20,
    I_M30,
    I_H1,
    I_H2,
    I_H3,
    I_H4,
    I_H6,
    I_H8,
    I_H12
};

std::string interval_t2str(interval_t interval);

#endif /* __INTERVAL_TYPE_HPP__ */
