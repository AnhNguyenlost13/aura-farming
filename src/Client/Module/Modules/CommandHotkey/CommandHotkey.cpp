#include "CommandHotkey.hpp"

#include "Events/EventManager.hpp"

void CommandHotkey::onSetup() {

    for (int i = 0; i < totalKeybinds; ++i) {
        keybindActions.push_back([this, i](std::vector<std::any> args) -> std::any {

            KeyEvent event = std::any_cast<KeyEvent>(args[0]);
            std::chrono::duration<double> duration = std::chrono::high_resolution_clock::now() - last_used;
            if (duration.count() >= 2.5) {
                std::string count;
                if (i > 0) count = "-" + FlarialGUI::cached_to_string(i);
                if (this->isKeybind(event.keys, i) && this->isKeyPartOfKeybind(event.key, i)) {
                    std::shared_ptr<Packet> packet = SDK::createPacket(77);
                    auto* command_packet = reinterpret_cast<CommandRequestPacket*>(packet.get());
                    command_packet->command = this->settings.getSettingByName<std::string>("command" + count)->value;
                    command_packet->origin.type = CommandOriginType::Player;
                    command_packet->InternalSource = true;
                    SDK::clientInstance->getPacketSender()->sendToServer(command_packet);

                    last_used = std::chrono::high_resolution_clock::now();
                }
            }

            return {};

        });
    }
}

void CommandHotkey::onEnable() {
    Listen(this, KeyEvent, &CommandHotkey::onKey)
    Module::onEnable();
}

void CommandHotkey::onDisable() {
    Deafen(this, KeyEvent, &CommandHotkey::onKey)
    Module::onDisable();
}

void CommandHotkey::defaultConfig() {
    getKeybind();
    Module::defaultConfig("core");
    setDef("command", (std::string)"");
}

void CommandHotkey::settingsRender(float settingsOffset) {
    float x = Constraints::PercentageConstraint(0.019, "left");
    float y = Constraints::PercentageConstraint(0.10, "top");

    const float scrollviewWidth = Constraints::RelativeConstraint(0.12, "height", true);

    FlarialGUI::ScrollBar(x, y, 140, 5000, 2);
    FlarialGUI::SetScrollView(x - settingsOffset, Constraints::PercentageConstraint(0.00, "top"),
                              Constraints::RelativeConstraint(1.0, "width"),
                              Constraints::RelativeConstraint(0.88f, "height"));

    this->addHeader("Command Hotkey");
    this->addButton("Add another Keybind", "Multi-Keybind command support!", "Add", [&] {

        std::string keybindName = "keybind-" + FlarialGUI::cached_to_string(totalKeybinds);
        std::string commandName = "command-" + FlarialGUI::cached_to_string(totalKeybinds);

        this->settings.addSetting(keybindName, (std::string)"");
        this->settings.addSetting(commandName, (std::string)"");


        int i = totalKeybinds;
        keybindActions.push_back([this, i](std::vector<std::any> args) -> std::any {
            KeyEvent event = std::any_cast<KeyEvent>(args[0]);
            std::chrono::duration<double> duration = std::chrono::high_resolution_clock::now() - last_used;
            if (duration.count() >= 2.5) {
                std::string count;
                if (i > 0) count = "-" + FlarialGUI::cached_to_string(i);
                if (this->isKeybind(event.keys, i) && this->isKeyPartOfKeybind(event.key, i)) {
                    std::shared_ptr<Packet> packet = SDK::createPacket(77);
                    auto* command_packet = reinterpret_cast<CommandRequestPacket*>(packet.get());
                    command_packet->command = this->settings.getSettingByName<std::string>("command" + count)->value;
                    command_packet->origin.type = CommandOriginType::Player;
                    command_packet->InternalSource = true;
                    SDK::clientInstance->getPacketSender()->sendToServer(command_packet);

                    last_used = std::chrono::high_resolution_clock::now();
                }
            }
            return {};
        });

        totalKeybinds++;
        FlarialGUI::Notify("Added! Scroll down for options.");
        this->saveSettings();
    });


    for (int i = 0; i < totalKeybinds; ++i) {

        std::string header = (i == 0) ? "Command" : "Command " + FlarialGUI::cached_to_string(i);
        std::string commandSettingName = (i == 0) ? "command" : "command-" + FlarialGUI::cached_to_string(i);

        if (settings.getSettingByName<std::string>(commandSettingName) != nullptr) {
            this->addHeader(header);

            this->addKeybind("Command Keybind", "Hold for 2 seconds!", "keybind" + i, true);

            this->addTextBox(
                "Command to Send",
                "No need for /, And there's a spam limit!",
                settings.getSettingByName<std::string>(commandSettingName)->value
            );
            this->extraPadding();
        }
    }

    FlarialGUI::UnsetScrollView();

    this->resetPadding();
}

void CommandHotkey::onKey(KeyEvent &event) {
    if (!SDK::clientInstance->getLocalPlayer()) return;
    if (this->isEnabled() && SDK::getCurrentScreen() == "hud_screen") {
        for (int i = 0; i <= totalKeybinds; ++i) {
            keybindActions[i]({ std::any(event) });
        }
    }
}
