/**********************************************************************\
**                                                                    **
**             -=≡≣ High Frequency Trading System ® ≣≡=-              **
**                                                                    **
**          Copyright © 2017 - 2026 by LLG Ryszard Gradowski          **
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

#ifndef __HFT_SESSION_HPP__
#define __HFT_SESSION_HPP__

#include <instrument_handler.hpp>
#include <session_transport.hpp>
#include <hft_session_state.hpp>

#include <memory>
#include <set>

class hft_session : public session_transport
{
public:

    hft_session(boost::asio::io_service &io_service);

    ~hft_session(void);

    static std::string get_session_dir(const std::string &sessid)
    {
        return std::string("/var/lib/hft/sessions/") + sessid;
    }

private:

    bool check_session_directory(const std::string &sessid);

    typedef std::map<std::string, std::shared_ptr<instrument_handler> > instrument_handler_container;

    virtual void handle_init_request(const hft::protocol::request::init &msg, std::string &response_payload);
    virtual void handle_sync_request(const hft::protocol::request::sync &msg, std::string &response_payload);
    virtual void handle_tick_request(const hft::protocol::request::tick &msg, std::string &response_payload);
    virtual void handle_open_notify_request(const hft::protocol::request::open_notify &msg, std::string &response_payload);
    virtual void handle_close_notify_request(const hft::protocol::request::close_notify &msg, std::string &response_payload);

    instrument_handler_container instrument_handlers_;

    static std::set<std::string> pending_sessions_;
    std::string sessid_;
    std::shared_ptr<session_state> pss_;
};

#endif /* __HFT_SESSION_HPP__ */
