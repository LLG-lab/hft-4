#include <iostream>
#include <vector>
#include <map>
#include <boost/program_options.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <stdexcept>

#include <utilities.hpp>

//#include <stdexcept>
//#include <boost/json.hpp>
//#include <utilities.hpp>
//#include <list>
//#include <vector>
//#include <memory>

//#include <hft_position.hpp>
//#include <hft_request.hpp>
//#include <hft_response.hpp>
//#include <boost/filesystem.hpp>

//#include <sstream>

/*
//----------------------------------------------------------------------
#include <boost/date_time/posix_time/posix_time.hpp>

int days_elapsed(boost::posix_time::ptime begin, boost::posix_time::ptime end)
{
    return (end.date() - begin.date()).days();
}

void test_days(void)
{
    // std::string ts("2002-01-20 23:59:59.000");
    // ptime t(time_from_string(ts))

    boost::posix_time::ptime begin = boost::posix_time::ptime(boost::posix_time::time_from_string("2021-07-13 23:59:58.000"));
    boost::posix_time::ptime end   = boost::posix_time::ptime(boost::posix_time::time_from_string("2021-07-15 00:00:09.000"));

    std::cout << "Upłynęło dni: " << days_elapsed(begin, end) << "\n";
}
//----------------------------------------------------------------------
*/

namespace prog_opts = boost::program_options;
/*
static struct dukas_emulator_options_type
{
    std::string host;
    std::string port;
    std::vector<std::string> instruments;
    std::string csv_file_names;
    std::string sessid;
    std::string config_file_name;
    int bankroll;
    bool check_bankruptcy;

} dukas_emulator_options;

#define hftOption(__X__) \
    dukas_emulator_options.__X__
*/

static std::map<std::string, std::string> mk_instrument_info_map(const std::vector<std::string> instruments)
{
    std::map<std::string, std::string> result;

    for (auto &x : instruments)
    {
        size_t delimiter_index = x.find_first_of(':');

        if (delimiter_index == std::string::npos)
        {
            std::string err_msg = "Required line <ticker>:<file_name>, no delimiter ‘:’ found in line: ‘"
                                  + x + "’";

            throw std::runtime_error(err_msg.c_str());
        }

        std::string ticker = x.substr(0, delimiter_index);

        if (ticker.length() == 0)
        {
            std::string err_msg = "No ticker specified in line ‘" + x + "’";

            throw std::runtime_error(err_msg.c_str());
        }

        std::string file_name = x.substr(delimiter_index+1, x.length()-delimiter_index);

        if (file_name.length() == 0)
        {
            std::string err_msg = "No file name specified in line ‘" + x + "’";

            throw std::runtime_error(err_msg.c_str());
        }

        if (result.find(ticker) != result.end())
        {
            std::string err_msg = "Duplicate ticker ‘" + ticker + "’";

            throw std::runtime_error(err_msg.c_str());
        }

        result[ticker] = file_name;
    }

    return result;
}

/////////////////////////////////////

unsigned long ptime2timestamp(const boost::posix_time::ptime &t)
{
    static boost::posix_time::ptime begin = boost::posix_time::ptime(boost::posix_time::time_from_string("1970-01-01 00:00:00.000"));

    return (t - begin).total_milliseconds();
}

void test_days(void)
{
    // std::string ts("2002-01-20 23:59:59.000");
    // ptime t(time_from_string(ts))

    boost::posix_time::ptime begin = boost::posix_time::ptime(boost::posix_time::time_from_string("2021-07-13 23:59:58.000"));
    boost::posix_time::ptime end   = boost::posix_time::ptime(boost::posix_time::time_from_string("2021-07-15 00:00:09.000"));

//    std::cout << "Upłynęło dni: " << days_elapsed(begin, end) << "\n";
}


int draft_main(int argc, char *argv[])
{
#if 0
    prog_opts::options_description hidden("Hidden options");
    hidden.add_options()
        ("dukascopy-emulator", "")
    ;

    prog_opts::options_description desc("Options for dukascopy-emulator");
    desc.add_options()
        ("help,h", "produce help message")
        ("host,H", prog_opts::value<std::string>(&hftOption(host)) -> default_value("localhost"), "HFT server hostname or ip address.")
        ("port,P", prog_opts::value<std::string>(&hftOption(port)) -> default_value("8137"), "HFT server listen port.")
        ("instrument,i", prog_opts::value<std::vector<std::string>>(&hftOption(instruments)), "<ticker>:<csv_file_name_or_file_name_with_list_of_csv_files>")
        ("sessid,s", prog_opts::value<std::string>(&hftOption(sessid)) -> default_value("dukascopy-emulator"), "Session ID")
        ("bankroll,b", prog_opts::value<int>(&hftOption(bankroll)) -> default_value(10000), "Initial virtual deposit")
        ("check-bankruptcy,B", prog_opts::value<bool>(&hftOption(check_bankruptcy)) -> default_value(false), "Stop simulation when equity drops to zero")
        ("config,c", prog_opts::value<std::string>(&hftOption(config_file_name)) -> default_value("/etc/hft/hft-config.xml"), "HFT configuration file name")
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

    for (auto &x : hftOption(instruments))
    {
        std::cout << x << "\n";
    }

    std::cout << "------\n";
    auto data = mk_instrument_info_map(hftOption(instruments));

    for (auto &x : data)
    {
        std::cout << "ticker: ‘" << x.first << "’, filename: ‘"
                  << x.second << "’\n";
    }
#endif

    boost::posix_time::ptime today = boost::posix_time::ptime(boost::posix_time::time_from_string("2021-12-10 06:55:00.000"));

    std::cout << ptime2timestamp(today) << "\n";
    std::cout << hft::utils::get_current_timestamp() << "\n";

    return 0;
}
