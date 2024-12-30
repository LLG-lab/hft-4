/**********************************************************************\
**                                                                    **
**             -=≡≣ High Frequency Trading System ® ≣≡=-              **
**                                                                    **
**          Copyright © 2017 - 2025 by LLG Ryszard Gradowski          **
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

#ifndef __HFT_HANDLER_RESOURCE__
#define __HFT_HANDLER_RESOURCE__

#include <map>
#include <string>
#include <stdexcept>

class hft_handler_resource
{
friend class autosaver;
public:

    class autosaver
    {
    friend class hft_handler_resource;
    public:
        autosaver(void) = delete;
        ~autosaver(void);

    private:

        autosaver(hft_handler_resource &hhr)
            : hhr_(hhr) {}

        hft_handler_resource &hhr_;
    };

    hft_handler_resource(void) = delete;

    hft_handler_resource(const std::string &file_name, const std::string &logger_id);

    ~hft_handler_resource(void);

    autosaver create_autosaver(void) { return autosaver(*this); }

    void persistent(void);
    void save(void);

    void set_int_var(const std::string &var_name, int value);
    void set_bool_var(const std::string &var_name, bool value);
    void set_double_var(const std::string &var_name, double value);
    void set_string_var(const std::string &var_name, const std::string &value);

    int get_int_var(const std::string &var_name) const;
    bool get_bool_var(const std::string &var_name) const;
    double get_double_var(const std::string &var_name) const;
    std::string get_string_var(const std::string &var_name) const;

private:

    void process_line(const std::string &line);

    static std::string string_to_hex(const std::string &in);
    static std::string hex_to_string(const std::string &in);

    std::map<std::string, int> ints_;
    std::map<std::string, bool> bools_;
    std::map<std::string, double> doubles_;
    std::map<std::string, std::string> strings_;

    bool initialized_;
    bool changed_;
    const std::string file_name_;
    const std::string logger_id_;
};

#endif /* __HFT_HANDLER_RESOURCE__ */
