#include <iostream>
#include <vector>
#include <map>
#include <boost/program_options.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <stdexcept>

#include <utilities.hpp>
#include <easylogging++.h>

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


#define hft_log(__X__) \
    CLOG(__X__, "test")

int draft_main(int argc, char *argv[])
{
    //
    // Define default configuration for all future server loggers.
    //

    el::Configurations logger_cfg;
    logger_cfg.setToDefault();
    logger_cfg.parseFromText("* GLOBAL:\n"
                             " FORMAT               =  \"%datetime %level [%logger] %msg\"\n"
                             " FILENAME             =  \"/var/log/hft/server.log\"\n"
                             " ENABLED              =  true\n"
                             " TO_FILE              =  true\n"
                             " TO_STANDARD_OUTPUT   =  false\n"
                             " SUBSECOND_PRECISION  =  1\n"
                             " PERFORMANCE_TRACKING =  true\n"
                             " MAX_LOG_FILE_SIZE    =  10485760 ## 10MiB\n"
                             " LOG_FLUSH_THRESHOLD  =  1 ## Flush after every single log\n"
                             "* DEBUG:\n"
                             " ENABLED              =  false\n"
                            );
    el::Loggers::setDefaultConfigurations(logger_cfg);

    START_EASYLOGGINGPP(argc, argv);

    //
    // Setup logging.
    //

    el::Logger *logger = el::Loggers::getLogger("test", true);

    hft_log(INFO) << "Testujemy logger w trybie info";
    hft_log(DEBUG) << "Testujemy logger w trybie debug";

    return 0;
}
