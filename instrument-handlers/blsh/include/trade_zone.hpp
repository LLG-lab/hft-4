#ifndef __TRADE_ZONE_HPP__
#define __TRADE_ZONE_HPP__

#include <list>
#include <map>
#include <chrono>
#include <boost/date_time/posix_time/posix_time.hpp>

enum class zone
{
    UNKNOWN,
    CPZ,
    HZ,
    NZ,
    LZ,
    CZ
};

class trade_zone
{
public:

    trade_zone(void) = delete;
    trade_zone(trade_zone &) = delete;
    trade_zone(trade_zone &&) = delete;
    trade_zone(int history_size);

    void tick(int ask_pips, int bid_pips, boost::posix_time::ptime pt);

    zone get_zone(int ask_pips, int bid_pips) const;

private:

    struct counter
    {
        counter(void) : c_ {0} {}
        counter &operator++(void) { ++c_; return *this; }
        counter &operator--(void) { if (c_) --c_; return *this; }
        operator int(void) const { return c_;}
        int get(void) const { return c_; }
    private:
        int c_;
    };

    const int history_size_;
    int last_minute_;

    int cpzl_;
    int hzl_;
    int nzl_;
    int lzl_;
    int czl_;

    std::list<int> history_;
    std::map<int, counter> range_;
};

#endif /* __TRADE_ZONE_HPP__ */
