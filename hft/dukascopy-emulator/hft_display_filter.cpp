/**********************************************************************\
**                                                                    **
**             -=≡≣ High Frequency Trading System ® ≣≡=-              **
**                                                                    **
**          Copyright © 2017 - 2022 by LLG Ryszard Gradowski          **
**                       All Rights Reserved.                         **
**                                                                    **
**  CAUTION! This application is an intellectual propery              **
**           of LLG Ryszard Gradowski. This application as            **
**           well as any part of source code cannot be used,          **
**           modified and distributed by third party person           **
**           without prior written permission issued by               **
**           intellectual property owner.                             **
**                                                                    **
\**********************************************************************/

#include <iostream>
#include <unistd.h>

#include <hft_display_filter.hpp>

hft_display_filter::hft_display_filter(void)
    : is_tty_output_(::isatty(::fileno(stdout)))
{
}

void hft_display_filter::display(const hft_dukascopy_emulator::emulation_result &data)
{
    int length = get_max_instrument_strlen(data);

    for (auto &x : data.trades)
    {
        print_string(x.instrument, length + 1);

        if (x.direction == hft::protocol::response::position_direction::POSITION_LONG)
        {
            std::cout << "LONG  ";
        }
        else
        {
            std::cout << "SHORT ";
        }

        print_number(x.pips_yield, 6);  std::cout << "  ";
        print_number(x.qty, 6);         std::cout << "  ";

        std::cout << boost::posix_time::to_simple_string(x.open_time) << " - " << boost::posix_time::to_simple_string(x.close_time) << "  ";

        print_number(x.equity, 10);     std::cout << "  ";
        print_number(x.total_swaps, 6); std::cout << "  ";

        std::cout << x.still_opened;

        if (x.closed_forcibly)
        {
            std::cout << " (forcibly closed)";
        }

        std::cout << "\n";
    }

    std::cout << "Additional info:\n";
    std::cout << "min equity: " << data.min_equity << "\n";
    std::cout << "max equity: " << data.max_equity << "\n";

    if (data.bankrupt)
    {
        if (! is_tty_output_)
        {
            std::cout << "*** BANKRUPT ***\n";
        }
        else
        {
            std::cout << "\033[0;31m*** BANKRUPT ***\033[0m\n";
        }
    }
}

void hft_display_filter::print_number(double num, int len)
{
    std::string num_str = std::to_string(num);
    num_str.resize(len, ' ');

    if (! is_tty_output_)
    {
        std::cout << num_str;
    }
    else
    {
        if (num < 0.0)
        {
            std::cout << "\033[0;31m";
        }
        else
        {
            std::cout << "\033[0;32m";
        }

        std::cout << num_str << "\033[0m";
    }
}

void hft_display_filter::print_string(const std::string &s, int len)
{
    auto l = s.length();

    if (l > len)
    {
        std::cout << s.substr(0, len);

        return;
    }

    std::string completion(len - l, ' ');
    std::cout << s << completion;
}

size_t hft_display_filter::get_max_instrument_strlen(const hft_dukascopy_emulator::emulation_result &data)
{
    size_t maxl = 0;

    for (auto &x : data.trades)
    {
        if (x.instrument.length() > maxl)
        {
            maxl = x.instrument.length();
        }
    }

    return maxl;
}

