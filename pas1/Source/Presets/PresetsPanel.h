#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

#include "PluginProcessor.h"
#include "UI/Utf8Text.h"

#include <memory>
#include <vector>

class PresetsPanel final : public juce::Component, private juce::ListBoxModel {
public:
    explicit PresetsPanel(Pas1AudioProcessor& processor);
    void paint(juce::Graphics&) override;
    void resized() override;
    void refreshLibrary();

private:
    struct Entry {
        PresetManager::PresetInfo info;
        juce::String id;
        int builtInIndex = -1;
    };

    int getNumRows() override;
    void paintListBoxItem(int row, juce::Graphics&, int width, int height, bool selected) override;
    void selectedRowsChanged(int lastRowSelected) override;
    void listBoxItemDoubleClicked(int row, const juce::MouseEvent&) override;

    void refresh();
    void rebuildCategoryFilter();
    void updateActionState();
    void selectPresetInLibrary(const juce::String& id);
    const Entry* selectedEntry() const;
    void loadSelected();
    void toggleFavorite();
    void saveSelected();
    void saveAs();
    void duplicateSelected();
    void renameSelected();
    void deleteSelected();
    void importPreset();
    void exportSelected();
    void showError(const juce::String& message);
    void setStatus(const juce::String& message);
    bool isFavorite(const juce::String& id) const;

    Pas1AudioProcessor& processor_;
    juce::TextEditor search;
    juce::ComboBox categoryFilter;
    juce::ComboBox sortOrder;
    juce::ListBox presetList;
    juce::TextEditor status;
    juce::TextButton loadButton{pas1::ui::utf8String("ЗАГРУЗИТЬ")};
    juce::TextButton favoriteButton{pas1::ui::utf8String("ИЗБРАННОЕ")};
    juce::TextButton saveButton{pas1::ui::utf8String("СОХРАНИТЬ")};
    juce::TextButton saveAsButton{pas1::ui::utf8String("СОХРАНИТЬ КАК")};
    juce::TextButton duplicateButton{pas1::ui::utf8String("ДУБЛИРОВАТЬ")};
    juce::TextButton renameButton{pas1::ui::utf8String("ПЕРЕИМЕНОВАТЬ")};
    juce::TextButton deleteButton{pas1::ui::utf8String("УДАЛИТЬ")};
    juce::TextButton importButton{pas1::ui::utf8String("ИМПОРТ")};
    juce::TextButton exportButton{pas1::ui::utf8String("ЭКСПОРТ")};
    std::unique_ptr<juce::FileChooser> fileChooser;
    std::vector<Entry> allEntries;
    std::vector<Entry> visibleEntries;
    juce::StringArray categories;
    juce::StringArray favoriteIds;
    juce::StringArray recentIds;
    juce::String selectedId;
};
