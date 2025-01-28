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

#if 0
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
#endif

double money_management::get_number_of_lots(double balance)
{
    int result;
    double temp;

    int remnant = remnant_svr_.get<int>();

    switch (mode_)
    {
        case mm_mode::MMM_FLAT:
        {
            result = 100.0*number_of_lots_;
            temp = 100.0*number_of_lots_ - result;
            break;
        }
        case mm_mode::MMM_PROGRESSIVE:
        {
            result = balance * slope_;
            temp = balance * slope_ - result;
            break;
        }
        default:
            return 0.0;
    }

    remnant += (100*temp);
    result += (remnant/100);
    remnant %= 100;
    remnant_svr_.set(remnant);

    return static_cast<double>(result) / 100.0;
}
