#ifndef __STRATEGINC_ENGINE_HPP__
#define __STRATEGINC_ENGINE_HPP__

#include <vector>
#include <memory>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <exchange_rates_collector.hpp>
#include <interval_processor.hpp>

class strategic_engine : public invalidable
{
public:

    strategic_engine(void) = delete;
    strategic_engine(strategic_engine &) = delete;
    strategic_engine(strategic_engine &&) = delete;

    strategic_engine(const std::string &logger_id, const std::string &work_dir, double probab_threshold);

    void configure_processors(interval_t interval, const std::vector<int> &depths, const std::vector<int> &pips_limits);

    investment_advice get_advice(void);

    void tick(int ask_pips, int bid_pips, boost::posix_time::ptime pt);

private:

    const std::string logger_id_;
    const std::string work_dir_;
    const double probab_threshold_;

    investment_advice last_investment_advice_;
    exchange_rates_collector erc_;

    std::vector<std::shared_ptr<interval_processor>> processors_;
};

#endif /* __STRATEGINC_ENGINE_HPP__ */
