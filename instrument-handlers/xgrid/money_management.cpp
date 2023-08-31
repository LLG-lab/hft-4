#include <money_management.hpp>

money_management::money_management(mm_flat_initializer &mmfi, hft_handler_resource &hs)
    : mode_ {mm_mode::MMM_FLAT},
      hs_ {hs},
      number_of_lots_ {mmfi.number_of_lots_},
      slope_ {0.0}
{
}

money_management::money_management(mm_progressive_initializer &mmpi, hft_handler_resource &hs)
    : mode_ {mm_mode::MMM_PROGRESSIVE},
      hs_ {hs},
      number_of_lots_ {0.0},
      slope_ {mmpi.slope_}
{
    hs_.set_int_var("money_management.remnant", 0);
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
            auto as = hs_.create_autosaver();

            int remnant = hs_.get_int_var("money_management.remnant");
            int result = balance * slope_;
            double temp = balance * slope_ - result;
            remnant += (100*temp);
            result += (remnant/100);
            remnant %= 100;
            hs_.set_int_var("money_management.remnant", remnant);

            return static_cast<double>(result) / 100.0;
        }
    }

    return 0.0;
}
