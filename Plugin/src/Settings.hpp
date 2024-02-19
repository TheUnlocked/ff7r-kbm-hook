#pragma once

#include "DKUtil/Config.hpp"
#include "Keyboard.hpp"

using namespace DKUtil::Alias; // For type alias

#define CONFIG_OPTION(Type, _Name) Type Config_ ## _Name { #_Name, Name() }

#define CONFIG_BIND(Option, ...) Settings::GetSingleton()->MainConfig.Bind(Option, __VA_ARGS__)

#define CONFIG_LOAD() Settings::GetSingleton()->MainConfig.Load();

#define CONFIG_SETUP() CONFIG_LOAD()

struct Settings : public dku::model::Singleton<Settings> {
    TomlConfig MainConfig = dku::Config::Proxy<dku::Config::FileType::kToml>(
        (std::filesystem::current_path() / fmt::format("{}.toml", Plugin::NAME)).string()
    );
};

class Key : public String {
    public:
        using String::String;

        int get_vkey_data() {
            auto current_data = get_data();
            if (current_data == _last_data) {
                return _last_vkey;
            }
            else {
                _last_data = current_data;
                _last_vkey = KeyNameToVKey(current_data).value_or(0);
                return _last_vkey;
            }
        }

        std::vector<int> get_vkey_collection() {
            auto const &current_collection = is_collection()
                ? get_collection()
                : std::vector { get_data() };

            if (current_collection == _last_collection) {
                return _last_vkey_collection;
            }
            else {
                _last_collection.clear();
                _last_vkey_collection.clear();

                for (auto keyname : current_collection) {
                    _last_collection.push_back(keyname);

                    auto vkey = KeyNameToVKey(keyname);
                    if (vkey.has_value()) {
                        _last_vkey_collection.push_back(vkey.value());
                    }
                }

                return _last_vkey_collection;
            }
        }

    private:
        std::string _last_data;
        int _last_vkey;
        std::vector<std::string> _last_collection;
        std::vector<int> _last_vkey_collection;
};
