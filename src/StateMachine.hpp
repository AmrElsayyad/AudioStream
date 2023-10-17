#pragma once

#include "State.hpp"

//==============================================================================
class StateMachine : public Component {
  public:
    StateMachine()
        : senderPtr(SharedResourcePointer<OSCSender>()),
          stoppedStatePtr(SharedResourcePointer<StoppedState>()),
          connectingStatePtr(SharedResourcePointer<ConnectingState>()),
          sendingStatePtr(SharedResourcePointer<SendingState>()),
          listeningStatePtr(SharedResourcePointer<ListeningState>()),
          receivingStatePtr(SharedResourcePointer<ReceivingState>()) {
        Logger::writeToLog(__FUNCTION__);

        addAndMakeVisible(stoppedStatePtr);

        setOpaque(true);
        setSize(500, 500);
    }

    void paint(Graphics& g) override {
        /* Our component is opaque, so we must completely fill the background
         * with a solid colour */
        g.fillAll(
            getLookAndFeel().findColour(ResizableWindow::backgroundColourId));
    }

    void resized() override {
        Component::resized();

        auto rect = getLocalBounds();
        rect.reduce(rect.getWidth() / 4, rect.getHeight() / 4);

        for (const auto& child : getChildren()) {
            child->setBounds(rect);
        }
    }

  private:
    //==============================================================================
    SharedResourcePointer<OSCSender> senderPtr;
    SharedResourcePointer<StoppedState> stoppedStatePtr;
    SharedResourcePointer<ConnectingState> connectingStatePtr;
    SharedResourcePointer<SendingState> sendingStatePtr;
    SharedResourcePointer<ListeningState> listeningStatePtr;
    SharedResourcePointer<ReceivingState> receivingStatePtr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StateMachine)
};
