#include "State.hpp"

#ifdef _MSC_VER
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

//==============================================================================
void StoppedState::resized() {
    Logger::writeToLog(__PRETTY_FUNCTION__);

    Component::resized();

    auto rect = getLocalBounds();
    const auto height = static_cast<int>(rect.getHeight() / 2.5);

    auto topRect = rect.removeFromTop(height);
    auto bottomRect = rect.removeFromBottom(height);

    senderButton.setBounds(topRect);
    receiverButton.setBounds(bottomRect);
}

StoppedState::StoppedState()
    : buttonLookAndFeel(std::make_shared<ButtonLookAndFeel>()) {
    Logger::writeToLog(__PRETTY_FUNCTION__);

    addAndMakeVisible(senderButton);
    senderButton.setLookAndFeel(buttonLookAndFeel.get());
    senderButton.setButtonText("Sender");
    senderButton.onClick = [this] { senderButtonClicked(); };

    addAndMakeVisible(receiverButton);
    receiverButton.setLookAndFeel(buttonLookAndFeel.get());
    receiverButton.setButtonText("Receiver");
    receiverButton.onClick = [this] { receiverButtonClicked(); };
}

void StoppedState::changeListenerCallback(ChangeBroadcaster* source) {
    Logger::writeToLog(__PRETTY_FUNCTION__);

    removeChangeListener(dynamic_cast<ChangeListener*>(source));

    if (const auto& broadcaster = dynamic_cast<Component*>(source)) {
        if (const auto& parent = broadcaster->getParentComponent()) {
            parent->removeChildComponent(broadcaster);
            parent->addAndMakeVisible(this);
            parent->resized();
        }
    }
}

void StoppedState::senderButtonClicked() {
    Logger::writeToLog(__PRETTY_FUNCTION__);

    addChangeListener(SharedResourcePointer<ConnectingState>());
    sendChangeMessage();
}

void StoppedState::receiverButtonClicked() {
    Logger::writeToLog(__PRETTY_FUNCTION__);

    addChangeListener(SharedResourcePointer<ListeningState>());
    sendChangeMessage();
}

//==============================================================================
void ConnectingState::resized() {
    Logger::writeToLog(__PRETTY_FUNCTION__);

    Component::resized();

    auto rect = getLocalBounds();
    const int rectHeight = static_cast<int>(rect.getHeight() / 8);
    const int fontHeight = static_cast<int>(rect.getHeight() / 12);

    ipLabel.setBounds(rect.removeFromTop(rectHeight));
    ipEditor.setBounds(rect.removeFromTop(rectHeight));
    portLabel.setBounds(rect.removeFromTop(rectHeight));
    portEditor.setBounds(rect.removeFromTop(rectHeight));
    errorLabel.setBounds(rect.removeFromBottom(rectHeight));
    connectButton.setBounds(rect.removeFromBottom(rectHeight * 2));

    ipLabel.setFont(fontHeight);
    portLabel.setFont(fontHeight);

    const auto& ipText = ipEditor.getText();
    ipEditor.clear();
    ipEditor.setFont(fontHeight);
    ipEditor.setText(ipText);

    const auto& portText = portEditor.getText();
    portEditor.clear();
    portEditor.setFont(fontHeight);
    portEditor.setText(portText);
}

ConnectingState::ConnectingState()
    : buttonLookAndFeel(std::make_shared<ButtonLookAndFeel>()) {
    Logger::writeToLog(__PRETTY_FUNCTION__);

    addAndMakeVisible(ipLabel);
    ipLabel.setEditable(false);
    ipLabel.setLookAndFeel(labelLookAndFeel.get());
    ipLabel.setJustificationType(Justification::bottomLeft);
    ipLabel.setText("IP:", dontSendNotification);

    addAndMakeVisible(ipEditor);
    ipEditor.setTextToShowWhenEmpty("127.0.0.1", Colours::grey);

    addAndMakeVisible(portLabel);
    portLabel.setEditable(false);
    portLabel.setLookAndFeel(labelLookAndFeel.get());
    portLabel.setJustificationType(Justification::bottomLeft);
    portLabel.setText("Port:", dontSendNotification);

    addAndMakeVisible(portEditor);
    portEditor.setTextToShowWhenEmpty("1234", Colours::grey);

    addAndMakeVisible(connectButton);
    connectButton.setLookAndFeel(buttonLookAndFeel.get());
    connectButton.setButtonText("Connect");
    connectButton.onClick = [this] { connectButtonClicked(); };

    addAndMakeVisible(errorLabel);
    errorLabel.setEditable(false);
    errorLabel.setLookAndFeel(labelLookAndFeel.get());
    errorLabel.setJustificationType(Justification::bottomLeft);
}

void ConnectingState::changeListenerCallback(ChangeBroadcaster* source) {
    Logger::writeToLog(__PRETTY_FUNCTION__);

    removeChangeListener(dynamic_cast<ChangeListener*>(source));

    if (const auto& broadcaster = dynamic_cast<Component*>(source)) {
        if (const auto& parent = broadcaster->getParentComponent()) {
            parent->removeChildComponent(broadcaster);
            parent->addAndMakeVisible(this);
            parent->resized();
        }
    }
}

void ConnectingState::connectButtonClicked() {
    Logger::writeToLog(__PRETTY_FUNCTION__);

    const auto& senderPtr = SharedResourcePointer<OSCSender>();

    errorLabel.setText("", dontSendNotification);
    if (!senderPtr->connect(ipEditor.getText(),
                            portEditor.getText().getIntValue())) {
        const auto& errorMsg = "Couldn't connect to " + ipEditor.getText() +
                               ":" + portEditor.getText();
        Logger::writeToLog(errorMsg);
        errorLabel.setText(errorMsg, dontSendNotification);
        return;
    }

    Logger::writeToLog("Connected to " + ipEditor.getText() + ":" +
                       portEditor.getText());

    addChangeListener(SharedResourcePointer<SendingState>());
    sendChangeMessage();
}

//==============================================================================
SendingState::~SendingState() { shutdownAudio(); }

void SendingState::paint(Graphics& g) {
    auto rect = getLocalBounds();
    rect.removeFromLeft(10);
    rect = rect.removeFromTop(rect.getHeight() / 2.5);

    g.setFont(rect.getHeight() / 5);
    g.setColour(juce::Colours::white);
    g.drawText("Audio Level:", rect, juce::Justification::centredLeft, true);
}

void SendingState::resized() {
    Logger::writeToLog(__PRETTY_FUNCTION__);

    Component::resized();

    auto rect = getLocalBounds();
    const auto height = static_cast<int>(rect.getHeight() / 2.5);

    auto topRect = rect.removeFromTop(height);
    auto bottomRect = rect.removeFromBottom(height);

    levelSlider.setBounds(topRect.removeFromBottom(height / 2));
    levelSlider.setTextBoxStyle(levelSlider.getTextBoxPosition(),
                                !levelSlider.isTextBoxEditable(),
                                rect.getWidth() / 5, height / 2);

    stopButton.setBounds(bottomRect);
}

SendingState::SendingState()
    : sliderTextBoxLookAndFeel(std::make_shared<SliderTextBoxLookAndFeel>()),
      buttonLookAndFeel(std::make_shared<ButtonLookAndFeel>()) {
    Logger::writeToLog(__PRETTY_FUNCTION__);

    addAndMakeVisible(levelSlider);
    levelSlider.setRange(0, 100, 1);
    levelSlider.setValue(100);
    levelSlider.setLookAndFeel(sliderTextBoxLookAndFeel.get());

    addAndMakeVisible(stopButton);
    stopButton.setLookAndFeel(buttonLookAndFeel.get());
    stopButton.setButtonText("Stop");
    stopButton.onClick = [this] { stopButtonClicked(); };
}

void SendingState::changeListenerCallback(ChangeBroadcaster* source) {
    Logger::writeToLog(__PRETTY_FUNCTION__);

    removeChangeListener(dynamic_cast<ChangeListener*>(source));

    if (const auto& broadcaster = dynamic_cast<Component*>(source)) {
        if (const auto& parent = broadcaster->getParentComponent()) {
            setAudioChannels(2, 0);

            parent->removeChildComponent(broadcaster);
            parent->addAndMakeVisible(this);
            parent->resized();
        }
    }
}

void SendingState::getNextAudioBlock(
    const AudioSourceChannelInfo& bufferToFill) {
    auto* device = deviceManager.getCurrentAudioDevice();
    auto activeInputChannels = device->getActiveInputChannels();
    auto maxInputChannels = activeInputChannels.getHighestBit() + 1;

    auto level = static_cast<float>(levelSlider.getValue());
    auto numSamples = static_cast<size_t>(bufferToFill.numSamples);
    float outBuffer[AUDIO_STREAM_AUDIO_BUFFER_SIZE];

    const auto& senderPtr = SharedResourcePointer<OSCSender>();

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

        senderPtr->send(AUDIO_STREAM_ADDRESS_PATTERN,
                        MemoryBlock(outBuffer, numSamples * sizeof(float)));
    }
}

void SendingState::stopButtonClicked() {
    Logger::writeToLog(__PRETTY_FUNCTION__);

    shutdownAudio();

    addChangeListener(SharedResourcePointer<StoppedState>());
    sendChangeMessage();
}

//==============================================================================
void ListeningState::resized() {
    Logger::writeToLog(__PRETTY_FUNCTION__);

    Component::resized();

    auto rect = getLocalBounds();
    const int rectHeight = static_cast<int>(rect.getHeight() / 8);
    const int fontHeight = static_cast<int>(rect.getHeight() / 12);
    rect.reduce(0, rectHeight);

    portLabel.setBounds(rect.removeFromTop(rectHeight));
    portEditor.setBounds(rect.removeFromTop(rectHeight));
    errorLabel.setBounds(rect.removeFromBottom(rectHeight));
    connectButton.setBounds(rect.removeFromBottom(rectHeight * 2));

    portLabel.setFont(fontHeight);

    const auto& portText = portEditor.getText();
    portEditor.clear();
    portEditor.setFont(fontHeight);
    portEditor.setText(portText);
}

ListeningState::ListeningState()
    : buttonLookAndFeel(std::make_shared<ButtonLookAndFeel>()) {
    Logger::writeToLog(__PRETTY_FUNCTION__);

    addAndMakeVisible(portLabel);
    portLabel.setEditable(false);
    portLabel.setLookAndFeel(labelLookAndFeel.get());
    portLabel.setJustificationType(Justification::bottomLeft);
    portLabel.setText("Port:", dontSendNotification);

    addAndMakeVisible(portEditor);
    portEditor.setTextToShowWhenEmpty("1234", Colours::grey);

    addAndMakeVisible(connectButton);
    connectButton.setLookAndFeel(buttonLookAndFeel.get());
    connectButton.setButtonText("Connect");
    connectButton.onClick = [this] { connectButtonClicked(); };

    addAndMakeVisible(errorLabel);
    errorLabel.setEditable(false);
    errorLabel.setLookAndFeel(labelLookAndFeel.get());
    errorLabel.setJustificationType(Justification::bottomLeft);
}

void ListeningState::changeListenerCallback(ChangeBroadcaster* source) {
    Logger::writeToLog(__PRETTY_FUNCTION__);

    removeChangeListener(dynamic_cast<ChangeListener*>(source));

    if (const auto& broadcaster = dynamic_cast<Component*>(source)) {
        if (const auto& parent = broadcaster->getParentComponent()) {
            parent->removeChildComponent(broadcaster);
            parent->addAndMakeVisible(this);
            parent->resized();
        }
    }
}

void ListeningState::connectButtonClicked() {
    Logger::writeToLog(__PRETTY_FUNCTION__);

    const auto& receiverPtr = SharedResourcePointer<ReceivingState>();

    errorLabel.setText("", dontSendNotification);
    if (!receiverPtr->connect(portEditor.getText().getIntValue())) {
        const auto& errorMsg =
            "Couldn't connect to port: " + portEditor.getText();
        Logger::writeToLog(errorMsg);
        errorLabel.setText(errorMsg, dontSendNotification);
        return;
    }
    Logger::writeToLog("Connected to port: " + portEditor.getText());

    // Tell the component to listen for OSC messages matching this address:
    receiverPtr->addListener(receiverPtr, AUDIO_STREAM_ADDRESS_PATTERN);

    addChangeListener(receiverPtr);
    sendChangeMessage();
}

//==============================================================================
ReceivingState::~ReceivingState() { shutdownAudio(); }

void ReceivingState::paint(Graphics& g) {
    auto rect = getLocalBounds();
    rect.removeFromLeft(10);
    rect = rect.removeFromTop(rect.getHeight() / 2.5);

    g.setFont(rect.getHeight() / 5);
    g.setColour(juce::Colours::white);
    g.drawText("Audio Level:", rect, juce::Justification::centredLeft, true);
}

void ReceivingState::resized() {
    Logger::writeToLog(__PRETTY_FUNCTION__);

    Component::resized();

    auto rect = getLocalBounds();
    const auto height = static_cast<int>(rect.getHeight() / 2.5);

    auto topRect = rect.removeFromTop(height);
    auto bottomRect = rect.removeFromBottom(height);

    levelSlider.setBounds(topRect.removeFromBottom(height / 2));
    levelSlider.setTextBoxStyle(levelSlider.getTextBoxPosition(),
                                !levelSlider.isTextBoxEditable(),
                                rect.getWidth() / 5, height / 2);

    stopButton.setBounds(bottomRect);
}

ReceivingState::ReceivingState()
    : sliderTextBoxLookAndFeel(std::make_shared<SliderTextBoxLookAndFeel>()),
      buttonLookAndFeel(std::make_shared<ButtonLookAndFeel>()) {
    Logger::writeToLog(__PRETTY_FUNCTION__);

    addAndMakeVisible(levelSlider);
    levelSlider.setRange(0, 100, 1);
    levelSlider.setValue(100);
    levelSlider.setLookAndFeel(sliderTextBoxLookAndFeel.get());

    addAndMakeVisible(stopButton);
    stopButton.setLookAndFeel(buttonLookAndFeel.get());
    stopButton.setButtonText("Stop");
    stopButton.onClick = [this] { stopButtonClicked(); };
}

void ReceivingState::changeListenerCallback(ChangeBroadcaster* source) {
    Logger::writeToLog(__PRETTY_FUNCTION__);

    removeChangeListener(dynamic_cast<ChangeListener*>(source));

    if (const auto& broadcaster = dynamic_cast<Component*>(source)) {
        if (const auto& parent = broadcaster->getParentComponent()) {
            setAudioChannels(0, 2);

            parent->removeChildComponent(broadcaster);
            parent->addAndMakeVisible(this);
            parent->resized();
        }
    }
}

void ReceivingState::oscMessageReceived(const OSCMessage& message) {
    for (const auto& item : message) {
        if (item.isBlob()) {
            auto& blob = item.getBlob();
            blob.copyTo(receivedBuffers[writeIndex++ %
                                        AUDIO_STREAM_CIRCULAR_BUFFER_SIZE],
                        0, blob.getSize());
        }
    }
}

void ReceivingState::getNextAudioBlock(
    const AudioSourceChannelInfo& bufferToFill) {
    auto* device = deviceManager.getCurrentAudioDevice();
    auto activeOutputChannels = device->getActiveOutputChannels();
    auto maxOutputChannels = activeOutputChannels.getHighestBit() + 1;

    auto level = static_cast<float>(levelSlider.getValue());
    auto numSamples = static_cast<size_t>(bufferToFill.numSamples);

    float* inBuffer;

    for (auto channel = 0; channel < maxOutputChannels; ++channel) {
        if (!activeOutputChannels[channel] || readIndex >= writeIndex) {
            // Clear the buffer
            bufferToFill.buffer->clear(channel, bufferToFill.startSample,
                                       bufferToFill.numSamples);
            return;
        }

        auto* outBuffer = bufferToFill.buffer->getWritePointer(
            channel, bufferToFill.startSample);
        inBuffer =
            receivedBuffers[readIndex++ % AUDIO_STREAM_CIRCULAR_BUFFER_SIZE];

        for (size_t sample = 0; sample < numSamples; ++sample) {
            outBuffer[sample] = inBuffer[sample] * level / 100.0;
        }
    }
}

void ReceivingState::stopButtonClicked() {
    Logger::writeToLog(__PRETTY_FUNCTION__);

    shutdownAudio();

    addChangeListener(SharedResourcePointer<StoppedState>());
    sendChangeMessage();
}

