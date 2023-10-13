#pragma once

#include <JuceHeader.h>

//==============================================================================
class AudioStreamLookAndFeel : public LookAndFeel_V4 {
  public:
    void drawButtonText(Graphics& g, TextButton& button,
                        bool /*shouldDrawButtonAsHighlighted*/,
                        bool /*shouldDrawButtonAsDown*/) override {
        Font font(button.getHeight() * 0.3f);
        g.setFont(font);
        g.setColour(button
                        .findColour(button.getToggleState()
                                        ? TextButton::textColourOnId
                                        : TextButton::textColourOffId)
                        .withMultipliedAlpha(button.isEnabled() ? 1.0f : 0.5f));

        const int yIndent = jmin(4, button.proportionOfHeight(0.3f));
        const int cornerSize = jmin(button.getHeight(), button.getWidth()) / 2;

        const int fontHeight = roundToInt(font.getHeight() * 0.6f);
        const int leftIndent = jmin(
            fontHeight, 2 + cornerSize / (button.isConnectedOnLeft() ? 4 : 2));
        const int rightIndent = jmin(
            fontHeight, 2 + cornerSize / (button.isConnectedOnRight() ? 4 : 2));
        const int textWidth = button.getWidth() - leftIndent - rightIndent;

        if (textWidth > 0) {
            g.drawFittedText(button.getButtonText(), leftIndent, yIndent,
                             textWidth, button.getHeight() - yIndent * 2,
                             Justification::centred, 1);
        }
    }

    void drawLabel(Graphics& g, Label& label) override {
        g.fillAll(label.findColour(Label::backgroundColourId));

        if (!label.isBeingEdited()) {
            auto alpha = label.isEnabled() ? 1.0f : 0.5f;
            const Font font(label.getHeight() * 0.5f);

            g.setColour(label.findColour(Label::textColourId)
                            .withMultipliedAlpha(alpha));
            g.setFont(font);

            auto textArea = getLabelBorderSize(label).subtractedFrom(
                label.getLocalBounds());

            g.drawFittedText(
                label.getText(), textArea, label.getJustificationType(),
                jmax(1, (int)((float)textArea.getHeight() / font.getHeight())),
                label.getMinimumHorizontalScale());

            g.setColour(label.findColour(Label::outlineColourId)
                            .withMultipliedAlpha(alpha));
        } else if (label.isEnabled()) {
            g.setColour(label.findColour(Label::outlineColourId));
        }

        g.drawRect(label.getLocalBounds());
    }

    Label* createSliderTextBox(Slider& slider) override {
        Label* l = LookAndFeel_V4::createSliderTextBox(slider);

        l->setFont(slider.getTextBoxHeight() * 0.6f);
        l->setColour(Label::outlineColourId,
                     slider.findColour(slider.textBoxBackgroundColourId));
        l->setColour(Label::outlineWhenEditingColourId,
                     slider.findColour(slider.textBoxBackgroundColourId));

        return l;
    }
};
