/**********************************************************************\
**                                                                    **
**             -=≡≣ High Frequency Trading System ® ≣≡=-              **
**                                                                    **
**          Copyright © 2017 - 2026 by LLG Ryszard Gradowski          **
**                       All Rights Reserved.                         **
**                                                                    **
**  CAUTION! This application is an intellectual property             **
**           of LLG Ryszard Gradowski. This application as            **
**           well as any part of source code cannot be used,          **
**           modified and distributed by third party person           **
**           without prior written permission issued by               **
**           intellectual property owner.                             **
**                                                                    **
\**********************************************************************/

#include <iostream>
#include <map>
#include <list>
#include <utility>

#include <boost/program_options.hpp>

#include <csv_loader.hpp>
#include <hft_instrument_property.hpp>
#include <utilities.hpp>

#include <easylogging++.h>

namespace prog_opts = boost::program_options;

static struct instrument_stats_options_type
{
    std::string config_file_name;
    std::string instrument_and_file;
    std::string csv_file_name;
    std::string instrument;
    unsigned int price_levels_ranges;

} instrument_stats_options;

#define hftOption(__X__) \
    instrument_stats_options.__X__

#define hft_log(__X__) \
    CLOG(__X__, "instrument_stats")

struct counter
{
    counter(void) : c_ {0} {}
    counter &operator++(void) { ++c_; return *this; }
    operator int(void) const { return c_;}
    int get(void) const { return c_; }
private:
    int c_;
};

struct range_counter
{
    range_counter(void) = delete;
    range_counter(int min, int max)
        : c_ {0}, min_ {min}, max_ {max} {}
    bool in_range(int value) { return (min_ <= value && max_ >= value); }
    void add(int value) { c_ += value; }
    int get_counter(void) const { return c_; }
    int get_min(void) const { return min_; }
    int get_max(void) const { return max_; }
private:
    int c_;
    int min_;
    int max_;
};

static std::pair<std::string, std::string> split_into_instrument_and_file(const std::string &arg)
{
    size_t delimiter_index = arg.find_first_of(':');

    if (delimiter_index == std::string::npos)
    {
        std::string err_msg = "Required line <ticker>:<file_name>, no delimiter ‘:’ found in line: ‘"
                              + arg + "’";

        throw std::runtime_error(err_msg.c_str());
    }

    std::string ticker = arg.substr(0, delimiter_index);

    if (ticker.length() == 0)
    {
        std::string err_msg = "No ticker specified in line ‘" + arg + "’";

        throw std::runtime_error(err_msg.c_str());
    }

    std::string file_name = arg.substr(delimiter_index+1, arg.length()-delimiter_index);

    if (file_name.length() == 0)
    {
        std::string err_msg = "No file name specified in line ‘" + arg + "’";

        throw std::runtime_error(err_msg.c_str());
    }

    return std::make_pair(ticker, file_name);
}


int hft_instrument_stats(int argc, char *argv[])
{
    //
    // Define default logger configuration.
    //

    el::Configurations logger_cfg;
    logger_cfg.setToDefault();
    logger_cfg.parseFromText("* GLOBAL:\n"
                             " FORMAT               =  \"%datetime %level [%logger] %msg\"\n"
                             " FILENAME             =  \"/dev/null\"\n"
                             " ENABLED              =  true\n"
                             " TO_FILE              =  false\n"
                             " TO_STANDARD_OUTPUT   =  true\n"
                             " SUBSECOND_PRECISION  =  1\n"
                             " PERFORMANCE_TRACKING =  true\n"
                             " MAX_LOG_FILE_SIZE    =  10485760 ## 10MiB\n"
                             " LOG_FLUSH_THRESHOLD  =  1 ## Flush after every single log\n"
                            );

    el::Loggers::setDefaultConfigurations(logger_cfg);

    START_EASYLOGGINGPP(argc, argv);

    //
    // Parsing options for emulator.
    //

    prog_opts::options_description hidden("Hidden options");
    hidden.add_options()
        ("instrument-stats", "")
    ;

    prog_opts::options_description desc("Options for instrument-stats");
    desc.add_options()
        ("help,h", "produce help message")
        ("config,c", prog_opts::value<std::string>(&hftOption(config_file_name)) -> default_value("/etc/hft/hft-config.xml"), "HFT configuration file name")
        ("price-levels-ranges,p", prog_opts::value<unsigned int>(&hftOption(price_levels_ranges)) -> default_value(10), "Ranges of price levels") /*XXX To nic nie mówi*/
        ("instrument,i", prog_opts::value<std::string>(&hftOption(instrument_and_file)), "<ticker>:<csv_file_name_or_file_name_with_list_of_csv_files>")

    ;

    prog_opts::options_description cmdline_options;
    cmdline_options.add(desc).add(hidden);

    prog_opts::variables_map vm;
    prog_opts::store(prog_opts::command_line_parser(argc, argv).options(cmdline_options).run(), vm);
    prog_opts::notify(vm);

    //
    // If user requested help, show help and quit
    // ignoring other options, if any.
    //

    if (vm.count("help"))
    {
        std::cout << desc << "\n";

        return 0;
    }

    //
    // Setup logging.
    //

    el::Logger *logger = el::Loggers::getLogger("instrument_stats", true);

    if (hftOption(instrument_and_file).size() == 0)
    {
        hft_log(ERROR) << "No instrument specified.";

        return 1;
    }
    else
    {
        auto p = split_into_instrument_and_file(hftOption(instrument_and_file));
        hftOption(instrument) = p.first;
        hftOption(csv_file_name) = p.second;
    }

    csv_loader csv{hftOption(csv_file_name)};
    hft_instrument_property instrument_property{hftOption(instrument), hftOption(config_file_name)};

    std::map<int, counter> spreads;
    std::map<int, counter> courses;
    double ath = 0.0;
    double atl = 10e8;
    double drowdown, max_drowdown = 0.0;

    int ask_pips = 0;
    int bid_pips = 0;
    int spread = 0;
    int avg_course_pips = 0;
    csv_loader::csv_record rec;

    while (csv.get_record(rec))
    {
        if (rec.ask > ath) ath = rec.ask;
        if (rec.bid < atl) atl = rec.bid;

        drowdown = ath - rec.bid;
        if (drowdown > max_drowdown) max_drowdown = drowdown;

        ask_pips = hft::utils::floating2pips(rec.ask, instrument_property.get_pip_significant_digit());
        bid_pips = hft::utils::floating2pips(rec.bid, instrument_property.get_pip_significant_digit());
        spread = ask_pips - bid_pips;
        avg_course_pips = (ask_pips + bid_pips) >> 1;

        ++spreads[spread];
        ++courses[avg_course_pips];
    }

    hft_log(INFO) << "General:";
    hft_log(INFO) << "ATH: " << ath;
    hft_log(INFO) << "ATL: " << atl;

    int all_s = 0;
    for (auto &x : spreads)
    {
        all_s += x.second;
    }

    hft_log(INFO) << "Spreads distribution:";

    int n = 0;
    int percentage = 0;
    for (auto &x : spreads)
    {
        n += x.second;
        percentage = static_cast<int>((1.0 - (static_cast<double>(n) / all_s))*100.0);
        hft_log(INFO) << "  > " << x.first << " pips – " << percentage << "%";

        if (percentage == 0)
        {
            break;
        }
    }

    hft_log(INFO) << "Price levels:";

    int ath_pips = hft::utils::floating2pips(ath, instrument_property.get_pip_significant_digit());
    int atl_pips = hft::utils::floating2pips(atl, instrument_property.get_pip_significant_digit());
    int delta = (ath_pips - atl_pips) / hftOption(price_levels_ranges);
    std::list<range_counter> ranges;
    int all_c = 0;

    for (int i = 0; i < hftOption(price_levels_ranges); i++)
    {
        int max = ath_pips - i*delta;
        int min = ath_pips - (i+1)*delta + 1;
        ranges.emplace_back(min, max);
    }

    for (auto &x : courses)
    {
        for (auto &y : ranges)
        {
            if (y.in_range(x.first))
            {
                y.add(x.second);
                all_c += x.second;
                break;
            }
        }
    }

    for (auto &y : ranges)
    {
        hft_log(INFO) << "  <" << (static_cast<double>(y.get_min()) / pow(10, static_cast<int>(instrument_property.get_pip_significant_digit()) - 48))
                      << "..." << (static_cast<double>(y.get_max()) / pow(10, static_cast<int>(instrument_property.get_pip_significant_digit()) - 48))
                      << "> – " << static_cast<int>((static_cast<double>(y.get_counter()) / all_c) * 100) << "%";
    }

    int sum = 0;
    for (auto &x : courses)
    {
        sum += x.second;

        if (static_cast<double>(sum)/all_c >= 0.5)
        {
            hft_log(INFO) << "Median: " << (static_cast<double>(x.first) / pow(10, static_cast<int>(instrument_property.get_pip_significant_digit()) - 48));
            break;
        }
    }

    hft_log(INFO) << "Max drowdown: " << hft::utils::floating2pips(max_drowdown, instrument_property.get_pip_significant_digit())
                  << " pips.";

    return 0;
}
