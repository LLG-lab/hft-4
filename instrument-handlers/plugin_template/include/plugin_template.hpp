#include <boost/dll.hpp>
#include <instrument_handler.hpp>

#ifndef __PLUGIN_TEMPLATE_HPP__
#define __PLUGIN_TEMPLATE_HPP__

#define API extern "C" BOOST_SYMBOL_EXPORT

namespace hft_ih_plugin {

class plugin_template : public instrument_handler
{
public:

    plugin_template(const instrument_handler::init_info &general_config);
    plugin_template(void) = delete;

    ~plugin_template(void) = default;

    virtual void init_handler(const boost::json::object &specific_config);

    virtual void on_sync(const hft::protocol::request::sync &msg, hft::protocol::response &market);
    virtual void on_tick(const hft::protocol::request::tick &msg, hft::protocol::response &market);
    virtual void on_position_open(const hft::protocol::request::open_notify &msg, hft::protocol::response &market);
    virtual void on_position_close(const hft::protocol::request::close_notify &msg, hft::protocol::response &market);
};

} /* namespace hft_ih_plugin */

API instrument_handler_ptr create_plugin(const instrument_handler::init_info &general_config)
{
    return new hft_ih_plugin::plugin_template(general_config);
}

#endif /* __PLUGIN_TEMPLATE_HPP__ */
