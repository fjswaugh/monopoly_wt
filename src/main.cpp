#include <Wt/WApplication.h>
#include <Wt/WContainerWidget.h>
#include <Wt/WServer.h>
#include <Wt/WPushButton.h>
#include <Wt/WLineEdit.h>

#include <mutex>
#include <memory>
#include <set>
#include <iostream>
#include <stdexcept>
#include <thread>

#include "game.h"
#include "servers.h"
#include "event.h"
#include "widgets.h"

namespace Const {
    const char* proper_name = "Modified Monopoly";
}

struct Application : Wt::WApplication {
    Application(const Wt::WEnvironment& environment, MainServer& server)
        : Wt::WApplication{environment}, server_{server}
    {
        this->enableUpdates();

        this->setTitle(Const::proper_name);
        this->useStyleSheet("style.css");
        this->messageResourceBundle().use(this->appRoot() + "monopoly");
        // JS test

        this->root()->addWidget(std::make_unique<Wt::WText>(Const::proper_name));
        this->root()->addWidget(std::make_unique<Wt::WBreak>());

        const auto login_function = [this](LoginWidget* lw) {
            game_server_ = server_.login(lw->game_name());

            const bool banker = lw->banker();
            const bool player = !lw->user_name().empty();
            const GameWidget::Type type = banker * GameWidget::Type::banker
                                          | player * GameWidget::Type::player;

            int player_id = 0;

            if (player) {
                const std::optional<int> id
                    = game_server_->login(lw->user_name());
                if (!id) {
                    lw->bad_login();
                    return;
                }
                player_id = *id;
            }

            game_widget_ = this->root()->addWidget(
                std::make_unique<GameWidget>(*game_server_, type, player_id));
            game_server_->connect(game_widget_);
            lw->hide();
        };

        login_widget_ = this->root()->addWidget(std::make_unique<LoginWidget>(login_function));
    }
private:
    MainServer& server_;
    GameServer* game_server_ = nullptr;

    LoginWidget* login_widget_ = nullptr;
    GameWidget* game_widget_ = nullptr;
};

int main(int argc, char** argv)
try {
    Wt::WServer wserver(argc, argv, WTHTTP_CONFIGURATION);
    MainServer main_server{wserver};

    std::thread thread([&main_server] { main_server.interaction_loop(); });

    wserver.addEntryPoint(
        Wt::EntryPointType::Application,
        [&main_server](const Wt::WEnvironment& env) -> std::unique_ptr<Wt::WApplication> {
            return std::make_unique<Application>(env, main_server);
        });

    wserver.run();
    thread.join();
} catch (Wt::WServer::Exception& e) {
    std::cerr << e.what() << std::endl;
} catch (std::exception& e) {
    std::cerr << "exception: " << e.what() << std::endl;
}

