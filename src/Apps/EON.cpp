#include "EON.h"

EON::EON() = default;

EON::~EON() {
    stop();
}

void EON::start() {
    system("adb shell am start -n com.ug.eon.android.tv/.TvActivity");
    running = true;

    // Navigate

}

void EON::stop() {
    int rc = system("adb shell am force-stop com.ug.eon.android.tv");
    (void)rc;
    running = false;
}

bool EON::isRunning() const {
    return running; 
}

int EON::channelToAlt(Channels ch) {
    // Map SVT channels to some internal code (placeholder)
    switch (ch) {
        case Channels::SVT1: return 1;
        case Channels::SVT2: return 2;
        case Channels::SVT24: return 5;
        default: return -1;
    }
}

void EON::setChannel(Channels ch) {
    int target = channelToAlt(ch);
    int current = channelToAlt(currentChannel);

    if (target < 0) {
        std::cerr << "Unknown target channel\n";
        return;
    }
    if (current < 0) {
        std::cerr << "Current channel unknown; refusing to navigate\n";
        return;
    }

    int delta = target - current;
    if (delta == 0) {
        std::cout << "Already on target channel\n";
        return;
    }

    for (int i = 0; i < std::abs(delta); ++i) {
        if (delta > 0) {
            system("adb shell input keyevent KEYCODE_DPAD_DOWN && sleep 0.5");
        } else {
            system("adb shell input keyevent KEYCODE_DPAD_UP && sleep 0.5");
        }
    }
    system("adb shell input keyevent KEYCODE_DPAD_CENTER && sleep 1");

    currentChannel = ch;
}

Channels EON::getChannel() const {
    return currentChannel;
}
