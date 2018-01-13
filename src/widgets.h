#pragma once

#include <Wt/WContainerWidget.h>
#include <Wt/WGroupBox.h>
#include <Wt/WButtonGroup.h>
#include <Wt/WLineEdit.h>
#include <Wt/WText.h>
#include <Wt/WPushButton.h>
#include <Wt/WCheckBox.h>
#include <Wt/WBreak.h>
#include <Wt/WComboBox.h>
#include <Wt/WTable.h>
#include <Wt/WHBoxLayout.h>

#include <cstdint>
#include <optional>
#include <memory>

#include "game.h"

struct GameServer;
struct Event;
struct MessageEvent;
struct NotificationEvent;
struct GameWidget;
struct GameEvent;

bool is_positive_int(const std::string&);

template <typename T>
std::optional<T> get(const Wt::WLineEdit* const input_box)
{
    if (!input_box) return {};

    if constexpr (std::is_same_v<T, int>) {
        const std::string s = input_box->text().narrow();
        if (!is_positive_int(s)) return {};
        return std::stoi(s);
    } else if constexpr (std::is_same_v<T, std::string>) {
        return input_box->text().narrow();
    }
}

template <typename InputType>
struct InputWidget : Wt::WGroupBox {
    InputWidget(std::string title, std::string button_text)
    {
        this->setTitle(title);
        input_box_ = this->addWidget(std::make_unique<Wt::WLineEdit>());
        button_ = this->addWidget(std::make_unique<Wt::WPushButton>(button_text));
    }

    template <typename F_of_InputType>
    void connect(F_of_InputType&& callback)
    {
        const auto function = [&callback, this] {
            auto i = get<InputType>(input_box_);
            input_box_->setText("");
            if (i) callback(*i);
        };

        button_->mouseWentDown().connect(function);
        input_box_->enterPressed().connect(function);
    }
private:
    Wt::WPushButton* button_ = nullptr;
    Wt::WLineEdit* input_box_ = nullptr;
};

// Widget that displays a log of messages and lets users send messages
struct MessageWidget : Wt::WContainerWidget {
    MessageWidget(GameServer&, std::optional<unsigned> player_id, bool banker);

    void push(std::string);
private:
    Wt::WLineEdit* input_box_ = nullptr;
    Wt::WPushButton* send_message_button_ = nullptr;

    Wt::WContainerWidget* messages_ = nullptr;

    GameServer& server_;
};

// Widget that displays general game information, accessible to everyone
struct InfoWidget : Wt::WContainerWidget {
    InfoWidget(GameServer& server);

    void update();
    void add_player(unsigned player_id);
private:
    Wt::WText* secured_interest_ = nullptr;
    Wt::WText* unsecured_interest_ = nullptr;
    Wt::WText* ppi_ = nullptr;

    Wt::WTable* player_table_ = nullptr;
    std::vector<std::array<Wt::WText*, 7>> player_info_;

    GameServer& server_;
};

// Widget containing controls specific to a certain player
struct PlayerWidget : Wt::WContainerWidget {
    PlayerWidget(GameServer&, unsigned player_id);
    
    void update();
private:
    struct PropertySelectWidget : Wt::WContainerWidget {
        PropertySelectWidget(const Property& property)
        {
            auto* container = this->addWidget(std::make_unique<Wt::WContainerWidget>());
            auto* hbox = container->setLayout(std::make_unique<Wt::WHBoxLayout>());
            checkbox_ = hbox->addWidget(std::make_unique<Wt::WCheckBox>());
            hbox->addWidget(std::make_unique<Wt::WText>(property.name));
        }

        bool checked() const {
            return checkbox_->isChecked();
        }
        void check() {
            checkbox_->setChecked(true);
        }
        void uncheck() {
            checkbox_->setChecked(false);
        }
    private:
        Wt::WCheckBox* checkbox_;
    };

    GameServer& server_;
    unsigned player_id_;

    // Actions:
    Wt::WComboBox* buy_combobox_ = nullptr;
    Wt::WLineEdit* buy_amount_ = nullptr;
    Wt::WPushButton* buy_button_ = nullptr;

    std::array<PropertySelectWidget*, 28> properties_;

    Wt::WPushButton* sell_properties_ = nullptr;
    Wt::WPushButton* mortgage_properties_ = nullptr;
    Wt::WPushButton* unmortgage_properties_ = nullptr;

    Wt::WLineEdit* amount_to_transfer_ = nullptr;
    Wt::WComboBox* players_combobox_ = nullptr;
    Wt::WPushButton* transfer_to_player_ = nullptr;

    Wt::WLineEdit* number_of_houses_buy_ = nullptr;
    Wt::WLineEdit* number_of_houses_sell_ = nullptr;
    Wt::WPushButton* buy_houses_button_ = nullptr;
    Wt::WPushButton* sell_houses_button_ = nullptr;
    // Pay repairs

    InputWidget<int>* pay_to_bank_ = nullptr;
    /*
    Wt::WLineEdit* amount_to_pay_ = nullptr;
    Wt::WPushButton* pay_to_bank_ = nullptr;
    */

    Wt::WLineEdit* amount_to_receive_ = nullptr;
    Wt::WPushButton* receive_from_bank_ = nullptr;

    Wt::WLineEdit* take_out_amount_ = nullptr;
    std::shared_ptr<Wt::WButtonGroup> take_out_type_ = nullptr;
    Wt::WPushButton* take_out_ = nullptr;

    Wt::WLineEdit* pay_off_amount_ = nullptr;
    std::shared_ptr<Wt::WButtonGroup> pay_off_type_ = nullptr;
    Wt::WPushButton* pay_off_ = nullptr;

    Wt::WPushButton* pass_go_ = nullptr;

    // Concede to another player (takes out max debt and transfers all assets to player)
    // Concede to the bank (transfers all assets to the bank)
    Wt::WComboBox* concede_combobox_ = nullptr;
    Wt::WPushButton* concede_ = nullptr;
};

// Widget containing controls for everything
struct BankerWidget : Wt::WContainerWidget {
    BankerWidget(GameServer&);

    void update();
private:
    GameServer& server_;

    // Probably has several player widgets here
    // Actions:
    //     Increase/decrease interest rates
    //     Perform any action a player can perform on behalf of any player
    //     Give money to any player
    //     Undo last action
    //     Redo an undo
    //     End the game and destroy the GameServer, ideally taking everyone back to the login page
    Wt::WPushButton* increase_rates_ = nullptr;
    Wt::WPushButton* decrease_rates_ = nullptr;

    Wt::WPushButton* undo_ = nullptr;
    Wt::WPushButton* redo_ = nullptr;
};

struct GameWidget : Wt::WContainerWidget {
    struct Type {
        constexpr Type(std::uint8_t i) noexcept : value_{i} {}
        constexpr operator std::uint8_t() const noexcept { return value_; }

        static const Type player;
        static const Type banker;
    private:
        std::uint8_t value_;
    };

    GameWidget(GameServer&, Type, unsigned player_id = 0);

    void handle_event(const Event&);
private:
    GameServer& server_;
    bool banker_;
    std::optional<unsigned> player_id_ = {};

    MessageWidget* message_widget_ = nullptr;
    InfoWidget* info_widget_ = nullptr;
    PlayerWidget* player_widget_ = nullptr;
    BankerWidget* banker_widget_ = nullptr;
};

struct LoginWidget : Wt::WContainerWidget {
    LoginWidget(const std::function<void(LoginWidget*)>& login_function);

    std::string game_name() const {
        return game_name_field_->text().narrow();
    }
    std::string user_name() const {
        return user_name_field_->text().narrow();
    }
    bool banker() const {
        return banker_checkbox_->isChecked();
    }

    void bad_login() {
        this->addWidget(std::make_unique<Wt::WText>("Bad login"));
    }
private:
    Wt::WLineEdit* game_name_field_;
    Wt::WLineEdit* user_name_field_;
    Wt::WCheckBox* banker_checkbox_;
    Wt::WPushButton* login_button_;
};

