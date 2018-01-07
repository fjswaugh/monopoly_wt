#include "game.h"

const PropertySet PropertySet::brown   = 0b0000000000000000000000000011;
const PropertySet PropertySet::lblue   = 0b0000000000000000000000011100;
const PropertySet PropertySet::pink    = 0b0000000000000000000011100000;
const PropertySet PropertySet::orange  = 0b0000000000000000011100000000;
const PropertySet PropertySet::red     = 0b0000000000000011100000000000;
const PropertySet PropertySet::yellow  = 0b0000000000011100000000000000;
const PropertySet PropertySet::green   = 0b0000000011100000000000000000;
const PropertySet PropertySet::dblue   = 0b0000001100000000000000000000;
const PropertySet PropertySet::station = 0b0011110000000000000000000000;
const PropertySet PropertySet::utility = 0b1100000000000000000000000000;

// Information functions ------------------------------------------------------

int expected_rent(const Property& p) noexcept {
    if (p.mortgaged()) return 0;

    const unsigned number_owned_in_set = [&p]() -> unsigned {
        if (p.owner) {
            return (p.owner->properties & p.set).count();
        }
        return 0;
    }();

    if (number_owned_in_set == 0) return 0;

    if (p.set == PropertySet::station) {
        assert(number_owned_in_set >= 1 && number_owned_in_set <= 4);
        return p.rents[number_owned_in_set-1];
    }

    if (p.set == PropertySet::utility) {
        assert(number_owned_in_set >= 1 && number_owned_in_set <= 2);
        return 7 * p.rents[number_owned_in_set-1];
    }

    // OK, so must be a normal property

    const bool owns_all_of_set = number_owned_in_set == p.set.count();
    const int multiplier = (p.houses == 0 && owns_all_of_set) ? 2 : 1;

    return p.rents[p.houses] * multiplier;
}

int asset_value(const Player& p, const Game& g) noexcept
{
    int sum = 0;
    for (int i = 0; i < 28; ++i) {
        sum += static_cast<int>(p.properties[i]) * g.properties[i].guide_price;
    }
    return sum * g.ppi;
}

int expected_income(const Player& player, const Game& game) noexcept
{
    int sum = 0;
    for (int i = 0; i < 28; ++i) {
        if (player.properties[i]) {
            sum += expected_rent(game.properties[i]);
        }
    }
    return sum;
}

int interest_to_pay(const Player& p, const Game& g) noexcept
{
    return p.secured_debt * g.secured_interest() / 100.0 +
           p.unsecured_debt * g.unsecured_interest() / 100.0;
}

int max_secured_debt(const Player& p, const Game& g) noexcept {
    return 5 * p.salary + std::min(3 * expected_income(p, g), asset_value(p, g));
}

int max_unsecured_debt(const Player&, const Game&) noexcept {
    return 200;
}

// Checking functions ---------------------------------------------------------

#define CHECK_PLAYER_OWNS_PROPERTY(player_id, property_id)\
    if (game.properties[property_id].owner != &game.players[player_id]) {\
        return {false, "Property is not owned by player_id"};\
    }\
\
    assert(game.players[player_id].properties[property_id] == true)

#define CHECK_PLAYER_ID_IN_RANGE(player_id)\
    assert(player_id < game.players.size())

#define CHECK_PROPERTY_ID_IN_RANGE(property_id)\
    assert(property_id < 28);

#define CHECK_PLAYER_HAS_CASH(player_id, amount)\
    if (amount > game.players[player_id].cash) {\
        return {false, game.players[player_id].name + " doesn't have enough cash"};\
    }

Result can_raise_interest(const Game&)
{
    return true;
}

Result can_lower_interest(const Game&)
{
    return true;
}

Result can_passgo(const Game& game, unsigned player_id)
{
    CHECK_PLAYER_ID_IN_RANGE(player_id);

    if (game.players[player_id].cash + game.players[player_id].salary <
        interest_to_pay(game.players[player_id], game)) {
        return {false, "Not enough funds to pay interest"};
    }

    return true;
}

Result can_buy_property(const Game& game, unsigned player_id, unsigned property_id, int price)
{
    CHECK_PLAYER_ID_IN_RANGE(player_id);
    CHECK_PROPERTY_ID_IN_RANGE(property_id);
    assert(price >= 0);

    if (game.properties[property_id].owner) {
        return {false, "Property not available"};
    }

    CHECK_PLAYER_HAS_CASH(player_id, price);

    return true;
}

Result can_sell_property(const Game& game, unsigned player_id, unsigned property_id)
{
    CHECK_PLAYER_ID_IN_RANGE(player_id);
    CHECK_PROPERTY_ID_IN_RANGE(property_id);

    CHECK_PLAYER_OWNS_PROPERTY(player_id, property_id);

    return true;
}

Result can_mortgage(const Game& game, unsigned player_id, unsigned property_id)
{
    CHECK_PLAYER_ID_IN_RANGE(player_id);
    CHECK_PROPERTY_ID_IN_RANGE(property_id);

    CHECK_PLAYER_OWNS_PROPERTY(player_id, property_id);

    if (game.properties[property_id].mortgaged()) {
        return {false, "Property is already mortgaged"};
    }

    return true;
}

Result can_unmortgage(const Game& game, unsigned player_id, unsigned property_id)
{
    CHECK_PLAYER_ID_IN_RANGE(player_id);
    CHECK_PROPERTY_ID_IN_RANGE(property_id);

    CHECK_PLAYER_OWNS_PROPERTY(player_id, property_id);

    if (!game.properties[property_id].mortgaged()) {
        return {false, "Cannot unmortgage - property_id is not mortgaged"};
    }

    const int to_pay = game.properties[property_id].mortgage_amount() * 1.1;
    CHECK_PLAYER_HAS_CASH(player_id, to_pay);

    return true;
}

Result can_build_houses(const Game& game, unsigned player_id, PropertySet set, int number)
{
    CHECK_PLAYER_ID_IN_RANGE(player_id);
    assert(number >= 0);

    if (set == PropertySet::station) {
        return {false, "Can't build on stations"};
    }

    if (set == PropertySet::utility) {
        return {false, "Can't build on utilities"};
    }

    if ((game.players[player_id].properties & set) != set) {
        return {false, game.players[player_id].name + " doesn't own all properties in set"};
    }

    const int house_price = game.properties[property_id(set)].house_price;
    CHECK_PLAYER_HAS_CASH(player_id, house_price * number);

    const int max_houses = set.count() * 5;
    int houses_sum = 0;
    for_each_property(set, game, [&houses_sum](const Property& p) { houses_sum += p.houses; });
    if (houses_sum + number > max_houses) {
        return {false,
                std::to_string(houses_sum) + " already built, maximum is " +
                    std::to_string(max_houses)};
    }

    return true;
}

Result can_sell_houses(const Game& game, unsigned player_id, PropertySet set, int number)
{
    CHECK_PLAYER_ID_IN_RANGE(player_id);
    assert(number >= 0);

    if ((game.players[player_id].properties & set) != set) {
        return {false, game.players[player_id].name + " doesn't own all properties in set"};
    }

    int houses_sum = 0;
    for_each_property(set, game, [&houses_sum](const Property& p) { houses_sum += p.houses; });
    if (houses_sum - number < 0) {
        return {false, "Can only remove " + std::to_string(houses_sum) + " houses"};
    }

    return true;
}

Result can_pay_repairs(const Game& game, unsigned player_id, int cost_per_house, int cost_per_hotel)
{
    CHECK_PLAYER_ID_IN_RANGE(player_id);
    assert(cost_per_house >= 0);
    assert(cost_per_hotel >= 0);

    int amount_to_pay = 0;
    for_each_property(game.players[player_id].properties, game,
                      [&amount_to_pay, cost_per_house, cost_per_hotel](const Property& p) {
                          if (p.houses == 5) {
                              amount_to_pay += cost_per_house;
                          } else {
                              amount_to_pay += (p.houses * cost_per_house);
                          }
                      });
    CHECK_PLAYER_HAS_CASH(player_id, amount_to_pay);

    return true;
}

Result can_pay_to_bank(const Game& game, unsigned player_id, int amount)
{
    CHECK_PLAYER_ID_IN_RANGE(player_id);
    CHECK_PLAYER_HAS_CASH(player_id, amount);

    return true;
}

Result can_pay_to_player(const Game& game, unsigned player_id, int amount)
{
    CHECK_PLAYER_ID_IN_RANGE(player_id);
    assert(amount >= 0);

    return true;
}

Result can_transfer(const Game& game, unsigned from_player_id, unsigned to_player_id, int amount,
                    PropertySet properties)
{
    CHECK_PLAYER_ID_IN_RANGE(from_player_id);
    CHECK_PLAYER_ID_IN_RANGE(to_player_id);
    assert(amount >= 0);
    
    if ((game.players[from_player_id].properties & properties) != properties) {
        return {false, game.players[from_player_id].name + " doesn't own all of those properties"};
    }

    bool houses_on_properties = false;
    for_each_property(properties, game, [&houses_on_properties](const Property& p) {
        if (p.houses > 0) houses_on_properties = true;
    });
    if (houses_on_properties) {
        return {false, "Cannot transfer properties with houses on them"};
    }

    CHECK_PLAYER_HAS_CASH(from_player_id, amount);

    return true;
}

Result can_take_out_secured_debt(const Game& game, unsigned player_id, int amount)
{
    CHECK_PLAYER_ID_IN_RANGE(player_id);
    assert(amount >= 0);

    if (game.players[player_id].secured_debt + amount >
        max_secured_debt(game.players[player_id], game)) {
        return {false, game.players[player_id].name + " cannot take out that much secured debt"};
    }

    return true;
}

Result can_take_out_unsecured_debt(const Game& game, unsigned player_id, int amount)
{
    CHECK_PLAYER_ID_IN_RANGE(player_id);
    assert(amount >= 0);

    if (game.players[player_id].unsecured_debt + amount >
        max_unsecured_debt(game.players[player_id], game)) {
        return {false, game.players[player_id].name + " cannot take out that much unsecured debt"};
    }

    return true;
}

Result can_pay_off_secured_debt(const Game& game, unsigned player_id, int amount)
{
    CHECK_PLAYER_ID_IN_RANGE(player_id);
    assert(amount >= 0);

    if (game.players[player_id].secured_debt < amount) {
        return {false, "Cannot overpay debt"};
    }

    CHECK_PLAYER_HAS_CASH(player_id, amount);

    return true;
}

Result can_pay_off_unsecured_debt(const Game& game, unsigned player_id, int amount)
{
    CHECK_PLAYER_ID_IN_RANGE(player_id);
    assert(amount >= 0);

    if (game.players[player_id].unsecured_debt < amount) {
        return {false, "Cannot overpay debt"};
    }

    CHECK_PLAYER_HAS_CASH(player_id, amount);

    return true;
}

Result can_concede_to_player(const Game& game, unsigned loser, unsigned victor)
{
    CHECK_PLAYER_ID_IN_RANGE(loser);
    CHECK_PLAYER_ID_IN_RANGE(victor);

    bool houses_on_properties = false;
    for_each_property(game.players[loser].properties, game,
                      [&houses_on_properties](const Property& p) {
                          if (p.houses > 0) houses_on_properties = true;
                      });
    if (houses_on_properties) {
        return {false, "Cannot transfer properties with houses on them"};
    }

    return true;
}

Result can_concede_to_bank(const Game& game, unsigned player_id)
{
    CHECK_PLAYER_ID_IN_RANGE(player_id);

    return true;
}


// Major functions ------------------------------------------------------------

Result raise_interest(Game& game)
{
    const auto result = can_raise_interest(game);
    if (!result) return result;

    game.raise_interest();

    return true;
}

Result lower_interest(Game& game)
{
    const auto result = can_lower_interest(game);
    if (!result) return result;

    game.lower_interest();

    return true;
}

Result passgo(Game& game, unsigned player_id)
{
    const auto result = can_passgo(game, player_id);
    if (!result) return result;

    game.players[player_id].cash += game.players[player_id].salary;
    game.players[player_id].cash -= interest_to_pay(game.players[player_id], game);

    return true;
}

Result buy_property(Game& game, unsigned player_id, unsigned property_id, int price)
{
    const auto result = can_buy_property(game, player_id, property_id, price);
    if (!result) return result;

    game.properties[property_id].owner = &game.players[player_id];
    game.players[player_id].properties[property_id] = true;

    game.players[player_id].cash -= price;

    game.ppi = update_ppi(game.ppi, price, game.properties[property_id].guide_price);

    return true;
}

Result sell_property(Game& game, unsigned player_id, unsigned property_id)
{
    const auto result = can_sell_property(game, player_id, property_id);
    if (!result) return result;

    game.properties[property_id].owner = nullptr;
    game.players[player_id].properties[property_id] = false;

    const int price = game.ppi * game.properties[property_id].guide_price;
    game.players[player_id].cash += price;

    return true;
}

Result mortgage(Game& game, unsigned player_id, unsigned property_id)
{
    const auto result = can_mortgage(game, player_id, property_id);
    if (!result) return result;

    auto& player = game.players[player_id];
    auto& property = game.properties[property_id];

    const int amount = property.guide_price * game.ppi / 2.0;
    property.mortgage(amount);
    player.cash += amount;

    return true;
}

Result unmortgage(Game& game, unsigned player_id, unsigned property_id)
{
    const auto result = can_unmortgage(game, player_id, property_id);
    if (!result) return result;

    auto& player = game.players[player_id];
    auto& property = game.properties[property_id];

    player.cash -= property.mortgage_amount() * 1.1;
    property.unmortgage();

    return true;
}

Result build_houses(Game& game, unsigned player_id, PropertySet set, int number)
{
    const auto result = can_build_houses(game, player_id, set, number);
    if (!result) return result;

    auto& player = game.players[player_id];
    const int house_price = game.properties[property_id(set)].house_price;

    player.cash -= number * house_price;

    std::vector<unsigned> ids = property_ids(set);
    // Sort by number of houses already on property ascending, then by property index descending.
    // The idea is to first put houses on properties with fewer houses, and if two properties have
    // the same number of houses then to first put houses on the more valuable property.
    std::sort(ids.begin(), ids.end(), [&game](unsigned ida, unsigned idb) {
        if (game.properties[ida].houses < game.properties[idb].houses) return true;
        return ida > idb;
    });

    for (unsigned i = 0; number > 0;) {
        game.properties[ids[i]].houses++;

        --number;
        i = (i + 1) % ids.size();
    }

    return true;
}

Result sell_houses(Game& game, unsigned player_id, PropertySet set, int number)
{
    const auto result = can_sell_houses(game, player_id, set, number);
    if (!result) return result;

    auto& player = game.players[player_id];
    const int house_price = game.properties[property_id(set)].house_price;

    player.cash += (number * house_price) / 2;

    std::vector<unsigned> ids = property_ids(set);
    // Sell houses in the opposite order to buying them...
    std::sort(ids.begin(), ids.end(), [&game](unsigned ida, unsigned idb) {
        if (game.properties[ida].houses > game.properties[idb].houses) return true;
        return ida < idb;
    });

    for (unsigned i = 0; number > 0;) {
        game.properties[ids[i]].houses--;

        --number;
        i = (i + 1) % ids.size();
    }

    return true;
}

Result pay_repairs(Game& game, unsigned player_id, int cost_per_house, int cost_per_hotel)
{
    const auto result = can_pay_repairs(game, player_id, cost_per_house, cost_per_hotel);
    if (!result) return result;

    int amount_to_pay = 0;
    for_each_property(game.players[player_id].properties, game,
                      [&amount_to_pay, cost_per_house, cost_per_hotel](const Property& p) {
                          if (p.houses == 5) {
                              amount_to_pay += cost_per_house;
                          } else {
                              amount_to_pay += (p.houses * cost_per_house);
                          }
                      });
    game.players[player_id].cash -= amount_to_pay;

    return true;
}

Result pay_to_bank(Game& game, unsigned player_id, int amount)
{
    const auto result = can_pay_to_bank(game, player_id, amount);
    if (!result) return result;

    game.players[player_id].cash -= amount;

    return true;
}

Result pay_to_player(Game& game, unsigned player_id, int amount)
{
    const auto result = can_pay_to_player(game, player_id, amount);
    if (!result) return result;

    game.players[player_id].cash += amount;

    return true;
}

Result transfer(Game& game, unsigned from_player_id, unsigned to_player_id, int amount,
                PropertySet properties)
{
    const auto result = can_transfer(game, from_player_id, to_player_id, amount, properties);
    if (!result) return result;

    auto& from_player = game.players[from_player_id];
    auto& to_player = game.players[to_player_id];

    from_player.cash -= amount;
    to_player.cash += amount;

    from_player.properties ^= properties;
    to_player.properties ^= properties;

    for_each_property(properties, game, [&to_player](Property& p) { p.owner = &to_player; });

    return true;
}

Result take_out_secured_debt(Game& game, unsigned player_id, int amount)
{
    const auto result = can_take_out_secured_debt(game, player_id, amount);
    if (!result) return result;

    game.players[player_id].secured_debt += amount;
    game.players[player_id].cash += amount;

    return true;
}

Result take_out_unsecured_debt(Game& game, unsigned player_id, int amount)
{
    const auto result = can_take_out_unsecured_debt(game, player_id, amount);
    if (!result) return result;

    game.players[player_id].unsecured_debt += amount;
    game.players[player_id].cash += amount;

    return true;
}

Result pay_off_secured_debt(Game& game, unsigned player_id, int amount)
{
    const auto result = can_pay_off_secured_debt(game, player_id, amount);
    if (!result) return result;

    game.players[player_id].secured_debt -= amount;
    game.players[player_id].cash -= amount;

    return true;
}

Result pay_off_unsecured_debt(Game& game, unsigned player_id, int amount)
{
    const auto result = can_pay_off_unsecured_debt(game, player_id, amount);
    if (!result) return result;

    game.players[player_id].unsecured_debt -= amount;
    game.players[player_id].cash -= amount;

    return true;
}

Result concede_to_player(Game& game, unsigned loser, unsigned victor)
{
    const auto result = can_concede_to_player(game, loser, victor);
    if (!result) return result;

    const auto res =
        transfer(game, loser, victor, game.players[loser].cash, game.players[loser].properties);

    // Don't erase player, just leave them there, otherwise all player ids are invalidated
    //game.players.erase(game.players.begin() + loser);

    return res;
}

Result concede_to_bank(Game& game, unsigned player_id)
{
    const auto result = can_concede_to_bank(game, player_id);
    if (!result) return result;

    game.players[player_id].cash = 0;
    game.players[player_id].properties = 0;

    return true;
}

