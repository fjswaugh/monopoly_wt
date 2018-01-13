#include "widgets.h"
#include "event.h"
#include "servers.h"
#include "game.h"

#include <optional>
#include <Wt/WVBoxLayout.h>
#include <Wt/WRadioButton.h>

#include "Popup.h"

using namespace std::literals;

bool is_positive_int(const std::string& s)
{
    return !s.empty() &&
           std::find_if(s.begin(), s.end(), [](char c) { return !std::isdigit(c); }) == s.end();
}

namespace {

void attempt_to_send(const UndoEvent& event, GameServer& server, Wt::WContainerWidget* widget)
{
    const Result r = server.undo();

    if (r) {
        server.post(Event{event});
        server.post(Event{NotificationEvent{r.description()}});
    } else {
        if (widget) {
            auto* popup = widget->addChild(
                std::make_unique<Popup>(Popup::Alert, r.description(), ""));
            popup->show.exec();
        }
    }
}
void attempt_to_send(const RedoEvent& event, GameServer& server, Wt::WContainerWidget* widget)
{
    const Result r = server.redo();

    if (r) {
        server.post(Event{event});
        server.post(Event{NotificationEvent{r.description()}});
    } else {
        if (widget) {
            auto* popup = widget->addChild(
                std::make_unique<Popup>(Popup::Alert, r.description(), ""));
            popup->show.exec();
        }
    }
}

void attempt_to_send(const GameEvent& event, GameServer& server, Wt::WContainerWidget* widget)
{
    const Result r = server.apply(event);

    if (r) {
        server.post(Event{event});
        server.post(Event{NotificationEvent{r.description()}});
    } else {
        if (widget) {
            auto* popup = widget->addChild(
                std::make_unique<Popup>(Popup::Alert, "Error: " + r.description(), ""));
            popup->show.exec();
        }
    }
}

// Returns -1 if it can't retrieve a positive int
int get_positive_int(const Wt::WLineEdit* const line_edit)
{
    if (!line_edit) return -1;

    const std::string s = line_edit->text().narrow();

    if (!is_positive_int(s)) return -1;

    return std::stoi(s);
}

}

// MessageWidget --------------------------------------------------------------

namespace {
    void scroll_to_bottom(Wt::WContainerWidget* w) {
        if (w) {
            Wt::WApplication::instance()->doJavaScript(
                w->jsRef() + ".scrollTop += " + w->jsRef() + ".scrollHeight;");
        }
    }

    std::string name_from_id(std::optional<unsigned> player_id, const Game& game) {
        if (player_id) return game.player(*player_id).name;
        return "Anonymous";
    }
}

MessageWidget::MessageWidget(GameServer& server, std::optional<unsigned> player_id,
                             bool banker)
    : server_{server}
{
    messages_ = this->addWidget(std::make_unique<Wt::WContainerWidget>());
    messages_->setHeight(100);
    messages_->setOverflow(Wt::Overflow::Auto);

    input_box_ = this->addWidget(std::make_unique<Wt::WLineEdit>(""));
    send_message_button_
        = this->addWidget(std::make_unique<Wt::WPushButton>("Send"));

    const auto send_message = [this, player_id] {
        const auto message_text = input_box_->text();
        input_box_->setText("");
        const auto e = Event(MessageEvent{
            message_text.narrow(), name_from_id(player_id, server_.game())});
        server_.post(e);
    };

    send_message_button_->mouseWentDown().connect(send_message);
    input_box_->enterPressed().connect(send_message);

    // Send a welcome message to the user
    const std::string welcome_message = [this, player_id, banker] {
        std::string txt
            = "Welcome "s + (banker ? "Banker " : "")
              + (player_id ? server_.game().player(*player_id).name : ""s);
        if (txt == "Welcome ") txt += "casual observer";
        return txt;
    }();
    this->push(welcome_message);
}

// Push a string to the message widget to display
void MessageWidget::push(std::string s)
{
    messages_->addWidget(std::make_unique<Wt::WBreak>());
    messages_->addWidget(std::make_unique<Wt::WText>(std::move(s)));

    scroll_to_bottom(messages_);
}

// InfoWidget -----------------------------------------------------------------

InfoWidget::InfoWidget(GameServer& server)
    : server_{server}
{
    secured_interest_ = this->addWidget(std::make_unique<Wt::WText>(""));
    this->addWidget(std::make_unique<Wt::WBreak>());
    unsecured_interest_ = this->addWidget(std::make_unique<Wt::WText>(""));
    this->addWidget(std::make_unique<Wt::WBreak>());
    ppi_ = this->addWidget(std::make_unique<Wt::WText>(""));

    // Player information
    player_table_ = this->addWidget(std::make_unique<Wt::WTable>());
    player_table_->elementAt(0, 0)->addWidget(std::make_unique<Wt::WText>("Name"));
    player_table_->elementAt(0, 1)->addWidget(std::make_unique<Wt::WText>("Cash"));
    player_table_->elementAt(0, 2)->addWidget(std::make_unique<Wt::WText>("Asset value"));
    player_table_->elementAt(0, 3)->addWidget(std::make_unique<Wt::WText>("Expected income"));
    player_table_->elementAt(0, 4)->addWidget(std::make_unique<Wt::WText>("Secured debt"));
    player_table_->elementAt(0, 5)->addWidget(std::make_unique<Wt::WText>("Unsecured debt"));
    player_table_->elementAt(0, 6)->addWidget(std::make_unique<Wt::WText>("Interest to pay"));
    for (unsigned player_id = 0; player_id < server_.game().num_players(); ++player_id) {
        this->add_player(player_id);
    }

    this->update();
}

void InfoWidget::add_player(unsigned player_id)
{
    player_info_.push_back(std::array<Wt::WText*, 7>{
        player_table_->elementAt(player_id + 1, 0)->addWidget(std::make_unique<Wt::WText>()),
        player_table_->elementAt(player_id + 1, 1)->addWidget(std::make_unique<Wt::WText>()),
        player_table_->elementAt(player_id + 1, 2)->addWidget(std::make_unique<Wt::WText>()),
        player_table_->elementAt(player_id + 1, 3)->addWidget(std::make_unique<Wt::WText>()),
        player_table_->elementAt(player_id + 1, 4)->addWidget(std::make_unique<Wt::WText>()),
        player_table_->elementAt(player_id + 1, 5)->addWidget(std::make_unique<Wt::WText>()),
        player_table_->elementAt(player_id + 1, 6)->addWidget(std::make_unique<Wt::WText>())
    });
}

void InfoWidget::update()
{
    const auto& game = server_.game();

    secured_interest_->setText("Secured interest: " + std::to_string(game.secured_interest()));
    unsecured_interest_->setText("Unsecured interest: " +
                                 std::to_string(game.unsecured_interest()));
    ppi_->setText("PPI: " + std::to_string(server_.game().ppi));

    // Player information
    assert(player_info_.size() <= game.num_players());
    for (unsigned player_id = 0; player_id < player_info_.size(); ++player_id) {
        const auto& player = game.player(player_id);
        player_info_[player_id][0]->setText(player.name);
        player_info_[player_id][1]->setText(std::to_string(player.cash));
        player_info_[player_id][2]->setText(std::to_string(asset_value(player, game)));
        player_info_[player_id][3]->setText(std::to_string(expected_income(player, game)));
        player_info_[player_id][4]->setText(std::to_string(player.secured_debt) + "/" +
                                            std::to_string(max_secured_debt(player, game)));
        player_info_[player_id][5]->setText(std::to_string(player.unsecured_debt) + "/" +
                                            std::to_string(max_unsecured_debt(player, game)));
        player_info_[player_id][6]->setText(std::to_string(interest_to_pay(player, game)));
    }
}

// PlayerWidget

PlayerWidget::PlayerWidget(GameServer& server, unsigned player_id)
    : server_{server}, player_id_{player_id}
{
    const auto& game = server_.game();
    const auto& player = game.player(player_id_);

    { // Buy property
        buy_combobox_ = this->addWidget(std::make_unique<Wt::WComboBox>());
        buy_amount_ = this->addWidget(std::make_unique<Wt::WLineEdit>());
        buy_button_ = this->addWidget(std::make_unique<Wt::WPushButton>("Buy property"));
        const auto buy_function = [this, &game, &player] {
            const int amount = get_positive_int(buy_amount_);
            if (amount < 0) return;
            const unsigned property_id =
                game.id_of_property(buy_combobox_->currentText().narrow());
            buy_amount_->setText("");

            const auto function = [=](Game& g) {
                return buy_property(g, player_id_, property_id, amount);
            };

            const GameEvent event{function};
            attempt_to_send(event, server_, this);
        };
        buy_button_->mouseWentDown().connect(buy_function);
    }

    // Property selector/displayer
    auto* container = this->addWidget(std::make_unique<Wt::WContainerWidget>());
    auto* vbox = container->setLayout(std::make_unique<Wt::WVBoxLayout>());
    for (unsigned i = 0; i < 28; ++i) {
        const auto& property = server_.game().properties[i];
        properties_[i] = vbox->addWidget(std::make_unique<PropertySelectWidget>(property));
        properties_[i]->setHidden(true);
    }

    { // Sell property
        sell_properties_ = this->addWidget(std::make_unique<Wt::WPushButton>("Sell properties"));
        const auto sell_function = [this] {
            for (unsigned property_id = 0; property_id < 28; ++property_id) {
                if (!properties_[property_id]->checked()) continue;

                const auto function = [=](Game& g) {
                    return sell_property(g, player_id_, property_id);
                };

                const GameEvent event{function};
                attempt_to_send(event, server_, this);
            }
        };
        sell_properties_->mouseWentDown().connect(sell_function);
    }

    { // Mortgage property
        mortgage_properties_ =
            this->addWidget(std::make_unique<Wt::WPushButton>("Mortgage properties"));
        const auto mortgage_function = [this] {
            for (unsigned property_id = 0; property_id < 28; ++property_id) {
                if (!properties_[property_id]->checked()) continue;

                const auto function = [=](Game& g) {
                    return mortgage(g, player_id_, property_id);
                };

                const GameEvent event{function};
                attempt_to_send(event, server_, this);
            }
        };
        mortgage_properties_->mouseWentDown().connect(mortgage_function);
    }

    { // Unmortgage property
        unmortgage_properties_ =
            this->addWidget(std::make_unique<Wt::WPushButton>("Unmortgage properties"));
        const auto unmortgage_function = [this] {
            for (unsigned property_id = 0; property_id < 28; ++property_id) {
                if (!properties_[property_id]->checked()) continue;

                const auto function = [=](Game& g) {
                    return unmortgage(g, player_id_, property_id);
                };

                const GameEvent event{function};
                attempt_to_send(event, server_, this);
            }
        };
        unmortgage_properties_->mouseWentDown().connect(unmortgage_function);
    }

    this->addWidget(std::make_unique<Wt::WBreak>());

    {
        amount_to_transfer_ = this->addWidget(std::make_unique<Wt::WLineEdit>());
        players_combobox_ = this->addWidget(std::make_unique<Wt::WComboBox>());
        transfer_to_player_ = this->addWidget(std::make_unique<Wt::WPushButton>("Transfer"));
        const auto transfer_function = [this] {
            const unsigned from_player_id = player_id_;
            const unsigned to_player_id = players_combobox_->currentIndex();
            const int amount = get_positive_int(amount_to_transfer_);
            if (amount < 0) return;
            amount_to_transfer_->setText("");
            const PropertySet properties = [this]() {
                PropertySet set;
                for (unsigned property_id = 0; property_id < 28; ++property_id) {
                    set[property_id] = properties_[property_id]->checked();
                }
                return set;
            }();

            const auto function = [=](Game& g) {
                return transfer(g, from_player_id, to_player_id, amount, properties);
            };

            const GameEvent event{function};
            attempt_to_send(event, server_, this);
        };
        amount_to_transfer_->enterPressed().connect(transfer_function);
        transfer_to_player_->mouseWentDown().connect(transfer_function);
    }

    this->addWidget(std::make_unique<Wt::WBreak>());

    {
        number_of_houses_buy_ = this->addWidget(std::make_unique<Wt::WLineEdit>());
        buy_houses_button_ = this->addWidget(std::make_unique<Wt::WPushButton>("Buy houses"));
        number_of_houses_sell_ = this->addWidget(std::make_unique<Wt::WLineEdit>());
        sell_houses_button_ = this->addWidget(std::make_unique<Wt::WPushButton>("Sell houses"));

        const auto buy_houses_function = [this] {
            const int number = get_positive_int(number_of_houses_buy_);
            if (number < 0) return;
            number_of_houses_buy_->setText("");

            const PropertySet properties = [this]() {
                PropertySet set;
                for (unsigned property_id = 0; property_id < 28; ++property_id) {
                    set[property_id] = properties_[property_id]->checked();
                }
                return set;
            }();

            const auto function = [=](Game& g) {
                return build_houses(g, player_id_, properties, number);
            };

            const GameEvent event{function};
            attempt_to_send(event, server_, this);
        };

        const auto sell_houses_function = [this] {
            const int number = get_positive_int(number_of_houses_sell_);
            if (number < 0) return;
            number_of_houses_sell_->setText("");

            const PropertySet properties = [this]() {
                PropertySet set;
                for (unsigned property_id = 0; property_id < 28; ++property_id) {
                    set[property_id] = properties_[property_id]->checked();
                }
                return set;
            }();

            const auto function = [=](Game& g) {
                return sell_houses(g, player_id_, properties, number);
            };

            const GameEvent event{function};
            attempt_to_send(event, server_, this);
        };

        number_of_houses_buy_->enterPressed().connect(buy_houses_function);
        buy_houses_button_->mouseWentDown().connect(buy_houses_function);
        number_of_houses_sell_->enterPressed().connect(sell_houses_function);
        sell_houses_button_->mouseWentDown().connect(sell_houses_function);
    }

    this->addWidget(std::make_unique<Wt::WBreak>());

    { // Pay to bank
        pay_to_bank_ = this->addWidget(
            std::make_unique<InputWidget<int>>("Pay to bank", "Pay"));
        pay_to_bank_->connect([this](int amount) {
            const GameEvent event{
                [=](Game& g) { return pay_to_bank(g, player_id_, amount); }};
            attempt_to_send(event, server_, this);
        });

        /*
        amount_to_pay_ = this->addWidget(std::make_unique<Wt::WLineEdit>());
        pay_to_bank_ = this->addWidget(std::make_unique<Wt::WPushButton>("Pay to bank"));

        const auto pay_to_bank_function = [this] {
            const int amount = get_positive_int(amount_to_receive_);
            if (amount < 0) return;

            const GameEvent event{[=](Game& g) { return pay_to_bank(g, player_id_, amount); }};
            attempt_to_send(event, server_, this);
            amount_to_pay_->setText("");
        };

        pay_to_bank_->mouseWentDown().connect(pay_to_bank_function);
        amount_to_pay_->enterPressed().connect(pay_to_bank_function);
        */
    }

    { // Receive from bank
        amount_to_receive_ = this->addWidget(std::make_unique<Wt::WLineEdit>());
        receive_from_bank_ = this->addWidget(std::make_unique<Wt::WPushButton>("Take from bank"));

        const auto take_from_bank_function = [this] {
            const int amount = get_positive_int(amount_to_receive_);
            if (amount < 0) return;

            const GameEvent event{[=](Game& g) { return pay_to_player(g, player_id_, amount); }};
            attempt_to_send(event, server_, this);
            amount_to_receive_->setText("");
        };

        receive_from_bank_->mouseWentDown().connect(take_from_bank_function);
        amount_to_receive_->enterPressed().connect(take_from_bank_function);
    }

    this->addWidget(std::make_unique<Wt::WBreak>());

    struct Debt {
        enum _ : int { secured, unsecured };
    };

    { // Take out debt
        take_out_amount_ = this->addWidget(std::make_unique<Wt::WLineEdit>());
        take_out_type_ = std::make_shared<Wt::WButtonGroup>();
        auto* secured = this->addWidget(std::make_unique<Wt::WRadioButton>("Secured"));
        auto* unsecured = this->addWidget(std::make_unique<Wt::WRadioButton>("Unsecured"));
        take_out_type_->addButton(secured, Debt::secured);
        take_out_type_->addButton(unsecured, Debt::unsecured);
        take_out_type_->setCheckedButton(secured);
        take_out_ = this->addWidget(std::make_unique<Wt::WPushButton>("Take out debt"));

        const auto take_out_debt_function = [this] {
            const std::string amount_str = take_out_amount_->text().narrow();
            if (!is_positive_int(amount_str)) return;

            const bool secured = take_out_type_->checkedId() == Debt::secured;
            const int amount = std::stoi(amount_str);
            take_out_amount_->setText("");

            const auto secured_function = [=](Game& g) {
                return take_out_secured_debt(g, player_id_, amount);
            };
            const auto unsecured_function = [=](Game& g) {
                return take_out_unsecured_debt(g, player_id_, amount);
            };

            const GameEvent event = secured ? GameEvent{secured_function}
                                            : GameEvent{unsecured_function};
            attempt_to_send(event, server_, this);
        };

        take_out_->mouseWentDown().connect(take_out_debt_function);
    }

    this->addWidget(std::make_unique<Wt::WBreak>());

    { // Pay off debt
        pay_off_amount_ = this->addWidget(std::make_unique<Wt::WLineEdit>());
        pay_off_type_ = std::make_shared<Wt::WButtonGroup>();
        auto* secured = this->addWidget(std::make_unique<Wt::WRadioButton>("Secured"));
        auto* unsecured = this->addWidget(std::make_unique<Wt::WRadioButton>("Unsecured"));
        pay_off_type_->addButton(secured, Debt::secured);
        pay_off_type_->addButton(unsecured, Debt::unsecured);
        pay_off_type_->setCheckedButton(secured);
        pay_off_ = this->addWidget(std::make_unique<Wt::WPushButton>("Pay off debt"));

        const auto pay_off_debt_function = [this] {
            const std::string amount_str = pay_off_amount_->text().narrow();
            if (!is_positive_int(amount_str)) return;

            const bool secured = pay_off_type_->checkedId() == Debt::secured;
            const int amount = std::stoi(amount_str);
            pay_off_amount_->setText("");

            const auto secured_function = [=](Game& g) {
                return pay_off_secured_debt(g, player_id_, amount);
            };
            const auto unsecured_function = [=](Game& g) {
                return pay_off_unsecured_debt(g, player_id_, amount);
            };

            const GameEvent event = secured ? GameEvent{secured_function}
                                            : GameEvent{unsecured_function};
            attempt_to_send(event, server_, this);
        };

        pay_off_->mouseWentDown().connect(pay_off_debt_function);
    }

    this->addWidget(std::make_unique<Wt::WBreak>());

    { // Pass go
        pass_go_ = this->addWidget(
            std::make_unique<Wt::WPushButton>("Pass go (collect salary and pay interest)"));

        const auto pass_go_function = [this] {
            const GameEvent event{[=](Game& g) { return passgo(g, player_id_); }};
            attempt_to_send(event, server_, this);
        };
        pass_go_->mouseWentDown().connect(pass_go_function);
    }

    this->update();
}

void PlayerWidget::update()
{
    // Property selector/display
    const auto& all_properties = server_.game().properties;
    for (unsigned i = 0; i < 28; ++i) {
        if (all_properties[i].owner_id == player_id_) {
            properties_[i]->setHidden(false);
        } else {
            // Make sure the box is unchecked before hiding it
            properties_[i]->uncheck();
            properties_[i]->setHidden(true);
        }
    }

    // Buy property
    buy_combobox_->clear();
    for (const auto& property : server_.game().properties) {
        if (!property.owner_id) buy_combobox_->addItem(property.name);
    }

    // Players combobox
    players_combobox_->clear();
    for (const auto& player : server_.game().players()) {
        players_combobox_->addItem(player.name);
    }
}

// BankerWidget ---------------------------------------------------------------

BankerWidget::BankerWidget(GameServer& server)
    : server_{server}
{
    increase_rates_ = this->addWidget(
        std::make_unique<Wt::WPushButton>("Increase interest rates"));
    decrease_rates_ = this->addWidget(
        std::make_unique<Wt::WPushButton>("Decrease interest rates"));

    increase_rates_->mouseWentDown().connect([this] {
        const GameEvent event{raise_interest};
        attempt_to_send(event, server_, this);
    });
    decrease_rates_->mouseWentDown().connect([this] {
        const GameEvent event{lower_interest};
        attempt_to_send(event, server_, this);
    });

    undo_ = this->addWidget(std::make_unique<Wt::WPushButton>("Undo"));
    redo_ = this->addWidget(std::make_unique<Wt::WPushButton>("Redo"));

    undo_->mouseWentDown().connect(
        [this] { attempt_to_send(UndoEvent{}, server_, this); });
    redo_->mouseWentDown().connect(
        [this] { attempt_to_send(RedoEvent{}, server_, this); });
}

void BankerWidget::update()
{
}

// GameWidget -----------------------------------------------------------------

const GameWidget::Type GameWidget::Type::player = 0b01;
const GameWidget::Type GameWidget::Type::banker = 0b10;

GameWidget::GameWidget(GameServer& server, Type type, unsigned player_id)
    : server_{server}, banker_{(type & Type::banker) > 0}
{
    if (type & Type::player) {
        player_id_ = player_id;
    }

    this->addWidget(std::make_unique<Wt::WText>("Monopoly game"));

    message_widget_ = this->addWidget(
        std::make_unique<MessageWidget>(server_, player_id_, banker_));

    info_widget_ = this->addWidget(std::make_unique<InfoWidget>(server_));

    if (player_id_) {
        player_widget_ = this->addWidget(
            std::make_unique<PlayerWidget>(server_, *player_id_));
    }

    if (banker_) {
        banker_widget_
            = this->addWidget(std::make_unique<BankerWidget>(server_));
    }
}

void GameWidget::handle_event(const Event& event)
{
    switch (event.type()) {
    case Event::Type::undo:
        if (info_widget_) info_widget_->update();
        if (player_widget_) player_widget_->update();
        if (banker_widget_) banker_widget_->update();
        break;
    case Event::Type::redo:
        if (info_widget_) info_widget_->update();
        if (player_widget_) player_widget_->update();
        if (banker_widget_) banker_widget_->update();
        break;
    case Event::Type::message:
        if (message_widget_) message_widget_->push(event.get<MessageEvent>().text);
        break;
    case Event::Type::notification:
        if (message_widget_) message_widget_->push(event.get<NotificationEvent>().text);
        break;
    case Event::Type::game:
        if (info_widget_) info_widget_->update();
        if (player_widget_) player_widget_->update();
        if (banker_widget_) banker_widget_->update();
        break;
    case Event::Type::add_player:
        if (info_widget_) {
            info_widget_->add_player(event.get<AddPlayerEvent>().player_id);
            info_widget_->update();
        }
        if (player_widget_) player_widget_->update();
        break;
    }

    Wt::WApplication::instance()->triggerUpdate();
}

// LoginWidget ----------------------------------------------------------------

LoginWidget::LoginWidget(
    const std::function<void(LoginWidget*)>& login_function)
{
    game_name_field_ = this->addWidget(std::make_unique<Wt::WLineEdit>(""));
    user_name_field_ = this->addWidget(std::make_unique<Wt::WLineEdit>(""));
    banker_checkbox_ = this->addWidget(std::make_unique<Wt::WCheckBox>());
    login_button_
        = this->addWidget(std::make_unique<Wt::WPushButton>("Login"));

    login_button_->mouseWentDown().connect(
        [this, login_function] { login_function(this); });
}

