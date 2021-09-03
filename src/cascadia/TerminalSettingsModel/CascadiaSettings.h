/*++
Copyright (c) Microsoft Corporation
Licensed under the MIT license.

Module Name:
- CascadiaSettings.h

Abstract:
- This class acts as the container for all app settings. It's composed of two
        parts: Globals, which are app-wide settings, and Profiles, which contain
        a set of settings that apply to a single instance of the terminal.
  Also contains the logic for serializing and deserializing this object.

Author(s):
- Mike Griese - March 2019

--*/
#pragma once

#include "CascadiaSettings.g.h"

#include "GlobalAppSettings.h"

#include "Profile.h"
#include "ColorScheme.h"

// fwdecl unittest classes
namespace SettingsModelLocalTests
{
    class SerializationTests;
    class DeserializationTests;
    class ProfileTests;
    class ColorSchemeTests;
    class KeyBindingsTests;
};
namespace TerminalAppUnitTests
{
    class DynamicProfileTests;
    class JsonTests;
};

namespace Microsoft::Terminal::Settings::Model
{
    class SettingsTypedDeserializationException;
};

class Microsoft::Terminal::Settings::Model::SettingsTypedDeserializationException final : public std::runtime_error
{
public:
    SettingsTypedDeserializationException(const std::string_view description) :
        runtime_error(description.data()) {}
};

namespace winrt::Microsoft::Terminal::Settings::Model::implementation
{
    struct ParsedSettings
    {
        [[nodiscard]] winrt::Windows::Foundation::Collections::IObservableVector<Model::Profile> foobar() const
        {
            std::vector<Model::Profile> vec;
            vec.reserve(profiles.size());

            for (const auto& p : profiles)
            {
                vec.emplace_back(*p);
            }

            return winrt::single_threaded_observable_vector(std::move(vec));
        }

        winrt::com_ptr<implementation::GlobalAppSettings> globals;
        winrt::com_ptr<implementation::Profile> baseLayerProfile;
        std::vector<winrt::com_ptr<implementation::Profile>> profiles;
        std::unordered_map<winrt::guid, winrt::com_ptr<implementation::Profile>> profilesByGuid;
    };

    struct CascadiaSettings : CascadiaSettingsT<CascadiaSettings>
    {
    public:
        static Model::CascadiaSettings LoadDefaults();
        static Model::CascadiaSettings LoadAll();
        static Model::CascadiaSettings LoadUniversal();

        static winrt::hstring SettingsPath();
        static winrt::hstring DefaultSettingsPath();
        static winrt::hstring ApplicationDisplayName();
        static winrt::hstring ApplicationVersion();

        CascadiaSettings() noexcept = default;
        CascadiaSettings(const std::string_view& defaultJson, const std::string_view& userJson = {});

        // user settings
        Model::CascadiaSettings Copy() const;
        void _CopyProfileInheritanceTree(winrt::com_ptr<CascadiaSettings>& cloneSettings) const;
        Model::GlobalAppSettings GlobalSettings() const;
        winrt::Windows::Foundation::Collections::IObservableVector<Model::Profile> AllProfiles() const noexcept;
        winrt::Windows::Foundation::Collections::IObservableVector<Model::Profile> ActiveProfiles() const noexcept;
        Model::ActionMap ActionMap() const noexcept;
        void WriteSettingsToDisk() const;
        Json::Value ToJson() const;
        Model::Profile ProfileDefaults() const;
        Model::Profile CreateNewProfile();
        Model::Profile FindProfile(const winrt::guid& guid) const noexcept;
        Model::ColorScheme GetColorSchemeForProfile(const Model::Profile& profile) const;
        void UpdateColorSchemeReferences(const winrt::hstring& oldName, const winrt::hstring& newName);
        Model::Profile GetProfileForArgs(const Model::NewTerminalArgs& newTerminalArgs) const;
        Model::Profile DuplicateProfile(const Model::Profile& source);

        // load errors
        winrt::Windows::Foundation::Collections::IVectorView<Model::SettingsLoadWarnings> Warnings() const;
        winrt::Windows::Foundation::IReference<Model::SettingsLoadErrors> GetLoadingError() const;
        winrt::hstring GetSerializationErrorMessage() const;

        // defterm
        static bool IsDefaultTerminalAvailable() noexcept;
        winrt::Windows::Foundation::Collections::IObservableVector<Model::DefaultTerminal> DefaultTerminals() const noexcept;
        Model::DefaultTerminal CurrentDefaultTerminal() noexcept;
        void CurrentDefaultTerminal(const Model::DefaultTerminal& terminal);

    private:
        static const std::filesystem::path& _settingsPath();

        winrt::com_ptr<implementation::Profile> _createNewProfile(const std::wstring_view& name) const;
        std::optional<winrt::guid> _getProfileGuidByName(const winrt::hstring& name) const;
        std::optional<winrt::guid> _getProfileGuidByIndex(std::optional<int> index) const;

        // parsing helpers
        static std::pair<size_t, size_t> _lineAndColumnFromPosition(const std::string_view string, const size_t position);
        static void _rethrowSerializationExceptionWithLocationInfo(const JsonUtils::DeserializationError& e, std::string_view settingsString);
        static Json::Value _parseJSON(const std::string_view& content);
        static const Json::Value& _getJSONValue(const Json::Value& json, const std::string_view& key) noexcept;
        static bool _isValidProfileObject(const Json::Value& profileJson);
        static ParsedSettings _parse(OriginTag origin, const std::string_view& content, std::vector<Model::SettingsLoadWarnings>& warnings);
        static void _parse(OriginTag origin, const std::string_view& content, std::vector<Model::SettingsLoadWarnings>& warnings, ParsedSettings& settings);
        static void _append(const winrt::com_ptr<implementation::Profile>& profile, std::vector<Model::SettingsLoadWarnings>& warnings, ParsedSettings& settings);

        // LoadAll helpers
        static std::unordered_set<std::wstring_view> _makeStringSet(const winrt::Windows::Foundation::Collections::IVector<winrt::hstring>& strings);
        static winrt::com_ptr<implementation::Profile> _reproduceProfile(const winrt::com_ptr<implementation::Profile>& parent);
        static void _generateProfiles(const std::unordered_set<std::wstring_view>& ignoredNamespaces, std::vector<winrt::com_ptr<implementation::Profile>>& generatedProfiles);
        static void _layerGeneratedProfiles(const std::vector<winrt::com_ptr<implementation::Profile>>& generatedProfiles, ParsedSettings& userSettings);
        static void _fillBlanksInDefaultsJson(const std::vector<winrt::com_ptr<implementation::Profile>>& generatedProfiles, const ParsedSettings& userSettings);
        static void _layerFragments(const std::unordered_set<std::wstring_view>& ignoredNamespaces, ParsedSettings& userSettings, std::vector<Model::SettingsLoadWarnings>& warnings);
        static void _disableDeletedProfiles(const gsl::span<winrt::com_ptr<implementation::Profile>>& dynamicProfiles);

        void _finalizeSettings() const;

        void _validateSettings();
        void _validateProfilesExist() const;
        void _validateDefaultProfileExists();
        void _validateAllSchemesExist();
        void _validateMediaResources();
        void _validateKeybindings() const;
        void _validateColorSchemesInCommands() const;
        bool _hasInvalidColorScheme(const Model::Command& command) const;

        // user settings
        winrt::com_ptr<implementation::GlobalAppSettings> _globals{ winrt::make_self<implementation::GlobalAppSettings>() };
        winrt::Windows::Foundation::Collections::IObservableVector<Model::Profile> _allProfiles{ winrt::single_threaded_observable_vector<Model::Profile>() };
        winrt::Windows::Foundation::Collections::IObservableVector<Model::Profile> _activeProfiles{ winrt::single_threaded_observable_vector<Model::Profile>() };
        winrt::com_ptr<implementation::Profile> _userDefaultProfileSettings;

        // load errors
        winrt::Windows::Foundation::Collections::IVector<Model::SettingsLoadWarnings> _warnings{ winrt::single_threaded_vector<SettingsLoadWarnings>() };
        winrt::Windows::Foundation::IReference<Model::SettingsLoadErrors> _loadError;
        winrt::hstring _deserializationErrorMessage;

        // defterm
        Model::DefaultTerminal _currentDefaultTerminal{ nullptr };

        friend class SettingsModelLocalTests::SerializationTests;
        friend class SettingsModelLocalTests::DeserializationTests;
        friend class SettingsModelLocalTests::ProfileTests;
        friend class SettingsModelLocalTests::ColorSchemeTests;
        friend class SettingsModelLocalTests::KeyBindingsTests;
        friend class TerminalAppUnitTests::DynamicProfileTests;
        friend class TerminalAppUnitTests::JsonTests;
    };
}

namespace winrt::Microsoft::Terminal::Settings::Model::factory_implementation
{
    BASIC_FACTORY(CascadiaSettings);
}
