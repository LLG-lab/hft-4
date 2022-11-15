#ifndef __ADVICE_TYPES_HPP__
#define __ADVICE_TYPES_HPP__

enum class decision_t
{
    E_OUT_OF_MARKET,
    E_LONG,
    E_SHORT
};

struct investment_advice
{
    investment_advice(void)
        : decision {decision_t::E_OUT_OF_MARKET}, pips_limit {0}
    {}

    virtual ~investment_advice(void) {}

    decision_t decision;
    int pips_limit;
};

struct investment_advice_ext : public investment_advice
{
    investment_advice_ext(void)
        : investment_advice {}, interest {0.0}
    {}

    double interest;
};

#endif /* __ADVICE_TYPES_HPP__ */