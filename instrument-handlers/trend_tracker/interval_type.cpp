#include <interval_type.hpp>

std::string interval_t2str(interval_t interval)
{
    switch (interval)
    {
        case interval_t::I_M1:
            return "m1";
        case interval_t::I_M2:
            return "m2";
        case interval_t::I_M5:
            return "m5";
        case interval_t::I_M10:
            return "m10";
        case interval_t::I_M15:
            return "m15";
        case interval_t::I_M20:
            return "m20";
        case interval_t::I_M30:
            return "m30";
        case interval_t::I_H1:
            return "h1";
        case interval_t::I_H2:
            return "h2";
        case interval_t::I_H3:
            return "h3";
        case interval_t::I_H4:
            return "h4";
        case interval_t::I_H6:
            return "h6";
        case interval_t::I_H8:
            return "h8";
        case interval_t::I_H12:
            return "h12";
    }

    return "???";
}
