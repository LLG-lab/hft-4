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

#include <ctrader_session.hpp>
#include <aux_functions.hpp>

#define hft2ctrader_log(__X__) \
    CLOG(__X__, "session")

namespace {

std::string payload_type2str(uint payload_type);

} // namespace.

ctrader_session::ctrader_session(ctrader_ssl_connection &connection, const hft2ctrader_bridge_config &config)
    : ctrader_api(connection), config_(config),
      last_heartbeat_(0ul)
{
   el::Loggers::getLogger("session", true);
}

//
// Transition action methods and guards.
//

void ctrader_session::start_app_authorization(new_connection_event const &event)
{
    hft2ctrader_log(INFO) << "Start application authorization.";

    authorize_application(config_.get_auth_client_id(), config_.get_auth_client_secret());
}

void ctrader_session::start_account_authorization(data_event const &event)
{
    hft2ctrader_log(INFO) << "Start account authorization.";

    authorize_account(config_.get_auth_access_token(), config_.get_auth_account_id());
}

bool ctrader_session::is_app_authorized(data_event const &event)
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
            ProtoErrorRes res;
            res.ParseFromString(payload);

            hft2ctrader_log(ERROR) << "Application authorization ERROR ("
                                   << res.errorcode() << ") – "
                                   << res.description();

            throw std::runtime_error("Application authorization failed");
        }
        default:
            hft2ctrader_log(WARNING) << "is_app_authorized: Received unhandled message from server #"
                                     << payload_type;
    }

    return false;
}

void ctrader_session::start_acquire_account_informations(data_event const &event)
{
    hft2ctrader_log(INFO) << "Start acquire account informations.";

    account_information(config_.get_auth_account_id());
}

bool ctrader_session::is_account_authorized(data_event const &event)
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
            ProtoErrorRes res;
            res.ParseFromString(payload);

            hft2ctrader_log(ERROR) << "Account authorization ERROR ("
                                   << res.errorcode() << ") – "
                                   << res.description();

            throw std::runtime_error("Account authorization failed");
        }
        default:
            hft2ctrader_log(WARNING) << "is_account_authorized: Received unhandled message from server #"
                                     << payload_type;
    }

    return false;
}

void ctrader_session::start_acquire_instrument_informations(data_event const &event)
{
    hft2ctrader_log(INFO) << "Start acquire available instrument informations.";

    available_instruments(config_.get_auth_account_id());
}

bool ctrader_session::has_account_informations(data_event const &event)
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

            hft2ctrader_log(INFO) << "Account information status:";

            if (res.trader().has_balance())
            {
                account_balance_ = (double)(res.trader().balance()) / 100.0;

                hft2ctrader_log(INFO) << "    Balance: "
                                      << account_balance_;
            }

            if (res.trader().has_balanceversion())
            {
                hft2ctrader_log(INFO) << "    Balance version: "
                                      << res.trader().balanceversion();
            }

            if (res.trader().has_leverageincents())
            {
                int leverage = res.trader().leverageincents() / 100;

                hft2ctrader_log(INFO) << "    Leverage: 1:"
                                      << leverage;
            }

            if (res.trader().has_brokername())
            {
                hft2ctrader_log(INFO) << "    Broker: "
                                      << res.trader().brokername();
            }

            return true;
        }
        case PROTO_OA_ERROR_RES:
        {
            ProtoErrorRes res;
            res.ParseFromString(payload);

            hft2ctrader_log(ERROR) << "Account information acquisition ERROR ("
                                   << res.errorcode() << ") – "
                                   << res.description();

            throw std::runtime_error("Account information acquisition failed");
        }
        default:
            hft2ctrader_log(WARNING) << "has_account_informations: Received unhandled message from server #"
                                     << payload_type;

    }

    return false;
}

void ctrader_session::initialize_bridge(data_event const &event)
{
    on_init();
}

bool ctrader_session::has_instrument_informations(data_event const &event)
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
            ProtoErrorRes res;
            res.ParseFromString(payload);

            hft2ctrader_log(ERROR) << "Acquisition of available instruments information ERROR ("
                                   << res.errorcode() << ") – "
                                   << res.description();

            throw std::runtime_error("Acquisition of available instruments information failed");
        }
        default:
            hft2ctrader_log(WARNING) << "has_instrument_informations: Received unhandled message from server #"
                                     << payload_type;
    }

    return false;
}

void ctrader_session::dispatch_event(data_event const &event)
{
    ProtoMessage msg;

    msg.ParseFromArray(&event.data_.front(), event.data_.size());
    uint payload_type   = msg.payloadtype();
    std::string payload = msg.payload();

    auto now = aux::get_current_timestamp();

    if (now - last_heartbeat_ >= 9000)
    {
        hft2ctrader_log(TRACE) << "Raised heart beat";

        heart_beat_ACK();

        last_heartbeat_ = now;
    }

    switch (payload_type)
    {
        case HEARTBEAT_EVENT:
        {
            hft2ctrader_log(TRACE) << "Heart beat event";

            heart_beat_ACK();

            last_heartbeat_ = now;

            break;
        }
        case PROTO_OA_SUBSCRIBE_SPOTS_RES:
        {
            hft2ctrader_log(INFO) << "Subscribe instruments SUCCESS.";

            break;
        }
        case PROTO_OA_TRADER_UPDATE_EVENT:
        {
            ProtoOATraderUpdatedEvent evt;
            evt.ParseFromString(payload);

            account_balance_ = (double)(evt.trader().balance()) / 100.0;

            hft2ctrader_log(INFO) << "Account balance: " << account_balance_;

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

            switch (evt.executiontype())
            {
                case ORDER_ACCEPTED:
                case ORDER_FILLED:
                case ORDER_REPLACED:
                case ORDER_CANCELLED:
                case ORDER_EXPIRED:
                case ORDER_REJECTED:
                case ORDER_CANCEL_REJECTED:
                case ORDER_PARTIAL_FILL:
                         on_order_execution_event(evt);
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
            ProtoErrorRes res;
            res.ParseFromString(payload);

            hft2ctrader_log(ERROR) << "Response ERROR ("
                                   << res.errorcode() << ") – "
                                   << res.description();

            // FIXME: W zależności od kodu błedu, można potem wołać jakieś handlery ewentualnie.

            break;
        }
        default:
            hft2ctrader_log(WARNING) << "dispatch_event: Received unhandled message from server #"
                                     << payload_type << " – " << payload_type2str(payload_type);
    }
}

//
// Utility routines for market_session.
//

void ctrader_session::subscribe_instruments_ex(const instruments_container &instruments)
{
    instrument_id_container aux;

    for (auto &instr : instruments)
    {
        auto x = ticker2id_.find(instr);

        if (x == ticker2id_.end())
        {
            hft2ctrader_log(ERROR) << "subscribe_instruments_ex: Unsupported instrument ‘"
                                   << instr << "’";
        }
        else
        {
            aux.push_back(x -> second);
        }
    }

    subscribe_instruments(aux, config_.get_auth_account_id());
}

void ctrader_session::create_market_order_ex(const std::string &position_id, const std::string &instrument, position_type pt, int volume)
{
    int instrument_id = 0;
    auto x = ticker2id_.find(instrument);

    if (x == ticker2id_.end())
    {
        hft2ctrader_log(ERROR) << "create_market_order_ex: Unsupported instrument ‘"
                               << instrument << "’";
    }
    else
    {
        instrument_id = x -> second;
    }

    create_market_order(position_id, instrument_id, pt, volume, config_.get_auth_account_id());
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

} // namespace
