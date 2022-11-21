#include <boost/asio.hpp>
#include <ctime>
#include <chrono>
//#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <iostream>
#include <vector>
#include <algorithm>

/*
    boost::asio::io_context ioctx_;
    boost::asio::steady_timer reconnect_timer_(ioctx_);

void foo()
{
    reconnect_timer_.expires_after(boost::asio::chrono::seconds(5));
    reconnect_timer_.async_wait(
        [=](const boost::system::error_code &error)
        {
            if (! error)
            {
                std::cout << "Attempt to connect to cTrader, trial #1\n";

                foo();
//                connect();
            }
            else if (error == boost::asio::error::operation_aborted)
            {
                std::cout << "Attempt to connect to cTrader aborted!\n";
            }
        }
    );

}
*/


struct chunk_info
{
    chunk_info(void)
        : from {0ul}, to {0ul} {}

    unsigned long from;
    unsigned long to;
};

chunk_info make_chunk_info(unsigned long week, unsigned long num_4h_interval)
{
    chunk_info result;

    static boost::posix_time::ptime begin_epoch      = boost::posix_time::ptime(boost::posix_time::time_from_string("1970-01-01 00:00:00.000"));
    static boost::posix_time::ptime begin_first_week = boost::posix_time::ptime(boost::posix_time::time_from_string("2015-01-05 00:00:00.000"));

    result.from = (begin_first_week - begin_epoch).total_milliseconds() + (week-1)*3600*24*7*1000 + (num_4h_interval-1)*3600*4*1000;
    result.to   = (begin_first_week - begin_epoch).total_milliseconds() + (week-1)*3600*24*7*1000 + (num_4h_interval)*3600*4*1000 - 1;

    return result;
}

enum class quote_type
{
    BID = 1,
    ASK = 2
};

struct download_data
{
    download_data(unsigned long t, int p, quote_type q)
        : timestamp {t}, price {p}, quote {q} {}

    bool operator<(const download_data& r)
    {
        return (timestamp < r.timestamp);
    }

    unsigned long timestamp;
    int price;
    quote_type quote;
};

std::string timestamp2string(unsigned long millis)
{
    int fraction = millis % 1000;
    time_t rawtime = millis / 1000;
    struct tm  ts;
    static char buf[20];

    ts = *gmtime(&rawtime);
    strftime(buf, sizeof(buf), "%d.%m.%Y %H:%M:%S", &ts);

    return std::string(buf) + std::string(".") + std::to_string(fraction);
}

int draft_main(int argc, char *argv[])
{
/*
    std::vector<download_data> c;
    c.emplace_back(10, 6, quote_type::ASK);
    c.emplace_back(4, 5, quote_type::BID);
    c.emplace_back(7, 4, quote_type::ASK);
    c.emplace_back(2, 3, quote_type::BID);

    std::sort(c.begin(), c.end());

    for (auto &x : c)
    {
        std::cout << "t=" << x.timestamp << " price=" << x.price << "\n";
    }
*/
/*
t=2 price=3
t=4 price=5
t=7 price=4
t=10 price=6
*/

    chunk_info info;

    for (int i = 1; i<=30; i++)
    {
        info = make_chunk_info(378, i);
        std::cout << timestamp2string(info.from) << " – " << timestamp2string(info.to) << "\n";
    }

    return 0;
}