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

#ifndef __HFT_SERVER_CONFIG__
#define __HFT_SERVER_CONFIG__

#include <sms_alert.hpp>

class hft_server_config
{
public:

    hft_server_config(const std::string &xml_file_name = "/var/log/hft/server.log");
    ~hft_server_config(void) = default;

    std::string get_logging_config(void) const;

    std::string get_ipc_address(void) const { return ipc_address_; }
    void override_ipc_address(const std::string &ipc_addr) { ipc_address_ = ipc_addr; }
    int get_ipc_port(void) const { return ipc_port_; }
    void override_ipc_port(int ipcport) { ipc_port_ = ipcport; }

    bool is_metrics_active(void) const { return metrics_active_; }
    void set_metrics_active(void) { metrics_active_ = true; }
    std::string get_metrics_address(void) const { return metrics_address_; }
    void override_metrics_address(const std::string &metaddr) { metrics_address_ = metaddr; }
    int get_metrics_port(void) const { return metrics_port_; }
    void override_metrics_port(int meport) { metrics_port_ = meport; }

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

    std::string ipc_address_;
    int ipc_port_;

    bool metrics_active_;
    std::string metrics_address_;
    int metrics_port_;

    sms::config sms_config_;
};

#endif /* __HFT_SERVER_CONFIG__ */

