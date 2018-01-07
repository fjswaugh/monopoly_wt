#pragma once

#include <Wt/WApplication.h>
#include <Wt/WContainerWidget.h>
#include <Wt/WServer.h>
#include <Wt/WPushButton.h>
#include <Wt/WLineEdit.h>

#include <mutex>
#include <memory>
#include <optional>

#include "game.h"
#include "game_history.h"

struct GameWidget;
struct Event;
struct GameEvent;
struct AddPlayerEvent;

struct GameServer {
    GameServer(Wt::WServer& server, std::string name)
        : wserver_{server}, name_{std::move(name)}
    {}
    GameServer(const GameServer&) = delete;
    GameServer& operator=(const GameServer&) = delete;

    bool connect(GameWidget*);

    bool disconnect(GameWidget*);

    // Login and, if necessary, create a new player in the game
    // Return the player_id if successful
    std::optional<unsigned> login(std::string username);

    // Logout but do not remove the user from the game
    void logout(unsigned player_id);

    void add_player(const AddPlayerEvent&);
    Result apply(const GameEvent&);
    Result undo() { return game_history_.undo(); }
    Result redo() { return game_history_.redo(); }

    void post(const Event&);

    const Game& game() const {
        return game_history_.current_game();
    }
private:
    struct ClientInfo {
        std::string session_id;
    };

    GameHistory game_history_;

    Wt::WServer& wserver_;
    std::string name_;
    std::map<GameWidget*, ClientInfo> clients_;

    // Set of connected player ids, a subset of the ids of the players in
    // the game.
    std::set<unsigned> player_ids_;

    std::recursive_mutex mutex_;
};

// The main job of the MainServer is to manage GameServers
struct MainServer {
    MainServer(Wt::WServer& server)
        : wserver_{server}
    {}
    MainServer(const MainServer&) = delete;
    MainServer& operator=(const MainServer&) = delete;

    GameServer* login(std::string game_name)
    {
        const auto it
            = game_servers_.try_emplace(game_name, wserver_, game_name).first;

        return &(it->second);
    }

    void interaction_loop();
private:
    // Probably need a mutex here
    Wt::WServer& wserver_;
    std::map<std::string, GameServer> game_servers_;
};

