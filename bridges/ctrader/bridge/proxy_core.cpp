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

#include <easylogging++.h>

#include <proxy_core.hpp>
#include <aux_functions.hpp>

#undef ORDER_EXECUTION_DEBUG

#define hft2ctrader_log(__X__) \
    CLOG(__X__, "session")

namespace {

std::string payload_type2str(uint payload_type);

#ifdef ORDER_EXECUTION_DEBUG
void display_execution_event(const ProtoOAExecutionEvent &event);
void display_protooaposition(const ProtoOAPosition &pos, const std::string &prepend);
void display_tradedata(const ProtoOATradeData &tradedata, const std::string &prepend);
void display_protooaorder(const ProtoOAOrder &order, const std::string &prepend);
void display_protooadeal(const ProtoOADeal &deal, const std::string &prepend);
void display_closepositiondetail(const ProtoOAClosePositionDetail &cpd, const std::string &prepend);
#endif /* ORDER_EXECUTION_DEBUG */


} // namespace.

proxy_core::proxy_core(ctrader_ssl_connection &ctrader_conn, hft_connection &hft_conn, const hft2ctrader_config &config)
    : ctrader_api {ctrader_conn}, hft_api {hft_conn}, config_{config},
      last_heartbeat_ {0ul},
      registration_timestamp_ {0ul}
{
   el::Loggers::getLogger("session", true);
}

//
// Transition action methods and guards.
//

void proxy_core::start_app_authorization(ctrader_connection_event const &event)
{
    hft2ctrader_log(INFO) << "Start application authorization.";

    ctrader_authorize_application(config_.get_auth_client_id(), config_.get_auth_client_secret());
}

void proxy_core::start_account_authorization(ctrader_data_event const &event)
{
    hft2ctrader_log(INFO) << "Start account authorization.";

    ctrader_authorize_account(config_.get_auth_access_token(), config_.get_auth_account_id());
}

bool proxy_core::is_app_authorized(ctrader_data_event const &event)
{
    ProtoMessage msg;

    msg.ParseFromArray(&event.data_.front(), event.data_.size());
    uint payload_type   = msg.payloadtype();
    std::string payload = msg.payload();

    switch (payload_type)
    {
        case PROTO_OA_APPLICATION_AUTH_RES:
        {
             hft2ctrader_log(INFO) << "Application authorization SUCCESS.";

             return true;
        }
        case PROTO_OA_ERROR_RES:
        {
            hft2ctrader_log(ERROR) << "Application authorization ERROR:\n"
                                   << aux::hexdump(payload);

            throw std::runtime_error("Application authorization failed");
        }
        default:
            hft2ctrader_log(WARNING) << "is_app_authorized: Received unhandled message from server #"
                                     << payload_type;
    }

    return false;
}

void proxy_core::start_acquire_account_informations(ctrader_data_event const &event)
{
    hft2ctrader_log(INFO) << "Start acquire account informations.";

    ctrader_account_information(config_.get_auth_account_id());
}

bool proxy_core::is_account_authorized(ctrader_data_event const &event)
{
    ProtoMessage msg;

    msg.ParseFromArray(&event.data_.front(), event.data_.size());
    uint payload_type   = msg.payloadtype();
    std::string payload = msg.payload();

    switch (payload_type)
    {
        case PROTO_OA_ACCOUNT_AUTH_RES:
        {
            ProtoOAAccountAuthRes res;
            res.ParseFromString(payload);

            if (res.has_ctidtraderaccountid())
            {
                hft2ctrader_log(INFO) << "Account ‘" << res.ctidtraderaccountid()
                                      << "’ authorization SUCCESS.";
            }

            return true;
        }
        case PROTO_OA_ERROR_RES:
        {
            hft2ctrader_log(ERROR) << "Account authorization ERROR:\n"
                                   << aux::hexdump(payload);

            throw std::runtime_error("Account authorization failed");
        }
        default:
            hft2ctrader_log(WARNING) << "is_account_authorized: Received unhandled message from server #"
                                     << payload_type;
    }

    return false;
}

void proxy_core::start_acquire_instrument_informations(ctrader_data_event const &event)
{
    hft2ctrader_log(INFO) << "Start acquire available instrument informations.";

    ctrader_available_instruments(config_.get_auth_account_id());
}

bool proxy_core::has_account_informations(ctrader_data_event const &event)
{
    ProtoMessage msg;

    msg.ParseFromArray(&event.data_.front(), event.data_.size());
    uint payload_type   = msg.payloadtype();
    std::string payload = msg.payload();

    switch (payload_type)
    {
        case PROTO_OA_TRADER_RES:
        {
            ProtoOATraderRes res;
            res.ParseFromString(payload);

            handle_oa_trader(res.trader());

            return true;
        }
        case PROTO_OA_ERROR_RES:
        {
            hft2ctrader_log(ERROR) << "Account information acquisition ERROR:\n"
                                   << aux::hexdump(payload);

            throw std::runtime_error("Account information acquisition failed");
        }
        default:
            hft2ctrader_log(WARNING) << "has_account_informations: Received unhandled message from server #"
                                     << payload_type;

    }

    return false;
}

void proxy_core::start_acquire_position_informations(ctrader_data_event const &event)
{
    hft2ctrader_log(INFO) << "Retrieving information about open positions";

    ctrader_opened_positions_list(config_.get_auth_account_id());
}

bool proxy_core::has_instrument_informations(ctrader_data_event const &event)
{
    ProtoMessage msg;

    msg.ParseFromArray(&event.data_.front(), event.data_.size());
    uint payload_type   = msg.payloadtype();
    std::string payload = msg.payload();

    switch (payload_type)
    {
        case PROTO_OA_SYMBOLS_LIST_RES:
        {
            ProtoOASymbolsListRes res;
            res.ParseFromString(payload);

            for (int i = 0; i < res.symbol_size(); i++)
            {
                if (res.symbol(i).enabled())
                {
                    ticker2id_[res.symbol(i).symbolname()] = res.symbol(i).symbolid();
                    id2ticker_[res.symbol(i).symbolid()] = res.symbol(i).symbolname();
                }

                hft2ctrader_log(TRACE) << "ID: " << res.symbol(i).symbolid() << ", "
                                       << "Name: " << res.symbol(i).symbolname() << ", "
                                       << "enabled: " << (res.symbol(i).enabled() ? "yes" : "no") << ", "
                                       << "description: " << res.symbol(i).description();
            }

            hft2ctrader_log(INFO) << "Received available instruments table.";

            return true;
        }
        case PROTO_OA_ERROR_RES:
        {
            hft2ctrader_log(ERROR) << "Acquisition of available instruments information ERROR:\n"
                                   << aux::hexdump(payload);

            throw std::runtime_error("Acquisition of available instruments information failed");
        }
        default:
            hft2ctrader_log(WARNING) << "has_instrument_informations: Received unhandled message from server #"
                                     << payload_type;
    }

    return false;
}

void proxy_core::initialize_bridge(ctrader_data_event const &event)
{
    on_init();
}

bool proxy_core::has_position_informations(ctrader_data_event const &event)
{
    ProtoMessage msg;

    msg.ParseFromArray(&event.data_.front(), event.data_.size());
    uint payload_type   = msg.payloadtype();
    std::string payload = msg.payload();

    switch (payload_type)
    {
        case PROTO_OA_RECONCILE_RES:
        {
            ProtoOAReconcileRes res;
            res.ParseFromString(payload);

            for (int i = 0; i < res.position_size(); i++)
            {
                arrange_position(res.position(i), true);
            }

            return true;
        }
        case PROTO_OA_ERROR_RES:
        {
            hft2ctrader_log(ERROR) << "Acquisition of open positions information ERROR:\n"
                                   << aux::hexdump(payload);

            throw std::runtime_error("Acquisition of open positions information failed");
        }
        default:
            hft2ctrader_log(WARNING) << "has_position_informations: Received unhandled message from server #"
                                     << payload_type;
    }

    return false;
}

void proxy_core::dispatch_ctrader_data_event(ctrader_data_event const &event)
{
    ProtoMessage msg;

    msg.ParseFromArray(&event.data_.front(), event.data_.size());
    uint payload_type   = msg.payloadtype();
    std::string payload = msg.payload();

    auto now = aux::get_current_timestamp();

    if (now - last_heartbeat_ >= 9000)
    {
        hft2ctrader_log(TRACE) << "Raised heart beat";

        ctrader_heart_beat();

        last_heartbeat_ = now;
    }

    switch (payload_type)
    {
        case HEARTBEAT_EVENT:
        {
            hft2ctrader_log(TRACE) << "Heart beat event";

            ctrader_heart_beat();

            last_heartbeat_ = now;

            break;
        }
        case PROTO_OA_ACCOUNT_DISCONNECT_EVENT:
        {
            // FIXME: Do ogarnięcia.
            hft2ctrader_log(FATAL) << "Received PROTO_OA_ACCOUNT_DISCONNECT_EVENT – "
                                   << "established session for an account is dropped"
                                   << " on the server side.";

            ::exit(0);

            break;
        }
        case PROTO_OA_CLIENT_DISCONNECT_EVENT:
        {
            ProtoOAClientDisconnectEvent evt;
            evt.ParseFromString(payload);

            if (evt.has_reason())
            {
                hft2ctrader_log(FATAL) << "Connection with the client "
                                       << "application is cancelled by "
                                       << "the server. Reason is ‘"
                                       << evt.reason() << "’";
            }
            else
            {
                hft2ctrader_log(FATAL) << "Connection with the client "
                                       << "application is cancelled by "
                                       << "the server with no explanation.";
            }

            ::exit(0);

            break;
        }
        case PROTO_OA_SUBSCRIBE_SPOTS_RES:
        {
            hft2ctrader_log(INFO) << "Subscribe instruments SUCCESS.";

            break;
        }
        case PROTO_OA_SYMBOL_BY_ID_RES:
        {
            ProtoOASymbolByIdRes res;
            res.ParseFromString(payload);

            for (int i = 0; i < res.symbol_size(); i++)
            {
                detailed_instrument_info dii;

                int symbolid = res.symbol(i).symbolid();
                dii.instrument_id_ = symbolid;
                dii.step_volume_ = res.symbol(i).stepvolume() / 100;
                dii.max_volume_  = res.symbol(i).maxvolume()  / 100;
                dii.min_volume_  = res.symbol(i).minvolume()  / 100;

                hft2ctrader_log(TRACE) << "Instrument: "
                                       << id2ticker_[symbolid]
                                       << ", min_volume: "
                                       << dii.min_volume_
                                       << ", max_volume: "
                                       << dii.max_volume_
                                       << ", step_volume: "
                                       << dii.step_volume_;

                instrument_info_[symbolid] = dii;
            }

            break;
        }
        case PROTO_OA_TRADER_UPDATE_EVENT:
        {
            ProtoOATraderUpdatedEvent evt;
            evt.ParseFromString(payload);

            handle_oa_trader(evt.trader());

            break;
        }
        case PROTO_OA_SPOT_EVENT:
        {
            ProtoOASpotEvent evt;
            evt.ParseFromString(payload);

            if (instruments_tick_.find(evt.symbolid()) == instruments_tick_.end())
            {
                instruments_tick_[evt.symbolid()] = tick_type();
                instruments_tick_[evt.symbolid()].instrument = id2ticker_[evt.symbolid()];

                if (evt.has_ask())
                {
                    instruments_tick_[evt.symbolid()].ask = (double)(evt.ask()) / 100000.0;
                }

                if (evt.has_bid())
                {
                    instruments_tick_[evt.symbolid()].bid = (double)(evt.bid()) / 100000.0;
                }

                break;
            }

            tick_type &tick = instruments_tick_[evt.symbolid()];

            if (evt.has_ask())
            {
                tick.ask = (double)(evt.ask()) / 100000.0;
            }

            if (evt.has_bid())
            {
                tick.bid = (double)(evt.bid()) / 100000.0;
            }

            if (evt.has_timestamp())
            {
                tick.timestamp = evt.timestamp();
            }
            else
            {
                tick.timestamp = now;
            }

            if (tick.ask > 0 && tick.bid > 0)
            {
                on_tick(tick);
            }

            break;
        }
        case PROTO_OA_EXECUTION_EVENT:
        {
            ProtoOAExecutionEvent evt;
            evt.ParseFromString(payload);

            #ifdef ORDER_EXECUTION_DEBUG
            display_execution_event(evt);
            #endif

            switch (evt.executiontype())
            {
                case ORDER_ACCEPTED:
                         break;
                case ORDER_FILLED:
                case ORDER_PARTIAL_FILL:
                         handle_order_fill(evt);
                         break;
                case ORDER_REPLACED:
                         break;
                case ORDER_CANCELLED:
                case ORDER_EXPIRED:
                case ORDER_REJECTED:
                         handle_order_reject(evt);
                         break;
                case ORDER_CANCEL_REJECTED:
                         break;
                default:
                         break;
            }

            break;
        }
        case PROTO_OA_ORDER_ERROR_EVENT:
        {
            ProtoOAOrderErrorEvent evt;
            evt.ParseFromString(payload);

            std::ostringstream log_err_msg;

            log_err_msg << "Received ORDER ERROR (" << evt.errorcode()
                        << ") – " << evt.description();

            if (evt.has_orderid())
            {
                log_err_msg << ", order ID: " << evt.orderid();
            }

            if (evt.has_positionid())
            {
                log_err_msg << ", position ID: " << evt.positionid();
            }

            log_err_msg << ".";

            hft2ctrader_log(ERROR) << log_err_msg.str();

            break;
        }
        case PROTO_OA_ERROR_RES:
        {
            hft2ctrader_log(ERROR) << "Response ERROR:\n"
                                   << aux::hexdump(payload);

            break;
        }
        default:
            hft2ctrader_log(WARNING) << "dispatch_ctrader_data_event: Received unhandled message from server #"
                                     << payload_type << " – " << payload_type2str(payload_type);
    }
}

void proxy_core::dispatch_hft_data_event(hft_data_event const &event)
{
    try
    {
        hft_api::hft_response rsp {event.data_};

        on_hft_advice(rsp, true);
    }
    catch (const std::exception &e)
    {
        hft2ctrader_log(ERROR) << "Response ERROR: ‘" << e.what() << "’";
    }
}

void proxy_core::hft_data_event_broker_disconnected(hft_data_event const &event)
{
    try
    {
        hft_api::hft_response rsp {event.data_};

        on_hft_advice(rsp, false);
    }
    catch (const std::exception &e)
    {
        hft2ctrader_log(ERROR) << "Response ERROR: ‘" << e.what() << "’";
    }
}

//
// Auxiliary private methods.
//

void proxy_core::arrange_position(const ProtoOAPosition &pos, bool historical)
{
    hft2ctrader_log(TRACE) << "****";
    hft2ctrader_log(TRACE) << "positionId: " << pos.positionid();

    switch (pos.positionstatus())
    {
        case POSITION_STATUS_OPEN:
            hft2ctrader_log(TRACE) << "positionStatus: POSITION_STATUS_OPEN";

            break;
        case POSITION_STATUS_CLOSED:
            hft2ctrader_log(TRACE) << "positionStatus: POSITION_STATUS_CLOSED";

            for (auto it = positions_.begin(); it != positions_.end(); it++)
            {
                if (it -> position_id_ == pos.positionid())
                {
                    hft2ctrader_log(INFO) << "Removed position #"
                                          << it -> position_id_
                                          << " i.e. ‘" << it -> label_
                                          << "’, instrument ‘"
                                          << id2ticker_[it -> instrument_id_]
                                          << "’.";

                    positions_.erase(it);

                    break;
                }
            }

            return; // Nothing to do after deletion from the positions_ list.
        case POSITION_STATUS_CREATED: // Empty position is created for pending order.
            hft2ctrader_log(TRACE) << "positionStatus: POSITION_STATUS_CREATED";

            return; // Nothing to do here.
        case POSITION_STATUS_ERROR:
            hft2ctrader_log(TRACE) << "positionStatus: POSITION_STATUS_ERROR";

            return; // Nothing to do here.
    }

    int money_divisor = 1;

    if (pos.has_moneydigits())
    {
        hft2ctrader_log(TRACE) << "moneyDigits: " << pos.moneydigits();

        int md = pos.moneydigits();

        while (md--) money_divisor *= 10;
    }

    position_info pinfo;

/*
  Field                  Type                        Label         Description
-------------------------------------------------------------------------------------------------------------------
positionid	            int64	                    required	The unique ID of the position. Note: trader might have
                                                                two positions with the same id if positions are taken
                                                                from accounts from different brokers.
                                                                ---
tradedata	            ProtoOATradeData	        required	Position details. See ProtoOATradeData for details.
                                                                --- 
positiondtatus	        ProtoOAPositionStatus	    required	Current status of the position.
                                                                ---
swap	                int64	                    required	Total amount of charged swap on open position.
                                                                ---
price	                double	                    optional	VWAP price of the position based on all executions
                                                                (orders) linked to the position.
                                                                ---
stoploss	            double	                    optional	Current stop loss price.
                                                                --- 
takeprofit	            double	                    optional	Current take profit price.
                                                                ---
utclastupdatetimestamp	int64	                    optional	Time of the last change of the position, including
                                                                amend SL/TP of the position, execution of related
                                                                order, cancel or related order, etc.
                                                                ---
commission	            int64	                    optional	Current unrealized commission related to the position.
                                                                ---
marginrate	            double	                    optional	Rate for used margin computation. Represented as Base/Deposit.
                                                                ---
mirroringcommission	    int64	                    optional	Amount of unrealized commission related to
                                                                following of strategy provider.
                                                                ---
guaranteedstoploss	    bool	                    optional	If TRUE then position's stop loss is guaranteedStopLoss.
                                                                ---
usedmargin	            uint64	                    optional	Amount of margin used for the position in deposit currency.
                                                                ---
stoplosstriggermethod	ProtoOAOrderTriggerMethod	optional	Stop trigger method for SL/TP of the position. Default: TRADE
                                                                ---
moneydigits	            uint32	                    optional	Specifies the exponent of the monetary values.
                                                                E.g. moneyDigits = 8 must be interpret as business
                                                                value multiplied by 10^8, then real balance
                                                                would be 10053099944 / 10^8 = 100.53099944.
                                                                Affects swap, commission, mirroringCommission, usedMargin.
                                                                ---
trailingstoploss	    bool	                    optional	If TRUE then the Trailing Stop Loss is applied.
*/

    pinfo.position_id_ = pos.positionid();

    //
    // Processing tradeData field.
    //

    hft2ctrader_log(TRACE) << "tradeData:";

/*
   Field               Type              Label       Description
------------------------------------------------------------------------
symbolid	          int64             required   The unique identifier of the symbol in specific server
                                                   environment within cTrader platform. Different brokers
                                                   might have different IDs.
                                                   ---
volume	              int64             required   Volume in cents.
                                                   ---
tradeside	          ProtoOATradeSide  required   Buy, Sell.
                                                   ---
opentimestamp         int64             optional   Time when position was opened or order was created.
                                                   ---
label                 string            optional   Text label specified during order request.
                                                   ---
guaranteedstoploss	  bool              optional   If TRUE then position/order stop loss is guaranteedStopLoss.
                                                   ---
comment               string            optional   User-specified comment.

*/

    hft2ctrader_log(TRACE) << "    symbolId: " << pos.tradedata().symbolid();
    pinfo.instrument_id_ = pos.tradedata().symbolid();

    hft2ctrader_log(TRACE) << "    volume: " << pos.tradedata().volume();
    pinfo.volume_ = pos.tradedata().volume() / 100;

    switch (pos.tradedata().tradeside())
    {
        case BUY:
            hft2ctrader_log(TRACE) << "    tradeSide: BUY";
            pinfo.trade_side_ = position_type::LONG_POSITION;

            break;
        case SELL:
            hft2ctrader_log(TRACE) << "    tradeSide: SELL";
            pinfo.trade_side_ = position_type::SHORT_POSITION;

            break;
    }

    if (pos.tradedata().has_opentimestamp())
    {
        hft2ctrader_log(TRACE) << "    openTimestamp: "
                               << pos.tradedata().opentimestamp();

        pinfo.timestamp_ = pos.tradedata().opentimestamp();
    }

    if (pos.tradedata().has_label())
    {
        hft2ctrader_log(TRACE) << "    label: " << pos.tradedata().label();

        pinfo.label_ = pos.tradedata().label();
    }

    if (pos.tradedata().has_guaranteedstoploss())
    {
        hft2ctrader_log(TRACE) << "    guaranteedStopLoss: "
                               << (pos.tradedata().guaranteedstoploss() ? "YES" : "NO");
    }

    if (pos.tradedata().has_comment())
    {
        hft2ctrader_log(TRACE) << "   comment: ‘"
                               << pos.tradedata().comment() << "’";
    }

    //
    // End of tradeData.
    //

    hft2ctrader_log(TRACE) << "swap: " << pos.swap();

    pinfo.swap_ = double(pos.swap()) / money_divisor;

    if (pos.has_price())
    {
        hft2ctrader_log(TRACE) << "price: " << pos.price();

        pinfo.execution_price_ = pos.price();
    }

    if (pos.has_stoploss())
    {
        hft2ctrader_log(TRACE) << "stopLoss: " << pos.stoploss();
    }

    if (pos.has_takeprofit())
    {
        hft2ctrader_log(TRACE) << "takeProfit: " << pos.takeprofit();
    }

    if (pos.has_utclastupdatetimestamp())
    {
        hft2ctrader_log(TRACE) << "utcLastUpdateTimestamp: "
                               << pos.utclastupdatetimestamp();
    }

    if (pos.has_commission())
    {
        hft2ctrader_log(TRACE) << "commission: " << pos.commission();

        pinfo.commission_ = double(pos.commission()) / money_divisor;
    }

    if (pos.has_marginrate())
    {
        hft2ctrader_log(TRACE) << "marginRate: " << pos.marginrate();
    }

    if (pos.has_mirroringcommission())
    {
        hft2ctrader_log(TRACE) << "mirroringCommission: "
                               << pos.mirroringcommission();
    }

    if (pos.has_guaranteedstoploss())
    {
        hft2ctrader_log(TRACE) << "guaranteedStopLoss: "
                               << (pos.guaranteedstoploss() ? "yes" : "no");
    }

    if (pos.has_usedmargin())
    {
        hft2ctrader_log(TRACE) << "usedMargin: " << pos.usedmargin();

        pinfo.used_margin_ = double(pos.usedmargin()) / money_divisor;
    }

    if (pos.has_stoplosstriggermethod())
    {
        switch (pos.stoplosstriggermethod())
        {
            case TRADE:
                // Stop Order: buy is triggered by ask, sell by bid;
                // Stop Loss Order: for buy position is triggered
                // by bid and for sell position by ask.

                hft2ctrader_log(TRACE) << "stopLossTriggerMethod: TRADE";

                break;
            case OPPOSITE:
                // Stop Order: buy is triggered by bid, sell by ask;
                // Stop Loss Order: for buy position is triggered
                // by ask and for sell position by bid.

                hft2ctrader_log(TRACE) << "stopLossTriggerMethod: OPPOSITE";

                break;
            case DOUBLE_TRADE:
                // The same as TRADE, but trigger is checked
                // after the second consecutive tick.

                hft2ctrader_log(TRACE) << "stopLossTriggerMethod: DOUBLE_TRADE";

                break;
            case DOUBLE_OPPOSITE:
                // The same as OPPOSITE, but trigger is checked
                // after the second consecutive tick.

                hft2ctrader_log(TRACE) << "stopLossTriggerMethod: DOUBLE_OPPOSITE";

                break;
        }
    }

    if (pos.has_trailingstoploss())
    {
        hft2ctrader_log(TRACE) << "trailingStopLoss: "
                               << pos.trailingstoploss();
    }

    auto it = positions_.begin();

    while (it != positions_.end())
    {
        if (it -> position_id_ == pinfo.position_id_)
        {
            (*it) = pinfo;

            break;
        }

        it++;
    }

    if (it == positions_.end())
    {
        positions_.push_back(pinfo);
    }

    if (! historical)
    {
        on_position_open(pinfo);
    }
}

void proxy_core::handle_order_fill(const ProtoOAExecutionEvent &evt)
{
    if (evt.has_position())
    {
        auto &pos = evt.position();

        arrange_position(pos, false);

        if (pos.positionstatus() == POSITION_STATUS_CLOSED)
        {
            closed_position_info cpi;

            cpi.instrument_id_ = pos.tradedata().symbolid();

            if (pos.tradedata().has_label())
            {
                cpi.label_ = pos.tradedata().label();
            }

            if (evt.has_deal())
            {
                auto &deal = evt.deal();

                if (deal.has_executionprice())
                {
                    cpi.execution_price_ = deal.executionprice();
                }

                //
                // Update account balance before call
                // ‘on_position_close’ handler.
                //

                if (deal.has_closepositiondetail())
                {
                    auto &cpd = deal.closepositiondetail();

                    int money_divisor = 1;

                    if (cpd.has_moneydigits())
                    {
                        int md = cpd.moneydigits();

                        while (md--) money_divisor *= 10;
                    }

                    account_balance_ = double(cpd.balance()) / money_divisor;
                }
            }

            on_position_close(cpi);
        }
    }
}

void proxy_core::handle_order_reject(const ProtoOAExecutionEvent &evt)
{
    if (evt.has_errorcode())
    {
        order_error_info oei;

        oei.error_message_ = evt.errorcode();

        if (evt.has_position())
        {
            auto &pos = evt.position();

            oei.instrument_id_ = pos.tradedata().symbolid();

            if (pos.tradedata().has_label())
            {
                oei.label_ = pos.tradedata().label();
            }

            switch (pos.positionstatus())
            {
                case POSITION_STATUS_CLOSED:
                {
                    //
                    // We assume that the error resulted from
                    // an attempt to open a position.
                    //

                    on_position_open_error(oei);

                    break;
                }
                case POSITION_STATUS_OPEN:
                {
                    //
                    // We assume that the error resulted from
                    // an attempt to close a position.
                    //

                    on_position_close_error(oei);

                    break;
                }
                case POSITION_STATUS_CREATED:
                {
                    hft2ctrader_log(TRACE) << "handle_order_reject: Don't know what to do with ‘POSITION_STATUS_CREATED’";

                    break;
                }
                case POSITION_STATUS_ERROR:
                {
                    hft2ctrader_log(TRACE) << "handle_order_reject: Don't know what to do with ‘POSITION_STATUS_ERROR’";

                    break;
                }
            }
        }
    }
}

void proxy_core::handle_oa_trader(const ProtoOATrader &trader)
{
    hft2ctrader_log(INFO) << "Account information status:";

    int money_divisor = 1;

    if (trader.has_moneydigits())
    {
        hft2ctrader_log(TRACE) << "moneyDigits: " << trader.moneydigits();

        int md = trader.moneydigits();

        while (md--) money_divisor *= 10;
    }

    if (trader.has_accounttype())
    {
        switch (trader.accounttype())
        {
            case HEDGED:
                hft2ctrader_log(INFO) << "    Accoun type: HEDGED – Allows multiple positions on a trading account for a symbol.";
                break;
            case NETTED:
                hft2ctrader_log(INFO) << "    Accoun type: NETTED – Only one position per symbol is allowed on a trading account.";
                break;
            case SPREAD_BETTING:
                hft2ctrader_log(INFO) << "    Accoun type: SPREAD_BETTING – Spread betting type account.";
                break;
        }
    }

    if (trader.has_balance())
    {
        account_balance_ = (double)(trader.balance()) / money_divisor;

        hft2ctrader_log(INFO) << "    Balance: " << account_balance_;
    }

    if (trader.has_balanceversion())
    {
        hft2ctrader_log(INFO) << "    Balance version: "
                              << trader.balanceversion();
    }

    if (trader.has_leverageincents())
    {
        int leverage = trader.leverageincents() / 100;

        hft2ctrader_log(INFO) << "    Leverage: 1:" << leverage;
    }

    if (trader.has_brokername())
    {
        broker_ = trader.brokername();

        hft2ctrader_log(INFO) << "    Broker: " << broker_;
    }

    if (trader.has_registrationtimestamp())
    {
        hft2ctrader_log(INFO) << "    Account opened: "
                              << aux::timestamp2string(trader.registrationtimestamp());

        registration_timestamp_ = trader.registrationtimestamp();
    }
}

//
// Utility routines for market_session.
//

std::string proxy_core::get_instrument_ticker(int instrument_id) const
{
    auto it = id2ticker_.find(instrument_id);

    if (it == id2ticker_.end())
    {
        throw std::invalid_argument("Unrecognized instrument id: " + std::to_string(instrument_id));
    }

    return it -> second;
}

std::string proxy_core::position_type_str(const position_type &pt)
{
    switch (pt)
    {
        case position_type::LONG_POSITION:
             return "LONG";
        case position_type::SHORT_POSITION:
             return "SHORT";
    }

    return "???";
}

double proxy_core::get_free_margin(void) //const
{
    double margin = get_balance();
    double price_diff = 0.0;

    for (auto &x : positions_)
    {
        margin -= x.used_margin_;

        if (x.trade_side_ == position_type::LONG_POSITION)
        {
            price_diff = x.execution_price_ - instruments_tick_[x.instrument_id_].bid;
        }
        else if (x.trade_side_ == position_type::SHORT_POSITION)
        {
            price_diff = x.execution_price_ - instruments_tick_[x.instrument_id_].ask;
        }

        margin += price_diff * x.volume_;
        margin += 2.0*x.commission_;
        margin += x.swap_;
/* FIXME: Trzeba jeszcze rozkminić czy to będzie poprawnie działać
 *        gdy pozycja nie będzie denominowana w USD. przeliczać po kursie?
 */
    }

    return margin;
}

bool proxy_core::ctrader_subscribe_instruments_ex(const instruments_container &instruments)
{
    instrument_id_container aux;
    bool status = true;

    for (auto &instr : instruments)
    {
        auto x = ticker2id_.find(instr);

        if (x == ticker2id_.end())
        {
            hft2ctrader_log(ERROR) << "ctrader_subscribe_instruments_ex: Unsupported instrument ‘"
                                   << instr << "’";

            status = false;
        }
        else
        {
            aux.push_back(x -> second);
        }
    }

    ctrader_subscribe_instruments(aux, config_.get_auth_account_id());

    return status;
}

bool proxy_core::ctrader_instruments_information_ex(const instruments_container &instruments)
{
    instrument_id_container aux;
    bool status = true;

    for (auto &instr : instruments)
    {
        auto x = ticker2id_.find(instr);

        if (x == ticker2id_.end())
        {
            hft2ctrader_log(ERROR) << "ctrader_instruments_information_ex: Unsupported instrument ‘"
                                   << instr << "’";

            status = false;
        }
        else
        {
            aux.push_back(x -> second);
        }
    }

    ctrader_instruments_information(aux, config_.get_auth_account_id());

    return status;
}

bool proxy_core::ctrader_create_market_order_ex(const std::string &identifier, const std::string &instrument, position_type pt, int volume)
{
    int instrument_id = 0;
    auto x = ticker2id_.find(instrument);

    if (x == ticker2id_.end())
    {
        hft2ctrader_log(ERROR) << "ctrader_create_market_order_ex: Unsupported instrument ‘"
                               << instrument << "’";

        return false;
    }

    instrument_id = x -> second;

    auto y = instrument_info_.find(instrument_id);

    if (y == instrument_info_.end())
    {
        hft2ctrader_log(ERROR) << "ctrader_create_market_order_ex: Have no information details for instrument ‘"
                               << instrument << "’";

        return false;
    }

    volume /= instrument_info_[instrument_id].step_volume_;
    volume *= instrument_info_[instrument_id].step_volume_;

    if (volume < instrument_info_[instrument_id].min_volume_)
    {
        hft2ctrader_log(ERROR) << "ctrader_create_market_order_ex: Requested volume ‘"
                               << volume << "’ is less than minimum required ‘"
                               << instrument_info_[instrument_id].min_volume_
                               << "’ for instrument ‘" << instrument << "’";

        return false;
    }

    if (volume > instrument_info_[instrument_id].max_volume_)
    {
        hft2ctrader_log(ERROR) << "ctrader_create_market_order_ex: Requested volume ‘"
                               << volume << "’ is more than maximum allowed ‘"
                               << instrument_info_[instrument_id].max_volume_
                               << "’ for instrument ‘" << instrument << "’";

        return false;
    }

    ctrader_create_market_order(identifier, instrument_id, pt, volume * 100, config_.get_auth_account_id());

    return true;
}

bool proxy_core::ctrader_close_position_ex(const std::string &identifier)
{
   bool status = false;

   for (auto &p : positions_)
   {
       if (p.label_ == identifier)
       {
           status = true;

           ctrader_close_position(p.position_id_, p.volume_ * 100, config_.get_auth_account_id());

           break;
       }
   }

   if (! status)
   {
        hft2ctrader_log(ERROR) << "ctrader_close_position_ex: Position identified as ‘"
                               << identifier << "’ was not found.";
   }

   return status;
}

//
// Auxiliary routines.
//

namespace {

std::string payload_type2str(uint payload_type)
{
    switch (payload_type)
    {
        case PROTO_OA_APPLICATION_AUTH_REQ:             return "PROTO_OA_APPLICATION_AUTH_REQ";
        case PROTO_OA_APPLICATION_AUTH_RES:             return "PROTO_OA_APPLICATION_AUTH_RES";
        case PROTO_OA_ACCOUNT_AUTH_REQ:                 return "PROTO_OA_ACCOUNT_AUTH_REQ";
        case PROTO_OA_ACCOUNT_AUTH_RES:                 return "PROTO_OA_ACCOUNT_AUTH_RES";
        case PROTO_OA_VERSION_REQ:                      return "PROTO_OA_VERSION_REQ";
        case PROTO_OA_VERSION_RES:                      return "PROTO_OA_VERSION_RES";
        case PROTO_OA_NEW_ORDER_REQ:                    return "PROTO_OA_NEW_ORDER_REQ";
        case PROTO_OA_TRAILING_SL_CHANGED_EVENT:        return "PROTO_OA_TRAILING_SL_CHANGED_EVENT";
        case PROTO_OA_CANCEL_ORDER_REQ:                 return "PROTO_OA_CANCEL_ORDER_REQ";
        case PROTO_OA_AMEND_ORDER_REQ:                  return "PROTO_OA_AMEND_ORDER_REQ";
        case PROTO_OA_AMEND_POSITION_SLTP_REQ:          return "PROTO_OA_AMEND_POSITION_SLTP_REQ";
        case PROTO_OA_CLOSE_POSITION_REQ:               return "PROTO_OA_CLOSE_POSITION_REQ";
        case PROTO_OA_ASSET_LIST_REQ:                   return "PROTO_OA_ASSET_LIST_REQ";
        case PROTO_OA_ASSET_LIST_RES:                   return "PROTO_OA_ASSET_LIST_RES";
        case PROTO_OA_SYMBOLS_LIST_REQ:                 return "PROTO_OA_SYMBOLS_LIST_REQ";
        case PROTO_OA_SYMBOLS_LIST_RES:                 return "PROTO_OA_SYMBOLS_LIST_RES";
        case PROTO_OA_SYMBOL_BY_ID_REQ:                 return "PROTO_OA_SYMBOL_BY_ID_REQ";
        case PROTO_OA_SYMBOL_BY_ID_RES:                 return "PROTO_OA_SYMBOL_BY_ID_RES";
        case PROTO_OA_SYMBOLS_FOR_CONVERSION_REQ:       return "PROTO_OA_SYMBOLS_FOR_CONVERSION_REQ";
        case PROTO_OA_SYMBOLS_FOR_CONVERSION_RES:       return "PROTO_OA_SYMBOLS_FOR_CONVERSION_RES";
        case PROTO_OA_SYMBOL_CHANGED_EVENT:             return "PROTO_OA_SYMBOL_CHANGED_EVENT";
        case PROTO_OA_TRADER_REQ:                       return "PROTO_OA_TRADER_REQ";
        case PROTO_OA_TRADER_RES:                       return "PROTO_OA_TRADER_RES";
        case PROTO_OA_TRADER_UPDATE_EVENT:              return "PROTO_OA_TRADER_UPDATE_EVENT";
        case PROTO_OA_RECONCILE_REQ:                    return "PROTO_OA_RECONCILE_REQ";
        case PROTO_OA_RECONCILE_RES:                    return "PROTO_OA_RECONCILE_RES";
        case PROTO_OA_EXECUTION_EVENT:                  return "PROTO_OA_EXECUTION_EVENT";
        case PROTO_OA_SUBSCRIBE_SPOTS_REQ:              return "PROTO_OA_SUBSCRIBE_SPOTS_REQ";
        case PROTO_OA_SUBSCRIBE_SPOTS_RES:              return "PROTO_OA_SUBSCRIBE_SPOTS_RES";
        case PROTO_OA_UNSUBSCRIBE_SPOTS_REQ:            return "PROTO_OA_UNSUBSCRIBE_SPOTS_REQ";
        case PROTO_OA_UNSUBSCRIBE_SPOTS_RES:            return "PROTO_OA_UNSUBSCRIBE_SPOTS_RES";
        case PROTO_OA_SPOT_EVENT:                       return "PROTO_OA_SPOT_EVENT";
        case PROTO_OA_ORDER_ERROR_EVENT:                return "PROTO_OA_ORDER_ERROR_EVENT";
        case PROTO_OA_DEAL_LIST_REQ:                    return "PROTO_OA_DEAL_LIST_REQ";
        case PROTO_OA_DEAL_LIST_RES:                    return "PROTO_OA_DEAL_LIST_RES";
        case PROTO_OA_SUBSCRIBE_LIVE_TRENDBAR_REQ:      return "PROTO_OA_SUBSCRIBE_LIVE_TRENDBAR_REQ";
        case PROTO_OA_UNSUBSCRIBE_LIVE_TRENDBAR_REQ:    return "PROTO_OA_UNSUBSCRIBE_LIVE_TRENDBAR_REQ";
        case PROTO_OA_GET_TRENDBARS_REQ:                return "PROTO_OA_GET_TRENDBARS_REQ";
        case PROTO_OA_GET_TRENDBARS_RES:                return "PROTO_OA_GET_TRENDBARS_RES";
        case PROTO_OA_EXPECTED_MARGIN_REQ:              return "PROTO_OA_EXPECTED_MARGIN_REQ";
        case PROTO_OA_EXPECTED_MARGIN_RES:              return "PROTO_OA_EXPECTED_MARGIN_RES";
        case PROTO_OA_MARGIN_CHANGED_EVENT:             return "PROTO_OA_MARGIN_CHANGED_EVENT";
        case PROTO_OA_ERROR_RES:                        return "PROTO_OA_ERROR_RES";
        case PROTO_OA_CASH_FLOW_HISTORY_LIST_REQ:       return "PROTO_OA_CASH_FLOW_HISTORY_LIST_REQ";
        case PROTO_OA_CASH_FLOW_HISTORY_LIST_RES:       return "PROTO_OA_CASH_FLOW_HISTORY_LIST_RES";
        case PROTO_OA_GET_TICKDATA_REQ:                 return "PROTO_OA_GET_TICKDATA_REQ";
        case PROTO_OA_GET_TICKDATA_RES:                 return "PROTO_OA_GET_TICKDATA_RES";
        case PROTO_OA_ACCOUNTS_TOKEN_INVALIDATED_EVENT: return "PROTO_OA_ACCOUNTS_TOKEN_INVALIDATED_EVENT";
        case PROTO_OA_CLIENT_DISCONNECT_EVENT:          return "PROTO_OA_CLIENT_DISCONNECT_EVENT";
        case PROTO_OA_GET_ACCOUNTS_BY_ACCESS_TOKEN_REQ: return "PROTO_OA_GET_ACCOUNTS_BY_ACCESS_TOKEN_REQ";
        case PROTO_OA_GET_ACCOUNTS_BY_ACCESS_TOKEN_RES: return "PROTO_OA_GET_ACCOUNTS_BY_ACCESS_TOKEN_RES";
        case PROTO_OA_GET_CTID_PROFILE_BY_TOKEN_REQ:    return "PROTO_OA_GET_CTID_PROFILE_BY_TOKEN_REQ";
        case PROTO_OA_GET_CTID_PROFILE_BY_TOKEN_RES:    return "PROTO_OA_GET_CTID_PROFILE_BY_TOKEN_RES";
        case PROTO_OA_ASSET_CLASS_LIST_REQ:             return "PROTO_OA_ASSET_CLASS_LIST_REQ";
        case PROTO_OA_ASSET_CLASS_LIST_RES:             return "PROTO_OA_ASSET_CLASS_LIST_RES";
        case PROTO_OA_DEPTH_EVENT:                      return "PROTO_OA_DEPTH_EVENT";
        case PROTO_OA_SUBSCRIBE_DEPTH_QUOTES_REQ:       return "PROTO_OA_SUBSCRIBE_DEPTH_QUOTES_REQ";
        case PROTO_OA_SUBSCRIBE_DEPTH_QUOTES_RES:       return "PROTO_OA_SUBSCRIBE_DEPTH_QUOTES_RES";
        case PROTO_OA_UNSUBSCRIBE_DEPTH_QUOTES_REQ:     return "PROTO_OA_UNSUBSCRIBE_DEPTH_QUOTES_REQ";
        case PROTO_OA_UNSUBSCRIBE_DEPTH_QUOTES_RES:     return "PROTO_OA_UNSUBSCRIBE_DEPTH_QUOTES_RES";
        case PROTO_OA_SYMBOL_CATEGORY_REQ:              return "PROTO_OA_SYMBOL_CATEGORY_REQ";
        case PROTO_OA_SYMBOL_CATEGORY_RES:              return "PROTO_OA_SYMBOL_CATEGORY_RES";
        case PROTO_OA_ACCOUNT_LOGOUT_REQ:               return "PROTO_OA_ACCOUNT_LOGOUT_REQ";
        case PROTO_OA_ACCOUNT_LOGOUT_RES:               return "PROTO_OA_ACCOUNT_LOGOUT_RES";
        case PROTO_OA_ACCOUNT_DISCONNECT_EVENT:         return "PROTO_OA_ACCOUNT_DISCONNECT_EVENT";
        case PROTO_OA_SUBSCRIBE_LIVE_TRENDBAR_RES:      return "PROTO_OA_SUBSCRIBE_LIVE_TRENDBAR_RES";
        case PROTO_OA_UNSUBSCRIBE_LIVE_TRENDBAR_RES:    return "PROTO_OA_UNSUBSCRIBE_LIVE_TRENDBAR_RES";
        case PROTO_OA_MARGIN_CALL_LIST_REQ:             return "PROTO_OA_MARGIN_CALL_LIST_REQ";
        case PROTO_OA_MARGIN_CALL_LIST_RES:             return "PROTO_OA_MARGIN_CALL_LIST_RES";
        case PROTO_OA_MARGIN_CALL_UPDATE_REQ:           return "PROTO_OA_MARGIN_CALL_UPDATE_REQ";
        case PROTO_OA_MARGIN_CALL_UPDATE_RES:           return "PROTO_OA_MARGIN_CALL_UPDATE_RES";
        case PROTO_OA_MARGIN_CALL_UPDATE_EVENT:         return "PROTO_OA_MARGIN_CALL_UPDATE_EVENT";
        case PROTO_OA_MARGIN_CALL_TRIGGER_EVENT:        return "PROTO_OA_MARGIN_CALL_TRIGGER_EVENT";
        case PROTO_OA_REFRESH_TOKEN_REQ:                return "PROTO_OA_REFRESH_TOKEN_REQ";
        case PROTO_OA_REFRESH_TOKEN_RES:                return "PROTO_OA_REFRESH_TOKEN_RES";
        case PROTO_OA_ORDER_LIST_REQ:                   return "PROTO_OA_ORDER_LIST_REQ";
        case PROTO_OA_ORDER_LIST_RES:                   return "PROTO_OA_ORDER_LIST_RES";
        case PROTO_OA_GET_DYNAMIC_LEVERAGE_REQ:         return "PROTO_OA_GET_DYNAMIC_LEVERAGE_REQ";
        case PROTO_OA_GET_DYNAMIC_LEVERAGE_RES:         return "PROTO_OA_GET_DYNAMIC_LEVERAGE_RES";
    }

    return "";
}

//
// Log dump execution event data for debuging purposes.
//

#ifdef ORDER_EXECUTION_DEBUG

void display_execution_event(const ProtoOAExecutionEvent &event)
{
    hft2ctrader_log(DEBUG) << "Execution event:";

    switch (event.executiontype())
    {
        case ORDER_ACCEPTED:
             hft2ctrader_log(DEBUG) << "\texecutionType: ORDER_ACCEPTED";
             break;
        case ORDER_FILLED:
             hft2ctrader_log(DEBUG) << "\texecutionType: ORDER_FILLED";
             break;
        case ORDER_REPLACED:
             hft2ctrader_log(DEBUG) << "\texecutionType: ORDER_REPLACED";
             break;
        case ORDER_CANCELLED:
             hft2ctrader_log(DEBUG) << "\texecutionType: ORDER_CANCELLED";
             break;
        case ORDER_EXPIRED:
             hft2ctrader_log(DEBUG) << "\texecutionType: ORDER_EXPIRED";
             break;
        case ORDER_REJECTED:
             hft2ctrader_log(DEBUG) << "\texecutionType: ORDER_REJECTED";
             break;
        case ORDER_CANCEL_REJECTED:
             hft2ctrader_log(DEBUG) << "\texecutionType: ORDER_CANCEL_REJECTED";
             break;
        case ORDER_PARTIAL_FILL:
             hft2ctrader_log(DEBUG) << "\texecutionType: ORDER_PARTIAL_FILL";
             break;
    }

    if (event.has_errorcode())
    {
        //
        // The name of the ProtoErrorCode or the other
        // custom ErrorCodes (e.g. ProtoCHErrorCode).
        //

        hft2ctrader_log(DEBUG) << "\terrorCode: " << event.errorcode();
    }

    if (event.has_position())
    {
        hft2ctrader_log(DEBUG) << "\tposition:";
        display_protooaposition(event.position(), "\t\t");
    }

    if (event.has_order())
    {
        hft2ctrader_log(DEBUG) << "\torder:";
        display_protooaorder(event.order(), "\t\t");
    }

    if (event.has_deal())
    {
        hft2ctrader_log(DEBUG) << "\tdeal:";
        display_protooadeal(event.deal(), "\t\t");
    }

    //
    // bonusDepositWithdraw and depositWithdraw
    // was deliberately omitted.
    //

    if (event.has_isserverevent())
    {
        //
        // If TRUE then the event generated by the
        // server logic instead of the trader's
        // request. (e.g. stop-out).
        //

        hft2ctrader_log(DEBUG) << "\tisServerEvent: "
                               << (event.isserverevent() ? "YES" : "NO");
    }
}

void display_protooaposition(const ProtoOAPosition &pos, const std::string &prepend)
{
/*
  Field                  Type                        Label         Description
-------------------------------------------------------------------------------------------------------------------
positionid	            int64	                    required	The unique ID of the position. Note: trader might have
                                                                two positions with the same id if positions are taken
                                                                from accounts from different brokers.
                                                                ---
tradedata	            ProtoOATradeData	        required	Position details. See ProtoOATradeData for details.
                                                                --- 
positiondtatus	        ProtoOAPositionStatus	    required	Current status of the position.
                                                                ---
swap	                int64	                    required	Total amount of charged swap on open position.
                                                                ---
price	                double	                    optional	VWAP price of the position based on all executions
                                                                (orders) linked to the position.
                                                                ---
stoploss	            double	                    optional	Current stop loss price.
                                                                --- 
takeprofit	            double	                    optional	Current take profit price.
                                                                ---
utclastupdatetimestamp	int64	                    optional	Time of the last change of the position, including
                                                                amend SL/TP of the position, execution of related
                                                                order, cancel or related order, etc.
                                                                ---
commission	            int64	                    optional	Current unrealized commission related to the position.
                                                                ---
marginrate	            double	                    optional	Rate for used margin computation. Represented as Base/Deposit.
                                                                ---
mirroringcommission	    int64	                    optional	Amount of unrealized commission related to
                                                                following of strategy provider.
                                                                ---
guaranteedstoploss	    bool	                    optional	If TRUE then position's stop loss is guaranteedStopLoss.
                                                                ---
usedmargin	            uint64	                    optional	Amount of margin used for the position in deposit currency.
                                                                ---
stoplosstriggermethod	ProtoOAOrderTriggerMethod	optional	Stop trigger method for SL/TP of the position. Default: TRADE
                                                                ---
moneydigits	            uint32	                    optional	Specifies the exponent of the monetary values.
                                                                E.g. moneyDigits = 8 must be interpret as business
                                                                value multiplied by 10^8, then real balance
                                                                would be 10053099944 / 10^8 = 100.53099944.
                                                                Affects swap, commission, mirroringCommission, usedMargin.
                                                                ---
trailingstoploss	    bool	                    optional	If TRUE then the Trailing Stop Loss is applied.
*/
    hft2ctrader_log(DEBUG) << prepend << "positionId: " << pos.positionid();
    hft2ctrader_log(DEBUG) << prepend << "tradeData:";

    display_tradedata(pos.tradedata(), prepend + "\t");

    switch (pos.positionstatus())
    {
        case POSITION_STATUS_OPEN:
            hft2ctrader_log(DEBUG) << prepend << "positionStatus: POSITION_STATUS_OPEN";
            break;
        case POSITION_STATUS_CLOSED:
            hft2ctrader_log(DEBUG) << prepend << "positionStatus: POSITION_STATUS_CLOSED";
            break;
        case POSITION_STATUS_CREATED: // Empty position is created for pending order.
            hft2ctrader_log(DEBUG) << prepend << "positionStatus: POSITION_STATUS_CREATED";
            break;
        case POSITION_STATUS_ERROR:
            hft2ctrader_log(DEBUG) << prepend << "positionStatus: POSITION_STATUS_ERROR";
            break;
    }

    hft2ctrader_log(DEBUG) << prepend << "swap: " << pos.swap();

    if (pos.has_price())
    {
        hft2ctrader_log(DEBUG) << prepend << "price: " << pos.price();
    }

    if (pos.has_stoploss())
    {
        hft2ctrader_log(DEBUG) << prepend << "stopLoss: " << pos.stoploss();
    }

    if (pos.has_takeprofit())
    {
        hft2ctrader_log(DEBUG) << prepend << "takeProfit: " << pos.takeprofit();
    }

    if (pos.has_utclastupdatetimestamp())
    {
        hft2ctrader_log(DEBUG) << prepend << "utcLastUpdateTimestamp: "
                               << pos.utclastupdatetimestamp();
    }

    if (pos.has_commission())
    {
        hft2ctrader_log(DEBUG) << prepend << "commission: " << pos.commission();
    }

    if (pos.has_marginrate())
    {
        hft2ctrader_log(DEBUG) << prepend << "marginRate: " << pos.marginrate();
    }

    if (pos.has_mirroringcommission())
    {
        hft2ctrader_log(DEBUG) << prepend << "mirroringCommission: "
                               << pos.mirroringcommission();
    }

    if (pos.has_guaranteedstoploss())
    {
        hft2ctrader_log(DEBUG) << prepend << "guaranteedStopLoss: "
                               << (pos.guaranteedstoploss() ? "yes" : "no");
    }

    if (pos.has_usedmargin())
    {
        hft2ctrader_log(DEBUG) << prepend << "usedMargin: " << pos.usedmargin();
    }

    if (pos.has_stoplosstriggermethod())
    {
        switch (pos.stoplosstriggermethod())
        {
            case TRADE:
                // Stop Order: buy is triggered by ask, sell by bid;
                // Stop Loss Order: for buy position is triggered
                // by bid and for sell position by ask.

                hft2ctrader_log(DEBUG) << prepend
                                       << "stopLossTriggerMethod: TRADE";

                break;
            case OPPOSITE:
                // Stop Order: buy is triggered by bid, sell by ask;
                // Stop Loss Order: for buy position is triggered
                // by ask and for sell position by bid.

                hft2ctrader_log(DEBUG) << prepend
                                       << "stopLossTriggerMethod: OPPOSITE";

                break;
            case DOUBLE_TRADE:
                // The same as TRADE, but trigger is checked
                // after the second consecutive tick.

                hft2ctrader_log(DEBUG) << prepend
                                       << "stopLossTriggerMethod: DOUBLE_TRADE";

                break;
            case DOUBLE_OPPOSITE:
                // The same as OPPOSITE, but trigger is checked
                // after the second consecutive tick.

                hft2ctrader_log(DEBUG) << prepend
                                       << "stopLossTriggerMethod: DOUBLE_OPPOSITE";

                break;
        }
    }

    if (pos.has_moneydigits())
    {
        hft2ctrader_log(DEBUG) << prepend << "moneyDigits: " << pos.moneydigits();
    }

    if (pos.has_trailingstoploss())
    {
        hft2ctrader_log(DEBUG) << prepend << "trailingStopLoss: "
                               << pos.trailingstoploss();
    }
}

void display_tradedata(const ProtoOATradeData &tradedata, const std::string &prepend)
{
/*
   Field               Type              Label       Description
------------------------------------------------------------------------
symbolid	          int64             required   The unique identifier of the symbol in specific server
                                                   environment within cTrader platform. Different brokers
                                                   might have different IDs.
                                                   ---
volume	              int64             required   Volume in cents.
                                                   ---
tradeside	          ProtoOATradeSide  required   Buy, Sell.
                                                   ---
opentimestamp         int64             optional   Time when position was opened or order was created.
                                                   ---
label                 string            optional   Text label specified during order request.
                                                   ---
guaranteedstoploss	  bool              optional   If TRUE then position/order stop loss is guaranteedStopLoss.
                                                   ---
comment               string            optional   User-specified comment.

*/
    hft2ctrader_log(DEBUG) << prepend << "symbolId: " << tradedata.symbolid();
    hft2ctrader_log(DEBUG) << prepend << "volume: " << tradedata.volume();

    switch (tradedata.tradeside())
    {
        case BUY:
            hft2ctrader_log(DEBUG) << prepend << "tradeSide: BUY";
            break;
        case SELL:
            hft2ctrader_log(DEBUG) << prepend << "tradeSide: SELL";
            break;
    }

    if (tradedata.has_opentimestamp())
    {
        hft2ctrader_log(DEBUG) << prepend << "openTimestamp: "
                               << tradedata.opentimestamp();
    }

    if (tradedata.has_label())
    {
        hft2ctrader_log(DEBUG) << prepend << "label: " << tradedata.label();
    }

    if (tradedata.has_guaranteedstoploss())
    {
        hft2ctrader_log(DEBUG) << prepend << "guaranteedStopLoss: "
                               << (tradedata.guaranteedstoploss() ? "YES" : "NO");
    }

    if (tradedata.has_comment())
    {
        hft2ctrader_log(DEBUG) << prepend << "comment: ‘"
                               << tradedata.comment() << "’";
    }
}

void display_protooaorder(const ProtoOAOrder &order, const std::string &prepend)
{
/*
--------------------------------------------------------------------------------------
orderid                int64                        required    The unique ID of the order. Note: trader might have
                                                                two orders with the same id if orders are taken from
                                                                accounts from different brokers.
                                                                ---
tradedata              ProtoOATradeData             required    Detailed trader data.
                                                                ---
orderType              ProtoOAOrderType             required    Order type.
                                                                ---
orderStatus            ProtoOAOrderStatus           required    Order status.
                                                                ---
expirationTimestamp    int64                        optional    If the order has time in force GTD then expiration is specified.
                                                                ---
executionPrice         double                       optional    Price at which an order was executed. For order with FILLED status.
                                                                ---
executedVolume         int64                        optional    Part of the volume that was filled.
                                                                ---
utcLastUpdateTimestamp int64                        optional    Timestamp of the last update of the order.
                                                                ---
baseSlippagePrice      double                       optional    Used for Market Range order with combination of slippageInPoints
                                                                to specify price range were order can be executed.
                                                                ---
slippageInPoints       int64                        optional    Used for Market Range and STOP_LIMIT orders to to specify price
                                                                range were order can be executed.
                                                                ---
closingOrder           bool                         optional    If TRUE then the order is closing part of whole position.
                                                                Must have specified positionId.
                                                                ---
limitPrice             double                       optional    Valid only for LIMIT orders.
                                                                ---
stopPrice              double                       optional    Valid only for STOP and STOP_LIMIT orders.
                                                                ---
stopLoss               double                       optional    Absolute stopLoss price.
                                                                ---
takeProfit             double                       optional    Absolute takeProfit price.
                                                                ---
clientOrderId          string                       optional    Optional ClientOrderId. Max Length = 50 chars.
                                                                ---
timeInForce            ProtoOATimeInForce           optional    Order's time in force. Depends on order type.
                                                                Default: IMMEDIATE_OR_CANCEL
                                                                ---
positionId             int64                        optional    ID of the position linked to the order (e.g. closing order,
                                                                order that increase volume of a specific position, etc.).
                                                                ---
relativeStopLoss       int64                        optional    Relative stopLoss that can be specified instead of absolute
                                                                as one. Specified in 1/100_000 of unit of a price.
                                                                For BUY stopLoss = entryPrice - relativeStopLoss,
                                                                for SELL stopLoss = entryPrice + relativeStopLoss.
                                                                ---
relativeTakeProfit     int64                        optional    Relative takeProfit that can be specified instead of absolute one.
                                                                Specified in 1/100_000 of unit of a price.
                                                                ForBUY takeProfit = entryPrice + relativeTakeProfit,
                                                                for SELL takeProfit = entryPrice - relativeTakeProfit.
                                                                ---
isStopOut              bool                         optional    If TRUE then order was stopped out from server side.
                                                                ---
trailingStopLoss       bool                         optional    If TRUE then order is trailingStopLoss. Valid for
                                                                STOP_LOSS_TAKE_PROFIT order.
                                                                --- 
stopTriggerMethod      ProtoOAOrderTriggerMethod    optional    Trigger method for the order. Valid only for STOP
                                                                and STOP_LIMIT orders. Default: TRADE
*/

    hft2ctrader_log(DEBUG) << prepend << "orderId: " << order.orderid();

    hft2ctrader_log(DEBUG) << prepend << "tradeData:";
    display_tradedata(order.tradedata(), prepend + "\t");

    switch (order.ordertype())
    {
        case MARKET:
            hft2ctrader_log(DEBUG) << prepend << "orderType: MARKET";
            break;

        case LIMIT:
            hft2ctrader_log(DEBUG) << prepend << "orderType: LIMIT";
            break;

        case STOP:
            hft2ctrader_log(DEBUG) << prepend << "orderType: STOP";
            break;

        case STOP_LOSS_TAKE_PROFIT:
            hft2ctrader_log(DEBUG) << prepend << "orderType: STOP_LOSS_TAKE_PROFIT";
            break;

        case MARKET_RANGE:
            hft2ctrader_log(DEBUG) << prepend << "orderType: MARKET_RANGE";
            break;

        case STOP_LIMIT:
            hft2ctrader_log(DEBUG) << prepend << "orderType: STOP_LIMIT";
            break;
    }

    switch (order.orderstatus())
    {
        case ORDER_STATUS_ACCEPTED:
	        // Order request validated and accepted for execution.
            hft2ctrader_log(DEBUG) << prepend << "orderStatus: ORDER_STATUS_ACCEPTED";
            break;

        case ORDER_STATUS_FILLED:
            // Order is fully filled.
            hft2ctrader_log(DEBUG) << prepend << "orderStatus: ORDER_STATUS_FILLED";
            break;

        case ORDER_STATUS_REJECTED:
            // Order is rejected due to validation.
            hft2ctrader_log(DEBUG) << prepend << "orderStatus: ORDER_STATUS_REJECTED";
            break;

        case ORDER_STATUS_EXPIRED:
            // Order expired. Might be valid for orders
            // with partially filled volume that
            // were expired on LP.
            hft2ctrader_log(DEBUG) << prepend << "orderStatus: ORDER_STATUS_EXPIRED";
            break;

        case ORDER_STATUS_CANCELLED:
            // Order is cancelled. Might be valid for
            // orders with partially filled volume that
            // were cancelled by LP.
            hft2ctrader_log(DEBUG) << prepend << "orderStatus: ORDER_STATUS_CANCELLED";
            break;
    }

    if (order.has_expirationtimestamp())
    {
        hft2ctrader_log(DEBUG) << prepend << "expirationTimestamp: " << order.expirationtimestamp();
    }

    if (order.has_executionprice())
    {
        hft2ctrader_log(DEBUG) << prepend << "executionPrice: " << order.executionprice();
    }

    if (order.has_executedvolume())
    {
        hft2ctrader_log(DEBUG) << prepend << "executedVolume: " << order.executedvolume();
    }

    if (order.has_utclastupdatetimestamp())
    {
        hft2ctrader_log(DEBUG) << prepend << "utcLastUpdateTimestamp: " << order.utclastupdatetimestamp();
    }

    if (order.has_baseslippageprice())
    {
        hft2ctrader_log(DEBUG) << prepend << "baseSlippagePrice: " << order.baseslippageprice();
    }

    if (order.has_slippageinpoints())
    {
        hft2ctrader_log(DEBUG) << prepend << "slippageInPoints: " << order.slippageinpoints();
    }

    if (order.has_closingorder())
    {
        hft2ctrader_log(DEBUG) << prepend << "closingOrder: " << (order.closingorder() ? "YES" : "NO");
    }

    if (order.has_limitprice())
    {
        hft2ctrader_log(DEBUG) << prepend << "limitPrice: " << order.limitprice();
    }

    if (order.has_stopprice())
    {
        hft2ctrader_log(DEBUG) << prepend << "stopPrice: " << order.stopprice();
    }

    if (order.has_stoploss())
    {
        hft2ctrader_log(DEBUG) << prepend << "stopLoss: " << order.stoploss();
    }

    if (order.has_takeprofit())
    {
        hft2ctrader_log(DEBUG) << prepend << "takeProfit: " << order.takeprofit();
    }

    if (order.has_clientorderid())
    {
        hft2ctrader_log(DEBUG) << prepend << "clientOrderId: ‘" << order.clientorderid() << "’";
    }

    if (order.has_timeinforce())
    {
        switch (order.timeinforce())
        {
            case GOOD_TILL_DATE:
                hft2ctrader_log(DEBUG) << prepend << "timeInForce: GOOD_TILL_DATE";
                break;

            case GOOD_TILL_CANCEL:
                hft2ctrader_log(DEBUG) << prepend << "timeInForce: GOOD_TILL_CANCEL";
                break;

            case IMMEDIATE_OR_CANCEL:
                hft2ctrader_log(DEBUG) << prepend << "timeInForce: IMMEDIATE_OR_CANCEL";
                break;

            case FILL_OR_KILL:
                hft2ctrader_log(DEBUG) << prepend << "timeInForce: FILL_OR_KILL";
                break;

            case MARKET_ON_OPEN:
                hft2ctrader_log(DEBUG) << prepend << "timeInForce: MARKET_ON_OPEN";
                break;
        }
    }

    if (order.has_positionid())
    {
        hft2ctrader_log(DEBUG) << prepend << "positionId: " << order.positionid();
    }

    if (order.has_relativestoploss())
    {
        hft2ctrader_log(DEBUG) << prepend << "relativeStopLoss: " << order.relativestoploss();
    }

    if (order.has_relativetakeprofit())
    {
        hft2ctrader_log(DEBUG) << prepend << "relativeTakeProfit: " << order.relativetakeprofit();
    }

    if (order.has_isstopout())
    {
        hft2ctrader_log(DEBUG) << prepend << "isStopOut: " << (order.isstopout() ? "YES" : "NO");
    }

    if (order.has_trailingstoploss())
    {
        hft2ctrader_log(DEBUG) << prepend << "trailingStopLoss: " << (order.trailingstoploss() ? "YES" : "NO");
    }

    if (order.has_stoptriggermethod())
    {
        switch (order.stoptriggermethod())
        {
            case TRADE:
                // Stop Order: buy is triggered by ask, sell by bid;
                // Stop Loss Order: for buy position is triggered by bid and for sell position by ask.

                hft2ctrader_log(DEBUG) << prepend << "stopTriggerMethod: TRADE";

                break;

            case OPPOSITE:
                // Stop Order: buy is triggered by bid, sell by ask;
                // Stop Loss Order: for buy position is triggered by ask and for sell position by bid.

                hft2ctrader_log(DEBUG) << prepend << "stopTriggerMethod: OPPOSITE";

                break;

            case DOUBLE_TRADE:
                // The same as TRADE, but trigger is checked after the second consecutive tick.

                hft2ctrader_log(DEBUG) << prepend << "stopTriggerMethod: DOUBLE_TRADE";

                break;

            case DOUBLE_OPPOSITE:
                // The same as OPPOSITE, but trigger is checked after the second consecutive tick.

                hft2ctrader_log(DEBUG) << prepend << "stopTriggerMethod: DOUBLE_OPPOSITE";

                break;
        }
    }
}

void display_protooadeal(const ProtoOADeal &deal, const std::string &prepend)
{
/*
   Field                   Type                           Label        Description
--------------------------------------------------------------------------------------------------
dealId                     int64                         required    The unique ID of the execution deal.
                                                                     ---
orderId                    int64                         required    Source order of the deal.
                                                                     ---
positionId                 int64                         required    Source position of the deal.
                                                                     ---
volume                     int64                         required    Volume sent for execution, in cents.
                                                                     ---
filledVolume               int64                         required    Filled volume, in cents.
                                                                     ---
symbolId                   int64                         required    The unique identifier of the symbol in specific server
                                                                     environment within cTrader platform. Different servers
                                                                     have different IDs.
                                                                     ---
createTimestamp            int64                         required    Time when the deal was sent for execution.
                                                                     ---
executionTimestamp         int64                         required    Time when the deal was executed.
                                                                     ---
utcLastUpdateTimestamp     int64                         optional    Timestamp when the deal was created, executed or rejected.
                                                                     ---
executionPrice             double                        optional    Execution price.
                                                                     ---
tradeSide                  ProtoOATradeSide              required    Buy/Sell.
                                                                     ---
dealStatus                 ProtoOADealStatus             required    Status of the deal.
                                                                     ---
marginRate                 double	                     optional    Rate for used margin computation. Represented as Base/Deposit.
                                                                     ---
commission                 int64	                     optional    Amount of trading commission associated with the deal.
                                                                     ---
baseToUsdConversionRate    double	                     optional    Base to USD conversion rate on the time of deal execution.
                                                                     ---
closePositionDetail        ProtoOAClosePositionDetail    optional    Closing position detail. Valid only for closing deal.
                                                                     ---
moneyDigits                uint32	                     optional    Specifies the exponent of the monetary values. E.g. moneyDigits = 8
                                                                     must be interpret as business value multiplied by 10^8, then real
                                                                     balance would be 10053099944 / 10^8 = 100.53099944. Affects commission.
*/
    hft2ctrader_log(DEBUG) << prepend << "dealId: " << deal.dealid();
    hft2ctrader_log(DEBUG) << prepend << "orderId: " << deal.orderid();
    hft2ctrader_log(DEBUG) << prepend << "volume: " << deal.volume();
    hft2ctrader_log(DEBUG) << prepend << "filledVolume: " << deal.filledvolume();
    hft2ctrader_log(DEBUG) << prepend << "symbolId: " << deal.symbolid();
    hft2ctrader_log(DEBUG) << prepend << "createTimestamp: " << deal.createtimestamp();
    hft2ctrader_log(DEBUG) << prepend << "executionTimestamp: " << deal.executiontimestamp();

    if (deal.has_utclastupdatetimestamp())
    {
        hft2ctrader_log(DEBUG) << prepend << "utcLastUpdateTimestamp: " << deal.utclastupdatetimestamp();
    }

    if (deal.has_executionprice())
    {
        hft2ctrader_log(DEBUG) << prepend << "executionPrice: " << deal.executionprice();
    }

    switch (deal.tradeside())
    {
        case BUY:
            hft2ctrader_log(DEBUG) << prepend << "tradeSide: BUY";
            break;

        case SELL:
            hft2ctrader_log(DEBUG) << prepend << "tradeSide: SELL";
            break;
    }

    switch (deal.dealstatus())
    {
        case FILLED:
            // Deal filled.
            hft2ctrader_log(DEBUG) << prepend << "dealStatus: FILLED";
            break;

        case PARTIALLY_FILLED:
            // Deal is partially filled.
            hft2ctrader_log(DEBUG) << prepend << "dealStatus: PARTIALLY_FILLED";
            break;

        case REJECTED:
            // Deal is correct but was rejected by liquidity provider (e.g. no liquidity).
            hft2ctrader_log(DEBUG) << prepend << "dealStatus: REJECTED";
            break;

        case INTERNALLY_REJECTED:
            // Deal rejected by server (e.g. no price quotes).
            hft2ctrader_log(DEBUG) << prepend << "dealStatus: INTERNALLY_REJECTED";
            break;

        case ERROR:
            // Deal is rejected by LP due to error (e.g. symbol is unknown).
            hft2ctrader_log(DEBUG) << prepend << "dealStatus: ERROR";
            break;

        case MISSED:
            // Liquidity provider did not sent response on the deal during specified execution time period.
            hft2ctrader_log(DEBUG) << prepend << "dealStatus: MISSED";
            break;
    }

    if (deal.has_marginrate())
    {
        hft2ctrader_log(DEBUG) << prepend << "marginRate: " << deal.marginrate();
    }

    if (deal.has_commission())
    {
        hft2ctrader_log(DEBUG) << prepend << "commission: " << deal.commission();
    }

    if (deal.has_basetousdconversionrate())
    {
        hft2ctrader_log(DEBUG) << prepend << "baseToUsdConversionRate: " << deal.basetousdconversionrate();
    }

    if (deal.has_closepositiondetail())
    {
        hft2ctrader_log(DEBUG) << prepend << "closePositionDetail:";

        display_closepositiondetail(deal.closepositiondetail(), prepend + "\t");
    }

    if (deal.has_moneydigits())
    {
        hft2ctrader_log(DEBUG) << prepend << "moneyDigits: " << deal.moneydigits();
    }
}

void display_closepositiondetail(const ProtoOAClosePositionDetail &cpd, const std::string &prepend)
{
/*
Field                            Type      Label         Description
------------------------------------------------------------------------------
entryPrice                      double    required    Position price at the moment of filling the closing order.
                                                      ---
grossProfit                     int64     required    Amount of realized gross profit after closing deal execution.
                                                      ---
swap                            int64     required    Amount of realized swap related to closed volume.
                                                      ---
commission                      int64     required    Amount of realized commission related to closed volume.
                                                      ---
balance                         int64     required    Account balance after closing deal execution.
                                                      ---
quoteToDepositConversionRate    double    optional    Quote/Deposit currency conversion rate on the time
                                                      of closing deal execution.
                                                      ---
closedVolume                    int64     optional    Closed volume in cents.
                                                      ---
balanceVersion                  int64     optional    Balance version of the account related to closing deal operation.
                                                      ---
moneyDigits                     uint32    optional    Specifies the exponent of the monetary values. E.g. moneyDigits = 8
                                                      must be interpret as business value multiplied by 10^8, then real
                                                      balance would be 10053099944 / 10^8 = 100.53099944. Affects
                                                      grossProfit, swap, commission, balance, pnlConversionFee.
                                                      ---
pnlConversionFee                int64     optional    Fee for conversion applied to the Deal in account's ccy when trader
                                                      symbol's quote asset id <> ProtoOATrader.depositAssetId.
*/

    hft2ctrader_log(DEBUG) << prepend << "entryPrice: " << cpd.entryprice();
    hft2ctrader_log(DEBUG) << prepend << "grossProfit: " << cpd.grossprofit();
    hft2ctrader_log(DEBUG) << prepend << "swap: " << cpd.swap();
    hft2ctrader_log(DEBUG) << prepend << "commission: " << cpd.commission();
    hft2ctrader_log(DEBUG) << prepend << "balance: " << cpd.balance();

    if (cpd.has_quotetodepositconversionrate())
    {
        hft2ctrader_log(DEBUG) << prepend << "quoteToDepositConversionRate: "
                               << cpd.quotetodepositconversionrate();
    }

    if (cpd.has_closedvolume())
    {
        hft2ctrader_log(DEBUG) << prepend << "closedVolume: " << cpd.closedvolume();
    }

    if (cpd.has_balanceversion())
    {
        hft2ctrader_log(DEBUG) << prepend << "balanceVersion: " << cpd.balanceversion();
    }

    if (cpd.has_moneydigits())
    {
        hft2ctrader_log(DEBUG) << prepend << "moneyDigits: " << cpd.moneydigits();
    }

    if (cpd.has_pnlconversionfee())
    {
        hft2ctrader_log(DEBUG) << prepend << "pnlConversionFee: " << cpd.pnlconversionfee();
    }
}

#endif /* ORDER_EXECUTION_DEBUG */

} // namespace
