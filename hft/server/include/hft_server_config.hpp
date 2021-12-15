/**********************************************************************\
**                                                                    **
**             -=≡≣ High Frequency Trading System ® ≣≡=-              **
**                                                                    **
**          Copyright © 2017 - 2021 by LLG Ryszard Gradowski          **
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

#ifndef __HFT_SERVER_CONFIG__
#define __HFT_SERVER_CONFIG__

#include <string>

class hft_server_config
{
public:

    hft_server_config(const std::string &xml_file_name = "/var/log/hft/server.log");
    ~hft_server_config(void) = default;

    std::string get_logging_config(void) const;

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
}; 

#endif /* __HFT_SERVER_CONFIG__ */

