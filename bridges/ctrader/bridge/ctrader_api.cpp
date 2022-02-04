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

#include <ctrader_api.hpp>

void ctrader_api::ctrader_heart_beat(void)
{
    ProtoHeartbeatEvent req;
    std::string payload;

    req.SerializeToString(&payload);

    send_message(req.payloadtype(), payload);
}

void ctrader_api::ctrader_authorize_application(const std::string &client_id, const std::string &client_secret)
{
    ProtoOAApplicationAuthReq req;
    std::string payload;

    req.set_clientid(client_id);
    req.set_clientsecret(client_secret);
    req.SerializeToString(&payload);

    send_message(req.payloadtype(), payload);
}

void ctrader_api::ctrader_authorize_account(const std::string &access_token, int account_id)
{
    ProtoOAAccountAuthReq req;
    std::string payload;

    req.set_accesstoken(access_token);
    req.set_ctidtraderaccountid(account_id);
    req.SerializeToString(&payload);

    send_message(req.payloadtype(), payload);
}

void ctrader_api::ctrader_account_information(int account_id)
{
    ProtoOATraderReq req;
    std::string payload;

    req.set_ctidtraderaccountid(account_id);
    req.SerializeToString(&payload);

    send_message(req.payloadtype(), payload);
}

void ctrader_api::ctrader_available_instruments(int account_id)
{
    ProtoOASymbolsListReq req;
    std::string payload;

    req.set_ctidtraderaccountid(account_id);
    req.SerializeToString(&payload);

    send_message(req.payloadtype(), payload);
}

void ctrader_api::ctrader_subscribe_instruments(const instrument_id_container &data, int account_id)
{
    ProtoOASubscribeSpotsReq req;
    std::string payload;

    req.set_ctidtraderaccountid(account_id);

    for (auto &id : data)
    {
        req.add_symbolid(id);
    }

    req.SerializeToString(&payload);

    send_message(req.payloadtype(), payload);
}

void ctrader_api::ctrader_create_market_order(const std::string &identifier, int instrument_id, position_type pt, int volume, int account_id)
{
    ProtoOANewOrderReq req;
    std::string payload;

    req.set_ctidtraderaccountid(account_id);
    req.set_symbolid(instrument_id);
    req.set_ordertype(MARKET);
    req.set_volume(volume);
    req.set_label(identifier);

    if (pt == position_type::LONG_POSITION)
    {
        req.set_tradeside(BUY);
    }
    else if (pt == position_type::SHORT_POSITION)
    {
        req.set_tradeside(SELL);
    }

    req.SerializeToString(&payload);

    send_message(req.payloadtype(), payload);
}

void ctrader_api::ctrader_close_position(int position_id, int volume, int account_id)
{
    ProtoOAClosePositionReq req;
    std::string payload;

    req.set_ctidtraderaccountid(account_id);
    req.set_volume(volume);
    req.set_positionid(position_id);

    req.SerializeToString(&payload);

    send_message(req.payloadtype(), payload);
}

void ctrader_api::ctrader_order_list(unsigned long from_timestamp, unsigned long to_timestamp, int account_id)
{
    ProtoOAOrderListReq req;
    std::string payload;

    req.set_ctidtraderaccountid(account_id);
    req.set_fromtimestamp(from_timestamp);
    req.set_totimestamp(to_timestamp);

    req.SerializeToString(&payload);

    send_message(req.payloadtype(), payload);
}

void ctrader_api::ctrader_opened_positions_list(int account_id)
{
    ProtoOAReconcileReq req;
    std::string payload;

    req.set_ctidtraderaccountid(account_id);

    req.SerializeToString(&payload);

    send_message(req.payloadtype(), payload);
}

//
// Helper methods.
//

void ctrader_api::send_message(uint payloadType, const std::string &payload)
{
    ProtoMessage proto_msg;

    proto_msg.set_payloadtype(payloadType);
    proto_msg.set_payload(payload);

    int size = proto_msg.ByteSize();
    std::vector<char> buffer(size);

    proto_msg.SerializeToArray(&buffer.front(), size);

    connection_.send_data(buffer);
}
