#ifndef __POSITION_HPP__
#define __POSITION_HPP__

#include <string>

class position
{
public:

    position(const std::string &id, unsigned long open_time, int open_price_pips)
        : id_ {id},
          open_time_ {open_time},
          open_price_pips_ {open_price_pips},
          position_confirmed_ { false }
    {}

    position(void) = delete;

   ~position(void) = default;

   std::string get_id(void) const { return id_; }
   unsigned long get_open_time(void) const { return open_time_; }
   int get_open_price_pips(void) const { return open_price_pips_; }

   bool is_confirmed(void) const { return position_confirmed_; }
   void confirm(void) { position_confirmed_ = true; }

private:

    std::string id_;
    unsigned long open_time_;
    int open_price_pips_;

    bool position_confirmed_;
};
    
#endif /* __POSITION_HPP__ */
