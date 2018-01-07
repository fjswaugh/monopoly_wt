#pragma once

#include "game.h"
#include "event.h"
#include <vector>
#include <cassert>

struct GameHistory {
    GameHistory(const Game& game)
    {
        history_[0] = game;
    }
    GameHistory(Game&& game = Game())
    {
        history_[0] = std::move(game);
    }

    Game& current_game() noexcept
    {
        return history_[current_game_index_];
    }
    const Game& current_game() const noexcept
    {
        return history_[current_game_index_];
    }

    void add_player(const AddPlayerEvent& event) {
        assert(event.player_id == this->current_game().num_players());

        const unsigned new_game_index = (current_game_index_ + 1) % games_stored;
        history_[new_game_index] = history_[current_game_index_];
        history_[new_game_index].add_player(event.name);

        past_games_ = std::min(past_games_ + 1, games_stored - 1);
        future_games_ = 0;
    }

    Result apply(const GameEvent& event) {
        Game new_game = history_[current_game_index_];
        const auto result = event.function()(new_game);

        if (result) {
            current_game_index_ = (current_game_index_ + 1) % games_stored;
            history_[current_game_index_] = std::move(new_game);

            past_games_ = std::min(past_games_ + 1, games_stored - 1);
            future_games_ = 0;
        }

        return result;
    }

    bool undo() noexcept {
        if (past_games_ == 0) return false;
        current_game_index_ = (current_game_index_ > 0 ? current_game_index_ - 1 : games_stored - 1);
        --past_games_;
        ++future_games_;
    }

    bool redo() noexcept {
        if (future_games_ == 0) return false;
        current_game_index_ = (current_game_index_ + 1) % games_stored;
        ++past_games_;
        --future_games_;
    }
private:
    static const unsigned games_stored = 100;

    std::vector<Game> history_ = std::vector<Game>(games_stored);
    unsigned current_game_index_ = 0;
    unsigned past_games_ = 0;
    unsigned future_games_ = 0;
};

