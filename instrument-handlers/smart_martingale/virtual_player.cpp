#include <virtual_player.hpp>
#include <easylogging++.h>

#define hft_log(__X__) \
    CLOG(__X__, logger_id_.c_str())

#define s_play_results (instance_name_+".play_results")
#define s_starting_price (instance_name_+".starting_price")

virtual_player::virtual_player(hft_handler_resource &hs, const std::string &instance_name, const std::string &logger_id)
        : pips_limit_(-1),
          logger_id_(logger_id),
          hs_(hs),
          instance_name_(instance_name),
          capacity_(3),
          long_pattern_("000"),
          short_pattern_("111")
{
    hs_.set_int_var(s_starting_price, -1);
    hs_.set_string_var(s_play_results, "");
}

void virtual_player::set_capacity(size_t capacity)
{
    capacity_ = capacity;

    long_pattern_  = std::string(capacity, '0');
    short_pattern_ = std::string(capacity, '1');
}

void virtual_player::new_market_data(int ask_pips, int bid_pips)
{
    if (pips_limit_ == -1)
    {
        throw std::runtime_error("virtual_player: pips limit uninitialized");
    }

    if (hs_.get_int_var(s_starting_price) == -1)
    {
        hs_.set_int_var(s_starting_price, ask_pips);
        return;
    }

    if (bid_pips - hs_.get_int_var(s_starting_price) >= pips_limit_)
    {
        update_play_result('1');

        hs_.set_int_var(s_starting_price, ask_pips);
    }
    else if (hs_.get_int_var(s_starting_price) - bid_pips >= pips_limit_)
    {
        update_play_result('0');

        hs_.set_int_var(s_starting_price, ask_pips);
    }
}

virtual_player::advice virtual_player::give_advice(void) const
{
    std::string play_results = hs_.get_string_var(s_play_results);

    if (play_results.length() < capacity_)
    {
        return advice::DO_NOTHING;
    }

    if (play_results == long_pattern_)
    {
        return advice::PLAY_LONG;
    }
    else if (play_results == short_pattern_)
    {
        return advice::PLAY_SHORT;
    }

    return advice::DO_NOTHING;
}

void virtual_player::update_play_result(char result)
{
    std::string play_results = hs_.get_string_var(s_play_results);

    if (play_results.length() == capacity_)
    {
        play_results = play_results.substr(1, capacity_ - 1);
    }

    play_results.push_back(result);

    hs_.set_string_var(s_play_results, play_results);
}
