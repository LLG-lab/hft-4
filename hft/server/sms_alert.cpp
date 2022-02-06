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

#include <memory>

#include <sms_alert.hpp>
#include <sms_messenger.hpp>

#ifdef HFT_TEST_SMS_CONFIG
#include <iostream>
#endif

namespace {

std::unique_ptr<sms_messenger> messenger;

}

namespace sms {

void initialize_sms_alert(const config &cfg)
{
    #ifdef HFT_TEST_SMS_CONFIG
    std::cout << "SMS configuration:\n";
    std::cout << " enabled:  " << ( cfg.enabled ? "yes" : "no") << "\n";
    std::cout << " sandbox:  " << ( cfg.sandbox ? "yes" : "no") << "\n";
    std::cout << " login:    " << cfg.login << "\n";
    std::cout << " password: " << cfg.password << "\n";
    std::cout << " recipients:\n";

    for (auto &x : cfg.recipients)
    {
        std::cout << "    " << x << "\n";
    }
    #endif /* HFT_TEST_SMS_CONFIG */

    if (cfg.enabled && ! messenger)
    {
        messenger.reset(new sms_messenger(cfg));
    }
}

void alert(const std::string &message)
{
    if (messenger)
    {
        sms_messenger_data message_data;

        message_data.process = "HFT";
        message_data.sms_data = message;

        messenger -> enqueue(message_data);
    }
}

} // namespace sms
