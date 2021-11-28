#include <iostream>
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

//----------------------------------------------------------------------
#include <boost/date_time/posix_time/posix_time.hpp>
//----------------------------------------------------------------------

int days_elapsed(boost::posix_time::ptime begin, boost::posix_time::ptime end)
{
    return (end.date() - begin.date()).days();
}

int draft_main(int argc, char *argv[])
{
/*
   std::string ts("2002-01-20 23:59:59.000");
   ptime t(time_from_string(ts))
*/
    boost::posix_time::ptime begin = boost::posix_time::ptime(boost::posix_time::time_from_string("2021-07-13 23:59:58.000"));
    boost::posix_time::ptime end   = boost::posix_time::ptime(boost::posix_time::time_from_string("2021-07-15 00:00:09.000"));

    std::cout << "Upłynęło dni: " << days_elapsed(begin, end) << "\n";

    return 0;
}
