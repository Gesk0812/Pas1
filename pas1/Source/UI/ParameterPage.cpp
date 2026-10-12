#include "ParameterPage.h"

#include <algorithm>
#include <utility>

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
                             ParameterPageTheme theme)
    : title_(std::move(title)),
      theme_(std::move(theme)),
      content_(std::make_unique<Content>(state, groups, theme_)) {
    viewport_.setViewedComponent(content_.get(), false);
    viewport_.setScrollBarsShown(true, false);
    addAndMakeVisible(viewport_);
}

ParameterPage::~ParameterPage() = default;

void ParameterPage::paint(juce::Graphics& graphics) {
    graphics.fillAll(theme_.background);
    graphics.setColour(theme_.accent);
    graphics.setFont(juce::FontOptions{13.0f, juce::Font::bold});
    graphics.drawText(title_, 12, 8, getWidth() - 24, 20, juce::Justification::centredLeft);
}

void ParameterPage::resized() {
    viewport_.setBounds(0, 34, getWidth(), getHeight() - 34);
    content_->setSize(viewport_.getWidth(), content_->preferredHeight());
}
