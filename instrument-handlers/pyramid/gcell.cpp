#include <stdexcept>
#include <gcell.hpp>

gcell::gcell(int pips_span, int gnumber, bool terminal)
    : is_terminal_ { terminal },
      position_id_ { "" },
      position_time_ { 0ul },
      position_price_pips_ { 0 },
      position_confirmed_ { false }
{
    gcell_id_ = std::string("g") + std::to_string(gnumber);

    trade_min_limit_ = gnumber*pips_span;
    trade_max_limit_ = trade_min_limit_ + pips_span;
}

gcell::gcell(int min_limit_pips, int max_limit_pips, int gnumber, bool terminal)
    : is_terminal_ { terminal },
      position_id_ { "" },
      position_time_ { 0ul },
      position_price_pips_ { 0 },
      position_confirmed_ { false }
{
    gcell_id_ = std::string("g") + std::to_string(gnumber);

    trade_min_limit_ = min_limit_pips;
    trade_max_limit_ = max_limit_pips;
}

void gcell::confirm_position(void)
{
    if (has_position())
    {
        if (position_price_pips_ <= 0)
        {
            std::string error_message = "Position ‘" + position_id_
                                        + "’ in gelement #"
                                        + get_id() + " has not assigned price";

            throw std::runtime_error(error_message);
        }

        position_confirmed_ = true;
    }
}

void gcell::confirm_position(int position_price_pips)
{
    if (has_position())
    {
        position_price_pips_ = position_price_pips;
        position_confirmed_ = true;
    }
}

void gcell::attach_position(const std::string &position_id, unsigned long position_time, int &counter)
{
    if (has_position())
    {
        std::string error_message = "Illegal attempt to cover position ‘"
                                    + position_id_ +  "’ by position ‘"
                                    + position_id  + "’ in gelement #"
                                    + get_id();

        throw std::runtime_error(error_message);
    }

    position_id_ = position_id;
    position_time_ = position_time;
    position_price_pips_ = 0;
    position_confirmed_ = false;
    counter++;
}

void gcell::attach_position(const std::string &position_id, unsigned long position_time, int position_price_pips, int &counter)
{
    attach_position(position_id, position_time, counter);
    position_price_pips_ = position_price_pips;
}

void gcell::detatch_position(int &counter)
{
    if (has_position())
    {
        position_id_.clear();
        position_time_ = 0ul;
        position_price_pips_ = 0;
        position_confirmed_ = false;
        counter--;

        return;
    }
}

void gcell::reloc_position(gcell &cell)
{
    if (! cell.has_position())
    {
        std::string error_message = "Source gelement #"
                                    + cell.get_id()
                                    + " has no position";

        throw std::runtime_error(error_message);
    }

    if (has_position())
    {
        std::string error_message = "Illegal attempt to cover position ‘"
                                    + position_id_
                                    + "’ by position ‘"
                                    + cell.position_id_
                                    + "’ in gelement #"
                                    + get_id();

        throw std::runtime_error(error_message);
    }

    position_id_ = cell.position_id_;
    position_time_ = cell.position_time_;
    position_price_pips_ = cell.position_price_pips_;
    position_confirmed_ = cell.position_confirmed_;

    cell.position_id_.clear();
    cell.position_time_ = 0ul;
    cell.position_price_pips_ = 0;
    cell.position_confirmed_ = false;
}
