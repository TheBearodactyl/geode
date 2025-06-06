#pragma once

#include <Geode/DefaultInclude.hpp>
#include "Setting.hpp"

namespace geode {
    class Mod;
    class SettingV3;

    class GEODE_DLL ModSettingsManager final {
    private:
        class Impl;
        std::unique_ptr<Impl> m_impl;

        friend class ::geode::SettingV3;
        friend class ::geode::Mod;

        void markRestartRequired();

    public:
        static ModSettingsManager* from(Mod* mod);

        ModSettingsManager(ModMetadata const& metadata);
        ~ModSettingsManager();

        ModSettingsManager(ModSettingsManager&&);
        ModSettingsManager(ModSettingsManager const&) = delete;

        /**
         * Load setting values from savedata.
         * The format of the savedata should be an object with the keys being
         * setting IDs and then the values the values of the saved settings
         * @returns Ok if no horrible errors happened. Note that a setting value
         * missing is not considered a horrible error, but will instead just log a
         * warning into the console!
         */
        Result<> load(matjson::Value const& json);
        /**
         * Save setting values to savedata.
         * The format of the savedata will be an object with the keys being
         * setting IDs and then the values the values of the saved settings
         * @note If saving a setting fails, it will log a warning to the console
         */
        matjson::Value save();

        /**
         * Get the savedata for settings, aka the JSON object that contains all
         * the settings' saved states that was loaded up from disk and will be
         * saved to disk
         * @warning Modifying this will modify the value of the settings - use
         * carefully!
         */
        matjson::Value& getSaveData();

        Result<> registerCustomSettingType(std::string_view type, SettingGenerator generator);

        std::shared_ptr<Setting> get(std::string_view key);

        /**
         * Returns true if any setting with the `"restart-required"` attribute
         * has been altered
         */
        bool restartRequired() const;
    };
}
