#pragma once

#include "DKUtil/Config.hpp"
#include "Keyboard.hpp"

using namespace DKUtil::Alias; // For type alias

#define CONFIG_OPTION(Type, _Name) Type Config_ ## _Name { #_Name, Name() }

#define CONFIG_BIND(Option, Default) Settings::GetSingleton()->MainConfig.Bind(Option, Default)

#define CONFIG_SETUP() { \
    Settings::GetSingleton()->MainConfig.Load(); \
    Settings::GetSingleton()->MainConfig.Generate(); \
    Settings::GetSingleton()->MainConfig.Write(); \
    INFO("Wrote config file"); \
}

#define CONFIG_LOAD() Settings::GetSingleton()->MainConfig.Load();

struct Settings : public dku::model::Singleton<Settings> {
    TomlConfig MainConfig = dku::Config::Proxy<dku::Config::FileType::kToml>(
        (std::filesystem::current_path() / "KbmHook.toml").string()
    );
};

class Key : public String {
    public:
        using String::String;

        int get_data_vkey() {
            auto current_val = get_data();
            if (current_val == _last_value) {
                return _last_vkey;
            }
            else {
                _last_value = current_val;
                _last_vkey = KeyNameToVKey(current_val).value_or(-1);
                return _last_vkey;
            }
        }

    private:
        std::string _last_value;
        int _last_vkey;
};
