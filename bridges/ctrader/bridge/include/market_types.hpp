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

#ifndef __MARKET_TYPES_HPP__
#define __MARKET_TYPES_HPP__

#include <string>
#include <vector>
#include <list>

typedef std::vector<int> instrument_id_container;


enum class position_type
{
    UNDEFINED_POSITION,
    LONG_POSITION,
    SHORT_POSITION
};

typedef std::vector<std::string> instruments_container;

typedef struct _tick_type
{
    _tick_type(void)
        : instrument {}, ask {-1.0}, bid {-1.0}, timestamp {0}
    {}

    std::string instrument;
    double ask;
    double bid;
    unsigned long timestamp;

} tick_type;

typedef struct _position_info
{
    _position_info(void)
        : position_id_ {0}, instrument_id_ {0}, timestamp_ {0ul},
          trade_side_ { position_type::UNDEFINED_POSITION },
          volume_ {0}, execution_price_ {0.0}, swap_ {0.0},
          commission_ {0.0}, used_margin_ {0.0}, label_ {}
    {}

    int position_id_;
    int instrument_id_;
    unsigned long timestamp_;
    position_type trade_side_;
    int volume_;
    double execution_price_;
    double swap_;
    double commission_;
    double used_margin_;
    std::string label_;

} position_info;

typedef std::list<position_info> positions_container;

typedef struct _closed_position_info
{
    _closed_position_info(void)
        : instrument_id_ {0}, label_ {}, execution_price_ {0.0}
    {}

    int instrument_id_;
    std::string label_;
    double execution_price_;

} closed_position_info;

typedef struct _order_error_info
{
    _order_error_info(void)
        : instrument_id_ {0}, label_ {}, error_message_ {}
    {}

    int instrument_id_;
    std::string label_;
    std::string error_message_;

} order_error_info;

#endif /* __MARKET_TYPES_HPP__ */
