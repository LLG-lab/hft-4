#include <stdexcept>
#include <game.hpp>

void game_player::new_game(double a, double delta, int price_pips)
{
    switch (gp_status_)
    {
        case game_player_status::E_IDLE:
        case game_player_status::E_COMPLETED:
            open_price_pips_ = price_pips;
            current_game_.a_ = a;
            current_game_.delta_ = delta;
            gp_status_ = game_player_status::E_PENDING;
            break;
        default:
            throw std::runtime_error("Game is pending");
    }
}

game game_player::get_game_result(void) const
{
    if (gp_status_ == game_player_status::E_COMPLETED)
    {
        return current_game_;
    }

    throw std::runtime_error("No game");
}

void long_game_player::tick(int ask_pips, int bid_pips)
{
    if (gp_status_ != game_player_status::E_PENDING)
    {
        return;
    }

    if (bid_pips - open_price_pips_ >= pips_limit_)
    {
        // Profit.
        current_game_.result_ = true;
        gp_status_ = game_player_status::E_COMPLETED;
    }
    else if (open_price_pips_ - bid_pips >= pips_limit_)
    {
        // Loss.
        current_game_.result_ = false;
        gp_status_ = game_player_status::E_COMPLETED;
    }
}

void short_game_player::tick(int ask_pips, int bid_pips)
{
    if (gp_status_ != game_player_status::E_PENDING)
    {
        return;
    }

    if (open_price_pips_ - ask_pips >= pips_limit_)
    {
        // Profit.
        current_game_.result_ = true;
        gp_status_ = game_player_status::E_COMPLETED;
    }
    else if (ask_pips - open_price_pips_ >= pips_limit_)
    {
        // Loss.
        current_game_.result_ = false;
        gp_status_ = game_player_status::E_COMPLETED;
    }
}
