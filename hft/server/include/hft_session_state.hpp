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

#ifndef __HFT_SESSION_STATE_HPP__
#define __HFT_SESSION_STATE_HPP__

#include <map>
#include <svr.hpp>

enum class session_mode
{
    VOLATILE,
    PERSISTENT
};

enum class varscope
{
    LOCAL,
    GLOBAL
};

class instrument_handler;

class session_state
{
public:

    class autosaver
    {
    public:

        autosaver(void) = delete;
        autosaver(session_state *pss) : pss_ {pss} {}
        ~autosaver(void) { pss_ -> save(); }

    private:

        session_state *pss_;
    };

    session_state(void) = delete;
    session_state(session_state &) = delete;
    session_state(session_state &&) = delete;
    session_state(const std::string &session_directory);
    ~session_state(void);

    autosaver create_autosaver(void) { return autosaver {this}; }

    session_mode get_mode(void) const { return mode_; }

    std::string get_session_directory(void) const { return session_directory_; }

    svr variable(instrument_handler *ph, const std::string &name, varscope scope);

    void save(void);

private:

    void load(void);

    const std::string state_filename_;
    const std::string session_directory_;
    session_mode mode_;
    bool changed_;

    std::map<std::string, std::map<std::string, std::string>> variables_;
};


#endif /* __HFT_SESSION_STATE_HPP__ */
