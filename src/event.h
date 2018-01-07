#pragma once

#include <string>
#include <variant>
#include <optional>
#include <functional>

#include "game.h"

struct UndoEvent {
};

struct RedoEvent {
};

struct MessageEvent {
    MessageEvent(std::string text, std::string sender_name)
        : text{sender_name + ": " + text}
    {}

    std::string text;
};

struct NotificationEvent {
    NotificationEvent(std::string text)
        : text{std::move(text)}
    {}

    std::string text;
};

struct AddPlayerEvent {
    AddPlayerEvent(std::string name, unsigned player_id)
        : name{std::move(name)}, player_id{player_id}
    {}

    std::string name;
    unsigned player_id;
};

struct GameEvent {
    using Function = std::function<Result(Game&)>;

    GameEvent(std::string description, Function apply_function)
        : description_{std::move(description)}, function_{std::move(apply_function)}
    {}

    const Function& function() const { return function_; }

    const std::string& description() const { return description_; }
private:
    std::string description_ = "An event";
    Function function_ = [](Game&) -> Result { return true; };
};

struct Event {
    using Data = std::variant<MessageEvent, NotificationEvent, GameEvent, AddPlayerEvent, UndoEvent, RedoEvent>;
    enum class Type {
        message, notification, game, add_player, undo, redo
    };

    Event(Data&& data)
        : data_(std::move(data))
    {}

    Event(const Data& data)
        : data_(data)
    {}

    Type type() const noexcept {
        return Type(data_.index());
    }

    template <Type T>
    const auto& get() const {
        return std::get<static_cast<int>(T)>(data_);
    }

    template <typename T>
    const auto& get() const {
        return std::get<T>(data_);
    }

    // Generates a higher level description of an event, useful for logging
    std::string description() const {
        struct {
            std::string operator()(const UndoEvent&) {
                return "Undo";
            }
            std::string operator()(const RedoEvent&) {
                return "Redo";
            }
            std::string operator()(const MessageEvent& e) {
                return "Message: " + e.text;
            }
            std::string operator()(const NotificationEvent& e) {
                return "Notification: " + e.text;
            }
            std::string operator()(const GameEvent& e) {
                return "Game event: " + e.description();
            }
            std::string operator()(const AddPlayerEvent& e) {
                return "Add player: " + e.name;
            }
        } v;
        return std::visit(v, data_);
    }
private:
    Data data_;
};

