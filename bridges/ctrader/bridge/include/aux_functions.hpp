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

#ifndef __AUX_FUNCTIONS_HPP__
#define __AUX_FUNCTIONS_HPP__

#include <boost/date_time/posix_time/posix_time.hpp>

namespace aux {

unsigned long get_current_timestamp(void);

unsigned long ptime2timestamp(const boost::posix_time::ptime &t);

std::string timestamp2string(unsigned long timestamp);

std::string hexdump(const std::string &data);

} // namespace aux

#endif /* __AUX_FUNCTIONS_HPP__ */
