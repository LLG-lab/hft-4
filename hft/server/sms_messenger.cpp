/**********************************************************************\
**                                                                    **
**             -=≡≣ High Frequency Trading System ® ≣≡=-              **
**                                                                    **
**          Copyright © 2017 - 2023 by LLG Ryszard Gradowski          **
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

#include <sms_messenger.hpp>

#include <easylogging++.h>

#define hft_log(__X__) \
    CLOG(__X__, "sms_messenger")

sms_messenger::sms_messenger(const sms::config &config)
    : config_ {config}
{
    //
    // Initialize logger.
    //

    el::Loggers::getLogger("sms_messenger", true);

    start_job();

    hft_log(INFO) << "Initialized SMS Messenger";
}

sms_messenger::~sms_messenger(void)
{
    terminate();
}

void sms_messenger::work(const sms_messenger_data &data)
{
    hft_log(INFO) << "Work got message from process ‘"
                    << data.process << "’, payload ‘"
                    << data.sms_data << "’.";

    try
    {
        for (auto &recipient : config_.recipients)
        {
            std::string url = "https://api.gsmservice.pl/v5/send.php?login="
                              + config_.login + std::string("&pass=") + config_.password
                              + std::string("&recipient=") + recipient
                              + std::string("&message=")
                              + url_encode(std::string("[") + data.process + std::string("] ") + data.sms_data)
                              + std::string("&msg_type=1&encoding=utf-8&unicode=0")
                              + (config_.sandbox ? std::string("&sandbox=1") : std::string("&sandbox=0"));

            hft_log(INFO) << "Sending SMS to recipient ‘" << recipient << "’.";

            http_client_.clear_buffer();
            http_client_.download_remote_target(url);

            hft_log(INFO) << "Remote server reply: ‘" << http_client_.get_buffer() << "’.";
        }
    }
    catch (const std::exception &e)
    {
        hft_log(ERROR) << "Failed to send SMS – " << e.what();
    }
}

std::string sms_messenger::url_encode(const std::string &data)
{
    std::string ret;

    CURL *curl = curl_easy_init();

    if (curl)
    {
        char *output = curl_easy_escape(curl, data.c_str(), 0);

        if (output)
        {
            ret = output;
            curl_free(output);
            curl_easy_cleanup(curl);
        }
        else
        {
            hft_log(ERROR) << "url_encode: URL encoding error";

            ret = "EmptyMsg";
        }
    }
    else
    {
        hft_log(ERROR) << "url_encode: Failed to initialize curl";

        ret = "EmptyMsg";
    }

    return ret;
}
