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

#ifndef __HISTORICAL_DATA_FEED_HPP__
#define __HISTORICAL_DATA_FEED_HPP__

#include <hft2ctrader_config.hpp>
#include <ctrader_ssl_connection.hpp>
#include <ctrader_api.hpp>

class historical_data_feed
{
public:

    historical_data_feed(void) = delete;

    historical_data_feed(historical_data_feed &) = delete;

    historical_data_feed(historical_data_feed &&) = delete;

    historical_data_feed(boost::asio::io_context &ioctx, const hft2ctrader_config &cfg);

    ~historical_data_feed(void) = default;

    bool completed(void) const { return feed_completed_; }

private:

    enum class stage
    {
        S_IDLE,
        S_WAIT_AUTH_APP,
        S_WAIT_AUTH_ACC,
        S_WAIT_INSTRUMENT_ID,
        S_WAIT_INSTRUMENT_INFO,
        S_OPERATIONAL
    };

    struct chunk_info
    {
        chunk_info(void)
            : from {0ul}, to {0ul} {}

        unsigned long from;
        unsigned long to;
    };

    struct download_info
    {
        download_info(void)
            : index {0}, payload_type { quote_type::ASK } {}

        size_t index;
        quote_type payload_type;
        std::vector<chunk_info> chunks_info;
    };

    struct download_data
    {
        download_data(unsigned long t, int p, quote_type q)
            : timestamp {t}, price {p}, quote {q} {}

        bool operator<(const download_data& r)
        {
            return (timestamp < r.timestamp);
        }

        unsigned long timestamp;
        int price;
        quote_type quote;
    };

    bool is_app_authorized(const std::vector<char> &data);
    bool is_account_authorized(const std::vector<char> &data);
    bool has_instrument_id(const std::vector<char> &data);
    bool has_instrument_info(const std::vector<char> &data);
    bool acquire_history(void);
    void process_history(const std::vector<char> &data);
    void create_csv(void);

    static chunk_info make_chunk_info(unsigned long week, unsigned long num_2h_interval);
    static std::string make_csv_entry_str(unsigned long timestamp, int ask, int bid);
    static std::string timestamp2string(unsigned long millis);

    bool feed_completed_;
    int instrument_id_;
    stage stage_;
    const hft2ctrader_config &config_;
    ctrader_ssl_connection broker_connection_;
    ctrader_api api_;
    download_info di_;
    std::vector<download_data> data_;
};

#endif /* __HISTORICAL_DATA_FEED_HPP__ */
