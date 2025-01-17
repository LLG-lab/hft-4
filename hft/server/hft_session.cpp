/**********************************************************************\
**                                                                    **
**             -=≡≣ High Frequency Trading System ® ≣≡=-              **
**                                                                    **
**          Copyright © 2017 - 2025 by LLG Ryszard Gradowski          **
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

#include <boost/filesystem.hpp>

#include <hft_session.hpp>

#define hft_log(__X__) \
    CLOG(__X__, "session")

//
// On production, should be undefined.
//

#undef HFT_DEBUG

//
// Static member init.
//

std::set<std::string> hft_session::pending_sessions_;

hft_session::hft_session(boost::asio::io_service &io_service)
    : session_transport(io_service)
{
    el::Loggers::getLogger("session", true);
}

hft_session::~hft_session(void)
{
    #ifdef HFT_DEBUG
    hft_log(INFO) << "Destructor got called";
    #endif

    if (! sessid_.empty())
    {
        auto it = pending_sessions_.find(sessid_);

        if (it != pending_sessions_.end())
        {
            hft_log(INFO) << "Complete session ‘" << sessid_ << "’";

            pending_sessions_.erase(it);
        }
    }
}

bool hft_session::check_session_directory(const std::string &sessid)
{
    using namespace boost::filesystem;

    boost::system::error_code ec;

    path sessdir = get_session_dir(sessid);

    if (exists(sessdir, ec))
    {
        if (is_directory(sessdir))
        {
            hft_log(INFO) << "Found session directory ‘"
                          << sessdir.string() << "’";

            return true;
        }
        else
        {
            hft_log(ERROR) << "Not a directory ‘" << sessdir.string()
                           << "’ – session cannot be established.";
        }
    }
    else
    {
        if (ec)
        {
            hft_log(ERROR) << "Raised system error: ‘"
                           << ec.message() << "’";
        }

        hft_log(ERROR) << "Not found directory ‘" << sessdir.string()
                       << "’ – session cannot be established.";
    }

    return false;
}


void hft_session::handle_init_request(const hft::protocol::request::init &msg, std::string &response_payload)
{
    #ifdef HFT_DEBUG
    hft_log(INFO) << "hft_session::handle_init_request: got called; sessid [" << msg.sessid << "], instruments:";
    for (auto &i : msg.instruments)
    {
        hft_log(INFO) << "hft_session::handle_init_request " << i;
    }
    #endif

    hft::protocol::response resp;

    if (! sessid_.empty())
    {
        hft_log(ERROR) << "Attempt to re-initialize session ‘"
                       << sessid_ << "’ to ‘" << msg.sessid
                       << "’";

        resp.error("Forbidden reinitialize session");

        response_payload = resp.serialize();

        return;
    }

    auto it = pending_sessions_.find(msg.sessid);

    if (it != pending_sessions_.end())
    {
        hft_log(ERROR) << "Cannot create session ‘" << msg.sessid << "’ because it is already pending";

        resp.error("Session already pending");

        response_payload = resp.serialize();

        return;
    }

    if (! check_session_directory(msg.sessid))
    {
        resp.error("Internal server error");

        response_payload = resp.serialize();

        return;
    }

    //
    // Session successfully initialized.
    //

    sessid_ = msg.sessid;
    pending_sessions_.insert(sessid_);
    pss_.reset(new session_state(sessid_));

    //
    // Create handlers for requested instruments.
    //

    for (auto &instrument : msg.instruments)
    {
        if (instrument_handlers_.find(instrument) == instrument_handlers_.end())
        {
            hft_log(INFO) << "Creating handler for instrument ‘"
                          << instrument << "’";

            try
            {
                std::shared_ptr<instrument_handler> handler;
                handler.reset(create_instrument_handler(pss_, instrument));
                instrument_handlers_.insert(std::pair<std::string, std::shared_ptr<instrument_handler> >(instrument, handler));
            }
            catch (std::exception &e)
            {
                hft_log(ERROR) << "Failed to create handler for instrument ‘"
                               << instrument << "’ : " << e.what();

                throw e;
            }
        }
        else
        {
            hft_log(WARNING) << "Instrument handler ‘" << instrument
                             << "’ already created, ignoring";
        }
    }

    response_payload = resp.serialize();

    return;
}

void hft_session::handle_sync_request(const hft::protocol::request::sync &msg, std::string &response_payload)
{
    #ifdef HFT_DEBUG
    hft_log(INFO) << "hft_session::handle_sync_request: got called";
    hft_log(INFO) << "hft_session::handle_sync_request: instrument [" << msg.instrument << "]";
    hft_log(INFO) << "hft_session::handle_sync_request: id [" << msg.id << "]";
    hft_log(INFO) << "hft_session::handle_sync_request: direction [" << (msg.is_long ? "LONG" : "SHORT") << "]";
    hft_log(INFO) << "hft_session::handle_sync_request: open price [" << msg.price << "]";
    hft_log(INFO) << "hft_session::handle_sync_request: qty [" << msg.qty << "]";
    #endif

    hft::protocol::response resp {msg.instrument};

    if (sessid_.empty())
    {
        hft_log(ERROR) << "Got SYNC request for uninitialized session.";

        resp.error("Session uninitialized");

        response_payload = resp.serialize();

        return;
    }

    //
    // Find appropriate instrument handler, then dispatch notify to it.
    //

    auto it = instrument_handlers_.find(msg.instrument);

    if (it == instrument_handlers_.end())
    {
        hft_log(ERROR) << "Unsubscribend instrument ‘"
                       << msg.instrument << "’";

        std::string error_message = std::string("Unsubscribend instrument ‘")
                                  + msg.instrument + std::string("’");

        resp.error(error_message);

        response_payload = resp.serialize();

        return;
    }

    auto as = pss_ -> create_autosaver();

    it -> second -> on_sync(msg, resp);

    response_payload = resp.serialize();

    return;
}

void hft_session::handle_tick_request(const hft::protocol::request::tick &msg, std::string &response_payload)
{
    #ifdef HFT_DEBUG
    hft_log(INFO) << "hft_session::handle_tick_request got called";
    hft_log(INFO) << "hft_session::handle_tick_request: instrument [" << msg.instrument << "]";
    hft_log(INFO) << "hft_session::handle_tick_request: ask [" << msg.ask << "]";
    hft_log(INFO) << "hft_session::handle_tick_request: bid [" << msg.bid << "]";
    hft_log(INFO) << "hft_session::handle_tick_request:  equity [" << msg.equity << "]";
    #endif

    hft::protocol::response resp {msg.instrument};

    if (sessid_.empty())
    {
        hft_log(ERROR) << "Got TICK request for uninitialized session.";

        resp.error("Session uninitialized");

        response_payload = resp.serialize();

        return;
    }

    //
    // Find appropriate instrument handler, then dispatch notify to it.
    //

    auto it = instrument_handlers_.find(msg.instrument);

    if (it == instrument_handlers_.end())
    {
        hft_log(ERROR) << "Unsubscribend instrument ‘"
                       << msg.instrument << "’";

        std::string error_message = std::string("Unsubscribend instrument ‘")
                                  + msg.instrument + std::string("’");

        resp.error(error_message);

        response_payload = resp.serialize();

        return;
    }

    auto as = pss_ -> create_autosaver();

    it -> second -> on_tick(msg, resp);

    response_payload = resp.serialize();

    return;
}

void hft_session::handle_open_notify_request(const hft::protocol::request::open_notify &msg, std::string &response_payload)
{
    #ifdef HFT_DEBUG
    hft_log(INFO) << "hft_session::handle_open_notify_request got called";
    hft_log(INFO) << "hft_session::handle_open_notify_request: instrument [" << msg.instrument << "]";
    hft_log(INFO) << "hft_session::handle_open_notify_request: id [" << msg.id << "]";
    hft_log(INFO) << "hft_session::handle_open_notify_request: status [" << msg.status << "]";
    hft_log(INFO) << "hft_session::handle_open_notify_request: price [" << msg.price << "]";
    #endif

    hft::protocol::response resp {msg.instrument};

    if (sessid_.empty())
    {
        hft_log(ERROR) << "Got OPEN_NOTIFY request for uninitialized session.";

        resp.error("Session uninitialized");

        response_payload = resp.serialize();

        return;
    }

    //
    // Find appropriate instrument handler, then dispatch notify to it.
    //

    auto it = instrument_handlers_.find(msg.instrument);

    if (it == instrument_handlers_.end())
    {
        hft_log(ERROR) << "Unsubscribend instrument ‘"
                       << msg.instrument << "’";

        std::string error_message = std::string("Unsubscribend instrument ‘")
                                  + msg.instrument + std::string("’");

        resp.error(error_message);

        response_payload = resp.serialize();

        return;
    }

    auto as = pss_ -> create_autosaver();

    it -> second -> on_position_open(msg, resp);

    response_payload = resp.serialize();

    return;
}

void hft_session::handle_close_notify_request(const hft::protocol::request::close_notify &msg, std::string &response_payload)
{
    #ifdef HFT_DEBUG
    hft_log(INFO) << "hft_session::handle_close_notify_request got called";
    hft_log(INFO) << "hft_session::handle_close_notify_request: instrument [" << msg.instrument << "]";
    hft_log(INFO) << "hft_session::handle_close_notify_request: id [" << msg.id << "]";
    hft_log(INFO) << "hft_session::handle_close_notify_request: status [" << msg.status << "]";
    hft_log(INFO) << "hft_session::handle_close_notify_request: price [" << msg.price << "]";
    #endif

    hft::protocol::response resp {msg.instrument};

    if (sessid_.empty())
    {
        hft_log(ERROR) << "Got CLOSE_NOTIFY request for uninitialized session.";

        resp.error("Session uninitialized");

        response_payload = resp.serialize();

        return;
    }

    //
    // Find appropriate instrument handler, then dispatch notify to it.
    //

    auto it = instrument_handlers_.find(msg.instrument);

    if (it == instrument_handlers_.end())
    {
        hft_log(ERROR) << "Unsubscribend instrument ‘"
                       << msg.instrument << "’";

        std::string error_message = std::string("Unsubscribend instrument ‘")
                                  + msg.instrument + std::string("’");

        resp.error(error_message);

        response_payload = resp.serialize();

        return;
    }

    auto as = pss_ -> create_autosaver();

    it -> second -> on_position_close(msg, resp);

    response_payload = resp.serialize();

    return;
}
