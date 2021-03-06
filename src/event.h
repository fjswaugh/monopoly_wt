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

    GameEvent(Function apply_function)
        : function_{std::move(apply_function)}
    {}

    const Function& function() const { return function_; }
private:
    Function function_ = [](Game&) -> Result { return true; };
};

struct Event {
    using Data = std::variant<MessageEvent, NotificationEvent, GameEvent,
                              AddPlayerEvent, UndoEvent, RedoEvent>;
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
                return "Undo event";
            }
            std::string operator()(const RedoEvent&) {
                return "Redo event";
            }
            std::string operator()(const MessageEvent& e) {
                return "Message: " + e.text;
            }
            std::string operator()(const NotificationEvent& e) {
                return "Notification: " + e.text;
            }
            std::string operator()(const GameEvent&) {
                return "Game event";
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

