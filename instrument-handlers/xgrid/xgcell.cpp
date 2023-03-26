#include <xgcell.hpp>

xgcell::xgcell(int start_price_pips, int pips_span, type cell_type,
                   int dayswap_per_lot_pips, bool &changed_state,
                       const std::string &logger_id, double basic_rate,
                           int base_rate_multiplicity, xgcell *precedessor)
    : lo_limit_ {start_price_pips},
      hi_limit_ {start_price_pips + pips_span},
      dayswap_per_lot_pips_ {dayswap_per_lot_pips},
      basic_rate_ {basic_rate},
      base_rate_multiplicity_ {base_rate_multiplicity},
      precedessor_ {precedessor},
      cell_type_ {cell_type},
      id_ {make_xgcell_label(start_price_pips, pips_span)},
      logger_id_ {logger_id},
      changed_state_ {changed_state}
{
}

//
// Positions query & manip.
//

bool xgcell::has_position(const std::string &position_id) const
{
    for (auto &p : positions_)
    {
        if (p.get_id() == position_id)
        {
            return true;
        }
    }

    return false;
}

void xgcell::confirm_position(const std::string &position_id)
{
    for (auto &p : positions_)
    {
        if (p.get_id() == position_id && !p.is_confirmed()
        {
            p.confirm();
            changed_state_ = true;

            return;
        }
    }
}

void xgcell::remove_unconfirmed(void)
{
    std::list<position>::iterator it = positions_.begin();

    while (it != positions_.end())
    {
        if (! it -> is_confirmed())
        {
            it = positions_.erase(it);
            changed_state_ = true;
        }
        else
        {
            it++;
        }
    }
}

//
// Main operational method. Return value is ‘true’,
// when market operation has been performed.
//

bool xgcell::proceed(int ask_pips, int bid_pips, unsigned long timestamp,
                         hft::protocol::response &market)
{
}

std::string xgcell::make_xgcell_label(int start_price_pips, int pips_span)
{
    return std::string("xgc") + std::to_string(start_price_pips)
           + std::string("s") + std::to_string(pips_span);
}
