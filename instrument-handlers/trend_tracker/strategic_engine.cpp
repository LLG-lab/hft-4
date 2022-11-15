#include <strategic_engine.hpp>

strategic_engine::strategic_engine(const std::string &logger_id, const std::string &work_dir, double probab_threshold)
    : invalidable {false}, logger_id_ {logger_id}, work_dir_ {work_dir},
      probab_threshold_ {probab_threshold}, erc_ {logger_id, work_dir}
{
    // NOT IMPLEMENTED.
}

void strategic_engine::configure_processors(interval_t interval, const std::vector<int> &depths, const std::vector<int> &pips_limits)
{
    erc_.register_invalidable(interval, this);

    for (int depth : depths)
    {
        for (int pips_limit : pips_limits)
        {
            interval_processor *p = new interval_processor(erc_, this, interval, depth, pips_limit, logger_id_, work_dir_, probab_threshold_);
            erc_.register_invalidable(interval, p);
            processors_.emplace_back(p);
        }
    }
}

investment_advice strategic_engine::get_advice(void)
{
    if (am_i_valid())
    {
        return last_investment_advice_;
    }

    investment_advice_ext result, temp;
    double max_interest = 0.0;

    for (auto &p : processors_)
    {
        temp = p -> get_advice();

        if (temp.interest > result.interest)
        {
            result = temp;
        }
    }

    last_investment_advice_ = result;

    validate();

    return result;
}

void strategic_engine::tick(int ask_pips, int bid_pips, boost::posix_time::ptime pt)
{
    erc_.tick(ask_pips, bid_pips, pt);

    for (auto &p : processors_)
    {
        p -> tick(ask_pips, bid_pips);
    }
}
