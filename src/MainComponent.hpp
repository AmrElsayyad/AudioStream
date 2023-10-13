#pragma once

#include "LookAndFeel.hpp"
#include "Receiver.hpp"

//==============================================================================
class MainComponent : public AudioAppComponent, private ChangeListener {
  public:
    MainComponent()
        : state(State::Stopped),
          audioStreamLookAndFeel{std::make_shared<AudioStreamLookAndFeel>()} {
        Logger::writeToLog(__FUNCTION__);

        sliderInit();
        buttonInit(textButton, "", false, false);
        buttonInit(stopButton, "Stop", false, true, Colours::red);
        buttonInit(senderButton, "Sender");
        buttonInit(receiverButton, "Receiver");

        stopButton.onClick = [this] { stopButtonClicked(); };
        senderButton.onClick = [this] { sendingButtonClicked(); };
        receiverButton.onClick = [this] { receivingButtonClicked(); };

        setOpaque(true);
        setSize(500, 500);
    }

    ~MainComponent() override {
        Logger::writeToLog(__FUNCTION__);

        shutdownAudio();
        stop();
    }

    void paint(Graphics& g) override {
        /* Our component is opaque, so we must completely fill the background
         * with a solid colour */
        g.fillAll(
            getLookAndFeel().findColour(ResizableWindow::backgroundColourId));
    }

    void resized() override {
        AudioAppComponent::resized();

        auto rect = getLocalBounds();
        rect.reduce(rect.getWidth() / 4, rect.getHeight() / 4);

        const int height = rect.getHeight() / 2.5;
        const auto width = rect.getWidth();

        auto topRect = rect.removeFromTop(height);
        auto bottomRect = rect.removeFromBottom(height);

        stopButton.setBounds(bottomRect);
        receiverButton.setBounds(bottomRect);

        textButton.setBounds(topRect);
        senderButton.setBounds(topRect);

        levelSlider.setBounds(topRect.removeFromBottom(height / 2));
        levelSlider.setTextBoxStyle(levelSlider.getTextBoxPosition(),
                                    !levelSlider.isTextBoxEditable(), width / 5,
                                    height / 2);

        sliderLabel.setBounds(topRect.removeFromBottom(height / 2));
    }

  private:
    //==============================================================================
    enum class State {
        Stopped,
        Connecting,
        Sending,
        WaitingForConnection,
        Receiving
    };

    State state;

    std::shared_ptr<AudioStreamLookAndFeel> audioStreamLookAndFeel;

    TextButton textButton;
    TextButton stopButton;
    TextButton senderButton;
    TextButton receiverButton;

    Label sliderLabel{{}, "Audio Level:"};
    Slider levelSlider{Slider::LinearHorizontal, Slider::TextBoxRight};

    std::unique_ptr<OSCSender> sender;
    std::unique_ptr<AudioStreamReceiver> receiver{nullptr};

    //==============================================================================
    void recordAndSend(const AudioSourceChannelInfo& bufferToFill) {
        if (!sender) {
            return;
        }

        auto* device = deviceManager.getCurrentAudioDevice();
        auto activeInputChannels = device->getActiveInputChannels();
        auto maxInputChannels = activeInputChannels.getHighestBit() + 1;

        auto level = static_cast<float>(levelSlider.getValue());
        auto numSamples = static_cast<size_t>(bufferToFill.numSamples);
        float outBuffer[AUDIO_STREAM_BUFFER_SIZE];

        for (auto channel = 0; channel < maxInputChannels; ++channel) {
            if (!activeInputChannels[channel]) {
                // Clear the buffer
                bufferToFill.buffer->clear(channel, bufferToFill.startSample,
                                           bufferToFill.numSamples);
                return;
            }

            auto actualInputChannel = channel % maxInputChannels;
            auto* inBuffer = bufferToFill.buffer->getReadPointer(
                actualInputChannel, bufferToFill.startSample);

            for (size_t sample = 0; sample < numSamples; ++sample) {
                outBuffer[sample] = inBuffer[sample] * level / 100.0;
            }

            sender->send(AUDIO_STREAM_ADDRESS_PATTERN,
                         MemoryBlock(outBuffer, numSamples * sizeof(float)));
        }
    }

    void receiveAndPlay(const AudioSourceChannelInfo& bufferToFill) {
        if (!receiver) {
            return;
        }

        auto* device = deviceManager.getCurrentAudioDevice();
        auto activeOutputChannels = device->getActiveOutputChannels();
        auto maxOutputChannels = activeOutputChannels.getHighestBit() + 1;

        auto level = static_cast<float>(levelSlider.getValue());
        auto numSamples = static_cast<size_t>(bufferToFill.numSamples);
        const float* inBuffer;

        for (auto channel = 0; channel < maxOutputChannels; ++channel) {
            inBuffer = receiver->getNextBuffer();

            if (!activeOutputChannels[channel] || inBuffer == nullptr) {
                // Clear the buffer
                bufferToFill.buffer->clear(channel, bufferToFill.startSample,
                                           bufferToFill.numSamples);
                return;
            }

            auto* outBuffer = bufferToFill.buffer->getWritePointer(
                channel, bufferToFill.startSample);

            for (size_t sample = 0; sample < numSamples; ++sample) {
                outBuffer[sample] = inBuffer[sample] * level / 100.0;
            }
        }
    }

    void getNextAudioBlock(
        const AudioSourceChannelInfo& bufferToFill) override {
        if (state == State::Sending) {
            recordAndSend(bufferToFill);
        } else if (state == State::Receiving) {
            receiveAndPlay(bufferToFill);
        }
    }

    void prepareToPlay(int /* samplesPerBlockExpected */,
                       double /* sampleRate */) override {
        Logger::writeToLog(__FUNCTION__);
    }

    void releaseResources() override { Logger::writeToLog(__FUNCTION__); }

    //==============================================================================
    inline void stop() {
        sender = nullptr;
        receiver = nullptr;
    }

    void changeState(State newState) {
        if (newState == state) {
            return;
        }

        state = newState;

        switch (state) {
            case State::Stopped:
                Logger::writeToLog("State: Stopped");
                stop();
                viewMainWindow();
                break;

            case State::Connecting:
                Logger::writeToLog("State: Connecting");
                viewWaitingWindow();
                setAudioChannels(2, 0);
                sender = std::make_unique<OSCSender>();
                if (!sender->connect(AUDIO_STREAM_HOST, AUDIO_STREAM_PORT)) {
                    Logger::writeToLog(String("Error: could not connect to ") +
                                       AUDIO_STREAM_HOST + ":" +
                                       String(AUDIO_STREAM_PORT));
                    changeState(State::Stopped);
                    break;
                }
                changeState(State::Sending);
                break;

            case State::Sending:
                Logger::writeToLog("State: Sending");
                viewConnectedWindow();
                break;

            case State::WaitingForConnection:
                Logger::writeToLog("State: WaitingForConnection");
                viewWaitingWindow();
                setAudioChannels(0, 2);
                if (receiver) {
                    receiver.release();
                }
                receiver = std::make_unique<AudioStreamReceiver>();
                receiver->addChangeListener(this);
                break;

            case State::Receiving:
                Logger::writeToLog("State: Receiving");
                viewConnectedWindow();
                break;
        }
    }

    inline void stopButtonClicked() {
        Logger::writeToLog(__FUNCTION__);
        changeState(State::Stopped);
    }

    inline void sendingButtonClicked() {
        Logger::writeToLog(__FUNCTION__);
        changeState(State::Connecting);
    }

    inline void receivingButtonClicked() {
        Logger::writeToLog(__FUNCTION__);
        changeState(State::WaitingForConnection);
    }

    //==============================================================================
    inline void sliderInit() {
        addChildComponent(sliderLabel);
        sliderLabel.setJustificationType(Justification::bottomLeft);
        sliderLabel.setEditable(false, false, false);
        sliderLabel.setColour(TextEditor::textColourId, Colours::black);
        sliderLabel.setColour(TextEditor::backgroundColourId,
                              Colours::transparentBlack);
        sliderLabel.setLookAndFeel(audioStreamLookAndFeel.get());

        addChildComponent(levelSlider);
        levelSlider.setRange(0, 100, 1);
        levelSlider.setValue(100);
        levelSlider.setLookAndFeel(audioStreamLookAndFeel.get());
    }

    inline void buttonInit(TextButton& button, const String& text,
                           bool visible = true, bool enabled = true,
                           Colour colour = Colours::transparentBlack) {
        addChildComponent(&button);
        button.setLookAndFeel(audioStreamLookAndFeel.get());
        button.setButtonText(text);
        button.setColour(TextButton::buttonColourId, colour);
        button.setEnabled(enabled);
        button.setVisible(visible);
    }

    //==============================================================================
    inline void viewMainWindow() {
        levelSlider.setVisible(false);
        sliderLabel.setVisible(false);

        textButton.setVisible(false);
        stopButton.setVisible(false);

        senderButton.setVisible(true);
        receiverButton.setVisible(true);
    }

    inline void viewWaitingWindow() {
        levelSlider.setVisible(false);
        sliderLabel.setVisible(false);

        senderButton.setVisible(false);
        receiverButton.setVisible(false);

        textButton.setButtonText("Connecting...");

        textButton.setVisible(true);
        stopButton.setVisible(true);
    }

    inline void viewConnectedWindow() {
        receiverButton.setVisible(false);
        senderButton.setVisible(false);
        textButton.setVisible(false);

        stopButton.setVisible(true);
        sliderLabel.setVisible(true);
        levelSlider.setVisible(true);
    }

    //==============================================================================
    inline void changeListenerCallback(ChangeBroadcaster* source) override {
        if (source == receiver.get()) {
            if (receiver->isReady()) {
                changeState(State::Receiving);
            } else if (!receiver->hasData()) {
                changeState(State::WaitingForConnection);
            }
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
