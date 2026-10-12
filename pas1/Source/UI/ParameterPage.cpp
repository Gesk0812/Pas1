#include "ParameterPage.h"
#include "Utf8Text.h"

#include <algorithm>
#include <utility>

namespace {
juce::String parameterTooltip(const juce::String& id) {
    if (id == "ancientinstrument")
        return pas1::ui::utf8String("Выберите тембр древнего слоя. Значение «Выкл.» отключает его; чтобы услышать инструмент, увеличьте громкость слоя.");
    if (id == "ancientmix")
        return pas1::ui::utf8String("Громкость древнего слоя. 0 полностью его заглушает; заводское значение позволяет сразу услышать выбранный инструмент.");
    if (id == "ancientbrightness")
        return pas1::ui::utf8String("Меняет яркость и количество высоких гармоник древнего тембра.");
    if (id == "ancientdecay")
        return pas1::ui::utf8String("Длительность затухания щипковых и ударных тембров. Аулос звучит, пока удерживается нота; его окончание задаёт параметр «Затухание».");
    if (id == "ancientair")
        return pas1::ui::utf8String("Для аулоса добавляет воздушный шум, для лиры и металлических тембров меняет шум атаки.");
    if (id == "ancientdetune")
        return pas1::ui::utf8String("Расстройка между трубками аулоса. На лиру и гонг этот параметр не влияет.");
    if (id == "musicboxmix")
        return pas1::ui::utf8String("Громкость слоя музыкальной шкатулки. 0 отключает его, увеличение ручки добавляет металлические обертоны.");
    if (id == "musicboxbrightness")
        return pas1::ui::utf8String("Яркость металлических обертонов музыкальной шкатулки.");
    if (id == "musicboxdecay")
        return pas1::ui::utf8String("Как долго звучат обертоны музыкальной шкатулки после удара.");
    if (id == "musicboxresonance")
        return pas1::ui::utf8String("Уровень резонанса обертонов музыкальной шкатулки.");
    if (id == "musicboxinharmonicity")
        return pas1::ui::utf8String("Насколько обертоны отклоняются от обычного гармонического ряда.");
    if (id == "musicboxstrike")
        return pas1::ui::utf8String("Меняет спектр атаки, имитируя точку удара по металлическому язычку.");
    if (id == "filtercutoff")
        return pas1::ui::utf8String("Частота среза. Действие зависит от типа фильтра: например, фильтр низких частот ослабляет частоты выше этой точки.");
    if (id == "filterresonance")
        return pas1::ui::utf8String("Усиливает область около частоты среза: 0 — ровный отклик без пика, середина — заметный резонанс, 1 — сильный пик.");
    if (id == "filterdrive")
        return pas1::ui::utf8String("Добавляет насыщение в фильтре. Увеличивайте постепенно, чтобы избежать резкого перегруза.");
    if (id == "filterenvamount")
        return pas1::ui::utf8String("Насколько огибающая фильтра сдвигает частоту среза. Отрицательное значение меняет направление.");
    if (id == "filterkeytracking")
        return pas1::ui::utf8String("Сдвигает частоту среза вслед за высотой ноты. Ноль отключает отслеживание, отрицательное значение меняет направление.");
    if (id == "filtervelocity")
        return pas1::ui::utf8String("Скорость нажатия смещает частоту среза вверх или вниз относительно среднего значения; ноль отключает это влияние.");
    if (id == "filtermodwheel")
        return pas1::ui::utf8String("Колесо модуляции повышает частоту среза; при максимальной глубине сдвиг достигает четырёх октав.");
    if (id == "filterrandom")
        return pas1::ui::utf8String("Добавляет случайный сдвиг частоты среза для каждой ноты. При нулевом значении отключён.");
    if (id == "attack")
        return pas1::ui::utf8String("Время нарастания громкости после нажатия клавиши.");
    if (id == "decay")
        return pas1::ui::utf8String("Время перехода от пика атаки к уровню удержания.");
    if (id == "sustain")
        return pas1::ui::utf8String("Уровень громкости, пока удерживается MIDI-нота.");
    if (id == "release")
        return pas1::ui::utf8String("Время затухания после отпускания MIDI-ноты.");
    if (id == "filterattack")
        return pas1::ui::utf8String("Время нарастания воздействия фильтровой огибающей после нажатия ноты.");
    if (id == "filterdecay")
        return pas1::ui::utf8String("Время перехода огибающей фильтра к уровню удержания.");
    if (id == "filtersustain")
        return pas1::ui::utf8String("Уровень огибающей фильтра, пока удерживается нота.");
    if (id == "filterrelease")
        return pas1::ui::utf8String("Время затухания огибающей фильтра после отпускания ноты.");
    if (id == "osc1level" || id == "osc2level" || id == "osc3level"
        || id == "sublevel" || id == "noiselevel")
        return pas1::ui::utf8String("Уровень этого источника в общем тембре. Чтобы услышать его, увеличьте значение выше нуля.");
    if (id == "unisonvoices")
        return pas1::ui::utf8String("Количество слегка расстроенных копий осциллятора 1. Большее число копий делает звук плотнее и расходует больше голосов полифонии.");
    if (id == "unisondetune")
        return pas1::ui::utf8String("Расстройка копий унисона в центах. Небольшие значения дают лёгкое утолщение, большие — более широкий и нестабильный тембр.");
    if (id == "pulsewidth")
        return pas1::ui::utf8String("Задаёт ширину импульса прямоугольной волны осциллятора 3. На другие формы волны не влияет.");
    if (id == "fmamount")
        return pas1::ui::utf8String("Глубина частотной модуляции между осцилляторами. Начните с малого значения: тембр быстро становится резче.");
    if (id == "ringamount")
        return pas1::ui::utf8String("Глубина кольцевой модуляции. Добавляет звонкие, негармонические составляющие.");
    if (id == "glideenabled")
        return pas1::ui::utf8String("Включает скольжение высоты между нотами. Для привычного монофонического портаменто выберите монофонию или легато.");
    if (id == "glidetime")
        return pas1::ui::utf8String("Длительность перехода между высотами нот при включённом глиссандо.");
    if (id == "delayenabled" || id == "chorusenabled" || id == "phaserenabled"
        || id == "reverbenabled" || id == "distortionenabled" || id == "eqenabled")
        return pas1::ui::utf8String("Включает эффект или обходит его. При выключенном переключателе эффект не слышен, даже если его уровень выше нуля.");
    if (id == "distortionmix" || id == "chorusmix" || id == "phasermix"
        || id == "delaymix" || id == "reverbmix")
        return pas1::ui::utf8String("Уровень эффекта. Для звучания эффекта соответствующий переключатель должен быть включён.");
    if (id == "delaytime")
        return pas1::ui::utf8String("Интервал между повторами задержки в секундах.");
    if (id == "delayfeedback")
        return pas1::ui::utf8String("Количество повторов задержки. Высокое значение создаёт длинный хвост.");
    if (id == "reverbdecay")
        return pas1::ui::utf8String("Длина затухания реверберации.");
    if (id == "reverbdamping")
        return pas1::ui::utf8String("Высокое демпфирование быстрее убирает верхние частоты из хвоста реверберации.");
    if (id == "eqlowgain" || id == "eqhighgain")
        return pas1::ui::utf8String("Уровень низких или высоких частот в децибелах. Для слышимого изменения включите эквалайзер.");
    if (id.startsWith("lfo")) {
        if (id.endsWith("rate"))
            return pas1::ui::utf8String("Скорость низкочастотного генератора. При выключенной синхронизации задаётся в герцах; при включённой — долей темпа.");
        if (id.endsWith("amount"))
            return pas1::ui::utf8String("Глубина изменения выбранного параметра низкочастотным генератором.");
        if (id.endsWith("target"))
            return pas1::ui::utf8String("Параметр, на который направляется низкочастотный генератор.");
        return pas1::ui::utf8String("Настройки низкочастотного генератора модуляции. Чтобы услышать результат, выберите цель и увеличьте глубину.");
    }
    return {};
}
} // namespace

class ParameterPage::Content final : public juce::Component {
private:
    class Row final : public juce::Component {
    public:
        Row(juce::AudioProcessorValueTreeState& state, const juce::String& id,
            ParameterPageTheme theme)
            : theme_(std::move(theme)) {
            auto* parameter = state.getParameter(id);
            if (parameter == nullptr)
                return;

            const auto parameterName = parameter->getName(72);
            label_.setText(parameterName, juce::dontSendNotification);
            label_.setColour(juce::Label::textColourId, theme_.muted);
            label_.setFont(juce::FontOptions{11.0f, juce::Font::bold});
            addAndMakeVisible(label_);

            if (dynamic_cast<juce::AudioParameterBool*>(parameter) != nullptr) {
                label_.setVisible(false);
                toggle_.setButtonText(parameterName);
                toggle_.setColour(juce::ToggleButton::textColourId, theme_.text);
                toggle_.setColour(juce::ToggleButton::tickColourId, theme_.accent);
                addAndMakeVisible(toggle_);
                buttonAttachment_ =
                    std::make_unique<ButtonAttachment>(state, id, toggle_);
            } else if (auto* choice = dynamic_cast<juce::AudioParameterChoice*>(parameter)) {
                combo_.addItemList(choice->choices, 1);
                combo_.setColour(juce::ComboBox::backgroundColourId, theme_.rowBackground);
                combo_.setColour(juce::ComboBox::textColourId, theme_.text);
                combo_.setColour(juce::ComboBox::outlineColourId, theme_.border);
                combo_.setColour(juce::ComboBox::arrowColourId, theme_.accent);
                addAndMakeVisible(combo_);
                comboAttachment_ =
                    std::make_unique<ComboBoxAttachment>(state, id, combo_);
            } else {
                const auto range = parameter->getNormalisableRange();
                slider_.setSliderStyle(juce::Slider::LinearHorizontal);
                slider_.setTextBoxStyle(juce::Slider::TextBoxRight, false, 76, 22);
                slider_.setRange(range.start, range.end, range.interval);
                if (range.skew != 1.0)
                    slider_.setSkewFactor(range.skew);
                slider_.setTextValueSuffix(parameter->getLabel().isNotEmpty()
                                               ? " " + parameter->getLabel()
                                               : juce::String{});
                 slider_.setColour(juce::Slider::trackColourId, theme_.accent);
                 slider_.setColour(juce::Slider::thumbColourId, theme_.text);
                 slider_.setColour(juce::Slider::textBoxTextColourId, theme_.text);
                 slider_.setColour(juce::Slider::textBoxBackgroundColourId, theme_.rowBackground);
                 slider_.setColour(juce::Slider::textBoxOutlineColourId, theme_.border);
                addAndMakeVisible(slider_);
                sliderAttachment_ =
                    std::make_unique<SliderAttachment>(state, id, slider_);
            }

            const auto tooltip = parameterTooltip(id);
            if (tooltip.isNotEmpty()) {
                label_.setTooltip(tooltip);
                slider_.setTooltip(tooltip);
                combo_.setTooltip(tooltip);
                toggle_.setTooltip(tooltip);
            }
        }

        void paint(juce::Graphics& graphics) override {
            graphics.setColour(theme_.rowBackground);
            graphics.fillRoundedRectangle(getLocalBounds().toFloat(), 8.0f);
        }

        void resized() override {
            auto bounds = getLocalBounds().reduced(10, 7);
            label_.setBounds(bounds.removeFromTop(17));
            if (toggle_.getParentComponent() != nullptr) {
                toggle_.setBounds(bounds.withY(bounds.getY() + 4).withHeight(30));
            } else if (combo_.getParentComponent() != nullptr) {
                combo_.setBounds(bounds.withHeight(30));
            } else if (slider_.getParentComponent() != nullptr) {
                slider_.setBounds(bounds.withHeight(30));
            }
        }

    private:
        using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
        using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
        using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

        juce::Label label_;
        ParameterPageTheme theme_;
        juce::Slider slider_;
        juce::ComboBox combo_;
        juce::ToggleButton toggle_;
        std::unique_ptr<SliderAttachment> sliderAttachment_;
        std::unique_ptr<ComboBoxAttachment> comboAttachment_;
        std::unique_ptr<ButtonAttachment> buttonAttachment_;
    };

    class Section final : public juce::Component {
    public:
        Section(juce::AudioProcessorValueTreeState& state, const ParameterPageGroup& group,
                const ParameterPageTheme& theme) {
            heading_.setText(group.title, juce::dontSendNotification);
            heading_.setColour(juce::Label::textColourId, theme.accent);
            heading_.setFont(juce::FontOptions{12.0f, juce::Font::bold});
            addAndMakeVisible(heading_);
            for (const auto& id : group.parameterIds) {
                auto row = std::make_unique<Row>(state, id, theme);
                addAndMakeVisible(*row);
                rows_.push_back(std::move(row));
            }
        }

        int preferredHeight() const noexcept {
            return 28 + static_cast<int>((rows_.size() + 1) / 2) * rowHeight + sectionGap;
        }

        void resized() override {
            auto bounds = getLocalBounds();
            heading_.setBounds(bounds.removeFromTop(24));
            bounds.removeFromTop(4);
            const auto columnGap = 12;
            const auto columnWidth = (bounds.getWidth() - columnGap) / 2;
            for (std::size_t index = 0; index < rows_.size(); ++index) {
                const auto column = static_cast<int>(index % 2);
                const auto row = static_cast<int>(index / 2);
                rows_[index]->setBounds(column * (columnWidth + columnGap),
                                        28 + row * rowHeight, columnWidth, rowHeight - 6);
            }
        }

    private:
        static constexpr int rowHeight = 64;
        static constexpr int sectionGap = 8;
        juce::Label heading_;
        std::vector<std::unique_ptr<Row>> rows_;
    };

public:
    Content(juce::AudioProcessorValueTreeState& state,
            const std::vector<ParameterPageGroup>& groups,
            const ParameterPageTheme& theme) {
        for (const auto& group : groups) {
            auto section = std::make_unique<Section>(state, group, theme);
            addAndMakeVisible(*section);
            sections_.push_back(std::move(section));
        }
        setSize(1, preferredHeight());
    }

    int preferredHeight() const noexcept {
        auto height = 0;
        for (const auto& section : sections_)
            height += section->preferredHeight();
        return std::max(height, 1);
    }

    void resized() override {
        auto y = 0;
        for (auto& section : sections_) {
            const auto height = section->preferredHeight();
            section->setBounds(0, y, getWidth(), height);
            y += height;
        }
    }

private:
    std::vector<std::unique_ptr<Section>> sections_;
};

ParameterPage::ParameterPage(juce::AudioProcessorValueTreeState& state,
                             juce::String title,
                             std::vector<ParameterPageGroup> groups,
                             ParameterPageTheme theme,
                             juce::Component* headerComponent,
                             int headerComponentHeight)
    : title_(std::move(title)),
      theme_(std::move(theme)),
      headerComponent_(headerComponent),
      headerComponentHeight_(std::max(0, headerComponentHeight)),
      content_(std::make_unique<Content>(state, groups, theme_)) {
    viewport_.setViewedComponent(content_.get(), false);
    viewport_.setScrollBarsShown(true, false);
    addAndMakeVisible(viewport_);
    if (headerComponent_ != nullptr && headerComponentHeight_ > 0)
        addAndMakeVisible(*headerComponent_);
}

ParameterPage::~ParameterPage() = default;

void ParameterPage::paint(juce::Graphics& graphics) {
    graphics.fillAll(theme_.background);
    graphics.setColour(theme_.accent);
    graphics.setFont(juce::FontOptions{13.0f, juce::Font::bold});
    graphics.drawText(title_, 12, 8, getWidth() - 24, 20, juce::Justification::centredLeft);
}

void ParameterPage::resized() {
    const auto viewportTop = 34 + (headerComponent_ != nullptr ? headerComponentHeight_ : 0);
    if (headerComponent_ != nullptr && headerComponentHeight_ > 0)
        headerComponent_->setBounds(12, 34, getWidth() - 24, headerComponentHeight_);
    viewport_.setBounds(0, viewportTop, getWidth(), std::max(0, getHeight() - viewportTop));
    content_->setSize(viewport_.getWidth(), content_->preferredHeight());
}
