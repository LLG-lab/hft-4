/**********************************************************************\
**                                                                    **
**             -=≡≣ High Frequency Trading System  ≣≡=-              **
**                                                                    **
**          Copyright  2017 - 2021 by LLG Ryszard Gradowski          **
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

#ifndef __HFT_DUKASCOPY_EMULATOR_HPP__
#define __HFT_DUKASCOPY_EMULATOR_HPP__

#include <map>
#include <limits>

#include <hft_server_connector.hpp>
#include <csv_data_supplier.hpp>
#include <hft_instrument_property.hpp>
#include <hft_response.hpp>
#include <utilities.hpp>

#include <boost/date_time/posix_time/posix_time.hpp>

class hft_dukascopy_emulator : private boost::noncopyable
{
public:

    //
    // ASACP - Account State After Close Position.
    //

    struct asacp
    {
        hft::protocol::response::position_direction direction;
        int pips_yield;
        int qty;
        boost::posix_time::ptime open_time;
        boost::posix_time::ptime close_time;
        double equity;
        double total_swaps;
        bool closed_forcibly;
        int still_opened;
    };

    typedef std::list<asacp> emulation_result;

    hft_dukascopy_emulator(void) = delete;

    hft_dukascopy_emulator(const std::string &host, const std::string &port,
                               const std::string &instrument, const std::string &sessid,
                                   double deposit, const std::string &csv_file_name,
                                       const std::string &config_file_name,
                                           bool check_bankruptcy)
        : hft_connection_(host, port, instrument, sessid),
          csv_faucet_(csv_file_name),
          instrument_property_(instrument, config_file_name),
          instrument_(instrument),
          equity_(deposit),
          check_bankruptcy_(check_bankruptcy),
          min_equity_(std::numeric_limits<double>::max()),
          max_equity_(std::numeric_limits<double>::min())
    { proceed(); }

    const emulation_result &get_result(void) const { return emulation_result_; }

    double get_min_equity(void) const { return min_equity_; }
    double get_max_equity(void) const { return max_equity_; }

private:

    struct opened_position
    {
        hft::protocol::response::position_direction direction;
        std::string id;
        int open_price_pips;
        int qty;
        boost::posix_time::ptime open_time;
    };

    typedef std::map<std::string, opened_position> opened_positions;

    void proceed(void);

    void handle_close_position(const std::string &id, const csv_data_supplier::csv_record &tick_info, bool is_forcibly = false);

    void handle_open_position(const hft::protocol::response::open_position_info &opi,
                                  const csv_data_supplier::csv_record &tick_info);

    double get_equity_at_moment(const csv_data_supplier::csv_record &tick_info) const;

    static int days_elapsed(boost::posix_time::ptime begin, boost::posix_time::ptime end)
    {
        return (end.date() - begin.date()).days();
    }

    int floating2pips(double price) const
    {
        return hft::utils::floating2pips(price, instrument_property_.get_pip_significant_digit());
    }

    emulation_result emulation_result_;
    opened_positions positions_;
    hft_server_connector hft_connection_;
    csv_data_supplier csv_faucet_;
    hft_instrument_property instrument_property_;
    const std::string instrument_;
    double equity_;
    bool check_bankruptcy_;
    double min_equity_;
    double max_equity_;
};

#endif /* __HFT_DUKASCOPY_EMULATOR_HPP__ */
