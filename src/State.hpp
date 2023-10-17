#pragma once

#include "LookAndFeel.hpp"

#define AUDIO_STREAM_ADDRESS_PATTERN "/AudioStream"
#define AUDIO_STREAM_CIRCULAR_BUFFER_SIZE 1024
#define AUDIO_STREAM_AUDIO_BUFFER_SIZE 1024

//==============================================================================
/**
 * @class StoppedState
 * @brief A class that represents the stopped state of a state machine.
 */
class StoppedState : public Component,
                     public ChangeListener,
                     public ChangeBroadcaster {
  public:
    void resized() override;

  protected:
    StoppedState();

  private:
    std::shared_ptr<ButtonLookAndFeel> buttonLookAndFeel;
    TextButton senderButton;
    TextButton receiverButton;

    void changeListenerCallback(ChangeBroadcaster* source) override;
    void senderButtonClicked();
    void receiverButtonClicked();

    friend class SharedResourcePointer<StoppedState>;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StoppedState)
};

//==============================================================================
/**
 * @class ConnectingState
 * @brief Represents the connecting state of a state machine.
 */
class ConnectingState : public Component,
                        public ChangeListener,
                        public ChangeBroadcaster {
  public:
    void resized() override;

  protected:
    ConnectingState();

  private:
    std::shared_ptr<LabelLookAndFeel> labelLookAndFeel;
    std::shared_ptr<ButtonLookAndFeel> buttonLookAndFeel;

    Label ipLabel;
    Label portLabel;
    Label errorLabel;
    TextEditor ipEditor;
    TextEditor portEditor;
    TextButton connectButton;

    void changeListenerCallback(ChangeBroadcaster* source) override;
    void connectButtonClicked();

    friend class SharedResourcePointer<ConnectingState>;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ConnectingState)
};

//==============================================================================
/**
 * @class SendingState
 * @brief Represents the sending state of a state machine.
 */
class SendingState : public AudioAppComponent,
                     public ChangeListener,
                     public ChangeBroadcaster {
  public:
    ~SendingState();
    void paint(Graphics& g) override;
    void resized() override;

  protected:
    SendingState();

  private:
    std::shared_ptr<SliderTextBoxLookAndFeel> sliderTextBoxLookAndFeel;
    std::shared_ptr<ButtonLookAndFeel> buttonLookAndFeel;

    TextButton stopButton;
    Slider levelSlider{Slider::LinearHorizontal, Slider::TextBoxRight};

    void prepareToPlay(int /* samplesPerBlockExpected */,
                       double /* sampleRate */) override {}
    void releaseResources() override {}
    void changeListenerCallback(ChangeBroadcaster* source) override;
    void getNextAudioBlock(const AudioSourceChannelInfo& bufferToFill) override;
    void stopButtonClicked();

    friend class SharedResourcePointer<SendingState>;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SendingState)
};

//==============================================================================
/**
 * @class ListeningState
 * @brief Represents the listening state of a state machine.
 */
class ListeningState : public Component,
                       public ChangeListener,
                       public ChangeBroadcaster {
  public:
    void resized() override;

  protected:
    ListeningState();

  private:
    std::shared_ptr<LabelLookAndFeel> labelLookAndFeel;
    std::shared_ptr<ButtonLookAndFeel> buttonLookAndFeel;

    Label portLabel;
    Label errorLabel;
    TextEditor portEditor;
    TextButton connectButton;

    void changeListenerCallback(ChangeBroadcaster* source) override;
    void connectButtonClicked();

    friend class SharedResourcePointer<ListeningState>;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ListeningState)
};

//==============================================================================
/**
 * @class ReceivingState
 * @brief Represents the receiving state of a state machine.
 */
class ReceivingState : public AudioAppComponent,
                       public ChangeListener,
                       public ChangeBroadcaster,
                       public OSCReceiver,
                       public OSCReceiver::ListenerWithOSCAddress<
                           OSCReceiver::MessageLoopCallback> {
  public:
    ~ReceivingState();
    void paint(Graphics& g) override;
    void resized() override;

  protected:
    ReceivingState();

  private:
    float receivedBuffers[AUDIO_STREAM_CIRCULAR_BUFFER_SIZE]
                         [AUDIO_STREAM_AUDIO_BUFFER_SIZE];
    size_t readIndex{0};
    size_t writeIndex{0};

    std::shared_ptr<SliderTextBoxLookAndFeel> sliderTextBoxLookAndFeel;
    std::shared_ptr<ButtonLookAndFeel> buttonLookAndFeel;

    TextButton stopButton;
    Slider levelSlider{Slider::LinearHorizontal, Slider::TextBoxRight};

    void prepareToPlay(int /* samplesPerBlockExpected */,
                       double /* sampleRate */) override {}
    void releaseResources() override {}
    void changeListenerCallback(ChangeBroadcaster* source) override;
    void oscMessageReceived(const OSCMessage& message) override;
    void getNextAudioBlock(const AudioSourceChannelInfo& bufferToFill) override;
    void stopButtonClicked();

    friend class SharedResourcePointer<ReceivingState>;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ReceivingState)
};
