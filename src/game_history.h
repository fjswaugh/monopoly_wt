#pragma once

#include "game.h"
#include "event.h"
#include <iostream>
#include <vector>
#include <cassert>

struct GameHistory {
    GameHistory(const Game& game)
    {
        history_[0] = game;
        descriptions_[0] = "Game started";
    }
    GameHistory(Game&& game = Game())
    {
        history_[0] = std::move(game);
        descriptions_[0] = "Game started";
    }

    Game& current_game() noexcept
    {
        return history_[current_game_index_];
    }
    const Game& current_game() const noexcept
    {
        return history_[current_game_index_];
    }

    // Adding a player resets the undo/redo for now
    void add_player(const AddPlayerEvent& event) {
        assert(event.player_id == this->current_game().num_players());

        history_[current_game_index_].add_player(event.name);
        descriptions_[current_game_index_] = "Player " + event.name + " added to game";

        past_games_ = 0;
        future_games_ = 0;
    }

    Result apply(const GameEvent& event) {
        Game new_game = history_[current_game_index_];
        const auto result = event.function()(new_game);

        if (result) {
            current_game_index_ = (current_game_index_ + 1) % games_stored;
            history_[current_game_index_] = std::move(new_game);
            descriptions_[current_game_index_] = result.description();

            past_games_ = std::min(past_games_ + 1, games_stored - 1);
            future_games_ = 0;
        }

        return result;
    }

    Result undo() noexcept {
        if (past_games_ == 0) return {false, "Cannot undo here"};

        auto description = "Undo: " + descriptions_[current_game_index_];
        current_game_index_ = (current_game_index_ > 0 ? current_game_index_ - 1 : games_stored - 1);
        --past_games_;
        ++future_games_;
        return {true, std::move(description)};
    }

    Result redo() noexcept {
        if (future_games_ == 0) return {false, "Cannot redo here"};

        current_game_index_ = (current_game_index_ + 1) % games_stored;
        ++past_games_;
        --future_games_;

        auto description = "Redo: " + descriptions_[current_game_index_];
        return {true, std::move(description)};
    }
private:
    static const unsigned games_stored = 100;

    std::vector<Game> history_ = std::vector<Game>(games_stored);

    // Description of the event that resulted in that game
    std::vector<std::string> descriptions_ = std::vector<std::string>(games_stored);

    unsigned current_game_index_ = 0;
    unsigned past_games_ = 0;
    unsigned future_games_ = 0;
};

