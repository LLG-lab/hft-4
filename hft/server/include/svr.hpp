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

#ifndef __SVR_HPP__
#define __SVR_HPP__

#include <string>
#include <cstring>

//
// Class svr – Session Variable Reference.
//

class svr
{
public:

    svr(void)
        : changed_ {nullptr}, value_ {nullptr} {}
    svr(bool &changed, std::string &v)
        : changed_ {&changed}, value_ {&v} {}

    template <typename T>
    T get(void) const;

    template <typename T>
    void set(T v);

private:

    bool *changed_;
    std::string *value_;
};

#endif /* __SVR_HPP__ */
