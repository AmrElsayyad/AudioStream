#pragma once

#include <JuceHeader.h>

constexpr const char* AUDIO_STREAM_ADDRESS_PATTERN = "/audiostream";
constexpr const char* AUDIO_STREAM_HOST = "127.0.0.1";
constexpr const int AUDIO_STREAM_PORT = 1234;
constexpr const size_t AUDIO_STREAM_QUEUE_READY_THRESH = 3;
constexpr const size_t AUDIO_STREAM_BUFFER_SIZE = 1024;
constexpr const size_t AUDIO_STREAM_CIRCULAR_BUFFER_SIZE = 1024;

//==============================================================================
class AudioStreamReceiver : public ChangeBroadcaster,
                            private OSCReceiver,
                            private OSCReceiver::ListenerWithOSCAddress<
                                OSCReceiver::MessageLoopCallback> {
  public:
    AudioStreamReceiver() {
        // specify here on which UDP port number to receive incoming messages
        if (!connect(AUDIO_STREAM_PORT)) {
            throw std::runtime_error("Error: could not connect to UDP port " +
                                     std::to_string(AUDIO_STREAM_PORT));
        }

        // tell the component to listen for OSC messages matching this address:
        addListener(this, AUDIO_STREAM_ADDRESS_PATTERN);
    }

    /** True if the queue size exceeds the threshold and is ready to be used */
    bool isReady() const {
        return ((writeIndex - readIndex) > AUDIO_STREAM_QUEUE_READY_THRESH);
    }

    /** True if the queue is not empty */
    bool hasData() const { return (writeIndex > readIndex); }
    /** Pops the first buffer from the queue if queue is not empty */

    const float* getNextBuffer() {
        if (!hasData()) {
            if (ready) {
                ready = false;
                sendChangeMessage();
            }
            return nullptr;
        }

        return receivedBuffers[readIndex++ % AUDIO_STREAM_CIRCULAR_BUFFER_SIZE];
    }

  private:
    //==============================================================================
    float receivedBuffers[AUDIO_STREAM_CIRCULAR_BUFFER_SIZE]
                         [AUDIO_STREAM_BUFFER_SIZE];
    size_t readIndex{0};
    size_t writeIndex{0};
    bool ready{false};

    //==============================================================================
    void oscMessageReceived(const OSCMessage& message) override {
        for (const auto& item : message) {
            if (item.isBlob()) {
                auto& blob = item.getBlob();
                blob.copyTo(receivedBuffers[writeIndex++ %
                                            AUDIO_STREAM_CIRCULAR_BUFFER_SIZE],
                            0, blob.getSize());

                if (!ready && isReady()) {
                    ready = true;
                    sendChangeMessage();
                }
            }
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioStreamReceiver)
};
