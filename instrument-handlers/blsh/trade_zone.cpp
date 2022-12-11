#include <trade_zone.hpp>

trade_zone::trade_zone(int history_size)
    : history_size_ {history_size},
      last_minute_ {0},
      cpzl_ {0},
      hzl_ {0},
      nzl_ {0},
      lzl_ {0},
      czl_ {0}
{
}

void trade_zone::tick(int ask_pips, int bid_pips, boost::posix_time::ptime pt)
{
    boost::posix_time::time_duration td = pt.time_of_day();
    int now_minute = td.minutes();

    if (now_minute == last_minute_)
    {
        return;
    }

    last_minute_ = now_minute;

    int price = (ask_pips + bid_pips) >> 1;

    if (history_.size() < history_size_)
    {
        history_.push_back(price);
        ++range_[price];
    }
    else
    {
        int elapsed_price = history_.front();
        history_.pop_front();
        --range_[elapsed_price];
        history_.push_back(price);
        ++range_[price];

        //
        // Recalculate ranges.
        //

        bool calculated_cpzl_ = false;
        bool calculated_hzl_ = false;
        bool calculated_nzl_ = false;
        bool calculated_lzl_ = false;
        bool calculated_czl_ = false;

        int sum = 0;

        std::map<int, counter>::const_iterator it = range_.begin();

        while (it != range_.end())
        {
            sum += it -> second.get();

            if (static_cast<double>(sum) / history_size_ >= 0.01)
            {
                czl_ = it -> first;
                calculated_czl_ = true;

                it++;

                break;
            }

            it++;
        }

        sum = 0;

        while (it != range_.end())
        {
            sum += it -> second.get();

            if (static_cast<double>(sum) / history_size_ >= 0.04)
            {
                lzl_ = it -> first;
                calculated_lzl_ = true;

                it++;

                break;
            }

            it++;
        }

        sum = 0;

        while (it != range_.end())
        {
            sum += it -> second.get();

            if (static_cast<double>(sum) / history_size_ >= 0.2)
            {
                nzl_ = it -> first;
                calculated_nzl_ = true;

                it++;

                break;
            }

            it++;
        }

        sum = 0;

        while (it != range_.end())
        {
            sum += it -> second.get();

            if (static_cast<double>(sum) / history_size_ >= 0.5)
            {
                hzl_ = it -> first;
                calculated_hzl_ = true;

                it++;

                break;
            }

            it++;
        }

        sum = 0;

        while (it != range_.end())
        {
            sum += it -> second.get();

            if (static_cast<double>(sum) / history_size_ >= 0.2)
            {
                cpzl_ = it -> first;
                calculated_cpzl_ = true;

                it++;

                break;
            }

            it++;
        }

        bool calculated = calculated_cpzl_ && calculated_hzl_ &&
                          calculated_nzl_ && calculated_lzl_ &&
                          calculated_czl_;

        if (! calculated)
        {
            throw std::runtime_error("trade_zone: Miscalculated ranges");
        }

    }
}

zone trade_zone::get_zone(int ask_pips, int bid_pips) const
{
    if (history_.size() < history_size_)
    {
        return zone::UNKNOWN;
    }

    int price = (ask_pips + bid_pips) >> 1;

    if (price < czl_)  return zone::CPZ;
    if (price < lzl_)  return zone::CZ;
    if (price < nzl_)  return zone::LZ;
    if (price < hzl_)  return zone::NZ;
    if (price < cpzl_) return zone::HZ;

    return zone::CPZ;
}
