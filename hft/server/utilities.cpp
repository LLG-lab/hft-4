/**********************************************************************\
**                                                                    **
**             -=≡≣ High Frequency Trading System ® ≣≡=-              **
**                                                                    **
**          Copyright © 2017 - 2022 by LLG Ryszard Gradowski          **
**                       All Rights Reserved.                         **
**                                                                    **
**  CAUTION! This application is an intellectual propery              **
**           of LLG Ryszard Gradowski. This application as            **
**           well as any part of source code cannot be used,          **
**           modified and distributed by third party person           **
**           without prior written permission issued by               **
**           intellectual property owner.                             **
**                                                                    **
\**********************************************************************/

#include <stdexcept>
#include <cstdlib>
#include <fstream>
#include <chrono>
#include <sstream>

#include <boost/filesystem.hpp>
#include <boost/xpressive/xpressive.hpp>
#include <boost/lexical_cast.hpp>

#include <utilities.hpp>

namespace hft {
namespace utils {

std::string file_get_contents(const std::string &filename)
{
    std::fstream in_stream;
    std::string line, ret;

    in_stream.open(filename, std::fstream::in);

    if (in_stream.fail())
    {
        std::string msg = "Unable to open file: " + filename;

        throw std::runtime_error(msg);
    }

    while (! in_stream.eof())
    {
        std::getline(in_stream, line);
        ret += (line + std::string("\n"));
    }

    in_stream.close();

    return ret;
}

void file_put_contents(const std::string &filename, const std::string &content)
{
    std::ofstream out_stream;
    out_stream.open(filename, std::fstream::out);

    if (out_stream.fail())
    {
        std::string msg = "Unable to open file: " + filename;

        throw std::runtime_error(msg);
    }

    out_stream << content;

    out_stream.close();
}

unsigned long get_current_timestamp(void)
{
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

    return millis;
}

unsigned long ptime2timestamp(const boost::posix_time::ptime &t)
{
    static boost::posix_time::ptime begin = boost::posix_time::ptime(boost::posix_time::time_from_string("1970-01-01 00:00:00.000"));

    return (t - begin).total_milliseconds();
}

std::string find_free_name(const std::string &file_name)
{
    int index = 0;
    std::string candidate;
    std::ostringstream oss;

    auto date = boost::posix_time::second_clock::local_time().date();

    oss << file_name << "-" << date.year()
                     << "-" << date.month()
                     << "-" << date.day();

    std::string backup_file = oss.str();

    while (true)
    {
        candidate = backup_file + std::string("_") + std::to_string(index++);

        if (! boost::filesystem::exists(boost::filesystem::path(candidate)))
        {
            break;
        }
    }

    return candidate;
}

std::string expand_env_variable(const std::string &input)
{
    using namespace boost::xpressive;

    sregex envar = "$(" >> (s1 = +_w) >> ')';

    std::string output = regex_replace(input, envar, [](smatch const &what) -> std::string
    {
        auto p = getenv(what[1].str().c_str());

        if (p == nullptr)
        {
            return std::string("");
        }

        return std::string(p);
    });

    return output;
}

int floating2pips(double price, char pips_digit)
{
    char buffer[15];
    char fmt[] = { '%', '0', '.', pips_digit, 'f', 0 };
    int i = 0, j = 0;

    int status = snprintf(buffer, 15, fmt, price);

    if (status <= 0 || status >= 15)
    {
        std::string msg = std::string("floating2pips: Floating point operation error");

        throw std::runtime_error(msg);
    }

    //
    // Traverse through all string including null character.
    //

    while (i <= status)
    {
        if (buffer[i] != '.')
        {
            buffer[j++] = buffer[i];
        }

        ++i;
    }

    return boost::lexical_cast<int>(buffer);
}


} // namespace utils
} // namespace hft
