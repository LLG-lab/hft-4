/**********************************************************************\
**                                                                    **
**             -=≡≣ High Frequency Trading System  ≣≡=-              **
**                                                                    **
**          Copyright  2017 - 2021 by LLG Ryszard Gradowski          **
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

#include <hft_handler_resource.hpp>
#include <utilities.hpp>

#include <sstream>
#include <iomanip>
#include <string>
#include <cstdint>

#include <boost/lexical_cast.hpp>
#include <boost/tokenizer.hpp>

#include <easylogging++.h>

#define hft_log(__X__) \
    CLOG(__X__, logger_id_.c_str())

typedef boost::tokenizer<boost::char_separator<char> >  tokenizer;

hft_handler_resource::autosaver::~autosaver(void)
{
    try
    {
       hhr_.save();
    }
    catch (const std::runtime_error &e)
    {
       CLOG(ERROR, hhr_.logger_id_.c_str())
            << "hft_handler_resource: Failed to save file: "
            << e.what();
    }
}

hft_handler_resource::hft_handler_resource(const std::string &file_name, const std::string &logger_id)
    : initialized_(false), changed_(false),
      file_name_(file_name), logger_id_(logger_id)
{
    el::Loggers::getLogger(logger_id_.c_str(), true);
}

hft_handler_resource::~hft_handler_resource(void)
{
    try
    {
        save();
    }
    catch (const std::runtime_error &e)
    {
       hft_log(ERROR) << "hft_handler_resource: Failed to save file: "
                      << e.what();
    }
}

void hft_handler_resource::persistent(void)
{
    std::string data;

    try
    {
        data = hft::utils::file_get_contents(file_name_);
    }
    catch (const std::runtime_error &e)
    {
        hft_log(WARNING) << "handler_resource: Unalbe to load file ‘"
                         << file_name_ << "’.";

        initialized_ = true;
        return;
    }

    //
    // Parse data.
    //

    boost::char_separator<char> sep("\n", "", boost::drop_empty_tokens);
    tokenizer tokens(data, sep);

    for (auto it = tokens.begin(); it != tokens.end(); ++it)
    {
        process_line(*it);
    }
}

void hft_handler_resource::save(void)
{
    if (initialized_ && changed_)
    {
        std::stringstream ss;

        for (auto &it : ints_)
        {
            ss << "i;" << it.first << ';' << it.second << "\n";
        }

        for (auto &it : bools_)
        {
            ss << "b;" << it.first << ';' << it.second << "\n";
        }

        for (auto &it : doubles_)
        {
            ss << "d;" << it.first << ';' << it.second << "\n";
        }

        for (auto &it : strings_)
        {
            ss << "s;" << it.first << ';';

            if (it.second.length() == 0)
            {
                ss << "(empty)\n";
            }
            else
            {
                ss << hft_handler_resource::string_to_hex(it.second) << "\n";
            }
        }

        std::string payload = ss.str();

        hft::utils::file_put_contents(file_name_, payload);

        changed_ = false;
    }
}

void hft_handler_resource::set_int_var(const std::string &var_name, int value)
{
    ints_[var_name] = value;
    changed_ = true;
}

void hft_handler_resource::set_bool_var(const std::string &var_name, bool value)
{
    bools_[var_name] = value;
    changed_ = true;
}

void hft_handler_resource::set_double_var(const std::string &var_name, double value)
{
    doubles_[var_name] = value;
    changed_ = true;
}

void hft_handler_resource::set_string_var(const std::string &var_name, const std::string &value)
{
    strings_[var_name] = value;
    changed_ = true;
}

int hft_handler_resource::get_int_var(const std::string &var_name) const
{
    auto x = ints_.find(var_name);

    if (x == ints_.end())
    {
        std::string msg = "hft_handler_resource: Undefined variable ‘" + var_name + "’";

        throw std::runtime_error(msg);
    }

    return x -> second;
}

bool hft_handler_resource::get_bool_var(const std::string &var_name) const
{
    auto x = bools_.find(var_name);

    if (x == bools_.end())
    {
        std::string msg = "hft_handler_resource: Undefined variable ‘" + var_name + "’";

        throw std::runtime_error(msg);
    }

    return x -> second;
}

double hft_handler_resource::get_double_var(const std::string &var_name) const
{
    auto x = doubles_.find(var_name);

    if (x == doubles_.end())
    {
        std::string msg = "hft_handler_resource: Undefined variable ‘" + var_name + "’";

        throw std::runtime_error(msg);
    }

    return x -> second;
}

std::string hft_handler_resource::get_string_var(const std::string &var_name) const
{
    auto x = strings_.find(var_name);

    if (x == strings_.end())
    {
        std::string msg = "hft_handler_resource: Undefined variable ‘" + var_name + "’";

        throw std::runtime_error(msg);
    }

    return x -> second;
}

void hft_handler_resource::process_line(const std::string &line)
{
    boost::char_separator<char> sep(";", "", boost::drop_empty_tokens);
    tokenizer tokens(line, sep);

    auto it = tokens.begin();

    if (it == tokens.end())
    {
        std::string msg = "hft_handler_resource: Invalid line ‘" + line + "’";
        throw std::runtime_error(msg);
    }

    std::string type = *it;
    it++;

    if (it == tokens.end())
    {
        std::string msg = "hft_handler_resource: Invalid line ‘" + line + "’";
        throw std::runtime_error(msg);
    }

    std::string var_name = *it;
    it++;

    if (it == tokens.end())
    {
        std::string msg = "hft_handler_resource: Invalid line ‘" + line + "’";
        throw std::runtime_error(msg);
    }

    std::string var_value = *it;

    if (type == "i")
    {
        ints_[var_name] = boost::lexical_cast<int>(var_value);

        hft_log(INFO) << "handler_resource: Restored integer "
                      << var_name << " → " << ints_[var_name]
                      << ".";
    }
    else if (type == "b")
    {
        bools_[var_name] = boost::lexical_cast<bool>(var_value);

        hft_log(INFO) << "handler_resource: Restored boolean "
                      << var_name << " → " << ints_[var_name]
                      << ".";
    }
    else if (type == "d")
    {
        doubles_[var_name] = boost::lexical_cast<double>(var_value);

        hft_log(INFO) << "handler_resource: Restored floating point "
                      << var_name << " → " << ints_[var_name]
                      << ".";
    }
    else if (type == "s")
    {
        if (var_value == "(empty)")
        {
            hft_log(INFO) << "handler_resource: Restored string "
                          << var_name << " → (empty string).";

            strings_[var_name] = "";
        }
        else
        {
            strings_[var_name] = hex_to_string(var_value);

            hft_log(INFO) << "handler_resource: Restored string "
                          << var_name << " → " << ints_[var_name]
                          << ".";
        }
    }
    else
    {
        std::string msg = "hft_handler_resource: Illegal type ‘" + type + "’";
        throw std::runtime_error(msg);
    }
}

std::string hft_handler_resource::string_to_hex(const std::string &in)
{
    std::stringstream ss;

    ss << std::hex << std::setfill('0');

    for (size_t i = 0; in.length() > i; ++i)
    {
        ss << std::setw(2) << static_cast<unsigned int>(static_cast<unsigned char>(in[i]));
    }

    return ss.str(); 
}

std::string hft_handler_resource::hex_to_string(const std::string &in)
{
    std::string output;

    if ((in.length() % 2) != 0)
    {
        throw std::runtime_error("string is not valid length");
    }

    size_t cnt = in.length() / 2;

    for (size_t i = 0; cnt > i; ++i)
    {
       uint32_t s = 0;
       std::stringstream ss;

       ss << std::hex << in.substr(i * 2, 2);
       ss >> s;

       output.push_back(static_cast<unsigned char>(s));
    }

    return output;
}
