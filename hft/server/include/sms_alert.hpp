/**********************************************************************\
**                                                                    **
**             -=≡≣ High Frequency Trading System ® ≣≡=-              **
**                                                                    **
**          Copyright © 2017 - 2024 by LLG Ryszard Gradowski          **
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

#ifndef __SMS_ALERT_HPP__
#define __SMS_ALERT_HPP__

#include <string>
#include <vector>

namespace sms {

struct config
{
    config(void)
        : enabled { false },
          sandbox { false }
    {}

    bool enabled;
    bool sandbox;
    std::string login;
    std::string password;
    std::vector<std::string> recipients;
};

void initialize_sms_alert(const config &cfg);
void alert(const std::string &message);

} // namespace sms

#endif /* __SMS_ALERT_HPP__ */
