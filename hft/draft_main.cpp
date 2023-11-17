#include <vector>
#include <cmath>
#include <iostream>
#include <csv_data_supplier.hpp>
#include <utilities.hpp>

class counter
{
public:

    counter(void) = delete;
    counter(int range)
        : range_ {range}, empty_ {true}, sum_ {0}, reference_ {0}
    {}

    int get_range(void) const { return range_; }
    int get_sum(void) const { return sum_; }

    void put_data(int ask_pips)
    {
        if (empty_)
        {
            reference_ = ask_pips;
            empty_ = false;
        }
        else
        {
            if (abs(ask_pips - reference_) >= range_)
            {
                sum_++;
                reference_ = ask_pips;
            }
        }
    }

private:

    const int range_;
    bool empty_;
    int sum_;
    int reference_;
};

// parametry programu
// Z Tczewa:
// https://www.roksa.sx/listing/mam-ochote/
std::string csv_file_name = "/home/ryszard/duperele/serie-fi/USDCHF_weeks_420_461.csv"; //USDCHF_weeks_1_458.csv";
char pip_position = '4';
int min_range = 5;
int max_range = 300;
//////////////////////

int draft_main(int argc, char *argv[])
{
    std::vector<counter> counters;

    for (int r = min_range; r <= max_range; r++)
    {
        counters.emplace_back(r);
    }

    csv_data_supplier faucet(csv_file_name);
    csv_data_supplier::csv_record rec;
    int ask_pips;

    while (faucet.get_record(rec))
    {
        ask_pips = hft::utils::floating2pips(rec.ask, pip_position);

        for (auto &item : counters)
        {
            item.put_data(ask_pips);
        }
    }

    std::cout << "int omega_fun(int x)\n";
    std::cout << "{\n";
    std::cout << "    switch (x)\n";
    std::cout << "    {\n";
    for (auto &item : counters)
    {
        std::cout << "        case " << item.get_range() << ": return " << item.get_sum() << ";\n";
    }
    std::cout << "        default:\n";
    std::cout << "            return 0;\n";
    std::cout << "    }\n";
    std::cout << "}\n";
    
    return 0;
}
