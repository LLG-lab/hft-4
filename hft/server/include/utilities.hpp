/**********************************************************************\
**                                                                    **
**             -=≡≣ High Frequency Trading System  ≣≡=-              **
**                                                                    **
**          Copyright  2017 - 2021 by LLG Ryszard Gradowski          **
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

#ifndef __UTILITIES_HPP__
#define __UTILITIES_HPP__

#include <string>

namespace hft {
namespace utils {

std::string file_get_contents(const std::string &filename);

void file_put_contents(const std::string &filename, const std::string &content);

unsigned long get_current_timestamp(void);

std::string find_free_name(const std::string &file_name);

std::string expand_env_variable(const std::string &input);

int floating2pips(double price, char pips_digit);

} // namespace utils
} // namespace hft

#endif /* __UTILITIES_HPP__ */
