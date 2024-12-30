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

#ifndef __SMS_MESSENGER_HPP__
#define __SMS_MESSENGER_HPP__

#include <thread_worker.hpp>
#include <curlpp.hpp>

#include <sms_alert.hpp>

typedef struct _sms_messenger_data
{
    std::string process;
    std::string sms_data;

} sms_messenger_data;

class sms_messenger : public thread_worker<sms_messenger_data>
{
public:

    sms_messenger(void) = delete;

    sms_messenger(sms_messenger &) = delete;

    sms_messenger(sms_messenger &&) = delete;

    sms_messenger(const sms::config &config);

    ~sms_messenger(void);

private:

    void work(const sms_messenger_data &data);

    static std::string url_encode(const std::string &data);

    curlpp http_client_;

    const sms::config config_;
};

#endif /* __SMS_MESSENGER_HPP__ */
