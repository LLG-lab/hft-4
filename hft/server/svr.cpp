/**********************************************************************\
**                                                                    **
**             -=≡≣ High Frequency Trading System ® ≣≡=-              **
**                                                                    **
**          Copyright © 2017 - 2023 by LLG Ryszard Gradowski          **
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

#include <svr.hpp>

template<> std::string svr::get<std::string>() const
{
    return *value_;
}

template<> int svr::get<int>() const
{
    if (value_ -> empty())
    {
        return 0;
    }

    return std::stoi(*value_);
}

template<> double svr::get<double>() const
{
    if (value_ -> empty())
    {
        return 0.0;
    }

    return std::stod(*value_);
}

template<> bool svr::get<bool>() const
{
    if (*value_ == "true" || *value_ == "TRUE" || *value_ == "True" || *value_ == "1")
    {
        return true;
    }

    return false;
}

template<> void svr::set<std::string>(std::string v)
{
    if (v != *value_)
    {
        *changed_ = true;
        *value_ = v;
    }
}

template<> void svr::set<char const *>(char const *v)
{
    if (strcmp(v, value_ -> c_str()) != 0)
    {
        *changed_ = true;
        *value_ = v;
    }
}

template<> void svr::set<int>(int v)
{
    std::string x = std::to_string(v);

    if (x != *value_)
    {
        *changed_ = true;
        *value_ = x;
    }
}

template<> void svr::set<double>(double v)
{
    std::string x = std::to_string(v);

    if (x != *value_)
    {
        *changed_ = true;
        *value_ = x;
    }
}

template<> void svr::set<bool>(bool v)
{
    std::string x = (v ? "true" : "false");

    if (x != *value_)
    {
        *changed_ = true;
        *value_ = x;
    }
}
