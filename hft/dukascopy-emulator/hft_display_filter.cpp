/**********************************************************************\
**                                                                    **
**             -=≡≣ High Frequency Trading System  ≣≡=-              **
**                                                                    **
**          Copyright  2017 - 2021 by LLG Ryszard Gradowski          **
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
    for (auto &x : data)
    {
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
