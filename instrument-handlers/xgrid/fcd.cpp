#ifndef __FCD_HPP__
#include <fcd.hpp>
#endif

fcd::fcd(svr actions)
    : enabled_{false},
      actions_{actions},
      dfc_{32}
{
}

bool fcd::can_open_position(void) const
{
    if (! enabled_)
    {
        return true;
    }

    int actions = actions_.get<int>();

    int n = 0;
    int c = 1;

    for (int i = 0; i < 32; i++)
    {
        if ((actions & c) != 0)
        {
            break;
        }

        c = (c << 1);
        n++;
    }

    if (n >= dfc_) return false;

    return true;
}

void fcd::feed(fcd::action act)
{
    if (! enabled_)
    {
        return;
    }

    int actions = actions_.get<int>();

    actions = (actions << 1);

    if (act == action::CLOSE_POS)
    {
        actions |= 1;
    }

    actions_.set(actions);
}
