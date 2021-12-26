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

#ifndef __DEALLOCATOR_HPP__
#define __DEALLOCATOR_HPP__

#include <set>

class deallocator
{
public:

    virtual ~deallocator(void) = default;

    static void *operator new(std::size_t size);
    static void operator delete(void *raw_memory, std::size_t size);

    static void cleanup(void) { records_.cleanup(); }

private:

    class resource
    {
    public:
        ~resource(void);
        void cleanup(void);

        void register_object(void *obj);
        void unregister_object(void *obj);

    private:

       std::set<void *> registered_;
    };

    static resource records_;
};

#endif /* __DEALLOCATOR_HPP__ */
