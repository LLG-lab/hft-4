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

#ifndef __HFT_API_HPP__
#define __HFT_API_HPP__

#include <market_types.hpp>
#include <hft_connection.hpp>
#include <string>
#include <vector>

class hft_api
{
public:

    hft_api(void) = delete;
    hft_api(hft_api &) = delete;
    hft_api(hft_api &&) = delete;

    hft_api(hft_connection &connection)
        : connection_ {connection}
    {}

    virtual ~hft_api(void) = default;

    //
    // Request methods.
    //

    void hft_init_session(const std::string &sessid, const instruments_container &instruments);
    void hft_sync(const std::string &instrument, unsigned long timestamp, const std::string &identifier, position_type direction, double price, int volume);
    void hft_send_tick(const std::string &instrument, unsigned long timestamp, double ask, double bid, double equity, double free_margin);
    void hft_send_open_notify(const std::string &instrument, const std::string &identifier, bool status, double price);
    void hft_send_close_notify(const std::string &instrument, const std::string &identifier, bool status, double price);

private:

    hft_connection &connection_;
};

#endif /* __HFT_API_HPP__ */
