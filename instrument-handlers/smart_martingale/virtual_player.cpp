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
          instance_name_(instance_name)         
{
    hs_.set_int_var(s_starting_price, -1);
    hs_.set_string_var(s_play_results, "");
}

void virtual_player::new_market_data(int ask_pips, int bid_pips)
{
    if (pips_limit_ == -1)
    {
        throw std::runtime_error("virtual_player: pips limit uninitialized");
    }

    if (hs_.get_int_var(s_starting_price) == -1)
    {
        // starting_price_ = ask_pips;
        hs_.set_int_var(s_starting_price, ask_pips);
        return;
    }

    if (bid_pips - hs_.get_int_var(s_starting_price) >= pips_limit_)
    {
        update_play_result('1');
        // starting_price_ = ask_pips;
        hs_.set_int_var(s_starting_price, ask_pips);
    }
    else if (hs_.get_int_var(s_starting_price) - bid_pips >= pips_limit_)
    {
        update_play_result('0');
        // starting_price_ = ask_pips;
        hs_.set_int_var(s_starting_price, ask_pips);
    }
}

virtual_player::advice virtual_player::give_advice(void) const
{
    std::string play_results = hs_.get_string_var(s_play_results);

    if (play_results.length() < 3)
    {
        return advice::DO_NOTHING;
    }

    if (play_results[0] == '0' && play_results[1] == '0' && play_results[2] == '0')
    {
        return advice::PLAY_LONG;
    }
    else if (play_results[0] == '1' && play_results[1] == '1' && play_results[2] == '1')
    {
        return advice::PLAY_SHORT;
    }

    return advice::DO_NOTHING;
}

void virtual_player::update_play_result(char result)
{
    std::string play_results = hs_.get_string_var(s_play_results);

    if (play_results.length() < 3)
    {
        play_results.push_back(result);
    }
    else
    {
        play_results[0] = play_results[1];
        play_results[1] = play_results[2];
        play_results[2] = result;
    }

    hs_.set_string_var(s_play_results, play_results);
}
