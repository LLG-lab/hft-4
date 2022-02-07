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

#ifndef __MARKETPLACE_GATEWAY_PROCESS_HPP__
#define __MARKETPLACE_GATEWAY_PROCESS_HPP__

#include <stdexcept>
#include <memory>
#include <vector>
#include <list>

#include <boost/noncopyable.hpp>
#include <boost/process.hpp>
#include <boost/asio.hpp>

#include <custom_except.hpp>

class marketplace_gateway_process  : private boost::noncopyable
{
public:

    DEFINE_CUSTOM_EXCEPTION_CLASS(exception, std::runtime_error)

    marketplace_gateway_process(boost::asio::io_context &ioctx, const std::string &config_xml);

    ~marketplace_gateway_process(void);

private:

    typedef std::vector<std::string> arguments;

    typedef struct _bridge_process_info
    {
        _bridge_process_info(void)
            : last_respawn {0ul}, respawn_attempts {0}
        {}

        std::string label;
        std::string program;
        std::string start_dir;
        std::string log_file;
        arguments argv;
        std::shared_ptr<boost::process::child> child;
        unsigned long last_respawn;
        int respawn_attempts;

    } bridge_process_info;

    typedef std::list<bridge_process_info> bridge_process_info_list;

    static std::string prepare_log_file(const std::string &log_file_name);
    void parse_proc_list_xml(const std::string &data);

    void execute_process(bridge_process_info &bpi);
    void process_exit_notify(int, const std::error_code &ec);

    boost::asio::io_context &ioctx_;
    boost::asio::steady_timer respawn_timer_;

    bridge_process_info_list process_list_;
};

#endif /*  __MARKETPLACE_GATEWAY_PROCESS_HPP__ */
