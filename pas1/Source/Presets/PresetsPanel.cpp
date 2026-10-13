#include "PresetsPanel.h"
#include "UI/Utf8Text.h"

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

juce::String displayCategory(const juce::String& category) {
    if (category == "Bass") return pas1::ui::utf8String("Бас");
    if (category == "Keys") return pas1::ui::utf8String("Клавиши");
    if (category == "Lead") return pas1::ui::utf8String("Лид");
    if (category == "Pad") return pas1::ui::utf8String("Пэд");
    if (category == "FX") return pas1::ui::utf8String("Эффекты");
    if (category == "Pluck") return pas1::ui::utf8String("Щипковые");
    if (category == "Music Box") return pas1::ui::utf8String("Музыкальная шкатулка");
    if (category == "Ancient") return pas1::ui::utf8String("Древние тембры");
    if (category == "User") return pas1::ui::utf8String("Пользовательские");
    return category;
}
}

PresetsPanel::PresetsPanel(Pas1AudioProcessor& processor) : processor_(processor) {
    search.setTextToShowWhenEmpty(pas1::ui::utf8String("Поиск пресетов по названию или категории"), muted);
    search.setColour(juce::TextEditor::backgroundColourId, background);
    search.setColour(juce::TextEditor::textColourId, text);
    search.setColour(juce::TextEditor::outlineColourId, outline);
    search.onTextChange = [this] { refresh(); };
    addAndMakeVisible(search);

    categoryFilter.setTextWhenNothingSelected(pas1::ui::utf8String("Все категории"));
    styleCombo(categoryFilter);
    categoryFilter.onChange = [this] { refresh(); };
    addAndMakeVisible(categoryFilter);

    sortOrder.addItem(pas1::ui::utf8String("Название А–Я"), 1);
    sortOrder.addItem(pas1::ui::utf8String("Категория"), 2);
    sortOrder.addItem(pas1::ui::utf8String("Недавние"), 3);
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

    search.setTooltip(pas1::ui::utf8String("Поиск пресетов по названию и категории."));
    categoryFilter.setTooltip(pas1::ui::utf8String("Показывает все пресеты, избранное или недавно использованные."));
    sortOrder.setTooltip(pas1::ui::utf8String("Меняет порядок пресетов в текущем списке."));
    loadButton.setTooltip(pas1::ui::utf8String("Загружает выбранный пресет и заменяет текущие настройки звука."));
    favoriteButton.setTooltip(pas1::ui::utf8String("Добавляет выбранный пресет в избранное или удаляет его оттуда."));
    saveButton.setTooltip(pas1::ui::utf8String("Обновляет выбранный пользовательский пресет. Заводские пресеты перезаписать нельзя."));
    saveAsButton.setTooltip(pas1::ui::utf8String("Сохраняет текущий звук как отдельный пользовательский пресет."));
    duplicateButton.setTooltip(pas1::ui::utf8String("Создаёт редактируемую копию выбранного заводского или пользовательского пресета."));
    renameButton.setTooltip(pas1::ui::utf8String("Переименовывает пользовательский пресет, не меняя его звук."));
    deleteButton.setTooltip(pas1::ui::utf8String("Безвозвратно удаляет файл выбранного пользовательского пресета."));
    importButton.setTooltip(pas1::ui::utf8String("Добавляет пресет PAS 1 из файла .pas1preset."));
    exportButton.setTooltip(pas1::ui::utf8String("Сохраняет выбранный пользовательский пресет в файл .pas1preset."));
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
    graphics.drawText(pas1::ui::utf8String("БИБЛИОТЕКА ПРЕСЕТОВ  ·  ЗАВОДСКИХ ЗВУКОВ: ") + factoryCount,
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
    graphics.drawText(displayCategory(entry.info.category)
                          + (entry.info.isBuiltIn ? pas1::ui::utf8String("  ·  Заводской")
                                                  : pas1::ui::utf8String("  ·  Пользовательский")),
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
            && !displayCategory(entry.info.category).toLowerCase().contains(searchText))
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
    categoryFilter.addItem(pas1::ui::utf8String("Все категории"), 1);
    categoryFilter.addItem(pas1::ui::utf8String("Избранное"), 2);
    categoryFilter.addItem(pas1::ui::utf8String("Недавние"), 3);
    for (int index = 0; index < categories.size(); ++index)
        categoryFilter.addItem(displayCategory(categories[index]), 100 + index);
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
                                     ? pas1::ui::utf8String("УБРАТЬ ИЗ ИЗБРАННОГО")
                                     : pas1::ui::utf8String("В ИЗБРАННОЕ"));
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
            showError(pas1::ui::utf8String("Не удалось загрузить выбранный заводской пресет."));
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
    setStatus(pas1::ui::utf8String("Загружен пресет «") + loadedName + pas1::ui::utf8String("»."));
    refresh();
}

void PresetsPanel::toggleFavorite() {
    const auto* entry = selectedEntry();
    if (entry == nullptr)
        return;
    const auto wasFavorite = isFavorite(entry->id);
    if (!processor_.presetManager.setPresetFavorite(entry->id, !wasFavorite)) {
        showError(pas1::ui::utf8String("Не удалось обновить файл библиотеки пресетов."));
        return;
    }
    setStatus(wasFavorite ? pas1::ui::utf8String("Удалено из избранного.")
                          : pas1::ui::utf8String("Добавлено в избранное."));
    refresh();
}

void PresetsPanel::saveSelected() {
    const auto* entry = selectedEntry();
    if (entry == nullptr || entry->info.isBuiltIn)
        return;
    const auto selected = *entry;
    juce::Component::SafePointer<PresetsPanel> safeThis(this);
    juce::AlertWindow::showOkCancelBox(
        juce::MessageBoxIconType::QuestionIcon, pas1::ui::utf8String("Обновить пользовательский пресет"),
        pas1::ui::utf8String("Заменить пресет «") + selected.info.name
            + pas1::ui::utf8String("» текущими настройками синтезатора?"),
        pas1::ui::utf8String("Обновить"), pas1::ui::utf8String("Отмена"), this,
        juce::ModalCallbackFunction::create([safeThis, selected](int result) {
            if (safeThis == nullptr || result != 1)
                return;
            juce::String error;
            if (!safeThis->processor_.presetManager.overwriteUserPreset(
                    selected.info.file, error)) {
                safeThis->showError(error);
                return;
            }
            safeThis->setStatus(pas1::ui::utf8String("Выбранный пользовательский пресет обновлён."));
            safeThis->refresh();
        }));
}

void PresetsPanel::saveAs() {
    const auto* selected = selectedEntry();
    const auto defaultName = selected != nullptr
                                 ? selected->info.name + pas1::ui::utf8String(" — копия")
                                 : pas1::ui::utf8String("Мой пресет");
    const auto defaultCategory = selected != nullptr ? displayCategory(selected->info.category)
                                                      : pas1::ui::utf8String("Пользовательские");
    auto* alert = new juce::AlertWindow(pas1::ui::utf8String("Сохранить пресет PAS 1"),
                                        pas1::ui::utf8String("Сохраните текущий звук в пользовательскую библиотеку."),
                                        juce::MessageBoxIconType::NoIcon);
    alert->addTextEditor("name", defaultName, pas1::ui::utf8String("Название"));
    alert->addTextEditor("category", defaultCategory, pas1::ui::utf8String("Категория"));
    alert->addButton(pas1::ui::utf8String("Сохранить"), 1, juce::KeyPress{juce::KeyPress::returnKey});
    alert->addButton(pas1::ui::utf8String("Отмена"), 0, juce::KeyPress{juce::KeyPress::escapeKey});
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
        safeThis->setStatus(pas1::ui::utf8String("Пользовательский пресет сохранён."));
        safeThis->selectPresetInLibrary(PresetManager::userPresetId(savedFile));
    }), true);
}

void PresetsPanel::duplicateSelected() {
    const auto* entry = selectedEntry();
    if (entry == nullptr)
        return;
    const auto source = *entry;
    auto* alert = new juce::AlertWindow(pas1::ui::utf8String("Дублировать пресет PAS 1"),
                                        pas1::ui::utf8String("Создать редактируемую копию в пользовательской библиотеке."),
                                        juce::MessageBoxIconType::NoIcon);
    alert->addTextEditor("name", source.info.name + pas1::ui::utf8String(" — копия"),
                         pas1::ui::utf8String("Новое название"));
    alert->addButton(pas1::ui::utf8String("Дублировать"), 1, juce::KeyPress{juce::KeyPress::returnKey});
    alert->addButton(pas1::ui::utf8String("Отмена"), 0, juce::KeyPress{juce::KeyPress::escapeKey});
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
        safeThis->setStatus(pas1::ui::utf8String("Копия добавлена в пользовательскую библиотеку."));
        safeThis->selectPresetInLibrary(PresetManager::userPresetId(copy));
    }), true);
}

void PresetsPanel::renameSelected() {
    const auto* entry = selectedEntry();
    if (entry == nullptr || entry->info.isBuiltIn)
        return;
    const auto source = *entry;
    auto* alert = new juce::AlertWindow(pas1::ui::utf8String("Переименовать пользовательский пресет"),
                                        pas1::ui::utf8String("Переименовать файл пресета и его метаданные."),
                                        juce::MessageBoxIconType::NoIcon);
    alert->addTextEditor("name", source.info.name, pas1::ui::utf8String("Новое название"));
    alert->addButton(pas1::ui::utf8String("Переименовать"), 1, juce::KeyPress{juce::KeyPress::returnKey});
    alert->addButton(pas1::ui::utf8String("Отмена"), 0, juce::KeyPress{juce::KeyPress::escapeKey});
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
        safeThis->setStatus(pas1::ui::utf8String("Пользовательский пресет переименован."));
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
        juce::MessageBoxIconType::WarningIcon, pas1::ui::utf8String("Удалить пользовательский пресет"),
        pas1::ui::utf8String("Безвозвратно удалить пресет «") + source.info.name
            + pas1::ui::utf8String("» из библиотеки?"),
        pas1::ui::utf8String("Удалить"), pas1::ui::utf8String("Отмена"), this,
        juce::ModalCallbackFunction::create([safeThis, source](int result) {
            if (safeThis == nullptr || result != 1)
                return;
            juce::String error;
            if (!safeThis->processor_.presetManager.deleteUserPreset(source.info.file, error)) {
                safeThis->showError(error);
                return;
            }
            safeThis->selectedId.clear();
            safeThis->setStatus(pas1::ui::utf8String("Пользовательский пресет удалён."));
            safeThis->refresh();
        }));
}

void PresetsPanel::importPreset() {
    fileChooser = std::make_unique<juce::FileChooser>(
        pas1::ui::utf8String("Импорт пресета PAS 1"), processor_.presetManager.getUserPresetDirectory(), "*.pas1preset");
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
        safeThis->setStatus(pas1::ui::utf8String("Пресет импортирован в пользовательскую библиотеку."));
        safeThis->selectPresetInLibrary(PresetManager::userPresetId(importedFile));
    });
}

void PresetsPanel::exportSelected() {
    const auto* entry = selectedEntry();
    if (entry == nullptr || entry->info.isBuiltIn)
        return;
    const auto source = *entry;
    fileChooser = std::make_unique<juce::FileChooser>(
        pas1::ui::utf8String("Экспорт пресета PAS 1"), juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
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
        safeThis->setStatus(pas1::ui::utf8String("Выбранный пресет экспортирован."));
    });
}

void PresetsPanel::showError(const juce::String& message) {
    juce::AlertWindow::showMessageBoxAsync(
        juce::MessageBoxIconType::WarningIcon, pas1::ui::utf8String("Пресеты PAS 1"), message,
        pas1::ui::utf8String("ОК"), this);
}

void PresetsPanel::setStatus(const juce::String& message) {
    status.setText(message, juce::dontSendNotification);
}

bool PresetsPanel::isFavorite(const juce::String& id) const {
    return favoriteIds.contains(id);
}
