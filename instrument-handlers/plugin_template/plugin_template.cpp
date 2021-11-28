#include <plugin_template.hpp>

#include <easylogging++.h>

#define hft_log(__X__) \
    CLOG(__X__, get_logger_id().c_str())

namespace hft_ih_plugin {

plugin_template::plugin_template(const instrument_handler::init_info &general_config)
    : instrument_handler(general_config)
{
    el::Loggers::getLogger(get_logger_id().c_str(), true);

    hft_log(INFO) << "Starting instrument handler ‘plugin_template’ for instrument ‘"
                  << get_ticker() << "’, i.e. ‘"
                  << get_instrument_description()
                  << "’";

    //
    // FIXME: Not implemented.
    //
}

void plugin_template::init_handler(const boost::json::object &specific_config)
{
    //
    // FIXME: Not implemented.
    //
}

void plugin_template::on_sync(const hft::protocol::request::sync &msg, hft::protocol::response &market)
{
    //
    // FIXME: Not implemented.
    //
}

void plugin_template::on_tick(const hft::protocol::request::tick &msg, hft::protocol::response &market)
{
    //
    // FIXME: Not implemented.
    //
}

void plugin_template::on_position_open(const hft::protocol::request::open_notify &msg, hft::protocol::response &market)
{
    //
    // FIXME: Not implemented.
    //
}

void plugin_template::on_position_close(const hft::protocol::request::close_notify &msg, hft::protocol::response &market)
{
    //
    // FIXME: Not implemented.
    //
}

} /* namespace hft_ih_plugin */
