#ifndef __GCELL_HPP__
#define __GCELL_HPP__

#include <string>
#include <positions.hpp>

class gcell
{
public:

    gcell(void) = delete;
    gcell(position_container &positions, int pips_span, int gnumber, bool terminal = false);
    gcell(position_container &positions, int min_limit_pips, int max_limit_pips, int gnumber, bool terminal);
    ~gcell(void) = default;

    bool has_position(void) const { return !cell_positions_.empty(); }

    std::string get_position_id(void)  { return head_pos() -> position_id_; }
    double get_position_volume(void)  { return head_pos() -> position_volume_; }
    unsigned long get_position_timestamp(void) { return head_pos() -> position_time_; }
    int get_position_price_pips(void)  { return head_pos() -> position_price_pips_; }

    void assign_position(position_container::iterator it);
    void attach_position(const std::string &position_id, double position_volume, unsigned long position_time);
    void attach_confirmed_virtual_position(unsigned long position_time, int position_price_pips);
    void detatch_position(void);
    void detatch_position(position_container::iterator it);

    bool inside_trading_zone(int ask_pips) const { return (ask_pips > trade_min_limit_ && ask_pips <= trade_max_limit_); }

    bool is_terminal(void) const { return is_terminal_; }
    int get_min_limit(void) const { return trade_min_limit_; }
    int get_max_limit(void) const { return trade_max_limit_; }
    int span(void) const { return trade_max_limit_ - trade_min_limit_; }

    int get_id(void) const { return gcell_id_; }

    static int active_cells(void) { return active_gcells_; }

private:

    position_container::iterator head_pos(void);
    //position_container::const_iterator head_pos(void) const;

    bool is_terminal_;

    int trade_min_limit_;
    int trade_max_limit_;

    std::list<position_container::iterator> cell_positions_;
    position_container &positions_;

    int gcell_id_;
    static int active_gcells_;
};

#endif /* __GCELL_HPP__ */
