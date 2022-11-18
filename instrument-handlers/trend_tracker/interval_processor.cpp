#include <interval_processor.hpp>

#include <easylogging++.h>

#define hft_log(__X__) \
    CLOG(__X__, logger_id_.c_str())

namespace {
    const int min_probe = 1000;
}

interval_processor::interval_processor(exchange_rates_collector &erc, invalidable *host_obj,
                                           interval_t interval, int depth, int pips_limit,
                                               const std::string &logger_id, const std::string &work_dir,
                                                   double probab_threshold)
    : invalidable {false}, erc_ {erc}, host_obj_ {host_obj},
      interval_ {interval}, depth_ {depth}, pips_limit_ {pips_limit},
      logger_id_ {logger_id}, work_dir_ {work_dir}, probab_threshold_ {probab_threshold},
      last_investment_advice_ {}, long_game_player_ {pips_limit},
      short_game_player_ {pips_limit}
{
    load_data();
}

void interval_processor::tick(int ask_pips, int bid_pips)
{
    params p = erc_.get_params(interval_, depth_);

    if (! p.valid_)
    {
        return;
    }

    switch (long_game_player_.get_status())
    {
        case game_player_status::E_IDLE:
            long_game_player_.new_game(p.a_, p.delta_, ask_pips);
            break;
        case game_player_status::E_PENDING:
            long_game_player_.tick(ask_pips, bid_pips);
            break;
        case game_player_status::E_COMPLETED:
            long_games_.push_back(long_game_player_.get_game_result());
            long_game_player_.new_game(p.a_, p.delta_, ask_pips);
            invalidate();
            host_obj_ -> invalidate();
            save_long_games();
            break;
    }

    switch (short_game_player_.get_status())
    {
        case game_player_status::E_IDLE:
            short_game_player_.new_game(p.a_, p.delta_, bid_pips);
            break;
        case game_player_status::E_PENDING:
            short_game_player_.tick(ask_pips, bid_pips);
            break;
        case game_player_status::E_COMPLETED:
            short_games_.push_back(short_game_player_.get_game_result());
            short_game_player_.new_game(p.a_, p.delta_, bid_pips);
            invalidate();
            host_obj_ -> invalidate();
            save_short_games();
            break;
    }
}

investment_advice_ext interval_processor::get_advice(void)
{
    if (am_i_valid())
    {
        return last_investment_advice_;
    }

    last_investment_advice_ = process();

    validate();

    return last_investment_advice_;
}

void interval_processor::load_data(void)
{
    // NOT IMPLEMENTED.
}

void interval_processor::save_long_games(void)
{
    // NOT IMPLEMENTED.
}

void interval_processor::save_short_games(void)
{
    // NOT IMPLEMENTED.
}

investment_advice_ext interval_processor::process(void)
{
    investment_advice_ext result;
    double long_interest = 0.0;
    double short_interest = 0.0;
    double max_a;
    double max_delta;

    params p = erc_.get_params(interval_, depth_);

    max_a = p.a_;
    max_delta = p.delta_;

    if (! p.valid_)
    {
        return result;
    }

    if (long_games_.size() >= min_probe)
    {
        for (auto &g : long_games_)
        {
            if (g.a_ > max_a) max_a = g.a_;
            if (g.delta_ > max_delta) max_delta = g.delta_;
        }

        long_interest = estimate_probability(p, long_games_, max_a, max_delta);
    }

    if (short_games_.size() >= min_probe)
    {
        for (auto &g : short_games_)
        {
            if (g.a_ > max_a) max_a = g.a_;
            if (g.delta_ > max_delta) max_delta = g.delta_;
        }

        short_interest = estimate_probability(p, short_games_, max_a, max_delta);
    }

    if (long_interest > short_interest && long_interest > probab_threshold_)
    {
        result.decision = decision_t::E_LONG;
        result.pips_limit = pips_limit_;
        result.interest = long_interest;
    }

    if (short_interest > long_interest && short_interest > probab_threshold_)
    {
        result.decision = decision_t::E_SHORT;
        result.pips_limit = pips_limit_;
        result.interest = short_interest;
    }

    return result;
}

double interval_processor::estimate_probability(const params &p, const std::vector<game> &data, double max_a, double max_delta)
{
    double min_radius = 0;
    double max_radius = sqrt(2.0);
    double radius;
    int n, k;

    while ((max_radius - min_radius) > 0.00001)
    {
        n = k = 0;
        radius = (min_radius + max_radius) / 2.0;

        for (auto &item : data)
        {
            if (sqrt(pow(p.a_/max_a - item.a_/max_a, 2) + pow(p.delta_/max_delta - item.delta_/max_delta, 2)) <= radius)
            {
                n++;
                if (item.result_) k++;
            }
        }

        if (n == min_probe)
        {
            break;
        }
        else if (n > min_probe)
        {
            max_radius = radius;
        }
        else if (n < min_probe)
        {
            min_radius = radius;
        }
    }

    return static_cast<double>(k) / static_cast<double>(n);
}
