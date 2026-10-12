#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <functional>
#include <utility>
#include <vector>

class PresetManager {
public:
    struct PresetInfo {
        juce::String name;
        juce::String category;
        bool isBuiltIn = false;
        juce::File file;
    };

    explicit PresetManager(juce::AudioProcessorValueTreeState& parameterState);
    void setStateLoadedCallback(std::function<void()> callback);

    static int getBuiltInPresetCount() noexcept;
    static PresetInfo getBuiltInPresetInfo(int index);
    static std::vector<std::pair<juce::String, float>> getBuiltInPresetValues(int index);
    static juce::String builtInPresetId(int index);
    static juce::String userPresetId(const juce::File& file);
    bool loadBuiltInPreset(int index);

    juce::File getUserPresetDirectory() const;
    std::vector<PresetInfo> getUserPresets() const;
    juce::StringArray getFavoritePresetIds() const;
    juce::StringArray getRecentPresetIds() const;
    bool setPresetFavorite(const juce::String& id, bool favorite) const;
    void recordRecentPreset(const juce::String& id) const;
    bool saveUserPreset(const juce::String& name, const juce::String& category,
                        juce::File& savedFile, juce::String& error) const;
    bool overwriteUserPreset(const juce::File& file, juce::String& error) const;
    bool duplicateBuiltInPreset(int index, const juce::String& newName,
                                juce::File& duplicateFile, juce::String& error) const;
    bool duplicateUserPreset(const juce::File& source, const juce::String& newName,
                             juce::File& duplicateFile, juce::String& error) const;
    bool renameUserPreset(const juce::File& source, const juce::String& newName,
                          juce::File& renamedFile, juce::String& error) const;
    bool deleteUserPreset(const juce::File& file, juce::String& error) const;
    bool exportPreset(const juce::File& destination, const juce::String& name,
                      const juce::String& category, juce::String& error) const;
    bool exportUserPreset(const juce::File& source, const juce::File& destination,
                          juce::String& error) const;
    bool loadPreset(const juce::File& file, juce::String& error);
    bool importPreset(const juce::File& source, juce::File& importedFile, juce::String& error) const;

private:
    bool makePresetDocument(const juce::String& name, const juce::String& category,
                            juce::String& document, juce::String& error) const;
    bool makePresetDocumentFromState(const juce::ValueTree& state, const juce::String& name,
                                     const juce::String& category, juce::String& document,
                                     juce::String& error) const;
    bool readPresetDocument(const juce::File& file, juce::ValueTree& state,
                            juce::String& name, juce::String& category,
                            juce::String& error) const;
    bool validateStateXml(const juce::String& xml, juce::ValueTree& state,
                          juce::String& error) const;
    bool writePresetFile(const juce::File& destination, const juce::String& name,
                         const juce::String& category, juce::String& error) const;
    bool writePresetFileFromState(const juce::File& destination, const juce::String& name,
                                  const juce::String& category, const juce::ValueTree& state,
                                  juce::String& error) const;
    bool isUserPresetFile(const juce::File& file) const;
    juce::File getLibraryMetadataFile() const;
    juce::StringArray getLibraryArray(const juce::Identifier& property) const;
    bool writeLibraryMetadata(const juce::StringArray& favorites,
                              const juce::StringArray& recent) const;

    juce::AudioProcessorValueTreeState& parameters_;
    std::function<void()> stateLoadedCallback_;
};
