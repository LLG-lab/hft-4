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

#ifndef __CTRADER_SESSION_HPP__
#define __CTRADER_SESSION_HPP__

#include <map>
#include <boost/msm/front/state_machine_def.hpp>

#include <ctrader_api.hpp>
#include <hft2ctrader_bridge_config.hpp>

namespace msm = boost::msm;
namespace mpl = boost::mpl;

class ctrader_session  : public msm::front::state_machine_def<ctrader_session>, public ctrader_api
{
public:

    ctrader_session(void) = delete;

    ctrader_session(ctrader_session &) = delete;

    ctrader_session(ctrader_session &&) = delete;

    ctrader_session(ctrader_ssl_connection &connection, const hft2ctrader_bridge_config &config);

    virtual ~ctrader_session(void) = default;

    //
    // Events.
    //

    struct new_connection_event {};

    struct error_event
    {
        error_event(void) = delete;
        error_event(const boost::system::error_code &e)
            : error_(e) {}

        const boost::system::error_code &error_;
    };

    struct data_event
    {
        data_event(void) = delete;
        data_event(const std::vector<char> &d)
            : data_(d) {}

        const std::vector<char> &data_;
    };

    //
    // List of FSM states.
    //

    struct wait_for_connect : public msm::front::state<> {};
    struct app_authorization : public msm::front::state<> {};
    struct account_authorization : public msm::front::state<> {};
    struct account_informations : public msm::front::state<> {};
    struct instrument_informations : public msm::front::state<> {};
    struct operational : public msm::front::state<> {};

    typedef wait_for_connect initial_state;

    //
    // Transition action methods and guards.
    //

    void start_app_authorization(new_connection_event const &event);
    void start_account_authorization(data_event const &event);
    bool is_app_authorized(data_event const &event);
    void start_acquire_account_informations(data_event const &event);
    bool is_account_authorized(data_event const &event);
    void start_acquire_instrument_informations(data_event const &event);
    bool has_account_informations(data_event const &event);
    void initialize_bridge(data_event const &event); // ma wywołać on_init
    bool has_instrument_informations(data_event const &event);
    void dispatch_event(data_event const &event);

    typedef ctrader_session cs; // Makes transition table cleaner.

    // Transition table for ctrader_session.
    struct transition_table : mpl::vector<
        //    Start                    Event                      Next                      Action                                         Guard
        //  +-------------------------+--------------------------+-------------------------+--------------------------------------------+----------------------------------+
      a_row < wait_for_connect        , cs::new_connection_event , app_authorization       , &cs::start_app_authorization                                                  >,
       _row < wait_for_connect        , cs::error_event          , wait_for_connect                                                                                        >,
        //  +-------------------------+--------------------------+-------------------------+--------------------------------------------+----------------------------------+
        row < app_authorization       , cs::data_event           , account_authorization   , &cs::start_account_authorization           , &cs::is_app_authorized           >,
       _row < app_authorization       , cs::error_event          , wait_for_connect                                                                                        >,
        //  +-------------------------+--------------------------+-------------------------+--------------------------------------------+----------------------------------+
        row < account_authorization   , cs::data_event           , account_informations    , &cs::start_acquire_account_informations    , &cs::is_account_authorized       >,
       _row < account_authorization   , cs::error_event          , wait_for_connect                                                                                        >,
        //  +-------------------------+--------------------------+-------------------------+--------------------------------------------+----------------------------------+
        row < account_informations    , cs::data_event           , instrument_informations , &cs::start_acquire_instrument_informations , &cs::has_account_informations    >,
       _row < account_informations    , cs::error_event          , wait_for_connect                                                                                        >,
        //  +-------------------------+--------------------------+-------------------------+--------------------------------------------+----------------------------------+
        row < instrument_informations , cs::data_event           , operational             , &cs::initialize_bridge                     , &cs::has_instrument_informations >,
       _row < instrument_informations , cs::error_event          , wait_for_connect                                                                                        >,
        //  +-------------------------+--------------------------+-------------------------+--------------------------------------------+----------------------------------+
      a_row < operational             , cs::data_event           , operational             , &cs::dispatch_event                                                           >,
       _row < operational             , cs::error_event          , wait_for_connect                                                                                        >
        //  +-------------------------+--------------------------+-------------------------+--------------------------------------------+----------------------------------+
    > {};

protected:

    //
    // Auxiliary types.
    //

    typedef std::vector<std::string> instruments_container;

    typedef struct _tick_type
    {
        _tick_type(void)
            : instrument { "" }, ask {-1.0}, bid {-1.0}, timestamp { 0 }
        {}

        std::string instrument;
        double ask;
        double bid;
        int timestamp;

    } tick_type;

    //
    // Utility routine for market_session.
    //

    double get_balance(void) const { return account_balance_; }

    void subscribe_instruments_ex(const instruments_container &instruments);
    void create_market_order_ex(const std::string &position_id, const std::string &instrument, position_type pt, int volume);

    //
    // Things to be implemented in market_session.
    //

    virtual void on_init(void) = 0;
    virtual void on_tick(const tick_type &tick) = 0;
    virtual void on_order_execution_event(const ProtoOAExecutionEvent &event) = 0;

private:

    const hft2ctrader_bridge_config &config_;

    std::map<std::string, int> ticker2id_;
    std::map<int, std::string> id2ticker_;
    std::map<int, tick_type>   instruments_tick_;

    double account_balance_;

    unsigned long last_heartbeat_;
};

#endif /* __CTRADER_SESSION_HPP__ */
