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

#ifndef __PROXY_CORE_HPP__
#define __PROXY_CORE_HPP__

#include <map>
#include <list>

#include <boost/msm/front/state_machine_def.hpp>

#include <ctrader_api.hpp>
#include <hft_api.hpp>
#include <hft2ctrader_config.hpp>

namespace msm = boost::msm;
namespace mpl = boost::mpl;

class proxy_core  : public msm::front::state_machine_def<proxy_core>, public ctrader_api, public hft_api
{
public:

    proxy_core(void) = delete;

    proxy_core(proxy_core &) = delete;

    proxy_core(proxy_core &&) = delete;

    proxy_core(ctrader_ssl_connection &ctrader_conn, hft_connection &hft_conn, const hft2ctrader_config &config);

    virtual ~proxy_core(void) = default;

    //
    // Events.
    //

    struct ctrader_connection_event {};

    struct ctrader_connection_error_event
    {
        ctrader_connection_error_event(void) = delete;
        ctrader_connection_error_event(const boost::system::error_code &e)
            : error_(e) {}

        const boost::system::error_code &error_;
    };

    struct ctrader_data_event
    {
        ctrader_data_event(void) = delete;
        ctrader_data_event(const std::vector<char> &d)
            : data_(d) {}

        const std::vector<char> &data_;
    };

    struct hft_data_event
    {
        hft_data_event(void) = delete;
        hft_data_event(const std::string &d)
            : data_(d) {}

        const std::string &data_;
    };

    //
    // List of FSM states.
    //

    struct wait_for_connect : public msm::front::state<> {};
    struct app_authorization : public msm::front::state<> {};
    struct account_authorization : public msm::front::state<> {};
    struct account_informations : public msm::front::state<> {};
    struct instrument_informations : public msm::front::state<> {};
    struct position_informations : public msm::front::state<> {};
    struct operational : public msm::front::state<> {};

    typedef wait_for_connect initial_state;

    //
    // Transition action methods and guards.
    //

    void start_app_authorization(ctrader_connection_event const &event);
    void start_account_authorization(ctrader_data_event const &event);
    bool is_app_authorized(ctrader_data_event const &event);
    void start_acquire_account_informations(ctrader_data_event const &event);
    bool is_account_authorized(ctrader_data_event const &event);
    void start_acquire_instrument_informations(ctrader_data_event const &event);
    bool has_account_informations(ctrader_data_event const &event);
    void start_acquire_position_informations(ctrader_data_event const &event);
    bool has_instrument_informations(ctrader_data_event const &event);
    void initialize_bridge(ctrader_data_event const &event); // ma wywołać on_init
    bool has_position_informations(ctrader_data_event const &event);
    void dispatch_ctrader_data_event(ctrader_data_event const &event);
    void dispatch_hft_data_event(hft_data_event const &event);
    void hft_data_event_broker_disconnected(hft_data_event const &event);

    typedef proxy_core pc; // Makes transition table cleaner.

    // Transition table for proxy_core.
    struct transition_table : mpl::vector<
        //    Start                    Event                                Next                      Action                                         Guard
        //  +-------------------------+-----------------------------------+-------------------------+--------------------------------------------+----------------------------------+
      a_row < wait_for_connect        , pc::hft_data_event                , wait_for_connect        , &pc::hft_data_event_broker_disconnected                                       >,
      a_row < wait_for_connect        , pc::ctrader_connection_event      , app_authorization       , &pc::start_app_authorization                                                  >,
       _row < wait_for_connect        , pc::ctrader_connection_error_event, wait_for_connect                                                                                        >,
        //  +-------------------------+-----------------------------------+-------------------------+--------------------------------------------+----------------------------------+
      a_row < app_authorization       , pc::hft_data_event                , app_authorization       , &pc::hft_data_event_broker_disconnected                                       >,
        row < app_authorization       , pc::ctrader_data_event            , account_authorization   , &pc::start_account_authorization           , &pc::is_app_authorized           >,
       _row < app_authorization       , pc::ctrader_connection_error_event, wait_for_connect                                                                                        >,
        //  +-------------------------+-----------------------------------+-------------------------+--------------------------------------------+----------------------------------+
      a_row < account_authorization   , pc::hft_data_event                , account_authorization   , &pc::hft_data_event_broker_disconnected                                       >,
        row < account_authorization   , pc::ctrader_data_event            , account_informations    , &pc::start_acquire_account_informations    , &pc::is_account_authorized       >,
       _row < account_authorization   , pc::ctrader_connection_error_event, wait_for_connect                                                                                        >,
        //  +-------------------------+-----------------------------------+-------------------------+--------------------------------------------+----------------------------------+
      a_row < account_informations    , pc::hft_data_event                , account_informations    , &pc::hft_data_event_broker_disconnected                                       >,
        row < account_informations    , pc::ctrader_data_event            , instrument_informations , &pc::start_acquire_instrument_informations , &pc::has_account_informations    >,
       _row < account_informations    , pc::ctrader_connection_error_event, wait_for_connect                                                                                        >,
        //  +-------------------------+-----------------------------------+-------------------------+--------------------------------------------+----------------------------------+
      a_row < instrument_informations , pc::hft_data_event                , instrument_informations , &pc::hft_data_event_broker_disconnected                                       >,
        row < instrument_informations , pc::ctrader_data_event            , position_informations   , &pc::start_acquire_position_informations   , &pc::has_instrument_informations >,
       _row < instrument_informations , pc::ctrader_connection_error_event, wait_for_connect                                                                                        >,
        //  +-------------------------+-----------------------------------+-------------------------+--------------------------------------------+----------------------------------+
      a_row < position_informations   , pc::hft_data_event                , position_informations   , &pc::hft_data_event_broker_disconnected                                       >,
        row < position_informations   , pc::ctrader_data_event            , operational             , &pc::initialize_bridge                     , &pc::has_position_informations   >,
       _row < position_informations   , pc::ctrader_connection_error_event, wait_for_connect                                                                                        >,
        //  +-------------------------+-----------------------------------+-------------------------+--------------------------------------------+----------------------------------+
      a_row < operational             , pc::hft_data_event                , operational             , &pc::dispatch_hft_data_event                                                  >,
      a_row < operational             , pc::ctrader_data_event            , operational             , &pc::dispatch_ctrader_data_event                                              >,
       _row < operational             , pc::ctrader_connection_error_event, wait_for_connect                                                                                        >
        //  +-------------------------+-----------------------------------+-------------------------+--------------------------------------------+----------------------------------+
    > {};

protected:

    //
    // Utility routine for proxy_session.
    //

    double get_balance(void) const { return account_balance_; }
    std::string get_broker(void) const { return broker_; }
    const positions_container &get_opened_positions(void) const { return positions_; }
    std::string get_instrument_ticker(int instrument_id) const;
    static std::string position_type_str(const position_type &pt);
    double get_free_margin(void); //const;

    bool ctrader_subscribe_instruments_ex(const instruments_container &instruments);
    bool ctrader_create_market_order_ex(const std::string &identifier, const std::string &instrument, position_type pt, int volume);
    bool ctrader_close_position_ex(const std::string &identifier);

    //
    // Things to be implemented in proxy_session.
    //

    virtual void on_init(void) = 0;
    virtual void on_tick(const tick_type &tick) = 0;
    virtual void on_position_open(const position_info &position) = 0;
    virtual void on_position_open_error(const order_error_info &order_error) = 0;
    virtual void on_position_close(const closed_position_info &closed_position) = 0;
    virtual void on_position_close_error(const order_error_info &order_error) = 0;
    virtual void on_hft_advice(const hft_api::hft_response &adv, bool broker_ready) = 0;

private:

    void arrange_position(const ProtoOAPosition &pos, bool historical);

    void handle_order_fill(const ProtoOAExecutionEvent &evt);
    void handle_order_reject(const ProtoOAExecutionEvent &evt);

    void handle_oa_trader(const ProtoOATrader &trader);

    positions_container positions_;

    const hft2ctrader_config &config_;

    std::map<std::string, int> ticker2id_;
    std::map<int, std::string> id2ticker_;
    std::map<int, tick_type>   instruments_tick_;

    double account_balance_;
    std::string broker_;

    unsigned long last_heartbeat_;
    unsigned long registration_timestamp_;
};

#endif /* __PROXY_CORE_HPP__ */
