#include <money_management.hpp>

money_management::money_management(mm_flat_initializer &mmfi)
    : mode_ {mm_mode::MMM_FLAT},
      number_of_lots_ {mmfi.number_of_lots_},
      slope_ {0.0}
{
}

money_management::money_management(mm_progressive_initializer &mmpi)
    : mode_ {mm_mode::MMM_PROGRESSIVE},
      number_of_lots_ {0.0},
      slope_ {mmpi.slope_},
      remnant_svr_ {mmpi.remnant_svr_}
{
}

double money_management::get_number_of_lots(double balance)
{
    switch (mode_)
    {
        case mm_mode::MMM_FLAT:
        {
            return number_of_lots_;
        }
        case mm_mode::MMM_PROGRESSIVE:
        {
            int remnant = remnant_svr_.get<int>();
            int result = balance * slope_;
            double temp = balance * slope_ - result;
            remnant += (100*temp);
            result += (remnant/100);
            remnant %= 100;
            remnant_svr_.set(remnant);

            return static_cast<double>(result) / 100.0;
        }
    }

    return 0.0;
}
