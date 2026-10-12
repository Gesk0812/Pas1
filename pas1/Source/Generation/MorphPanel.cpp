#include "MorphPanel.h"

#include <algorithm>

namespace {
const juce::Colour background{0xff1a1c21};
const juce::Colour accent{0xffcf7bff};
const juce::Colour text{0xfff4f0f7};
const juce::Colour muted{0xffaaa4b1};
const juce::Colour border{0xff393641};

void styleButton(juce::TextButton& button) {
    button.setColour(juce::TextButton::buttonColourId, background);
    button.setColour(juce::TextButton::textColourOffId, text);
    button.setColour(juce::TextButton::textColourOnId, accent);
}
}

MorphPad::MorphPad(Pas1AudioProcessor& processor) : processor_(processor) {
    xParameter = processor_.parameters.getParameter("morphx");
    yParameter = processor_.parameters.getParameter("morphy");
    startTimerHz(30);
}

juce::Rectangle<float> MorphPad::padArea() const {
    return getLocalBounds().toFloat().reduced(20.0f, 18.0f);
}

void MorphPad::paint(juce::Graphics& graphics) {
    const auto area = padArea();
    juce::ColourGradient gradient{juce::Colour{0xff342441}, area.getX(), area.getY(),
                                  juce::Colour{0xff242a37}, area.getRight(), area.getBottom(), false};
    graphics.setGradientFill(gradient);
    graphics.fillRoundedRectangle(area, 10.0f);
    graphics.setColour(border);
    graphics.drawRoundedRectangle(area, 10.0f, 1.0f);

    graphics.setColour(juce::Colour{0x55ffffff});
    for (int division = 1; division < 4; ++division) {
        const auto x = area.getX() + area.getWidth() * static_cast<float>(division) / 4.0f;
        const auto y = area.getY() + area.getHeight() * static_cast<float>(division) / 4.0f;
        graphics.drawVerticalLine(static_cast<int>(x), area.getY(), area.getBottom());
        graphics.drawHorizontalLine(static_cast<int>(y), area.getX(), area.getRight());
    }

    graphics.setFont(juce::FontOptions{12.0f, juce::Font::bold});
    graphics.setColour(text);
    graphics.drawText("C · GLASS", area.getX() + 8.0f, area.getY() + 5.0f, 100.0f, 18.0f,
                      juce::Justification::topLeft);
    graphics.drawText("D · PAD", area.getRight() - 108.0f, area.getY() + 5.0f, 100.0f, 18.0f,
                      juce::Justification::topRight);
    graphics.drawText("A · SOFT BASS", area.getX() + 8.0f, area.getBottom() - 23.0f, 118.0f, 18.0f,
                      juce::Justification::bottomLeft);
    graphics.drawText("B · DIGITAL BASS", area.getRight() - 138.0f, area.getBottom() - 23.0f, 130.0f, 18.0f,
                      juce::Justification::bottomRight);

    const auto x = processor_.parameters.getRawParameterValue("morphx")->load();
    const auto y = processor_.parameters.getRawParameterValue("morphy")->load();
    const juce::Point<float> position{area.getX() + std::clamp(x, 0.0f, 1.0f) * area.getWidth(),
                                      area.getBottom() - std::clamp(y, 0.0f, 1.0f) * area.getHeight()};
    graphics.setColour(juce::Colour{0x55cf7bff});
    graphics.fillEllipse(position.x - 17.0f, position.y - 17.0f, 34.0f, 34.0f);
    graphics.setColour(accent);
    graphics.fillEllipse(position.x - 8.0f, position.y - 8.0f, 16.0f, 16.0f);
    graphics.setColour(text);
    graphics.drawEllipse(position.x - 10.0f, position.y - 10.0f, 20.0f, 20.0f, 1.2f);
}

void MorphPad::mouseDown(const juce::MouseEvent& event) {
    if (xParameter == nullptr || yParameter == nullptr)
        return;
    dragging = true;
    xParameter->beginChangeGesture();
    yParameter->beginChangeGesture();
    updatePosition(event.position);
}

void MorphPad::mouseDrag(const juce::MouseEvent& event) {
    if (dragging)
        updatePosition(event.position);
}

void MorphPad::mouseUp(const juce::MouseEvent&) {
    if (!dragging)
        return;
    xParameter->endChangeGesture();
    yParameter->endChangeGesture();
    dragging = false;
}

void MorphPad::timerCallback() {
    repaint();
}

void MorphPad::updatePosition(juce::Point<float> position) {
    const auto area = padArea();
    const auto x = std::clamp((position.x - area.getX()) / area.getWidth(), 0.0f, 1.0f);
    const auto y = std::clamp(1.0f - (position.y - area.getY()) / area.getHeight(), 0.0f, 1.0f);
    xParameter->setValueNotifyingHost(xParameter->convertTo0to1(x));
    yParameter->setValueNotifyingHost(yParameter->convertTo0to1(y));
    repaint();
}

MorphPanel::MorphPanel(Pas1AudioProcessor& processor) : processor_(processor), pad(processor) {
    addAndMakeVisible(pad);
    const std::array<juce::String, 4> names{
        "CAPTURE A · SOFT BASS", "CAPTURE B · DIGITAL BASS",
        "CAPTURE C · GLASS", "CAPTURE D · PAD"};
    for (int index = 0; index < static_cast<int>(captureButtons.size()); ++index) {
        auto& button = captureButtons[static_cast<std::size_t>(index)];
        button.setButtonText(names[static_cast<std::size_t>(index)]);
        styleButton(button);
        button.onClick = [this, index] {
            processor_.captureMorphCorner(index);
            status.setText("Captured corner " + juce::String(index + 1),
                           juce::dontSendNotification);
        };
        addAndMakeVisible(button);
    }
    styleButton(factoryButton);
    factoryButton.onClick = [this] {
        processor_.captureFactoryMorphCorners();
        status.setText("Loaded factory corners: soft bass, digital bass, glass keys, and slow pad.",
                       juce::dontSendNotification);
    };
    addAndMakeVisible(factoryButton);
    styleButton(randomizeButton);
    randomizeButton.onClick = [this] {
        const auto seed = static_cast<std::uint32_t>(juce::Random::getSystemRandom().nextInt());
        processor_.randomizeMorphCorners(seed);
        status.setText("Corner variations generated. The random seed is " + juce::String(static_cast<juce::int64>(seed)) + ".",
                       juce::dontSendNotification);
    };
    addAndMakeVisible(randomizeButton);

    for (std::size_t index = 0; index < locks.size(); ++index) {
        auto& lock = locks[index];
        lock.setButtonText(Pas1AudioProcessor::getMorphParameterName(static_cast<int>(index)));
        lock.setToggleState(processor_.isMorphParameterLocked(static_cast<int>(index)),
                            juce::dontSendNotification);
        lock.setColour(juce::ToggleButton::textColourId, muted);
        lock.setColour(juce::ToggleButton::tickColourId, accent);
        lock.onClick = [this, index] {
            processor_.setMorphParameterLocked(static_cast<int>(index),
                                                locks[index].getToggleState());
        };
        addAndMakeVisible(lock);
    }
    status.setColour(juce::Label::textColourId, muted);
    status.setText(processor_.isMorphConfigured()
                       ? "Morph Matrix active. X/Y are host-automatable, including with this window closed."
                       : "Capture or load the four corners. X/Y are host-automatable, including with this window closed.",
                   juce::dontSendNotification);
    addAndMakeVisible(status);
}

void MorphPanel::paint(juce::Graphics& graphics) {
    graphics.setColour(background);
    graphics.fillRoundedRectangle(getLocalBounds().toFloat(), 12.0f);
}

void MorphPanel::resized() {
    const auto width = getWidth();
    const auto height = getHeight();
    const auto leftWidth = static_cast<int>(width * 0.53f);
    pad.setBounds(10, 12, leftWidth - 18, height - 70);

    const auto rightX = leftWidth;
    const auto rightWidth = width - rightX - 12;
    const auto gap = 8;
    const auto buttonWidth = (rightWidth - gap) / 2;
    for (std::size_t index = 0; index < captureButtons.size(); ++index) {
        const auto column = static_cast<int>(index % 2);
        const auto row = static_cast<int>(index / 2);
        captureButtons[index].setBounds(rightX + column * (buttonWidth + gap),
                                        14 + row * 38, buttonWidth, 32);
    }
    factoryButton.setBounds(rightX, 96, rightWidth, 30);
    randomizeButton.setBounds(rightX, 132, rightWidth, 30);

    const auto lockTop = 176;
    const auto lockGapX = 4;
    const auto lockGapY = 3;
    const auto lockWidth = (rightWidth - 2 * lockGapX) / 3;
    const auto lockHeight = 31;
    for (std::size_t index = 0; index < locks.size(); ++index) {
        const auto row = static_cast<int>(index / 3);
        const auto column = static_cast<int>(index % 3);
        locks[index].setBounds(rightX + column * (lockWidth + lockGapX),
                               lockTop + row * (lockHeight + lockGapY), lockWidth, lockHeight);
    }
    status.setBounds(16, height - 48, width - 32, 32);
}
