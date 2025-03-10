#pragma once

#include <random>
#include "../Module.hpp"
#include "../../../../SDK/Client/Actor/ItemActor.hpp"
#include "../../../../Utils/Memory/CustomAllocator/Buffer.hpp"
#include "../../../../SDK/Client/Render/ActorRenderData.hpp"

class INeedADecentHookClassForMemory {
public:
    void* pointer = nullptr;
    void* trampoline = nullptr;
    bool valid = false;

    INeedADecentHookClassForMemory(void* function, void* hook) {
        pointer = function;

        if (IsBadReadPtr(pointer, sizeof(pointer)))
            return;

        if (const auto status = MH_CreateHook(pointer, hook, &trampoline); status == MH_OK)
            valid = true;
    }

    ~INeedADecentHookClassForMemory() {
        disable();
    }

    void enable() const {
        if (valid)
            MH_QueueEnableHook(pointer);
        MH_ApplyQueued();
    }

    void disable() const {
        if (valid)
            MH_DisableHook(pointer);
    }
};

static char data[0x5], data2[0x5];

std::unique_ptr<INeedADecentHookClassForMemory> ItemRenderer_renderHook, glm_rotateHook;

class ItemPhysics : public Module {
private:
    uint32_t origPosRel = 0;
    float* newPosRel = nullptr;
    bool patched = false;

    std::unordered_map<Actor*, std::tuple<float, Vec3<float>, Vec3<int>>> actorData;
    ActorRenderData* renderData = nullptr;
public:
    ItemPhysics() : Module("Item Physics", "Changes rotation behavior of dropped items", IDR_ITEM_PHYSICS_PNG, "") {
        Module::setup();
    }

    void onEnable() override {
        Listen(this, SetupAndRenderEvent, &ItemPhysics::onSetupAndRender)

        static auto posAddr = GET_SIG_ADDRESS("ItemPositionConst") + 4;
        origPosRel = *reinterpret_cast<uint32_t*>(posAddr);
        patched = true;

        static auto rotateAddr = reinterpret_cast<void*>(GET_SIG_ADDRESS("glm_rotateRef"));

        if (glm_rotateHook == nullptr) {
            if (VersionUtils::checkAboveOrEqual(21, 40)) {
                glm_rotateHook = std::make_unique<INeedADecentHookClassForMemory>(rotateAddr, glm_rotate2);
            }
            else {
                glm_rotateHook = std::make_unique<INeedADecentHookClassForMemory>(rotateAddr, glm_rotate);
            }
        }

        static auto ItemRenderer_renderAddr = reinterpret_cast<void*>(GET_SIG_ADDRESS("ItemRenderer::render"));

        if (ItemRenderer_renderHook == nullptr)
            ItemRenderer_renderHook = std::make_unique<INeedADecentHookClassForMemory>(ItemRenderer_renderAddr, ItemRenderer_render);

        newPosRel = static_cast<float*>(AllocateBuffer(reinterpret_cast<void*>(posAddr)));
        *newPosRel = 0.f;

        const auto newRipRel = Memory::getRipRel(posAddr, reinterpret_cast<uintptr_t>(newPosRel));

        Memory::patchBytes(reinterpret_cast<void*>(posAddr), newRipRel.data(), 4);

        glm_rotateHook->enable();

        Memory::patchBytes(rotateAddr, (BYTE*)"\xE8", 1);

        ItemRenderer_renderHook->enable();

        static auto translateAddr = reinterpret_cast<void*>(GET_SIG_ADDRESS("glm_translateRef"));
        Memory::copyBytes(translateAddr, data, 5);
        Memory::nopBytes(translateAddr, 5);

        if (VersionUtils::checkAboveOrEqual(21, 0)) {
            static auto translateAddr2 = reinterpret_cast<void*>(GET_SIG_ADDRESS("glm_translateRef2"));
            Memory::copyBytes(translateAddr2, data2, 5);
            Memory::nopBytes(translateAddr2, 5);
        }

        Module::onEnable();
    }

    void onDisable() override {
        Deafen(this, SetupAndRenderEvent, &ItemPhysics::onSetupAndRender)

        if (patched) {
            static auto posAddr = GET_SIG_ADDRESS("ItemPositionConst") + 4;

            Memory::patchBytes(reinterpret_cast<void*>(posAddr), &origPosRel, 4);
            if(newPosRel) FreeBuffer(newPosRel);
        }

        if(glm_rotateHook)
        glm_rotateHook->disable();
        if(ItemRenderer_renderHook)
        ItemRenderer_renderHook->disable();

        static auto translateAddr = reinterpret_cast<void*>(GET_SIG_ADDRESS("glm_translateRef"));
        Memory::patchBytes(translateAddr, data, 5);

        if (VersionUtils::checkAboveOrEqual(21, 0)) {
            static auto translateAddr2 = reinterpret_cast<void*>(GET_SIG_ADDRESS("glm_translateRef2"));
            Memory::patchBytes(translateAddr2, data2, 5);
        }

        actorData.clear();
        Module::onDisable();
    }

    void defaultConfig() override {
        if (settings.getSettingByName<float>("speed") == nullptr)
            settings.addSetting("speed", 8.f);

        if (settings.getSettingByName<float>("xmul") == nullptr)
            settings.addSetting("xmul", 18.f);

        if (settings.getSettingByName<float>("ymul") == nullptr)
            settings.addSetting("ymul", 16.f);

        if (settings.getSettingByName<float>("zmul") == nullptr)
            settings.addSetting("zmul", 18.f);

        if (settings.getSettingByName<bool>("preserverots") == nullptr)
            settings.addSetting("preserverots", false);

        if (settings.getSettingByName<bool>("smoothrots") == nullptr)
            settings.addSetting("smoothrots", true);
    }

    void settingsRender(float settingsOffset) override {
        /* Border Start */

        float xPos = Constraints::PercentageConstraint(0.019, "left");
        float yPos = Constraints::PercentageConstraint(0.10, "top");

        const float textWidth = Constraints::RelativeConstraint(0.12, "height", true);
        const float textHeight = Constraints::RelativeConstraint(0.029, "height", true);

        /* Speed Start */

        FlarialGUI::FlarialTextWithFont(xPos, yPos, L"Speed", textWidth * 6.9f,
                                        textHeight, DWRITE_TEXT_ALIGNMENT_LEADING,
                                        Constraints::RelativeConstraint(0.12, "height", true),
                                        DWRITE_FONT_WEIGHT_NORMAL);

        float speed = FlarialGUI::Slider(0, xPos + FlarialGUI::SettingsTextWidth("Speed "),
                                         yPos,
                                         this->settings.getSettingByName<float>("speed")->value, 15.f, 3.f);

        this->settings.getSettingByName<float>("speed")->value = speed;

        /* Speed End */

        yPos += Constraints::SpacingConstraint(0.35, textWidth);

        /* X Multiplier Start */

        FlarialGUI::FlarialTextWithFont(xPos, yPos + 0.05f, L"X Multiplier", textWidth * 6.9f,
                                        textHeight, DWRITE_TEXT_ALIGNMENT_LEADING,
                                        Constraints::RelativeConstraint(0.12, "height", true),
                                        DWRITE_FONT_WEIGHT_NORMAL);

        float xmul = FlarialGUI::Slider(1, xPos + FlarialGUI::SettingsTextWidth("X Multiplier "),
                                        yPos,
                                        this->settings.getSettingByName<float>("xmul")->value, 30.f, 7.f);

        this->settings.getSettingByName<float>("xmul")->value = xmul;

        /* X Multiplier End */

        yPos += Constraints::SpacingConstraint(0.35, textWidth);

        /* Y Multiplier Start */

        FlarialGUI::FlarialTextWithFont(xPos, yPos + 0.1f, L"Y Multiplier", textWidth * 6.9f,
                                        textHeight, DWRITE_TEXT_ALIGNMENT_LEADING,
                                        Constraints::RelativeConstraint(0.12, "height", true),
                                        DWRITE_FONT_WEIGHT_NORMAL);

        float ymul = FlarialGUI::Slider(1, xPos + FlarialGUI::SettingsTextWidth("Y Multiplier "),
                                        yPos,
                                        this->settings.getSettingByName<float>("ymul")->value, 30.f, 7.f);

        this->settings.getSettingByName<float>("ymul")->value = ymul;

        /* Y Multiplier End */

        yPos += Constraints::SpacingConstraint(0.35, textWidth);

        /* Z Multiplier Start */

        FlarialGUI::FlarialTextWithFont(xPos, yPos + 0.15f, L"Z Multiplier", textWidth * 6.9f,
                                        textHeight, DWRITE_TEXT_ALIGNMENT_LEADING,
                                        Constraints::RelativeConstraint(0.12, "height", true),
                                        DWRITE_FONT_WEIGHT_NORMAL);

        float zmul = FlarialGUI::Slider(2, xPos + FlarialGUI::SettingsTextWidth("Z Multiplier "),
                                        yPos,
                                        this->settings.getSettingByName<float>("zmul")->value, 30.f, 7.f);

        this->settings.getSettingByName<float>("zmul")->value = zmul;

        /* Z Multiplier End */

        yPos += Constraints::SpacingConstraint(0.35, textWidth);

        /* Preserve Rotations Start */

        FlarialGUI::FlarialTextWithFont(xPos + Constraints::SpacingConstraint(0.60, textWidth), yPos,
                                        L"Preserve Rotations", textWidth * 6.9f, textHeight,
                                        DWRITE_TEXT_ALIGNMENT_LEADING, Constraints::SpacingConstraint(1.05, textWidth),
                                        DWRITE_FONT_WEIGHT_NORMAL);

        if (FlarialGUI::Toggle(0, xPos, yPos, this->settings.getSettingByName<bool>(
                "preserverots")->value))
            this->settings.getSettingByName<bool>("preserverots")->value = !this->settings.getSettingByName<bool>(
                    "preserverots")->value;

        /* Preserve Rotations End */

        yPos += Constraints::SpacingConstraint(0.35, textWidth);

        /* Smooth Rotations Start */

        FlarialGUI::FlarialTextWithFont(xPos + Constraints::SpacingConstraint(0.60, textWidth), yPos,
                                        L"Smooth Rotations", textWidth * 6.9f, textHeight,
                                        DWRITE_TEXT_ALIGNMENT_LEADING, Constraints::SpacingConstraint(1.05, textWidth),
                                        DWRITE_FONT_WEIGHT_NORMAL);

        if (FlarialGUI::Toggle(1, xPos, yPos, this->settings.getSettingByName<bool>(
                "smoothrots")->value))
            this->settings.getSettingByName<bool>("smoothrots")->value = !this->settings.getSettingByName<bool>(
                    "smoothrots")->value;

        /* Smooth Rotations End */
    }

    static void ItemRenderer_render(ItemRenderer* _this, BaseActorRenderContext* renderCtx, ActorRenderData* renderData) {
        using func_t = void(*)(ItemRenderer*, BaseActorRenderContext*, ActorRenderData*);
        static auto oFunc = reinterpret_cast<func_t>(ItemRenderer_renderHook->trampoline);

        if(!ModuleManager::initialized) return oFunc(_this, renderCtx, renderData);

        static auto mod = reinterpret_cast<ItemPhysics*>(ModuleManager::getModule("Item Physics").get());
        if(!mod) return;
        mod->renderData = renderData;

        oFunc(_this, renderCtx, renderData);
    }

    static void applyTransformation(glm::mat4x4& mat) {
        static auto mod = reinterpret_cast<ItemPhysics*>(ModuleManager::getModule("Item Physics").get());
        if(!mod) return;
        const auto renderData = mod->renderData;

        auto curr = reinterpret_cast<ItemActor*>(renderData->actor);

        static float height = 0.5f;

        bool isOnGround = curr->isOnGround();

        if (!mod->actorData.contains(curr)) {
            std::random_device rd;
            std::mt19937 gen(rd());

            std::uniform_int_distribution dist(0, 1);
            std::uniform_int_distribution dist2(0, 359);

            const auto vec = Vec3<float>(dist2(gen), dist2(gen), dist2(gen));
            const auto sign = Vec3(dist(gen) * 2 - 1, dist(gen) * 2 - 1, dist(gen) * 2 - 1);

            auto def = std::tuple{ isOnGround ? 0.f : height, vec, sign};
            mod->actorData.emplace(curr, def);
        }

        const float deltaTime = 1.f / static_cast<float>(MC::fps);

        float& yMod = std::get<0>(mod->actorData.at(curr));

        yMod -= height * deltaTime;

        if (yMod <= 0.f)
            yMod = 0.f;

        Vec3<float> pos = renderData->position;
        pos.y += yMod;

        auto& vec = std::get<1>(mod->actorData.at(curr));
        auto& sign = std::get<2>(mod->actorData.at(curr));

        auto& settings = mod->settings;
        const auto speed = settings.getSettingByName<float>("speed")->value;
        const auto xMul = settings.getSettingByName<float>("xmul")->value;
        const auto yMul = settings.getSettingByName<float>("ymul")->value;
        const auto zMul = settings.getSettingByName<float>("zmul")->value;

        if (!isOnGround || yMod > 0.f) {
            vec.x += static_cast<float>(sign.x) * deltaTime * speed * xMul;
            vec.y += static_cast<float>(sign.y) * deltaTime * speed * yMul;
            vec.z += static_cast<float>(sign.z) * deltaTime * speed * zMul;

            if (vec.x > 360.f)
                vec.x -= 360.f;

            if (vec.x < 0.f)
                vec.x += 360.f;

            if (vec.y > 360.f)
                vec.y -= 360.f;

            if (vec.y < 0.f)
                vec.y += 360.f;

            if (vec.z > 360.f)
                vec.z -= 360.f;

            if (vec.z < 0.f)
                vec.z += 360.f;
        }

        Vec3<float> renderVec = vec;

        const auto smoothRotations = settings.getSettingByName<bool>("smoothrots")->value;
        const auto preserveRotations = settings.getSettingByName<bool>("preserverots")->value;

        if (isOnGround && yMod == 0.f && !preserveRotations && (sign.x != 0 || sign.y != 0 && sign.z != 0)) {
            if (smoothRotations && (sign.x != 0 || sign.y != 0 && sign.z != 0)) {
                vec.x += static_cast<float>(sign.x) * deltaTime * speed * xMul;

                if (curr->getStack().block != nullptr) {
                    vec.z += static_cast<float>(sign.z) * deltaTime * speed * zMul;

                    if (vec.x > 360.f || vec.x < 0.f) {
                        vec.x = 0.f;
                        sign.x = 0;
                    }

                    if (vec.z > 360.f || vec.z < 0.f) {
                        vec.z = 0.f;
                        sign.z = 0;
                    }
                }
                else {
                    vec.y += static_cast<float>(sign.y) * deltaTime * speed * yMul;

                    if (vec.x - 90.f > 360.f || vec.x - 90.f < 0.f) {
                        vec.x = 90.f;
                        sign.x = 0;
                    }

                    if (vec.y > 360.f || vec.y < 0.f) {
                        vec.y = 0.f;
                        sign.y = 0;
                    }
                }
            }

            if (!smoothRotations) {
                if (curr->getStack().block != nullptr) {
                    renderVec.x = 0.f;
                    renderVec.z = 0.f;
                }
                else {
                    renderVec.x = 90.f;
                    renderVec.y = 0.f;
                }
            }
        }

        if(isOnGround) {
            if(VersionUtils::checkAboveOrEqual(21, 40)) {
                if (curr->getStack().block == nullptr) {
                    pos.y -= 0.12;
                } else {
                    pos.y -= 0.3;
                }
            } else {
                if (curr->getStack().block == nullptr) {
                    pos.y -= 0.12;
                }
            }
        }

        mat = translate(mat, {pos.x, pos.y, pos.z});

        if(VersionUtils::checkAboveOrEqual(21, 40)) {
            mat = rotate(mat, glm::radians(renderVec.x), {1.f, 0.f, 0.f});
            mat = rotate(mat, glm::radians(renderVec.y), {0.f, 1.f, 0.f});
            mat = rotate(mat, glm::radians(renderVec.z), {0.f, 0.f, 1.f});
        } else {
            static auto rotateSig = GET_SIG_ADDRESS("glm_rotate");
            using glm_rotate_t = void (__fastcall *)(glm::mat4x4 &, float, float, float, float);
            static auto glm_rotate = reinterpret_cast<glm_rotate_t>(rotateSig);

            glm_rotate(mat, renderVec.x, 1.f, 0.f, 0.f);
            glm_rotate(mat, renderVec.y, 0.f, 1.f, 0.f);
            glm_rotate(mat, renderVec.z, 0.f, 0.f, 1.f);
        }
    }

    static void glm_rotate(glm::mat4x4& mat, float angle, float x, float y, float z) {
        if(!ModuleManager::initialized)
            return;

        static auto mod = reinterpret_cast<ItemPhysics*>(ModuleManager::getModule("Item Physics").get());
        if(!mod) return;
        const auto renderData = mod->renderData;

        if(renderData == nullptr)
            return;

        applyTransformation(mat);
    }

    static glm::mat4x4 glm_rotate2(glm::mat4x4& mat, float angle, const glm::vec3& axis) {
        if(!ModuleManager::initialized)
            return rotate(mat, angle, axis);

        static auto mod = reinterpret_cast<ItemPhysics*>(ModuleManager::getModule("Item Physics").get());
        if(!mod) return rotate(mat, angle, axis);
        const auto renderData = mod->renderData;

        if(renderData == nullptr)
            return rotate(mat, angle, axis);

        applyTransformation(mat);

        return mat;
    }

    void onSetupAndRender(SetupAndRenderEvent& event) {
        if (!isEnabled())
            return;

        const auto player = SDK::clientInstance->getLocalPlayer();

        static bool playerNull = player == nullptr;

        if (playerNull != (player == nullptr)) {
            playerNull = player == nullptr;

            if (playerNull) {
                actorData.clear();
                renderData = nullptr;
            }
        }
    }
};
