#include <stdexcept>
#include <algorithm>
#include <gcell.hpp>

gcell::gcell(int &opened_positions_counter, position_container &positions, int pips_span, int gnumber, bool terminal)
    : is_terminal_ { terminal },
      positions_ {positions},
      opened_positions_counter_ {opened_positions_counter},
      gcell_id_ {gnumber}
{
    trade_min_limit_ = gnumber*pips_span;
    trade_max_limit_ = trade_min_limit_ + pips_span;
}

gcell::gcell(int &opened_positions_counter, position_container &positions, int min_limit_pips, int max_limit_pips, int gnumber, bool terminal)
    : is_terminal_ { terminal },
      positions_ {positions},
      opened_positions_counter_ {opened_positions_counter},
      gcell_id_ {gnumber}
{
    trade_min_limit_ = min_limit_pips;
    trade_max_limit_ = max_limit_pips;
}

void gcell::assign_position(position_container::iterator it)
{
    if (std::find(cell_positions_.begin(), cell_positions_.end(), it) == cell_positions_.end())
    {
        cell_positions_.push_back(it);
        it -> gcell_number_ = gcell_id_;

        if (it -> position_id_ != "virtual")
        {
            ++opened_positions_counter_;
        }
    }
}

void gcell::attach_position(const std::string &position_id, double position_volume, unsigned long position_time)
{
    position_container::iterator it = positions_.insert(positions_.end(), position_record(position_id, position_volume, position_time));
    cell_positions_.push_back(it);
    it -> gcell_number_ = gcell_id_;
    ++opened_positions_counter_;
}

void gcell::attach_confirmed_virtual_position(unsigned long position_time, int position_price_pips)
{
    position_container::iterator it = positions_.insert(positions_.end(), position_record("virtual", 1.0, position_time, position_price_pips));
    cell_positions_.push_back(it);
    it -> gcell_number_ = gcell_id_;
    it -> position_confirmed_ = true;
}

void gcell::detatch_position(void)
{
    position_container::iterator it = head_pos();
    detatch_position(it);
}

void gcell::detatch_position(position_container::iterator it)
{
    auto rit = std::find(cell_positions_.begin(), cell_positions_.end(), it);

    if (rit == cell_positions_.end())
    {
        return;
    }

    if (it -> position_id_ != "virtual")
    {
        --opened_positions_counter_;
    }

    cell_positions_.erase(rit);
    positions_.erase(it);
}

position_container::iterator gcell::head_pos(void)
{
    if (cell_positions_.empty())
    {
        std::string error_message = "No position in cell #"
                                    + std::to_string(gcell_id_);

        throw std::runtime_error(error_message);
    }
    else if (cell_positions_.size() == 1)
    {
        return *(cell_positions_.begin());
    }

    std::list<position_container::iterator>::iterator my_it = cell_positions_.begin();

    for (std::list<position_container::iterator>::iterator it = cell_positions_.begin(); it != cell_positions_.end(); it++)
    {
        if ( (*it) -> position_price_pips_ < (*my_it) -> position_price_pips_)
        {
            my_it = it;
        }
    }

    return (*my_it);
}

