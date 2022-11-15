#ifndef __EXCHANGE_RATES_COLLECTOR_HPP__
#define __EXCHANGE_RATES_COLLECTOR_HPP__

#include <vector>
#include <map>
#include <set>
#include <chrono>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <interval_type.hpp>
#include <invalidable.hpp>

struct params
{
    params(double a, double delta)
        : a_ {a}, delta_ {delta}, valid_ {true}
    {}

    params(void)
        : a_ {0.0}, delta_ {0.0}, valid_ {false}
    {}

    double a_;
    double delta_;
    bool valid_;
};

class exchange_rates_collector
{
public:

    exchange_rates_collector(void) = delete;
    exchange_rates_collector(exchange_rates_collector &) = delete;
    exchange_rates_collector(exchange_rates_collector &&) = delete;

    exchange_rates_collector(const std::string &logger_id, const std::string &work_dir);
    ~exchange_rates_collector(void);

    void tick(int ask_pips, int bid_pips, boost::posix_time::ptime pt);

    params get_params(interval_t interval, int depth);

    void register_invalidable(interval_t interval, invalidable *obj);

private:

    void load_data(void);
    void save_data(void);
    void save_data(interval_t interval);

    const std::string logger_id_;
    const std::string work_dir_;

    int last_hour_;
    int last_minute_;

    void on_interval(interval_t interval, int ask_pips, int bid_pips);
    params calculate_params(interval_t interval, int depth);

    std::map<interval_t, std::vector<int>> exchange_rates_;
    std::map<interval_t, std::set<invalidable *>> observers_;
    std::map<interval_t, std::map<int /*depth*/, params>> parameters_;
};

#endif /* __EXCHANGE_RATES_COLLECTOR_HPP__ */
