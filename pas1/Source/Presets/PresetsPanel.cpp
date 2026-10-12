#include "PresetsPanel.h"

#include <algorithm>

namespace {
const juce::Colour background{0xff302015};
const juce::Colour accent{0xffd8b36a};
const juce::Colour text{0xfff4e8cf};
const juce::Colour muted{0xffbba483};
const juce::Colour outline{0xff745332};

void styleButton(juce::TextButton& button) {
    button.setColour(juce::TextButton::buttonColourId, background);
    button.setColour(juce::TextButton::textColourOffId, text);
    button.setColour(juce::TextButton::textColourOnId, accent);
}

void styleCombo(juce::ComboBox& combo) {
    combo.setColour(juce::ComboBox::backgroundColourId, background);
    combo.setColour(juce::ComboBox::textColourId, text);
    combo.setColour(juce::ComboBox::outlineColourId, outline);
}
}

PresetsPanel::PresetsPanel(Pas1AudioProcessor& processor) : processor_(processor) {
    search.setTextToShowWhenEmpty("Search presets by name or category", muted);
    search.setColour(juce::TextEditor::backgroundColourId, background);
    search.setColour(juce::TextEditor::textColourId, text);
    search.setColour(juce::TextEditor::outlineColourId, outline);
    search.onTextChange = [this] { refresh(); };
    addAndMakeVisible(search);

    categoryFilter.setTextWhenNothingSelected("All categories");
    styleCombo(categoryFilter);
    categoryFilter.onChange = [this] { refresh(); };
    addAndMakeVisible(categoryFilter);

    sortOrder.addItem("Name A–Z", 1);
    sortOrder.addItem("Category", 2);
    sortOrder.addItem("Recently used", 3);
    sortOrder.setSelectedId(1, juce::dontSendNotification);
    styleCombo(sortOrder);
    sortOrder.onChange = [this] { refresh(); };
    addAndMakeVisible(sortOrder);

    presetList.setModel(this);
    presetList.setRowHeight(48);
    presetList.setColour(juce::ListBox::backgroundColourId, background);
    presetList.setColour(juce::ListBox::outlineColourId, outline);
    addAndMakeVisible(presetList);

    status.setMultiLine(true);
    status.setReadOnly(true);
    status.setScrollbarsShown(false);
    status.setColour(juce::TextEditor::textColourId, muted);
    status.setColour(juce::TextEditor::backgroundColourId, background);
    status.setColour(juce::TextEditor::outlineColourId, outline);
    status.setFont(juce::FontOptions{12.0f});
    addAndMakeVisible(status);

    for (auto* button : {&loadButton, &favoriteButton, &saveButton, &saveAsButton, &duplicateButton,
                         &renameButton, &deleteButton, &importButton, &exportButton}) {
        styleButton(*button);
        addAndMakeVisible(*button);
    }
    loadButton.onClick = [this] { loadSelected(); };
    favoriteButton.onClick = [this] { toggleFavorite(); };
    saveButton.onClick = [this] { saveSelected(); };
    saveAsButton.onClick = [this] { saveAs(); };
    duplicateButton.onClick = [this] { duplicateSelected(); };
    renameButton.onClick = [this] { renameSelected(); };
    deleteButton.onClick = [this] { deleteSelected(); };
    importButton.onClick = [this] { importPreset(); };
    exportButton.onClick = [this] { exportSelected(); };

    search.setTooltip("Search the preset name and category.");
    categoryFilter.setTooltip("Filter the library by category, favorites, or recent use.");
    sortOrder.setTooltip("Choose how the filtered preset list is ordered.");
    favoriteButton.setTooltip("Add or remove the selected preset from favorites.");
    duplicateButton.setTooltip("Create a user-library copy of the selected preset.");
    renameButton.setTooltip("Rename a user preset without changing its sound.");
    deleteButton.setTooltip("Permanently delete the selected user preset file.");
    refresh();
}

void PresetsPanel::refreshLibrary() {
    refresh();
}

void PresetsPanel::paint(juce::Graphics& graphics) {
    graphics.setColour(background);
    graphics.fillRoundedRectangle(getLocalBounds().toFloat(), 12.0f);
    graphics.setColour(outline);
    graphics.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 12.0f, 1.0f);
    graphics.setColour(muted);
    graphics.setFont(juce::FontOptions{10.0f, juce::Font::bold});
    const auto factoryCount = juce::String(PresetManager::getBuiltInPresetCount());
    graphics.drawText(juce::String{"PRESET LIBRARY  ·  "} + factoryCount + " FACTORY SOUNDS",
                      16, 48, 300, 14, juce::Justification::centredLeft);
}

void PresetsPanel::resized() {
    const auto width = getWidth();
    const auto height = getHeight();
    search.setBounds(14, 12, std::max(180, width - 530), 30);
    categoryFilter.setBounds(search.getRight() + 8, 12, 190, 30);
    sortOrder.setBounds(categoryFilter.getRight() + 8, 12, 150, 30);

    const auto actionsX = width - 250;
    presetList.setBounds(14, 66, actionsX - 26, height - 80);
    const auto compact = height < 430;
    const auto buttonHeight = compact ? 28 : 34;
    const auto buttonGap = compact ? 5 : 8;
    auto actionY = 68;
    for (auto* button : {&loadButton, &favoriteButton, &saveButton, &saveAsButton, &duplicateButton,
                         &renameButton, &deleteButton}) {
        button->setBounds(actionsX, actionY, 236, buttonHeight);
        actionY += buttonHeight + buttonGap;
    }
    importButton.setBounds(actionsX, actionY, 112, buttonHeight);
    exportButton.setBounds(actionsX + 120, actionY, 116, buttonHeight);
    status.setBounds(actionsX, actionY + buttonHeight + 8, 236,
                     std::max(32, height - actionY - buttonHeight - 16));
}

int PresetsPanel::getNumRows() {
    return static_cast<int>(visibleEntries.size());
}

void PresetsPanel::paintListBoxItem(int row, juce::Graphics& graphics, int width, int height,
                                    bool selected) {
    if (row < 0 || row >= static_cast<int>(visibleEntries.size()))
        return;
    const auto& entry = visibleEntries[static_cast<std::size_t>(row)];
    graphics.setColour(selected ? juce::Colour{0xff49331f} : background);
    graphics.fillRect(0, 0, width, height);
    graphics.setColour(selected ? accent : outline);
    graphics.fillRect(0, 0, 3, height);

    graphics.setColour(text);
    graphics.setFont(juce::FontOptions{14.0f, juce::Font::bold});
    graphics.drawText(entry.info.name, 14, 5, width - 140, 20,
                      juce::Justification::centredLeft, true);
    graphics.setColour(muted);
    graphics.setFont(juce::FontOptions{11.0f});
    graphics.drawText(entry.info.category + (entry.info.isBuiltIn ? "  ·  Factory" : "  ·  User"),
                      14, 26, width - 140, 16, juce::Justification::centredLeft, true);
    if (isFavorite(entry.id)) {
        graphics.setColour(accent);
        graphics.setFont(juce::FontOptions{15.0f});
        graphics.drawText("★", width - 34, 9, 24, 24, juce::Justification::centred);
    }
}

void PresetsPanel::selectedRowsChanged(int) {
    const auto* entry = selectedEntry();
    selectedId = entry != nullptr ? entry->id : juce::String{};
    updateActionState();
}

void PresetsPanel::listBoxItemDoubleClicked(int row, const juce::MouseEvent&) {
    presetList.selectRow(row);
    loadSelected();
}

void PresetsPanel::refresh() {
    const auto previousSelection = selectedId;
    allEntries.clear();
    for (int index = 0; index < PresetManager::getBuiltInPresetCount(); ++index) {
        auto info = PresetManager::getBuiltInPresetInfo(index);
        allEntries.push_back({info, PresetManager::builtInPresetId(index), index});
    }
    for (auto& info : processor_.presetManager.getUserPresets())
        allEntries.push_back({info, PresetManager::userPresetId(info.file), -1});

    rebuildCategoryFilter();
    favoriteIds = processor_.presetManager.getFavoritePresetIds();
    recentIds = processor_.presetManager.getRecentPresetIds();
    const auto searchText = search.getText().trim().toLowerCase();
    const auto filterId = categoryFilter.getSelectedId();
    const auto categoryIndex = filterId >= 100 ? filterId - 100 : -1;
    const auto selectedCategory = categoryIndex >= 0 && categoryIndex < categories.size()
                                      ? categories[categoryIndex]
                                      : juce::String{};
    const auto favoritesOnly = filterId == 2;
    const auto recentOnly = filterId == 3;

    visibleEntries.clear();
    for (const auto& entry : allEntries) {
        if (favoritesOnly && !favoriteIds.contains(entry.id))
            continue;
        if (recentOnly && !recentIds.contains(entry.id))
            continue;
        if (selectedCategory.isNotEmpty() && entry.info.category != selectedCategory)
            continue;
        if (searchText.isNotEmpty()
            && !entry.info.name.toLowerCase().contains(searchText)
            && !entry.info.category.toLowerCase().contains(searchText))
            continue;
        visibleEntries.push_back(entry);
    }

    const auto selectedSort = sortOrder.getSelectedId();
    std::sort(visibleEntries.begin(), visibleEntries.end(),
              [this, selectedSort](const Entry& left, const Entry& right) {
        if (selectedSort == 3) {
            const auto leftRecent = recentIds.indexOf(left.id);
            const auto rightRecent = recentIds.indexOf(right.id);
            if (leftRecent != rightRecent)
                return leftRecent >= 0 && (rightRecent < 0 || leftRecent < rightRecent);
        } else if (selectedSort == 2 && left.info.category != right.info.category) {
            return left.info.category.compareIgnoreCase(right.info.category) < 0;
        }
        const auto leftFavorite = favoriteIds.contains(left.id);
        const auto rightFavorite = favoriteIds.contains(right.id);
        if (leftFavorite != rightFavorite)
            return leftFavorite;
        return left.info.name.compareIgnoreCase(right.info.name) < 0;
    });

    presetList.updateContent();
    int selectedRow = -1;
    for (std::size_t index = 0; index < visibleEntries.size(); ++index)
        if (visibleEntries[index].id == previousSelection)
            selectedRow = static_cast<int>(index);
    if (selectedRow < 0 && !visibleEntries.empty())
        selectedRow = 0;
    presetList.selectRow(selectedRow, juce::dontSendNotification);
    selectedId = selectedRow >= 0
                     ? visibleEntries[static_cast<std::size_t>(selectedRow)].id
                     : juce::String{};
    updateActionState();
    presetList.repaint();
}

void PresetsPanel::rebuildCategoryFilter() {
    const auto previous = categoryFilter.getSelectedId();
    const auto previousCategory = previous >= 100 && previous - 100 < categories.size()
                                      ? categories[previous - 100]
                                      : juce::String{};
    categories.clear();
    for (const auto& entry : allEntries)
        if (!categories.contains(entry.info.category))
            categories.add(entry.info.category);
    categories.sort(true);

    categoryFilter.clear(juce::dontSendNotification);
    categoryFilter.addItem("All categories", 1);
    categoryFilter.addItem("Favorites", 2);
    categoryFilter.addItem("Recent", 3);
    for (int index = 0; index < categories.size(); ++index)
        categoryFilter.addItem(categories[index], 100 + index);
    auto nextId = 1;
    if (previous == 2 || previous == 3)
        nextId = previous;
    else if (previousCategory.isNotEmpty()) {
        const auto index = categories.indexOf(previousCategory);
        if (index >= 0)
            nextId = 100 + index;
    }
    categoryFilter.setSelectedId(nextId, juce::dontSendNotification);
}

void PresetsPanel::updateActionState() {
    const auto* entry = selectedEntry();
    const auto hasSelection = entry != nullptr;
    loadButton.setEnabled(hasSelection);
    favoriteButton.setEnabled(hasSelection);
    saveButton.setEnabled(hasSelection && !entry->info.isBuiltIn);
    saveAsButton.setEnabled(true);
    favoriteButton.setButtonText(hasSelection && isFavorite(entry->id)
                                     ? "REMOVE FAVORITE" : "ADD FAVORITE");
    duplicateButton.setEnabled(hasSelection);
    renameButton.setEnabled(hasSelection && !entry->info.isBuiltIn);
    deleteButton.setEnabled(hasSelection && !entry->info.isBuiltIn);
    exportButton.setEnabled(hasSelection && !entry->info.isBuiltIn);
}

void PresetsPanel::selectPresetInLibrary(const juce::String& id) {
    selectedId = id;
    search.clear();
    categoryFilter.setSelectedId(1, juce::dontSendNotification);
    refresh();
}

const PresetsPanel::Entry* PresetsPanel::selectedEntry() const {
    const auto row = presetList.getSelectedRow();
    return row >= 0 && row < static_cast<int>(visibleEntries.size())
               ? &visibleEntries[static_cast<std::size_t>(row)]
               : nullptr;
}

void PresetsPanel::loadSelected() {
    const auto* entry = selectedEntry();
    if (entry == nullptr)
        return;
    auto loadedName = entry->info.name;
    if (entry->builtInIndex >= 0) {
        if (!processor_.presetManager.loadBuiltInPreset(entry->builtInIndex)) {
            showError("The selected built-in preset could not be loaded.");
            return;
        }
    } else {
        juce::String error;
        if (!processor_.presetManager.loadPreset(entry->info.file, error)) {
            showError(error);
            return;
        }
    }
    processor_.presetManager.recordRecentPreset(entry->id);
    setStatus("Loaded “" + loadedName + "”.");
    refresh();
}

void PresetsPanel::toggleFavorite() {
    const auto* entry = selectedEntry();
    if (entry == nullptr)
        return;
    const auto wasFavorite = isFavorite(entry->id);
    if (!processor_.presetManager.setPresetFavorite(entry->id, !wasFavorite)) {
        showError("Could not update the preset library metadata file.");
        return;
    }
    setStatus(wasFavorite ? "Removed from favorites." : "Added to favorites.");
    refresh();
}

void PresetsPanel::saveSelected() {
    const auto* entry = selectedEntry();
    if (entry == nullptr || entry->info.isBuiltIn)
        return;
    const auto selected = *entry;
    juce::Component::SafePointer<PresetsPanel> safeThis(this);
    juce::AlertWindow::showOkCancelBox(
        juce::MessageBoxIconType::QuestionIcon, "Update User Preset",
        "Replace “" + selected.info.name + "” with the current synth settings?",
        "Update", "Cancel", this,
        juce::ModalCallbackFunction::create([safeThis, selected](int result) {
            if (safeThis == nullptr || result != 1)
                return;
            juce::String error;
            if (!safeThis->processor_.presetManager.overwriteUserPreset(
                    selected.info.file, error)) {
                safeThis->showError(error);
                return;
            }
            safeThis->setStatus("Updated the selected user preset.");
            safeThis->refresh();
        }));
}

void PresetsPanel::saveAs() {
    const auto* selected = selectedEntry();
    const auto defaultName = selected != nullptr ? selected->info.name + " Copy"
                                                 : juce::String{"My Preset"};
    const auto defaultCategory = selected != nullptr ? selected->info.category
                                                     : juce::String{"User Presets"};
    auto* alert = new juce::AlertWindow("Save PAS 1 Preset",
                                        "Save the current sound to your user library.",
                                        juce::MessageBoxIconType::NoIcon);
    alert->addTextEditor("name", defaultName, "Name");
    alert->addTextEditor("category", defaultCategory, "Category");
    alert->addButton("Save", 1, juce::KeyPress{juce::KeyPress::returnKey});
    alert->addButton("Cancel", 0, juce::KeyPress{juce::KeyPress::escapeKey});
    juce::Component::SafePointer<juce::AlertWindow> safeAlert(alert);
    juce::Component::SafePointer<PresetsPanel> safeThis(this);
    alert->enterModalState(true, juce::ModalCallbackFunction::create([safeAlert, safeThis](int result) {
        if (safeThis == nullptr || safeAlert == nullptr || result != 1)
            return;
        juce::File savedFile;
        juce::String error;
        if (!safeThis->processor_.presetManager.saveUserPreset(
                safeAlert->getTextEditorContents("name"),
                safeAlert->getTextEditorContents("category"), savedFile, error)) {
            safeThis->showError(error);
            return;
        }
        safeThis->setStatus("Saved a user preset.");
        safeThis->selectPresetInLibrary(PresetManager::userPresetId(savedFile));
    }), true);
}

void PresetsPanel::duplicateSelected() {
    const auto* entry = selectedEntry();
    if (entry == nullptr)
        return;
    const auto source = *entry;
    auto* alert = new juce::AlertWindow("Duplicate PAS 1 Preset",
                                        "Create an editable copy in the user library.",
                                        juce::MessageBoxIconType::NoIcon);
    alert->addTextEditor("name", source.info.name + " Copy", "New name");
    alert->addButton("Duplicate", 1, juce::KeyPress{juce::KeyPress::returnKey});
    alert->addButton("Cancel", 0, juce::KeyPress{juce::KeyPress::escapeKey});
    juce::Component::SafePointer<juce::AlertWindow> safeAlert(alert);
    juce::Component::SafePointer<PresetsPanel> safeThis(this);
    alert->enterModalState(true, juce::ModalCallbackFunction::create([safeAlert, safeThis, source](int result) {
        if (safeThis == nullptr || safeAlert == nullptr || result != 1)
            return;
        juce::File copy;
        juce::String error;
        const auto name = safeAlert->getTextEditorContents("name");
        const auto success = source.builtInIndex >= 0
                                 ? safeThis->processor_.presetManager.duplicateBuiltInPreset(
                                       source.builtInIndex, name, copy, error)
                                 : safeThis->processor_.presetManager.duplicateUserPreset(
                                       source.info.file, name, copy, error);
        if (!success) {
            safeThis->showError(error);
            return;
        }
        safeThis->setStatus("Created a user-library copy.");
        safeThis->selectPresetInLibrary(PresetManager::userPresetId(copy));
    }), true);
}

void PresetsPanel::renameSelected() {
    const auto* entry = selectedEntry();
    if (entry == nullptr || entry->info.isBuiltIn)
        return;
    const auto source = *entry;
    auto* alert = new juce::AlertWindow("Rename User Preset", "Rename the preset file and its metadata.",
                                        juce::MessageBoxIconType::NoIcon);
    alert->addTextEditor("name", source.info.name, "New name");
    alert->addButton("Rename", 1, juce::KeyPress{juce::KeyPress::returnKey});
    alert->addButton("Cancel", 0, juce::KeyPress{juce::KeyPress::escapeKey});
    juce::Component::SafePointer<juce::AlertWindow> safeAlert(alert);
    juce::Component::SafePointer<PresetsPanel> safeThis(this);
    alert->enterModalState(true, juce::ModalCallbackFunction::create([safeAlert, safeThis, source](int result) {
        if (safeThis == nullptr || safeAlert == nullptr || result != 1)
            return;
        juce::File renamed;
        juce::String error;
        if (!safeThis->processor_.presetManager.renameUserPreset(
                source.info.file, safeAlert->getTextEditorContents("name"), renamed, error)) {
            safeThis->showError(error);
            return;
        }
        safeThis->setStatus("Renamed the user preset.");
        safeThis->selectPresetInLibrary(PresetManager::userPresetId(renamed));
    }), true);
}

void PresetsPanel::deleteSelected() {
    const auto* entry = selectedEntry();
    if (entry == nullptr || entry->info.isBuiltIn)
        return;
    const auto source = *entry;
    juce::Component::SafePointer<PresetsPanel> safeThis(this);
    juce::AlertWindow::showOkCancelBox(
        juce::MessageBoxIconType::WarningIcon, "Delete User Preset",
        "Permanently delete “" + source.info.name + "” from the user preset library?",
        "Delete", "Cancel", this,
        juce::ModalCallbackFunction::create([safeThis, source](int result) {
            if (safeThis == nullptr || result != 1)
                return;
            juce::String error;
            if (!safeThis->processor_.presetManager.deleteUserPreset(source.info.file, error)) {
                safeThis->showError(error);
                return;
            }
            safeThis->selectedId.clear();
            safeThis->setStatus("Deleted the user preset.");
            safeThis->refresh();
        }));
}

void PresetsPanel::importPreset() {
    fileChooser = std::make_unique<juce::FileChooser>(
        "Import PAS 1 preset", processor_.presetManager.getUserPresetDirectory(), "*.pas1preset");
    juce::Component::SafePointer<PresetsPanel> safeThis(this);
    fileChooser->launchAsync(juce::FileBrowserComponent::openMode
                                 | juce::FileBrowserComponent::canSelectFiles,
                             [safeThis](const juce::FileChooser& chooser) {
        if (safeThis == nullptr)
            return;
        const auto source = chooser.getResult();
        if (!source.existsAsFile())
            return;
        juce::File importedFile;
        juce::String error;
        if (!safeThis->processor_.presetManager.importPreset(source, importedFile, error)) {
            safeThis->showError(error);
            return;
        }
        safeThis->setStatus("Imported a preset into the user library.");
        safeThis->selectPresetInLibrary(PresetManager::userPresetId(importedFile));
    });
}

void PresetsPanel::exportSelected() {
    const auto* entry = selectedEntry();
    if (entry == nullptr || entry->info.isBuiltIn)
        return;
    const auto source = *entry;
    fileChooser = std::make_unique<juce::FileChooser>(
        "Export PAS 1 preset", juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                                  .getChildFile(source.info.file.getFileName()),
        "*.pas1preset");
    juce::Component::SafePointer<PresetsPanel> safeThis(this);
    fileChooser->launchAsync(juce::FileBrowserComponent::saveMode
                                 | juce::FileBrowserComponent::canSelectFiles
                                 | juce::FileBrowserComponent::warnAboutOverwriting,
                             [safeThis, source](const juce::FileChooser& chooser) {
        if (safeThis == nullptr)
            return;
        const auto destination = chooser.getResult();
        if (destination == juce::File{})
            return;
        juce::String error;
        if (!safeThis->processor_.presetManager.exportUserPreset(
                source.info.file, destination, error)) {
            safeThis->showError(error);
            return;
        }
        safeThis->setStatus("Exported the selected preset.");
    });
}

void PresetsPanel::showError(const juce::String& message) {
    juce::AlertWindow::showMessageBoxAsync(
        juce::MessageBoxIconType::WarningIcon, "PAS 1 Presets", message, "OK", this);
}

void PresetsPanel::setStatus(const juce::String& message) {
    status.setText(message, juce::dontSendNotification);
}

bool PresetsPanel::isFavorite(const juce::String& id) const {
    return favoriteIds.contains(id);
}
