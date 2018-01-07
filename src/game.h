#pragma once

#include <climits>
#include <functional>
#include <unordered_map>
#include <cassert>
#include <vector>
#include <set>
#include <bitset>
#include <array>

struct PropertySet : std::bitset<28> {
    constexpr PropertySet(unsigned long long bits = 0)
        : std::bitset<28>{bits}
    {}

    static const PropertySet brown;
    static const PropertySet lblue;
    static const PropertySet pink;
    static const PropertySet orange;
    static const PropertySet red;
    static const PropertySet yellow;
    static const PropertySet green;
    static const PropertySet dblue;
    static const PropertySet station;
    static const PropertySet utility;
};

struct Player {
    Player(std::string name)
        : name{std::move(name)}
    {}

    std::string name;
    int salary = 200;

    int cash = 200;
    int secured_debt = 0;
    int unsecured_debt = 0;
    PropertySet properties = 0;
};

struct Property {
    Property(std::string name, int guide_price, int house_price, PropertySet set,
             std::array<int, 6> rents)
        : name{std::move(name)}, guide_price{guide_price},
          house_price{house_price}, set{set}, rents{rents}
    {}

    void mortgage(int amount) noexcept {
        assert(!mortgaged_);

        mortgaged_ = true;
        mortgage_amount_ = amount;
    }

    bool mortgaged() const noexcept {
        return mortgaged_;
    }

    int mortgage_amount() const noexcept {
        return mortgage_amount_;
    }

    void unmortgage() noexcept {
        mortgaged_ = false;
    }

    std::string name;
    int guide_price;
    int house_price;
    PropertySet set;
    std::array<int, 6> rents;

    int houses = 0;
    Player* owner = nullptr;
private:
    bool mortgaged_ = false;
    int mortgage_amount_ = 0;
};

struct Game {
    Game(std::vector<Player> players = {}) : players{std::move(players)} {
        for (unsigned i = 0; i < 28; ++i) {
            property_map_[properties[i].name] = i;
        }
    }

    Game(const Game&) = default;
    Game& operator=(const Game&) = default;
    Game(Game&&) = default;
    Game& operator=(Game&&) = default;

    void raise_interest() noexcept {
        ++secured_interest_;
        ++unsecured_interest_;
    }
    void lower_interest() noexcept {
        if (secured_interest_ > 1) --secured_interest_;
        if (unsecured_interest_ > 1) --unsecured_interest_;
    }

    int secured_interest() const noexcept {
        return secured_interest_;
    }

    int unsecured_interest() const noexcept {
        return unsecured_interest_;
    }

    unsigned id_of_property(const std::string& name) const {
        auto it = property_map_.find(name);
        assert(it != property_map_.end());
        return it->second;
    }

    std::vector<Player> players;
    std::array<Property, 28> properties = {{
        {"Old Kent Road",    60, 50, PropertySet::brown, {2, 10, 30, 90,  160, 250}},
        {"Whitechapel Road", 60, 50, PropertySet::brown, {4, 20, 60, 180, 360, 450}},

        {"The Angel Islington", 100, 50, PropertySet::lblue, {6, 30, 90,  270, 400, 550}},
        {"Euston Road",         100, 50, PropertySet::lblue, {6, 30, 90,  270, 400, 550}},
        {"Pentonville Road",    120, 50, PropertySet::lblue, {8, 40, 100, 300, 450, 600}},

        {"Pall Mall",             140, 100, PropertySet::pink, {10, 50, 150, 450, 625, 750}},
        {"Whitehall",             140, 100, PropertySet::pink, {10, 50, 150, 450, 625, 750}},
        {"Northumberland Avenue", 160, 100, PropertySet::pink, {12, 60, 180, 500, 700, 900}},

        {"Bow Street",         140, 100, PropertySet::orange, {10, 50, 150, 450, 625, 750}},
        {"Marlborough Street", 140, 100, PropertySet::orange, {10, 50, 150, 450, 625, 750}},
        {"Vine Street",        160, 100, PropertySet::orange, {12, 60, 180, 500, 700, 900}},

        {"Strand",           140, 100, PropertySet::red, {10, 50, 150, 450, 625, 750}},
        {"Fleet Street",     140, 100, PropertySet::red, {10, 50, 150, 450, 625, 750}},
        {"Trafalgar Square", 160, 100, PropertySet::red, {12, 60, 180, 500, 700, 900}},

        {"Leicester Square", 140, 100, PropertySet::yellow, {10, 50, 150, 450, 625, 750}},
        {"Coventry Street",  140, 100, PropertySet::yellow, {10, 50, 150, 450, 625, 750}},
        {"Piccadiliy",       160, 100, PropertySet::yellow, {12, 60, 180, 500, 700, 900}},

        {"Regent Street", 140, 100, PropertySet::green, {10, 50, 150, 450, 625, 750}},
        {"Oxford Street", 140, 100, PropertySet::green, {10, 50, 150, 450, 625, 750}},
        {"Bond Street",   160, 100, PropertySet::green, {12, 60, 180, 500, 700, 900}},

        {"Park lane", 140, 100, PropertySet::dblue, {10, 50, 150, 450, 625, 750}},
        {"Mayfair",   160, 100, PropertySet::dblue, {12, 60, 180, 500, 700, 900}},

        {"Kings Cross Station",   200, 0, PropertySet::station, {25, 50, 100, 200, 0, 0}},
        {"Marylebone Station",    200, 0, PropertySet::station, {25, 50, 100, 200, 0, 0}},
        {"Fenchurch St. Station", 200, 0, PropertySet::station, {25, 50, 100, 200, 0, 0}},
        {"Liverpool St. Station", 200, 0, PropertySet::station, {25, 50, 100, 200, 0, 0}},

        {"Electric Company", 150, 0, PropertySet::utility, {10, 50, 150, 450, 625, 750}},
        {"Water Works",      150, 0, PropertySet::utility, {12, 60, 180, 500, 700, 900}},

    }};
    double ppi = 1.0;
private:
    int secured_interest_ = 5;
    int unsecured_interest_ = 25;

    std::unordered_map<std::string, unsigned> property_map_;
};

inline double update_ppi(double old_ppi, int bought_for, int guide_price) noexcept
{
    return 0.5 * old_ppi + 0.5 * double(bought_for) / double(guide_price);
}

// Returns the smallest property id of all properties in the set
inline unsigned property_id(PropertySet set) noexcept
{
    static_assert(sizeof(unsigned) >= 28 / CHAR_BIT);
    unsigned l = set.to_ulong();
    // There must be some bits set, or the result is undefined
    assert(l);

    return __builtin_clz(l);
}

template <typename F_of_Property>
void for_each_property(PropertySet set, Game& game, F_of_Property function) noexcept
{
    // Not the fastest but it should work
    for (unsigned i = 0; i < set.size(); ++i) {
        if (set[i]) function(game.properties[i]);
    }
}

template <typename F_of_Property>
void for_each_property(PropertySet set, const Game& game, F_of_Property function) noexcept
{
    // Not the fastest but it should work
    for (unsigned i = 0; i < set.size(); ++i) {
        if (set[i]) function(game.properties[i]);
    }
}

inline std::vector<unsigned> property_ids(PropertySet set) noexcept
{
    std::vector<unsigned> ids;
    for (unsigned id = 0; id < set.size(); ++id) {
        if (set[id]) ids.push_back(id);
    }
    return ids;
}

// Information functions ------------------------------------------------------

int expected_rent(const Property&) noexcept;
int asset_value(const Player&, const Game&) noexcept;
int expected_income(const Player&, const Game&) noexcept;
int interest_to_pay(const Player&, const Game&) noexcept;

int max_secured_debt(const Player&, const Game&) noexcept;
int max_unsecured_debt(const Player&, const Game&) noexcept;

// Checking functions ---------------------------------------------------------

struct Result {
    Result(bool result, std::string error_message = "")
        : result_{result}, error_message_{error_message}
    {}

    constexpr operator bool() const noexcept {
        return result_;
    }

    const std::string& error_message() const noexcept {
        return error_message_;
    }
private:
    bool result_;
    std::string error_message_;
};

// Major functions ------------------------------------------------------------

// TODO more control over house building functions

Result raise_interest(Game& game);
Result lower_interest(Game& game);
Result passgo(Game& game, unsigned player);
Result buy_property(Game& game, unsigned player, unsigned property, int price);
Result sell_property(Game& game, unsigned player, unsigned property);
Result mortgage(Game& game, unsigned player, unsigned property);
Result unmortgage(Game& game, unsigned player, unsigned property);
Result build_houses(Game& game, unsigned player, PropertySet set, int number);
Result sell_houses(Game& game, unsigned player, PropertySet set, int number);
Result pay_repairs(Game& game, unsigned player, int cost_per_house, int cost_per_hotel);
Result pay_to_bank(Game& game, unsigned player, int amount);
Result pay_to_player(Game& game, unsigned player, int amount);
Result transfer(Game& game, unsigned from_player, unsigned to_player, int amount,
                PropertySet properties);
Result take_out_secured_debt(Game& game, unsigned player, int amount);
Result take_out_unsecured_debt(Game& game, unsigned player, int amount);
Result pay_off_secured_debt(Game& game, unsigned player, int amount);
Result pay_off_unsecured_debt(Game& game, unsigned player, int amount);
Result concede_to_player(Game& game, unsigned loser, unsigned victor);
Result concede_to_bank(Game& game, unsigned player);

