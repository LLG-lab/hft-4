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

#ifndef __HFT_SERVER_CONFIG__
#define __HFT_SERVER_CONFIG__

#include <sms_alert.hpp>

class hft_server_config
{
public:

    hft_server_config(const std::string &xml_file_name = "/var/log/hft/server.log");
    ~hft_server_config(void) = default;

    std::string get_logging_config(void) const;
    const sms::config &get_sms_alert_config(void) const { return sms_config_; }

private:

    enum logging_severity
    {
        HFT_SEVERITY_FATAL   = 0,
        HFT_SEVERITY_ERROR   = 1,
        HFT_SEVERITY_WARNING = 2,
        HFT_SEVERITY_INFO    = 3,
        HFT_SEVERITY_TRACE   = 4,
        HFT_SEVERITY_DEBUG   = 5
    };

    logging_severity log_severity_;
    sms::config sms_config_;
};

#endif /* __HFT_SERVER_CONFIG__ */

