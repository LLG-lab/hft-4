#ifndef __INTERVAL_PROCESSOR_HPP__
#define __INTERVAL_PROCESSOR_HPP__

#include <exchange_rates_collector.hpp>
#include <advice_types.hpp>
#include <game.hpp>

class interval_processor : public invalidable
{
public:

    interval_processor(void) = delete;
    interval_processor(interval_processor &) = delete;
    interval_processor(interval_processor &&) = delete;

    interval_processor(exchange_rates_collector &erc, invalidable *host_obj,
                           interval_t interval, int depth, int pips_limit,
                               const std::string &logger_id, const std::string &work_dir,
                                   double probab_threshold);

    void tick(int ask_pips, int bid_pips);

    investment_advice_ext get_advice(void);

private:

    void load_data(void);
    void save_long_games(void);
    void save_short_games(void);

    investment_advice_ext process(void);
    double estimate_probability(const params &p, const std::vector<game> &data, double max_a, double max_delta);

    exchange_rates_collector &erc_;
    invalidable *host_obj_;
    const interval_t interval_;
    const int depth_;
    const int pips_limit_;
    const std::string logger_id_;
    const std::string work_dir_;
    const double probab_threshold_;
    investment_advice_ext last_investment_advice_;

    std::vector<game> long_games_;
    std::vector<game> short_games_;

    long_game_player long_game_player_;
    short_game_player short_game_player_;
};

#endif /* __INTERVAL_PROCESSOR_HPP__ */
