#ifndef __MONEY_MANAGEMENT_HPP__
#define __MONEY_MANAGEMENT_HPP__

#include <svr.hpp>

struct mm_flat_initializer
{
    //
    // Specifies the transaction volume in lots.
    //

    double number_of_lots_;
    svr remnant_svr_;
};

struct mm_progressive_initializer
{
    //
    // Specifies the ratio of microlot volume
    // to total grid margin for that volume.
    //

    double slope_;
    svr remnant_svr_;
};

class money_management
{
public:

    money_management(void) = delete;
    money_management(money_management &) = delete;
    money_management(money_management &&) = delete;

    money_management(mm_flat_initializer &mmfi);
    money_management(mm_progressive_initializer &mmpi);

    ~money_management(void) = default;

    double get_number_of_lots(double balance);

private:

    enum class mm_mode
    {
        MMM_FLAT,
        MMM_PROGRESSIVE
    };

    mm_mode mode_;

    // Mode flat.
    double number_of_lots_;

    // Mode progressive.
    double slope_;

    svr remnant_svr_;
};

#endif /* __MONEY_MANAGEMENT_HPP__ */
