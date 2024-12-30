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

#include <deallocator.hpp>

#undef DEALLOCATOR_TEST

#ifdef DEALLOCATOR_TEST
#include <iostream>
#endif

//
// Static member initialization.
//

deallocator::resource deallocator::records_;

void *deallocator::operator new(std::size_t size)
{
    #ifdef DEALLOCATOR_TEST
    std::cout << "deallocator: got called [operator new]\n";
    #endif

    void *obj = ::operator new(size);
    deallocator::records_.register_object(obj);

    return obj;
}

void deallocator::operator delete(void *raw_memory, std::size_t size)
{
    #ifdef DEALLOCATOR_TEST
    std::cout << "deallocator: got called [operator delete("
              << raw_memory << ")]\n";
    #endif

    deallocator::records_.unregister_object(raw_memory);
    ::operator delete(raw_memory);
}

deallocator::resource::~resource(void)
{
    cleanup();
}

void deallocator::resource::cleanup(void)
{
    std::set<void *>::iterator it;

    while (true)
    {
        it = registered_.begin();

        if (it == registered_.end())
        {
            break;
        }

        delete reinterpret_cast<deallocator *>(*it);
    }
}

void deallocator::resource::register_object(void *obj)
{
    #ifdef DEALLOCATOR_TEST
    std::cout << "deallocator::resource: Registering object ["
              << obj << "]\n";
    #endif

    registered_.insert(obj);
}

void deallocator::resource::unregister_object(void *obj)
{
    #ifdef DEALLOCATOR_TEST
    std::cout << "deallocator::resource: Unregistering object ["
              << obj << "]\n";
    #endif

    registered_.erase(obj);
}
