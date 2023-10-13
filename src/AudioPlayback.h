#pragma once

#include <JuceHeader.h>

constexpr static const char* filePatternsAllowed{"*.wav;*.aif;*.aiff;*.mp3;"};

inline std::unique_ptr<InputSource> makeInputSource(const URL& url) {
    if (const auto doc = AndroidDocument::fromDocument(url))
        return std::make_unique<AndroidDocumentInputSource>(doc);

#if !JUCE_IOS
    if (url.isLocalFile())
        return std::make_unique<FileInputSource>(url.getLocalFile());
#endif

    return std::make_unique<URLInputSource>(url);
}

inline std::unique_ptr<OutputStream> makeOutputStream(const URL& url) {
    if (const auto doc = AndroidDocument::fromDocument(url))
        return doc.createOutputStream();

#if !JUCE_IOS
    if (url.isLocalFile()) return url.getLocalFile().createOutputStream();
#endif

    return url.createOutputStream();
}

//==============================================================================
class AudioPlayback : public AudioAppComponent, private ChangeListener {
  public:
    AudioPlayback() : state(Stopped) {
        addAndMakeVisible(&openButton);
        openButton.setButtonText("Open...");
        openButton.onClick = [this] { openButtonClicked(); };

        addAndMakeVisible(&playButton);
        playButton.setButtonText("Play");
        playButton.onClick = [this] { playButtonClicked(); };
        playButton.setColour(juce::TextButton::buttonColourId,
                             juce::Colours::green);
        playButton.setEnabled(false);

        addAndMakeVisible(&stopButton);
        stopButton.setButtonText("Stop");
        stopButton.onClick = [this] { stopButtonClicked(); };
        stopButton.setColour(juce::TextButton::buttonColourId,
                             juce::Colours::red);
        stopButton.setEnabled(false);

        // audio setup
        formatManager.registerBasicFormats();

        thread.startThread(Thread::Priority::normal);

        audioDeviceManager.initialise(
            0, /* the maximum number of input channels your app would like to
                  use (the actual number of channels opened may be less than the
                  number requested) */
            2, /* the maximum number of output channels your app would like to
                  use (the actual number of channels opened may be less than the
                  number requested) */
            nullptr, /* either a previously-saved state that was produced by
                        createStateXml(), or nullptr if you want the manager to
                        choose the best device to open. */
            true, /* if true, then if the device specified in the XML fails to
                     open, then a default device will be used instead. If false,
                     then on failure, no device is opened. */
            {},   /* if this is not empty, and there's a device with this name,
                     then that will be used as the default device (assuming that
                     there wasn't one specified in the XML). The string can
                     actually be a simple wildcard, containing "*" and "?"
                     characters */
            nullptr); /* if this is non-null, the structure will be used as the
                         set of preferred settings when opening the device. If
                         you use this parameter, the preferredDefaultDeviceName
                         field will be ignored. If you set the outputDeviceName
                         or inputDeviceName data members of the AudioDeviceSetup
                         to empty strings, then a default device will be used.
                       */

        audioDeviceManager.addAudioCallback(&audioSourcePlayer);
        audioSourcePlayer.setSource(&transportSource);

        setOpaque(true);
        setSize(500, 500);
    }

    ~AudioPlayback() override {
        transportSource.setSource(nullptr);
        audioSourcePlayer.setSource(nullptr);

        audioDeviceManager.removeAudioCallback(&audioSourcePlayer);
    }

    void paint(Graphics& g) override {
        /* Our component is opaque, so we must completely fill the background
         * with a solid colour */
        g.fillAll(getLookAndFeel().findColour(
            juce::ResizableWindow::backgroundColourId));
    }

    void resized() override {
        auto rect = getLocalBounds();
        rect.reduce(rect.getWidth() / 4, rect.getHeight() / 4);
        auto height = rect.getHeight() / 4;

        openButton.setBounds(rect.removeFromTop(height));
        stopButton.setBounds(rect.removeFromBottom(height));
        playButton.setBounds(
            rect.withSizeKeepingCentre(rect.getWidth(), height));
    }

  private:
    enum TransportState {
        Stopped,
        Starting,
        Playing,
        Pausing,
        Paused,
        Stopping
    };

    juce::TextButton openButton;
    juce::TextButton playButton;
    juce::TextButton stopButton;

    juce::AudioDeviceManager audioDeviceManager;
    juce::AudioFormatManager formatManager;
    TimeSliceThread thread{"audio file preview"};

    juce::AudioSourcePlayer audioSourcePlayer;
    juce::AudioTransportSource transportSource;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;

    TransportState state;
    std::unique_ptr<juce::FileChooser> chooser;

    //==============================================================================
    void getNextAudioBlock(
        const juce::AudioSourceChannelInfo& bufferToFill) override {
        if (readerSource.get() == nullptr) {
            bufferToFill.clearActiveBufferRegion();
            return;
        }

        transportSource.getNextAudioBlock(bufferToFill);
    }

    void prepareToPlay(int samplesPerBlockExpected,
                       double sampleRate) override {
        transportSource.prepareToPlay(samplesPerBlockExpected, sampleRate);
    }

    void releaseResources() override { transportSource.releaseResources(); }

    void changeState(TransportState newState) {
        if (state != newState) {
            state = newState;

            switch (state) {
                case Stopped:
                    juce::Logger::writeToLog("State: Stopped");
                    playButton.setButtonText("Play");
                    stopButton.setButtonText("Stop");
                    stopButton.setEnabled(false);
                    transportSource.setPosition(0.0);
                    break;

                case Starting:
                    juce::Logger::writeToLog("State: Starting");
                    transportSource.start();
                    changeState(Playing);
                    break;

                case Playing:
                    juce::Logger::writeToLog("State: Playing");
                    playButton.setButtonText("Pause");
                    stopButton.setButtonText("Stop");
                    stopButton.setEnabled(true);
                    break;

                case Pausing:
                    juce::Logger::writeToLog("State: Pausing");
                    transportSource.stop();
                    changeState(Paused);
                    break;

                case Paused:
                    juce::Logger::writeToLog("State: Paused");
                    playButton.setButtonText("Resume");
                    break;

                case Stopping:
                    juce::Logger::writeToLog("State: Stopping");
                    transportSource.stop();
                    changeState(Stopped);
                    break;
            }
        }
    }

    void changeListenerCallback(juce::ChangeBroadcaster*) override {}

    void openButtonClicked() {
        chooser = std::make_unique<juce::FileChooser>(
            "Select a file to play...", juce::File{}, filePatternsAllowed);
        auto chooserFlags = juce::FileBrowserComponent::openMode |
                            juce::FileBrowserComponent::canSelectFiles;

        chooser->launchAsync(chooserFlags, [this](const juce::FileChooser& fc) {
            juce::Logger::writeToLog("File chooser is launched");
            auto file = fc.getResult();

            if (file != juce::File{}) {
                juce::Logger::writeToLog("File is not empty");
                auto* reader = formatManager.createReaderFor(file);

                if (reader != nullptr) {
                    juce::Logger::writeToLog("Reader is not empty");
                    auto newSource =
                        std::make_unique<juce::AudioFormatReaderSource>(reader,
                                                                        true);

                    // ..and plug it into our transport source
                    transportSource.setSource(
                        newSource.get(),
                        0, /* tells it to buffer this many samples ahead */
                        nullptr, /* this is the background thread to use for
                                    reading-ahead */
                        reader->sampleRate); /* allows for sample rate
                                                correction */
                    juce::Logger::writeToLog("Source is set");

                    playButton.setEnabled(true);
                    readerSource.reset(newSource.release());
                }
            }
        });
    }

    void playButtonClicked() {
        if ((state == Stopped) || (state == Paused)) {
            changeState(Starting);
        } else if (state == Playing) {
            changeState(Pausing);
        }
    }

    void stopButtonClicked() {
        if (state == Paused) {
            changeState(Stopped);
        } else {
            changeState(Stopping);
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPlayback)
};
