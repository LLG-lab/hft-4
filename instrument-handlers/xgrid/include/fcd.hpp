#ifndef __FCD_HPP__
#define __FCD_HPP__

#include <string>
#include <list>
#include <algorithm>

#include <svr.hpp>

// Flash-Crash Detector class.

class fcd
{
public:

    fcd(svr actions);
    fcd(const fcd &) = delete;
    fcd(const fcd &&) = delete;

    enum class action
    {
        OPEN_POS,
        CLOSE_POS
    };

    void enable(void)
    {
        enabled_ = true;
    }

    bool is_enabled(void) const { return enabled_; }

    bool can_open_position(void) const;

    //
    // Setup ‘depth_fall_cutoff’.
    //

    void setup_dfc(int dfc) { dfc_ = dfc; }

    void feed(action act);

private:

    bool enabled_;
    svr actions_;
    int dfc_;
};

#endif /* __FCD_HPP__ */

