#include <exchange_rates_collector.hpp>

#include <easylogging++.h>

#define hft_log(__X__) \
    CLOG(__X__, logger_id_.c_str())

exchange_rates_collector::exchange_rates_collector(const std::string &logger_id, const std::string &work_dir)
    : logger_id_ {logger_id}, work_dir_ {work_dir},
      last_hour_ {0}, last_minute_ {0}
{
    exchange_rates_[interval_t::I_M1] = std::vector<int>();
    exchange_rates_[interval_t::I_M2] = std::vector<int>();
    exchange_rates_[interval_t::I_M5] = std::vector<int>();
    exchange_rates_[interval_t::I_M10] = std::vector<int>();
    exchange_rates_[interval_t::I_M15] = std::vector<int>();
    exchange_rates_[interval_t::I_M20] = std::vector<int>();
    exchange_rates_[interval_t::I_M30] = std::vector<int>();
    exchange_rates_[interval_t::I_H1] = std::vector<int>();
    exchange_rates_[interval_t::I_H2] = std::vector<int>();
    exchange_rates_[interval_t::I_H3] = std::vector<int>();
    exchange_rates_[interval_t::I_H4] = std::vector<int>();
    exchange_rates_[interval_t::I_H6] = std::vector<int>();
    exchange_rates_[interval_t::I_H8] = std::vector<int>();
    exchange_rates_[interval_t::I_H12] = std::vector<int>();

    load_data();
}

exchange_rates_collector::~exchange_rates_collector(void)
{
    try
    {
        save_data();
    }
    catch (const std::exception &e)
    {
        hft_log(ERROR) << "exchange_rates_collector: Unable to save data!";
    }
}

void exchange_rates_collector::tick(int ask_pips, int bid_pips, boost::posix_time::ptime pt)
{
    boost::posix_time::time_duration td = pt.time_of_day();
    int now_hour = td.hours();
    int now_minute = td.minutes();

    if (last_minute_ != now_minute)
    {
        if (now_minute % 30 == 0) { on_interval(interval_t::I_M30, ask_pips, bid_pips); }
        if (now_minute % 20 == 0) { on_interval(interval_t::I_M20, ask_pips, bid_pips); }
        if (now_minute % 15 == 0) { on_interval(interval_t::I_M15, ask_pips, bid_pips); }
        if (now_minute % 10 == 0) { on_interval(interval_t::I_M10, ask_pips, bid_pips); }
        if (now_minute % 5 == 0)  { on_interval(interval_t::I_M5,  ask_pips, bid_pips); }
        if (now_minute % 2 == 0)  { on_interval(interval_t::I_M2,  ask_pips, bid_pips); }
        on_interval(interval_t::I_M1, ask_pips, bid_pips);

        last_minute_ = now_minute;
    }

    if (last_hour_ != now_hour)
    {
        if (now_hour % 12 == 0) { on_interval(interval_t::I_H12, ask_pips, bid_pips); }
        if (now_hour % 8 == 0)  { on_interval(interval_t::I_H8,  ask_pips, bid_pips); }
        if (now_hour % 6 == 0)  { on_interval(interval_t::I_H6,  ask_pips, bid_pips); }
        if (now_hour % 4 == 0)  { on_interval(interval_t::I_H4,  ask_pips, bid_pips); }
        if (now_hour % 3 == 0)  { on_interval(interval_t::I_H3,  ask_pips, bid_pips); }
        if (now_hour % 2 == 0)  { on_interval(interval_t::I_H2,  ask_pips, bid_pips); }
        on_interval(interval_t::I_H1,  ask_pips, bid_pips);

        last_hour_ = now_hour;
    }
}

params exchange_rates_collector::get_params(interval_t interval, int depth)
{

    std::map<interval_t, std::map<int, params>>:: iterator it1;
    it1 = parameters_.find(interval);

    if (it1 != parameters_.end())
    {
        std::map<int, params> &p = it1 -> second;
        std::map<int, params>::iterator it2 = p.find(depth);

        if (it2 == p.end())
        {
            parameters_[interval][depth] = calculate_params(interval, depth);
        }
    }
    else
    {
        parameters_[interval][depth] = calculate_params(interval, depth);
    }

    return parameters_[interval][depth];
}

void exchange_rates_collector::register_invalidable(interval_t interval, invalidable *obj)
{
    observers_[interval].insert(obj);
}

void exchange_rates_collector::load_data(void)
{
    // NOT IMPLEMENTED.
}

void exchange_rates_collector::save_data(void)
{
    interval_t intervals[] = { interval_t::I_M1,  interval_t::I_M2,
                               interval_t::I_M5,  interval_t::I_M10,
                               interval_t::I_M15, interval_t::I_M20,
                               interval_t::I_M30, interval_t::I_H1,
                               interval_t::I_H2,  interval_t::I_H3,
                               interval_t::I_H4,  interval_t::I_H6,
                               interval_t::I_H8,  interval_t::I_H12
                             };

    for (auto i : intervals)
    {
        save_data(i);
    }
}

void exchange_rates_collector::save_data(interval_t interval)
{
    // NOT IMPLEMENTED.
}

void exchange_rates_collector::on_interval(interval_t interval, int ask_pips, int bid_pips)
{
    const int max_store_size = 1000;
    std::vector<int> &store = exchange_rates_[interval];
    int price = (ask_pips + bid_pips) >> 1;

    //
    // Update store & save.
    //

    if (store.size() < max_store_size)
    {
        store.push_back(price);
    }
    else
    {
        for (int i = 1; i < max_store_size; i++)
        {
            store[i - 1] = store[i];
        }

        store[max_store_size - 1] = price;
    }

    save_data(interval);

    //
    // Clear params table.
    //

    std::map<interval_t, std::map<int, params>>:: iterator it1;
    it1 = parameters_.find(interval);

    if (it1 != parameters_.end())
    {
        std::map<int, params> &p = it1 -> second;
        p.clear();
    }

    //
    // Invalidate observers.
    //

    std::map<interval_t, std::set<invalidable *>>::iterator it2;
    it2 = observers_.find(interval);

    if (it2 != observers_.end())
    {
        std::set<invalidable *> &o = it2 -> second;

        for (auto &x : o)
        {
            x -> invalidate();
        }
    }
}

params exchange_rates_collector::calculate_params(interval_t interval, int depth)
{
    std::vector<int> &store = exchange_rates_[interval];

    if (store.size() < depth || depth <= 0)
    {
        return params();
    }

    const int findex = store.size() - depth;
    int x = 0, y = 0, numerator = 0, denominator = 0;

    //
    // Calculate a parameter.
    //

    for (int i = findex; i < store.size(); i++)
    {
        y = store[i] - store[findex];

        numerator += x*y;
        denominator += x*x;

        x++;
    }

    double a = static_cast<double>(numerator) / static_cast<double>(denominator);

    //
    // Calculate delta parameter.
    //


    x = 0;
    double delta = 0.0;
/*
XXX Temporary ignoring delta.

    for (int i = findex; i < store.size(); i++)
    {
        y = store[i] - store[findex];
        delta += pow(a*x - y, 2);
    }

    delta = sqrt((1.0 / depth) * delta);
*/
    delta = 1.0;

    return params(a, delta);
}
