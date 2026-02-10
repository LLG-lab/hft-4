#ifndef __POSITIONS_HPP__
#define __POSITIONS_HPP__

#include <list>

struct position_record
{
    position_record(void)
        : position_volume_{0.0},
          position_time_{0ul},
          position_price_pips_{0},
          position_confirmed_{false},
          gcell_number_{-1}
    {}

    position_record(const std::string &position_id, double position_volume, unsigned long position_time, int position_price_pips = -1, bool position_confirmed = false)
        : position_id_{position_id},
          position_volume_{position_volume},
          position_time_{position_time},
          position_price_pips_{position_price_pips},
          position_confirmed_{position_confirmed},
          gcell_number_{-1}
    {}

    std::string position_id_;
    double position_volume_;
    unsigned long position_time_;
    int position_price_pips_;
    bool position_confirmed_;
    int gcell_number_;
};

typedef std::list<position_record> position_container;

#endif /* __POSITIONS_HPP__ */
