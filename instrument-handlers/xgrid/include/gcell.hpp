#ifndef __GCELL_HPP__
#define __GCELL_HPP__

#include <string>
#include <boost/json.hpp>

class gcell
{
public:

    gcell(void) = delete;
    gcell(int pips_span, int gnumber, bool terminal = false);
    gcell(int min_limit_pips, int max_limit_pips, int gnumber, bool terminal);
    ~gcell(void) = default;

    bool is_terminal(void) const { return is_terminal_; }

    bool has_position(void) const { return position_id_.length() != 0; }

    std::string get_position_id(void) const { return position_id_; }
    double get_position_volume(void) const { return position_volume_; }
    unsigned long get_position_timestamp(void) const { return position_time_; }
    int get_position_price_pips(void) const { return position_price_pips_; }

    bool has_position_confirmed(void) const { return position_confirmed_; }
    void confirm_position(void);
    void confirm_position(int position_price_pips);

    void attach_position(const std::string &position_id, double position_volume, unsigned long position_time, int &counter);
    void attach_position(const std::string &position_id, double position_volume, unsigned long position_time, int position_price_pips, int &counter);
    void detatch_position(int &counter);
    void reloc_position(gcell &cell);

    bool inside_trading_zone(int ask_pips) const { return (ask_pips > trade_min_limit_ && ask_pips <= trade_max_limit_); }

    int get_min_limit(void) const { return trade_min_limit_; }
    int get_max_limit(void) const { return trade_max_limit_; }
    int span(void) const { return trade_max_limit_ - trade_min_limit_; }

    std::string get_id(void) const { return gcell_id_; }

private:

    bool is_terminal_;

    int trade_min_limit_;
    int trade_max_limit_;

    std::string position_id_;
    double position_volume_;
    unsigned long position_time_;
    int position_price_pips_;

    bool position_confirmed_;

    std::string gcell_id_;
};

#endif /* __GCELL_HPP__ */
