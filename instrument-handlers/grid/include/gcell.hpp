#ifndef __GCELL_HPP__
#define __GCELL_HPP__

#include <string>
#include <boost/json.hpp>

class gcell
{
public:

    gcell(void) = delete;
    gcell(int pips_span, int gnumber, bool terminal = false);
    ~gcell(void) = default;

    bool is_terminal(void) const { return is_terminal_; }

    bool has_position(void) const { return position_id_.length() != 0; }
    std::string get_position(void) const { return position_id_; }
    bool has_position_confirmed(void) const { return position_confirmed_; }
    void confirm_position(void);
    void attach_position(const std::string &position_id, int &counter);
    void detatch_position(const std::string &position_id, int &counter);

    bool inside_trading_zone(int ask_pips) const { return (trade_min_limit_ <= ask_pips && trade_max_limit_ >= ask_pips); }

    std::string get_id(void) const { return gcell_id_; }

private:

    bool is_terminal_;

    int trade_min_limit_;
    int trade_max_limit_;

    std::string position_id_;
    bool position_confirmed_;
    
    std::string gcell_id_;
};

#endif /* __GCELL_HPP__ */
