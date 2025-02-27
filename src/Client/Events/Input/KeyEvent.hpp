
#pragma once

#include <array>
#include "../Event.hpp"
#include "../Cancellable.hpp"
#include "../../../Utils/Utils.hpp"

enum class ActionType {
    Released = 0,
    Pressed = 1
};

class KeyEvent : public Event, public Cancellable {

public:
    int key;
    int vkKey;
    int action;
    std::array<bool, 256> keys{};

    KeyEvent(int key, int vk, int action, const std::array<bool, 256> &keys) : Event() {
        this->key = key;
        this->vkKey = vk;
        this->action = action;
        this->keys = keys;
    };

    [[nodiscard]] int getKey() const {
        return key;
    }

    [[nodiscard]] int getVirtualKey() const {
        return vkKey;
    }

    void setKey(int e) {
        this->key = e;
    }

    [[nodiscard]] ActionType getAction() const {
        return (ActionType)this->action;
    }

    [[nodiscard]] std::string getKeyAsString(bool isCapital) const {

        return Utils::getKeyAsString(key, isCapital, false);

    }


    void setAction(int e) {
        this->action = e;
    }

    std::string getPressedKeysAsString() {

        std::string result;
        int i = 0;
        bool found = false;

        for (bool b: keys) {

            if (b) {
                found = true;
                if (!result.empty()) {
                    result += "+";
                }
                result += Utils::getKeyAsString(i, false);
            }

            i++;
        }

        if (found)
            return result;
        else return "no";

    }

    [[nodiscard]] std::array<bool, 256> getPressedKeysAsArray() const {
        return this->keys;
    }
};



