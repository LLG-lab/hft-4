/**********************************************************************\
**                                                                    **
**             -=≡≣ High Frequency Trading System ® ≣≡=-              **
**                                                                    **
**          Copyright © 2017 - 2023 by LLG Ryszard Gradowski          **
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

#include <sstream>
#include <fstream>
#include <csignal>
#include <ctime>
#include <algorithm>
#include <chrono>
#include <thread>

#include <easylogging++.h>

#include <aux_functions.hpp>
#include <historical_data_feed.hpp>

#define hft2ctrader_log(__X__) \
    CLOG(__X__, "historical_data_feed")

historical_data_feed::historical_data_feed(boost::asio::io_context &ioctx, const hft2ctrader_config &cfg)
    : feed_completed_ {false}, instrument_id_ {0}, stage_ {stage::S_IDLE}, config_ {cfg},
      broker_connection_ {ioctx, cfg}, api_ {broker_connection_}
{
    broker_connection_.set_on_connected_callback([this](void)
        {
            api_.ctrader_authorize_application(config_.get_auth_client_id(), config_.get_auth_client_secret());
            stage_ = stage::S_WAIT_AUTH_APP;
        });

    broker_connection_.set_on_error_callback([this](const boost::system::error_code &ec)
        {
            throw std::runtime_error("Connection error.");
        });

    broker_connection_.set_on_data_callback([this](const std::vector<char> &data)
        {
            switch (stage_)
            {
                case stage::S_WAIT_AUTH_APP:
                    if (is_app_authorized(data))
                    {
                        api_.ctrader_authorize_account(config_.get_auth_access_token(), config_.get_auth_account_id());
                        stage_ = stage::S_WAIT_AUTH_ACC;
                    }
                    else
                    {
                        throw std::runtime_error("Application authorization failed");
                    }

                    break;
                case stage::S_WAIT_AUTH_ACC:
                    if (is_account_authorized(data))
                    {
                        api_.ctrader_available_instruments(config_.get_auth_account_id());
                        stage_ = stage::S_WAIT_INSTRUMENT_ID;
                    }
                    else
                    {
                        throw std::runtime_error("Account authorization failed");
                    }

                    break;
                case stage::S_WAIT_INSTRUMENT_ID:
                    if (has_instrument_id(data))
                    {
                        instrument_id_container id;
                        id.push_back(instrument_id_);
                        api_.ctrader_instruments_information(id, config_.get_auth_account_id());
                        stage_ = stage::S_WAIT_INSTRUMENT_INFO;
                    }
                    else
                    {
                        throw std::runtime_error("Specified instrument is unknown or unavailable");
                    }

                    break;
                case stage::S_WAIT_INSTRUMENT_INFO:
                    if (has_instrument_info(data))
                    {
                        acquire_history();
                        stage_ = stage::S_OPERATIONAL;
                    }
                    else
                    {
                        throw std::runtime_error("Failed to obtain instrument informations");
                    }

                case stage::S_OPERATIONAL:
                    process_history(data);
                    break;
            }
        });

    //
    // Create chunks for requested week number.
    //

    const int max_num_intervals = config_.is_crypto_mode() ? 84 : 60;

    for (int num_intervals = 1; num_intervals <= max_num_intervals; num_intervals++)
    {
        di_.chunks_info.push_back(make_chunk_info(config_.get_week_number(), num_intervals));
    }

    broker_connection_.connect();
}

bool historical_data_feed::is_app_authorized(const std::vector<char> &data)
{
    ProtoMessage msg;

    msg.ParseFromArray(&data.front(), data.size());
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

            break;
        }
        default:
            hft2ctrader_log(WARNING) << "is_app_authorized: Received unhandled message from server #"
                                     << payload_type;
    }

    return false;
}

bool historical_data_feed::is_account_authorized(const std::vector<char> &data)
{
    ProtoMessage msg;

    msg.ParseFromArray(&data.front(), data.size());
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

            break;
        }
        default:
            hft2ctrader_log(WARNING) << "is_account_authorized: Received unhandled message from server #"
                                     << payload_type;
    }

    return false;
}

bool historical_data_feed::has_instrument_id(const std::vector<char> &data)
{
    ProtoMessage msg;

    msg.ParseFromArray(&data.front(), data.size());
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
                    if (res.symbol(i).symbolname() == config_.get_instrument())
                    {
                        hft2ctrader_log(TRACE) << "Found instrument. ID: " << res.symbol(i).symbolid() << ", "
                                               << "Name: " << res.symbol(i).symbolname() << ", "
                                               << "description: " << res.symbol(i).description();

                        instrument_id_ = res.symbol(i).symbolid();

                        return true;
                    }
                }

            }

            hft2ctrader_log(ERROR) << "Cannot find ID for requested instrument ‘"
                                   << config_.get_instrument() << "’";

            break;
        }
        case PROTO_OA_ERROR_RES:
        {
            hft2ctrader_log(ERROR) << "Acquisition of available instruments information ERROR:\n"
                                   << aux::hexdump(payload);

            break;
        }
        default:
            hft2ctrader_log(WARNING) << "has_instrument_id: Received unhandled message from server #"
                                     << payload_type;
    }

    return false;
}

bool historical_data_feed::has_instrument_info(const std::vector<char> &data)
{
    ProtoMessage msg;

    msg.ParseFromArray(&data.front(), data.size());
    uint payload_type   = msg.payloadtype();
    std::string payload = msg.payload();

    switch (payload_type)
    {
        case PROTO_OA_SYMBOL_BY_ID_RES:
        {
            ProtoOASymbolByIdRes res;
            res.ParseFromString(payload);

            hft2ctrader_log(INFO) << "Detailed informations about ‘" << config_.get_instrument() << "’:";
            hft2ctrader_log(INFO) << "  pipPosition: " << res.symbol(0).pipposition();
            hft2ctrader_log(INFO) << "  swapLong: " << res.symbol(0).swaplong();
            hft2ctrader_log(INFO) << "  swapShort: " << res.symbol(0).swapshort();

            std::string commission_type;

            switch (res.symbol(0).commissiontype())
            {
                case USD_PER_MILLION_USD:
                    // USD per million USD volume - usually used for FX.
                    // Example: 50 USD for 1 mil USD of trading volume.
                    commission_type = "USD_PER_MILLION_USD";
                    break;
                case USD_PER_LOT:
                    // USD per 1 lot - usually used for CFDs and futures
                    // for commodities, and indices. Example: 15 USD
                    // for 1 contract.
                    commission_type = "USD_PER_LOT";
                    break;
                case PERCENTAGE_OF_VALUE:
                    // Percentage of trading volume - usually used for
                    // Equities. Example: 0.005% of notional trading
                    // volume. Multiplied by 100,000.
                    commission_type = "PERCENTAGE_OF_VALUE";
                    break;
                case QUOTE_CCY_PER_LOT:
                    // Quote ccy of Symbol per 1 lot - will be used for
                    // CFDs and futures for commodities, and indices.
                    // Example: 15 EUR for 1 contract of DAX.
                    commission_type = "QUOTE_CCY_PER_LOT";
                    break;
            }

            hft2ctrader_log(INFO) << "  commission: " << res.symbol(0).precisetradingcommissionrate()
                                  << " " << commission_type;

            return true;
        }
        case PROTO_OA_ERROR_RES:
        {
            hft2ctrader_log(ERROR) << "Acquisition of detailed instrument information ERROR:\n"
                                   << aux::hexdump(payload);

            break;
        }
        default:
            hft2ctrader_log(WARNING) << "has_instrument_info: Received unhandled message from server #"
                                     << payload_type;
    }

    return false;
}

bool historical_data_feed::acquire_history(void)
{
    if (di_.index >= di_.chunks_info.size())
    {
        return false;
    }

    std::string payload_type_str;

    if (di_.payload_type == quote_type::ASK)
    {
        payload_type_str = "ASK";
    }
    else if (di_.payload_type == quote_type::BID)
    {
        payload_type_str = "BID";
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(250));

    hft2ctrader_log(INFO) << "Downloading chunk " << (di_.index + 1)
                          << " of " << di_.chunks_info.size()
                          << ", type " << payload_type_str;

    api_.ctrader_historical_tick_data(instrument_id_,
                                      di_.payload_type,
                                      di_.chunks_info[di_.index].from,
                                      di_.chunks_info[di_.index].to,
                                      config_.get_auth_account_id());

    if (di_.payload_type == quote_type::ASK)
    {
        di_.payload_type = quote_type::BID;
    }
    else if (di_.payload_type == quote_type::BID)
    {
        di_.payload_type = quote_type::ASK;
        di_.index++;
    }

    return true;
}

void historical_data_feed::process_history(const std::vector<char> &data)
{
    ProtoMessage msg;

    msg.ParseFromArray(&data.front(), data.size());
    uint payload_type   = msg.payloadtype();
    std::string payload = msg.payload();

    switch (payload_type)
    {
        case PROTO_OA_GET_TICKDATA_RES:
        {
            ProtoOAGetTickDataRes res;
            res.ParseFromString(payload);

            unsigned long timestamp;
            int price;
            quote_type quote;

            if (res.hasmore())
            {
                std::runtime_error("Chunk incompleted.");
            }

            if (di_.payload_type == quote_type::ASK)
            {
                quote = quote_type::BID;
            }
            else if (di_.payload_type == quote_type::BID)
            {
                quote = quote_type::ASK;
            }

            for (int i = 0; i < res.tickdata_size(); i++)
            {
                if (i == 0)
                {
                    timestamp = res.tickdata(0).timestamp();
                    price     = res.tickdata(0).tick();
                }
                else
                {
                    timestamp += res.tickdata(i).timestamp();
                    price     += res.tickdata(i).tick();
                }

                data_.emplace_back(timestamp, price, quote);
            }

            if (! acquire_history())
            {
               // XXX Wywala błąd.
               // broker_connection_.close();
                create_csv();
                feed_completed_ = true;
                std::raise(SIGTERM);
            }

            break;
        }
        case PROTO_OA_ERROR_RES:
        {
            hft2ctrader_log(ERROR) << "Data ERROR:\n"
                                   << aux::hexdump(payload);

            throw std::runtime_error("Data ERROR");

            break;
        }
        case HEARTBEAT_EVENT:
            api_.ctrader_heart_beat();
            break;
        default:
            hft2ctrader_log(WARNING) << "process_history: Received unhandled message from server #"
                                     << payload_type;
    }
}

void historical_data_feed::create_csv(void)
{
    std::sort(data_.begin(), data_.end());

    std::string file_name = config_.get_instrument() + "_WEEK"
                            + std::to_string(config_.get_week_number())
                            + std::string(".csv");

    hft2ctrader_log(INFO) << "Writing to " << file_name << "...";

    std::ofstream output;
    output.open(file_name.c_str(), std::ofstream::out);

    if (! output.is_open())
    {
        std::string err_msg = "Cannot open file: " + file_name;

        throw std::runtime_error(err_msg);
    }

    //
    // Write header line.
    //

    output << "Gmt time,Ask,Bid,AskVolume,BidVolume\r\n";

    int ask = 0;
    int bid = 0;
    bool valid_ask = false;
    bool valid_bid = false;
    unsigned long timestamp = 0ul;

    for (auto &entry : data_)
    {
        if (timestamp == entry.timestamp)
        {
            if (entry.quote == quote_type::ASK)
            {
                ask = entry.price;
                valid_ask = true;
            }
            else if (entry.quote == quote_type::BID)
            {
                bid = entry.price;
                valid_bid = true;
            }
        }
        else
        {
            if (valid_ask && valid_bid && (ask > bid))
            {
                output << make_csv_entry_str(timestamp, ask, bid) << "\r\n";
            }

            if (entry.quote == quote_type::ASK)
            {
                ask = entry.price;
                valid_ask = true;
            }
            else if (entry.quote == quote_type::BID)
            {
                bid = entry.price;
                valid_bid = true;
            }

            timestamp = entry.timestamp;
        }
    }

    output << make_csv_entry_str(timestamp, ask, bid) << "\r\n";

    output.close();
}

historical_data_feed::chunk_info historical_data_feed::make_chunk_info(unsigned long week, unsigned long num_2h_interval)
{
    chunk_info result;

    static boost::posix_time::ptime begin_epoch      = boost::posix_time::ptime(boost::posix_time::time_from_string("1970-01-01 00:00:00.000"));
    static boost::posix_time::ptime begin_first_week = boost::posix_time::ptime(boost::posix_time::time_from_string("2015-01-05 00:00:00.000"));

    result.from = (begin_first_week - begin_epoch).total_milliseconds() + (week-1)*3600*24*7*1000 + (num_2h_interval-1)*3600*2*1000;
    result.to   = (begin_first_week - begin_epoch).total_milliseconds() + (week-1)*3600*24*7*1000 + (num_2h_interval)*3600*2*1000 - 1;

    return result;
}

std::string historical_data_feed::make_csv_entry_str(unsigned long timestamp, int ask, int bid)
{
    //
    // Example entry:
    // 18.11.2023 00:00:00.119,1.03613,1.03606,3.68,4.04
    //

    return timestamp2string(timestamp) + std::string(",")
           + std::to_string(static_cast<double>(ask)/100000.0)
           + std::string(",")
           + std::to_string(static_cast<double>(bid)/100000.0)
           + std::string(",0.00,0.00");
}

std::string historical_data_feed::timestamp2string(unsigned long millis)
{
    int fraction = millis % 1000;
    time_t rawtime = millis / 1000;
    struct tm  ts;
    static char buf[20];

    ts = *gmtime(&rawtime);
    strftime(buf, sizeof(buf), "%d.%m.%Y %H:%M:%S", &ts);

    std::string result = std::string(buf) + std::string(".");

    if (fraction < 100)
    {
        result += "0";
    }

    if (fraction < 10)
    {
        result += "0";
    }

    result += std::to_string(fraction);

    return result;
}
