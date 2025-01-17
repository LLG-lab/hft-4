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

#ifndef __METRICS_HPP__
#define __METRICS_HPP__

#include <string>
#include <boost/asio.hpp>

namespace metrics {

    void create_server(boost::asio::io_context &ioctx, const std::string &addr, int p);

    bool is_service_enabled(void);

    void setup_percentage_use_of_margin(const std::string &market, const std::string &instrument, double value);

    void setup_opened_positions(const std::string &market, const std::string &instrument, int value);

} // namespace metrics

#endif /* __METRICS_HPP__ */
