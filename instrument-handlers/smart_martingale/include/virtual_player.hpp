#ifndef __VIRTUAL_PLAYER_HPP__
#define __VIRTUAL_PLAYER_HPP__

#include <hft_handler_resource.hpp>

class virtual_player
{
public:

    enum class advice
    {
        DO_NOTHING,
        PLAY_SHORT,
        PLAY_LONG
    };

    virtual_player(void) = delete;
    virtual_player(hft_handler_resource &hs, const std::string &instance_name, const std::string &logger_id);

    ~virtual_player(void) = default;

    void set_pips_limit(int limit) { pips_limit_ = limit; }
    void set_capacity(size_t capacity);
    void new_market_data(int ask_pips, int bid_pips);
    advice give_advice(void) const;

private:

    void update_play_result(char result);

    int pips_limit_;
    std::string logger_id_;
    hft_handler_resource &hs_;
    std::string instance_name_;

    size_t capacity_;
    std::string long_pattern_;
    std::string short_pattern_;
};

#endif /* __VIRTUAL_PLAYER_HPP__ */
