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

    //
    // Response from HFT.
    //

    struct hft_response
    {
        hft_response(void) = delete;
        hft_response(const std::string &payload) { unserialize(payload); }

        struct pos_open_advice_info
        {
            pos_open_advice_info(position_type d, const std::string &i, double v)
                : direction {d}, identifier {i}, volume {v}
            {}

            position_type direction;
            std::string identifier;
            double volume;
        };

        typedef std::list<pos_open_advice_info> pos_open_advice_info_container;
        typedef std::list<std::string> pos_close_advice_info_container;

        bool is_error(void) const { return !error_message_.empty(); }
        bool has_instrument(void) const { return !instrument_.empty(); }
        std::string get_error_message(void) const { return error_message_; }
        std::string get_instrument(void) const { return instrument_; }
        bool has_for_open(void) const { return !new_positions_info_.empty(); }
        bool has_for_close(void) const { return !close_positions_info_.empty(); }
        const pos_open_advice_info_container &get_for_open(void) const { return new_positions_info_; }
        const pos_close_advice_info_container &get_for_close(void) const { return close_positions_info_; }

    private:

        void unserialize(const std::string &payload);

        std::string error_message_;
        std::string instrument_;
        pos_open_advice_info_container new_positions_info_;
        pos_close_advice_info_container close_positions_info_;
    };

private:

    hft_connection &connection_;
};

#endif /* __HFT_API_HPP__ */
