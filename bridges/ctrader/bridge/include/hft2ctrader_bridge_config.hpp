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

#ifndef __HFT2CTRADER_BRIDGE_CONFIG_HPP__
#define __HFT2CTRADER_BRIDGE_CONFIG_HPP__

#include <string>
#include <vector>

class hft2ctrader_bridge_config
{
public:

    enum class account_type
    {
        UNDEFINED = 0,
        DEMO_ACCOUNT = 1,
        LIVE_ACCOUNT = 2
    };

    hft2ctrader_bridge_config(void) = delete;
    hft2ctrader_bridge_config(hft2ctrader_bridge_config &) = delete;
    hft2ctrader_bridge_config(hft2ctrader_bridge_config &&) = delete;

    hft2ctrader_bridge_config(const std::string &config_file_name, const std::string &broker, const std::string &hft_host, int hft_port)
        : broker_(broker), auth_account_id_(0), account_(account_type::UNDEFINED),
          hft_host_(hft_host), hft_port_(hft_port)
    { xml_parse(config_file_name); }

    ~hft2ctrader_bridge_config(void) = default;

    std::string get_logging_config(const std::string &severity_str) const;

    std::string get_auth_client_id(void) const { return auth_client_id_; }
    std::string get_auth_client_secret(void) const { return auth_client_secret_; }
    std::string get_auth_access_token(void) const { return auth_access_token_; }
    int get_auth_account_id(void) const { return auth_account_id_; }
    account_type get_account_type(void) const { return account_; }
    std::string get_session_id(void) const { return session_id_; }
    std::string get_hft_host(void) const { return hft_host_; }
    int get_hft_port(void) const { return hft_port_; }

    std::vector<std::string> get_instruments(void) const { return instruments_; }

private:

    enum class logging_severity_type
    {
        FATAL   = 0,
        ERROR   = 1,
        WARNING = 2,
        INFO    = 3,
        TRACE   = 4,
        DEBUG   = 5
    };

    static logging_severity_type severity_str2severity_type(const std::string &s);

    void xml_parse(const std::string &xml_file_name);
    void xml_parse_auth(void *node, const std::string &xml_file_name);
    void xml_parse_instrument(void *node, const std::string &xml_file_name);

    std::string broker_;
    std::string auth_client_id_;
    std::string auth_client_secret_;
    std::string auth_access_token_;
    int auth_account_id_;
    account_type account_;
    std::string session_id_;

    std::vector<std::string> instruments_;

    std::string hft_host_;
    int hft_port_;
};

#endif /* __HFT2CTRADER_BRIDGE_CONFIG_HPP__ */
