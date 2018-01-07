#include <Wt/WLogger.h>

#include "servers.h"

#include "event.h"
#include "widgets.h"

namespace {
    void log(const std::string& message) { std::cout << message << std::endl; }
}

// GameServer -----------------------------------------------------------------

bool GameServer::connect(GameWidget* client)
{
    std::unique_lock<std::recursive_mutex> lock(mutex_);

    if (clients_.count(client) == 0) {
        const auto session_id = Wt::WApplication::instance()->sessionId();
        clients_[client] = ClientInfo{session_id};
        return true;
    } else {
        return false;
    }
}

bool GameServer::disconnect(GameWidget* client)
{
    std::unique_lock<std::recursive_mutex> lock(mutex_);
    return clients_.erase(client) == 1;
}

std::optional<unsigned> GameServer::login(std::string username)
{
    std::unique_lock<std::recursive_mutex> lock(mutex_);

    const auto it
        = std::find_if(game_.players().begin(), game_.players().end(),
                       [&username](auto& x) { return x.name == username; });

    const unsigned player_id = std::distance(game_.players().begin(), it);
    if (it == game_.players().end()) {
        AddPlayerEvent e(username, game_.players().size());
        this->add_player(e);
        this->post(Event{e});
    }

    if (player_ids_.find(player_id) == player_ids_.end()) {
        player_ids_.insert(player_id);

        Event e{NotificationEvent{username + " logged in"}};
        this->post(e);

        return player_id;
    } else {
        return {};
    }
}

void GameServer::logout(unsigned player_id)
{
    std::unique_lock<std::recursive_mutex> lock(mutex_);

    const auto it = player_ids_.find(player_id);

    if (it != player_ids_.end()) {
        player_ids_.erase(it);
    }
}

void GameServer::add_player(const AddPlayerEvent& event)
{
    std::unique_lock<std::recursive_mutex> lock(mutex_);

    assert(event.player_id == game_.num_players());
    game_.add_player(event.name);
}

Result GameServer::apply(const GameEvent& event)
{
    std::unique_lock<std::recursive_mutex> lock(mutex_);

    return event.function()(game_);
}

void GameServer::post(const Event& event)
{
    log("[Game " + name_ + "] " + event.description());

    std::unique_lock<std::recursive_mutex> lock(mutex_);

    Wt::WApplication* app = Wt::WApplication::instance();

    for (auto& [client, info] : clients_) {
        /*
         * If the user corresponds to the current application, we directly
         * call the call back method. This avoids an unnecessary delay for
         * the update to the user causing the event.
         *
         * For other uses, we post it to their session. By posting the
         * event, we avoid dead-lock scenarios, race conditions, and
         * delivering the event to a session that is just about to be
         * terminated.
         */
        if (app && app->sessionId() == info.session_id) {
            client->handle_event(event);
        } else {
            // Must capture the event by value, or it may be destroyed before
            // it can be used (client here is just a pointer)
            wserver_.post(info.session_id,
                          [client, event] { client->handle_event(event); });
        }
    }
}

// MainServer -----------------------------------------------------------------

void MainServer::interaction_loop()
{
    std::string line;
    while (std::getline(std::cin, line)) {
        for (auto& [server_name, server] : game_servers_) {
            (void)server_name;
            server.post(Event{NotificationEvent{line}});
        }
    }
}

