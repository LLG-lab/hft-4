#include <stdexcept>
#include <gcell.hpp>

gcell::gcell(int pips_span, int gnumber, bool terminal)
    : is_terminal_ { terminal },
      position_confirmed_ { false }
{
    gcell_id_ = std::string("g") + std::to_string(gnumber);

    int min_limit = gnumber*pips_span;
    int max_limit = min_limit + pips_span;

    int s = min_limit + pips_span / 2;

    trade_min_limit_ = s - 3; //10; //min_limit + pips_span / 3;
    trade_max_limit_ = s + 3; //10; //max_limit - pips_span / 3;
}

void gcell::confirm_position(void)
{
    if (has_position())
    {
        position_confirmed_ = true;
    }
}

void gcell::attach_position(const std::string &position_id)
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
    position_confirmed_ = false;
}

void gcell::detatch_position(const std::string &position_id)
{
    if (position_id == position_id_)
    {
        position_id_.clear();
        position_confirmed_ = false;

        return;
    }

    std::string error_message = "Attempt to detatch position ‘"
                                + position_id 
                                + "’ not owned by gelement #"
                                + get_id();

    throw std::runtime_error(error_message);
}

/*
void gcell::parse_from_json(const boost::json::object &json_obj)
{
// FIXME: To be implemented.
}

std::string gcell::export_to_json(void) const
{
    return std::string("\"") + get_id() + std::string("\": \"")
           + position_id_ + std::string("\"");
}
*/
