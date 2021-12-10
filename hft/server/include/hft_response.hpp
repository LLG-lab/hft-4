/**********************************************************************\
**                                                                    **
**             -=≡≣ High Frequency Trading System ® ≣≡=-              **
**                                                                    **
**          Copyright © 2017 - 2021 by LLG Ryszard Gradowski          **
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

#ifndef __HFT_RESPONSE_HPP__
#define __HFT_RESPONSE_HPP__

#include <list>
#include <custom_except.hpp>

namespace hft {
namespace protocol {

class response
{
public:

    DEFINE_CUSTOM_EXCEPTION_CLASS(violation_error, std::runtime_error)

    enum class position_direction
    {
        UNDEFINED,
        POSITION_LONG,
        POSITION_SHORT
    };

    struct open_position_info
    {
        open_position_info(position_direction pd, const std::string &id, int qty)
            : pd_(pd), id_(id), qty_(qty) {}

        position_direction pd_;
        std::string id_;
        int qty_;
    };

    response(void) {};
    ~response(void) = default;

    //
    // Methods used by server.
    //

    std::string serialize(void) const;

    void error(const std::string &message) { error_message_ = message; }
    void close_position(const std::string &id) { close_positions_.push_back(id); }
    void open_long(const std::string &id, int qty) { new_positions_.emplace_back(position_direction::POSITION_LONG, id, qty); }
    void open_short(const std::string &id, int qty) { new_positions_.emplace_back(position_direction::POSITION_SHORT, id, qty); }

    //
    // Methods used by client.
    //

    void unserialize(const std::string &payload);

    bool is_error(void) const { return !error_message_.empty(); }
    std::string get_error_message(void) const { return error_message_; }
    const std::list<open_position_info> &get_new_positions(void) const { return new_positions_; }
    const std::list<std::string> &get_close_positions(void) const { return close_positions_; }

private:

    std::string error_message_;
    std::list<open_position_info> new_positions_;
    std::list<std::string> close_positions_;
};

} /* namespace protocol */
} /* namespace hft */

#endif /* __HFT_RESPONSE_HPP__ */
