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

#ifndef __HFT_DUKASCOPY_EMULATOR_HPP__
#define __HFT_DUKASCOPY_EMULATOR_HPP__

#include <map>
#include <limits>
#include <memory>

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
        std::string instrument;
        hft::protocol::response::position_direction direction;
        int pips_yield;
        double qty;
        boost::posix_time::ptime open_time;
        boost::posix_time::ptime close_time;
        double equity;
        double total_swaps;
        double money_yield;
        bool closed_forcibly;
        int still_opened;
    };

    struct emulation_result
    {
        emulation_result(void)
            : min_equity(std::numeric_limits<double>::max()),
              max_equity(std::numeric_limits<double>::min()),
              bankrupt(false)
        {}

        std::list<asacp> trades;
        double min_equity;
        double max_equity;
        bool bankrupt;
    };

    hft_dukascopy_emulator(void) = delete;

    hft_dukascopy_emulator(const std::string &host, const std::string &port, const std::string &sessid,
                               const std::map<std::string, std::string> &instrument_data, double deposit,
                                    const std::string &config_file_name, bool check_bankruptcy, bool invert_hft_decision);

    const emulation_result &get_result(void) const { return emulation_result_; }

private:

    struct opened_position
    {
        std::string instrument;
        hft::protocol::response::position_direction direction;
        std::string id;
        int open_price_pips;
        double qty;
        boost::posix_time::ptime open_time;
    };

    struct position_status
    {
        boost::posix_time::ptime moment;
        double total_swaps;
        int pips_yield;
        double money_yield;
    };

    typedef std::map<std::string, opened_position> opened_positions;

    struct tick_record : public csv_data_supplier::csv_record
    {
        tick_record &operator=(const csv_data_supplier::csv_record &r)
        {
            instrument = "";
            static_cast<csv_data_supplier::csv_record &>(*this) = r;

            return *this;
        }

        std::string instrument;
    };

    struct instrument_data_info
    {
        enum class data_state
        {
            DS_EMPTY,
            DS_LOADED,
            DS_EOF
        };

        instrument_data_info(void) = delete;
        instrument_data_info(const std::string &instr, const std::string &csv_file, const std::string &config_file_name)
            : instrument(instr), csv_faucet(csv_file), property(instr, config_file_name), state(data_state::DS_EMPTY)
        {}

        std::string instrument;
        csv_data_supplier csv_faucet;
        hft_instrument_property property;

        data_state state;
        csv_data_supplier::csv_record loaded;
        csv_data_supplier::csv_record official;
    };

    typedef std::map<std::string, std::shared_ptr<instrument_data_info>> instruments_info;

    void proceed(void);

    void close_worst_losing_position(void);

    bool get_record(tick_record &tick);

    void handle_response(const tick_record &tick_info, const hft::protocol::response &reply);

    void handle_close_position(const std::string &id, const tick_record &tick_info, bool is_forcibly = false);

    void handle_open_position(const hft::protocol::response::open_position_info &opi, const tick_record &tick_info);

    position_status get_position_status_at_moment(const opened_position &pos, const tick_record &tick_info);

    double get_equity_at_moment(void) const;

    double get_used_margin_at_moment(void) const;

    double get_free_margin_at_moment(double equity_at_moment) const;

    double get_margin_level_at_moment(double equity_at_moment) const;

    static int days_elapsed(boost::posix_time::ptime begin, boost::posix_time::ptime end)
    {
        return (end.date() - begin.date()).days();
    }

    int floating2pips(const std::string &instrument, double price) const
    {
        return hft::utils::floating2pips(price, instruments_.at(instrument) -> property.get_pip_significant_digit());
    }

    emulation_result emulation_result_;
    opened_positions positions_;
    hft_server_connector hft_connection_;

    double balance_;
    bool check_bankruptcy_;
    bool invert_hft_decision_;
    bool forbid_new_positions_;

    instruments_info instruments_;
};

#endif /* __HFT_DUKASCOPY_EMULATOR_HPP__ */
