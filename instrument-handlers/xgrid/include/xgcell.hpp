#ifndef __XGCELL_HPP__
#define __XGCELL_HPP__

#include <list>

#include <hft_response.hpp>
#include <position.hpp>

class xgcell
{
public:

    enum class type
    {
        STANDARD,
        SELLONLY,
        NEUTRAL0
    };

    xgcell(void) = delete;

    xgcell(int start_price_pips, int pips_span, type cell_type,
               int dayswap_per_lot_pips, bool &changed_state,
                   const std::string &logger_id, double basic_rate,
                       int base_rate_multiplicity, xgcell *precedessor);

    int get_lo_limit(void) const { return lo_limit_; }
    int get_hi_limit(void) const { return hi_limit_; }
    int get_span(void) const { return hi_limit_ - lo_limit_; }
    bool has_position(void) const { return positions_.size() > 0; }
    std::string get_id(void) const { return id; }

    //
    // Positions query & manip.
    //

    bool has_position(const std::string &position_id) const;
    void confirm_position(const std::string &position_id);
    void remove_unconfirmed(void);

    //
    // Main operational method. Return value is ‘true’,
    // when market operation has been performed.
    //

    bool proceed(int ask_pips, int bid_pips, unsigned long timestamp,
                     hft::protocol::response &market);

private:

    static std::string make_xgcell_label(int start_price_pips, int pips_span);

    int lo_limit_;
    int hi_limit_;

    const int dayswap_per_lot_pips_;
    double basic_rate_;
    int base_rate_multiplicity_;

    std::list<position> positions_;

    xgcell const *precedessor_;

    const xgcell::type cell_type_;

    const std::string id_;
    const std::string logger_id_;

    bool &changed_state_;
};

#endif /* __XGCELL_HPP__ */
