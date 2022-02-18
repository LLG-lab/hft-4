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

#ifndef __HISTORY_TICKS_HPP__
#define __HISTORY_TICKS_HPP__

class history_ticks
{
public:

    history_ticks(void) = delete;

    history_ticks(history_ticks &) = delete;

    history_ticks(history_ticks &&) = delete;

    history_ticks(boost::asio::io_context &io_context, const hft2ctrader_config &cfg)

    ~history_ticks(void) = default;
};

#endif /* __HISTORY_TICKS_HPP__ */
