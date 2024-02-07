#pragma once

#include "../Settings.hpp"
#include "../InputManager.hpp"

template<typename T>
struct Hook : public dku::model::Singleton<T> {
    virtual std::string Name() {
        return "GenericHook";
    }

    // Must be run before other calls
    virtual void Prepare() = 0;

    virtual void Enable() = 0;
    virtual void Disable() = 0;

    virtual bool ShouldEnable() {
        return true;
    }
};

template<class T>
concept IsHook = std::is_base_of<Hook<T>, T>::value;
