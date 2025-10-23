#include "SVT.h"
#include <unistd.h>

SVT::SVT() = default;

SVT::~SVT() {
    stop();
}

void SVT::start() {
    // Launch using host waydroid CLI (as requested)
    system("waydroid app launch se.svt.android.svtplay");
    system("waydroid app launch se.svt.android.svtplay");
    sleep(3);
    running = true;

    // Navigate to live stream SVT1
    system("adb shell input keyevent KEYCODE_DPAD_LEFT && sleep 1");
    system("adb shell input keyevent KEYCODE_DPAD_CENTER && sleep 5");
    system("adb shell input keyevent KEYCODE_DPAD_LEFT && sleep 1");
    system("adb shell input keyevent KEYCODE_DPAD_DOWN && sleep 1");
    system("adb shell input keyevent KEYCODE_DPAD_DOWN && sleep 1");
    system("adb shell input keyevent KEYCODE_DPAD_DOWN && sleep 1");
    system("adb shell input keyevent KEYCODE_DPAD_CENTER && sleep 5");
}

void SVT::stop() {
    int rc = system("adb shell am force-stop se.svt.android.svtplay");
    (void)rc;
    running = false;
}

bool SVT::isRunning() const {
    return running;
}

int SVT::channelToAlt(Channels ch) {
    // Map SVT channels to some internal code (placeholder)
    switch (ch) {
        case Channels::SVT1: return 1;
        case Channels::SVT2: return 2;
        case Channels::KUNSKAPSKANALEN: return 4;
        case Channels::SVT24: return 5;
        default: return -1;
    }
}

void SVT::setChannel(Channels ch) {
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
            system("adb shell input keyevent KEYCODE_DPAD_RIGHT && sleep 1");
        } else {
            system("adb shell input keyevent KEYCODE_DPAD_LEFT && sleep 1");
        }
    }
    system("adb shell input keyevent KEYCODE_DPAD_CENTER && sleep 1");

    currentChannel = ch;
}

Channels SVT::getChannel() const {
    return currentChannel;
}
