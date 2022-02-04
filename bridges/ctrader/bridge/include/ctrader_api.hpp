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

#ifndef __CTRADER_API_HPP__
#define __CTRADER_API_HPP__

#include <OpenApiMessages.pb.h>
#include <OpenApiModelMessages.pb.h>
#include <OpenApiCommonMessages.pb.h>
#include <OpenApiCommonModelMessages.pb.h>

#include <vector>
#include <market_types.hpp>
#include <ctrader_ssl_connection.hpp>

class ctrader_api
{
public:

    ctrader_api(void) = delete;
    ctrader_api(ctrader_api &) = delete;
    ctrader_api(ctrader_api &&) = delete;

    ctrader_api(ctrader_ssl_connection &connection)
        : connection_(connection)
    {}

    virtual ~ctrader_api(void) = default;

    //
    // Request methods.
    //

    void heart_beat_ACK(void);
    void authorize_application(const std::string &client_id, const std::string &client_secret);
    void authorize_account(const std::string &access_token, int account_id);
    void account_information(int account_id);
    void available_instruments(int account_id);
    void subscribe_instruments(const instrument_id_container &data, int account_id);
    void create_market_order(const std::string &identifier, int instrument_id, position_type pt, int volume, int account_id);
    void close_position(int position_id, int volume, int account_id);
    void opened_positions_list(int account_id);
    void order_list(unsigned long from_timestamp, unsigned long to_timestamp, int account_id);

private:

    void send_message(uint payloadType, const std::string &payload);

    ctrader_ssl_connection &connection_;
};

#endif /* __CTRADER_API_HPP__ */
