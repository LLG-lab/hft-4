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

#ifndef __INSTRUMENT_HANDLER_HPP__
#define __INSTRUMENT_HANDLER_HPP__

#include <hft_request.hpp>
#include <hft_response.hpp>
#include <hft_handler_resource.hpp>

#include <trade_time_frame.hpp>

//
// Instrument handler base class.
//

class instrument_handler
{
public:

    struct init_info
    {
        std::string ticker;
        std::string description;
        std::string ticker_fmt2;
        std::string work_dir;
        int pips_digit;
        trade_time_frame ttf;
    };

    instrument_handler(const init_info &general_config)
        : handler_informations_(general_config),
          // hs_(get_work_dir() + "/handler.state", get_logger_id()) XXX O dziwo handler insformations_ jest inicjalizowany po hs_, mimo że na liście inicjalizacyjnej figuruje jako pierwszy
          hs_(general_config.work_dir + "/handler.state", std::string("handler_") + general_config.ticker_fmt2)
    {}

    instrument_handler(void) = delete;

    virtual ~instrument_handler(void) = default;

    virtual void init_handler(const boost::json::object &specific_config) = 0;

    //
    // Main event interface.
    // Methods to be implemented
    // in derived classes.
    //

    virtual void on_sync(const hft::protocol::request::sync &msg, hft::protocol::response &market) = 0;
    virtual void on_tick(const hft::protocol::request::tick &msg, hft::protocol::response &market) = 0;
    virtual void on_position_open(const hft::protocol::request::open_notify &msg, hft::protocol::response &market) = 0;
    virtual void on_position_close(const hft::protocol::request::close_notify &msg, hft::protocol::response &market) = 0;

protected:

    //
    // Access methods to handler informations.
    //

    std::string get_ticker(void) const { return handler_informations_.ticker; }
    std::string get_ticker_fmt2(void) const { return handler_informations_.ticker_fmt2; }
    std::string get_work_dir(void) const { return handler_informations_.work_dir; }
    std::string get_logger_id(void) const { return std::string("handler_") + get_ticker_fmt2(); }
    std::string get_instrument_description(void) const { return handler_informations_.description; }
    int floating2pips(double price) const;
    static std::string uid(void);
    bool can_play(const boost::posix_time::ptime &current_time_point) const { return handler_informations_.ttf.can_play(current_time_point); }

    //
    // Functions to help retrieve parameters from a JSON object.
    //

    static bool json_exist_attribute(const boost::json::object &obj, const std::string &attr_name);
    static bool json_get_bool_attribute(const boost::json::object &obj, const std::string &attr_name);
    static double json_get_double_attribute(const boost::json::object &obj, const std::string &attr_name);
    static int json_get_int_attribute(const boost::json::object &obj, const std::string &attr_name);
    static std::string json_get_string_attribute(const boost::json::object &obj, const std::string &attr_name);
    static const boost::json::object &json_get_object_attribute(const boost::json::object &obj, const std::string &attr_name);

    //
    // Extra.
    //

    void sms_alert(const std::string &message);

    //
    //  Handler state keeper.
    //

    hft_handler_resource hs_;

private:

    init_info handler_informations_;
};

typedef instrument_handler *instrument_handler_ptr;

//
// Instrument handler factory function.
//

instrument_handler_ptr create_instrument_handler(const std::string &session_dir, const std::string &instrument);

#endif /*  __INSTRUMENT_HANDLER_HPP__ */
