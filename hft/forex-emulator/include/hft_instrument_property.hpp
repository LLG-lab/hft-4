/**********************************************************************\
**                                                                    **
**             -=≡≣ High Frequency Trading System ® ≣≡=-              **
**                                                                    **
**          Copyright © 2017 - 2024 by LLG Ryszard Gradowski          **
**                       All Rights Reserved.                         **
**                                                                    **
**  CAUTION! This application is an intellectual property             **
**           of LLG Ryszard Gradowski. This application as            **
**           well as any part of source code cannot be used,          **
**           modified and distributed by third party person           **
**           without prior written permission issued by               **
**           intellectual property owner.                             **
**                                                                    **
\**********************************************************************/

#ifndef __HFT_INSTRUMENT_PROPERTY_HPP__
#define __HFT_INSTRUMENT_PROPERTY_HPP__

#include <string>

class hft_instrument_property
{
public:

    hft_instrument_property(void) = delete;
    hft_instrument_property(const std::string &instrument, const std::string &config_file_name);
    ~hft_instrument_property(void) = default;

    char   get_pip_significant_digit(void) const { return pip_significant_digit_; }
    double get_pip_value_per_lot(void) const { return pip_value_per_lot_; }
    double get_long_dayswap_per_lot(void) const { return long_dayswap_per_lot_; }
    double get_short_dayswap_per_lot(void) const { return short_dayswap_per_lot_; }
    double get_commision_per_lot(void) const { return commision_per_lot_; }
    double get_margin_required_per_lot(void) const { return margin_required_per_lot_; }

private:

    char   pip_significant_digit_;
    double pip_value_per_lot_;
    double long_dayswap_per_lot_;
    double short_dayswap_per_lot_;
    double commision_per_lot_;
    double margin_required_per_lot_;

};

#endif /*  __HFT_INSTRUMENT_PROPERTY_HPP__ */
