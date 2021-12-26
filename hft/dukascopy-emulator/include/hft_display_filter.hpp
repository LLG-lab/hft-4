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

#ifndef __HFT_DISPLAY_FILTER_HPP__
#define __HFT_DISPLAY_FILTER_HPP__

#include <hft_dukascopy_emulator.hpp>

class hft_display_filter
{
public:

    hft_display_filter(void);

    void display(const hft_dukascopy_emulator::emulation_result &data);

private:

    void print_number(double num, int len);

    void print_string(const std::string &s, int len);

    size_t get_max_instrument_strlen(const hft_dukascopy_emulator::emulation_result &data);

    const bool is_tty_output_;
};

#endif /* __HFT_DISPLAY_FILTER_HPP__ */
