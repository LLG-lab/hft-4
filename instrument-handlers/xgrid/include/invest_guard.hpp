#ifndef __INVEST_GUARD_HPP__
#define __INVEST_GUARD_HPP__

#include <cmath>
#include <svr.hpp>

class virtual_grid
{
public:

    virtual_grid(void)
        : min_{0}, max_{0}, excluded_{-1}, previous_ask_pips_{-1}
    {}

    virtual_grid(int min, int max) // Podajemy kursy w pipsach
        : min_{min}, max_{max}, excluded_{-1}, previous_ask_pips_{-1}
    {}

    void setup_range(int min, int max)
    {
        min_ = min;
        max_ = max;
    }

    bool fall(int ask_pips)
    {
        if (previous_ask_pips_ < 0)
        {
            previous_ask_pips_ = ask_pips;

            return false;
        }

        int cur_cell_num = cell_num(ask_pips);

        if (cur_cell_num < 0)
        {
            // Out of grid.
            previous_ask_pips_ = ask_pips;

            return false;
        }

        int prev_cell_num = cell_num(previous_ask_pips_);

        if (prev_cell_num < 0)
        {
            // Out of grid.
            previous_ask_pips_ = ask_pips;

            return false;
        }

        previous_ask_pips_ = ask_pips;

        if (prev_cell_num > cur_cell_num)
        {
            if (prev_cell_num == excluded_)
            {
                return false;
            }

            excluded_ = prev_cell_num;

            return true;
        }

        return false;
    }

private:

    int cell_num(int ask_pips) const
    {
        if (ask_pips < min_ || ask_pips >= max_)
        {
            // Out of grid.
            return -1;
        }

        return (ask_pips - min_) / 10;
    }

    int min_;
    int max_;

    // Numer komórki, z przejścia której do komórki
    // poprzedniej poprzedniej nie jest traktowany
    // jako spadek.
    int excluded_;
    int previous_ask_pips_;
};

/*

Ta klasa opisuje mechanizm jak mogłaby wyglądać gra bota.
– mamy odrębnie zaimplementowany grid, który gdy nastąpi zejście
  z komórki wyższej do niższej, wywołuje dla obiektu klasy
  invest_guard metodę fall().
– mamy następnie drugi grid, inwestycyjny, na którym znajdują się
  prawdziwe pozycje. Gdy w wyniku zmiany ceny nastąpi zprzejście
  z jednej komórki do drugiej dla tego grida, zanim otworzymy
  pozycję dla tej komórki odputamy obiekt invest_guard wołając
  metodę „can_play()” jeśli tak, otwieramy pozycję, jeśli nie
  nie robimy nic.
- podczas każdego ticku dla obiektu invest_guard wołamy metodę
  tick() przekazując timestampa bieżącego ticka.

Kluczem sukcesu jest odpowiedni dobór parametrów (alfa, beta, gamma).
*/

class invest_guard
{
public:

    invest_guard(void)
        : enabled_ {false}, alpha_{0.0}, beta_{0.0}, gamma_{1.0}, last_tick_ts_{0ul}
    {}

    invest_guard(double alpha, double beta, double gamma)
        : enabled_ {false}, alpha_{alpha}, beta_{beta}, gamma_{gamma}, last_tick_ts_{0ul}
    {}

    void setup_range(int min, int max)
    {
        vg_.setup_range(min, max);
    }

    void set_alpha(double alpha)
    {
        alpha_ = alpha;
    }

    void set_beta(double beta)
    {
        beta_ = beta;
    }

    void enable(void)
    {
        enabled_ = true;
    }

    void disable(void)
    {
        enabled_ = false;
    }

    bool can_play(void) const
    {
        if (! enabled_) return true;

        return (pain_svr_.get<double>() < gamma_);
    }

    void set_pain_svr(svr pain_svr)
    {
        pain_svr_ = pain_svr;
    }

    void tick(int ask_pips, unsigned long int timestamp)
    {
        if (! enabled_) return;

        unsigned long int rest = 0;

        if (last_tick_ts_)
        {
            double pain = pain_svr_.get<double>();

            if (vg_.fall(ask_pips))
            {
                pain += alpha_;
            }

            // Tu skomplikowane obliczenia
            rest = (timestamp - last_tick_ts_) % 100;
            // Liczba całych 100 milisekundowych interwałów
            auto cnt = (timestamp - last_tick_ts_) / 100;

            pain *= pow(beta_, cnt);

            pain_svr_.set(pain);
        }

        last_tick_ts_ = timestamp - rest;
    }

private:

    bool enabled_;
    double alpha_;   // Jak bardzo bota „boli” spadek.
    double beta_;    // Liczba mniejsza od jeden, ale niewiele. Na przykład 0.99998888
    double gamma_;   // eeee
    svr pain_svr_;
    unsigned long int last_tick_ts_;
    virtual_grid vg_;
};

#endif /* __INVEST_GUARD_HPP__ */
