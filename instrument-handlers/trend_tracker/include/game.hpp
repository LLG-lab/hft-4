#ifndef __GAME_HPP__
#define __GAME_HPP__

struct game
{
    double a_;
    double delta_;
    bool result_;
};

enum class game_player_status
{
    E_IDLE,
    E_PENDING,
    E_COMPLETED
};

class game_player
{
public:

     game_player(int pips_limit)
         : pips_limit_ {pips_limit},
           gp_status_ {game_player_status::E_IDLE},
           open_price_pips_ {0}
     {}

     game_player(void) = delete;
     game_player(game_player &) = delete;
     game_player(game_player &&) = delete;

     virtual ~game_player(void) {}

     virtual void tick(int ask_pips, int bid_pips) = 0;
     game_player_status get_status(void) const { return gp_status_; }
     void new_game(double a, double delta, int price_pips);
     game get_game_result(void) const;

protected:

     const int pips_limit_;
     game_player_status gp_status_;
     int open_price_pips_;

     game current_game_;
};

class long_game_player : public game_player
{
public:

    long_game_player(int pips_limit)
        : game_player {pips_limit}
    {}

    long_game_player(void) = delete;
    long_game_player(long_game_player &) = delete;
    long_game_player(long_game_player &&) = delete;

    virtual void tick(int ask_pips, int bid_pips);
};

class short_game_player : public game_player
{
public:

    short_game_player(int pips_limit)
        : game_player {pips_limit}
    {}

    short_game_player(void) = delete;
    short_game_player(short_game_player &) = delete;
    short_game_player(short_game_player &&) = delete;

    virtual void tick(int ask_pips, int bid_pips);
};

#endif /* __GAME_HPP__ */
