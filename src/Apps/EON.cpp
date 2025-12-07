#include "EON.h"
#include <unistd.h>

EON::EON() = default;

EON::~EON() {
    stop();
}

void EON::start() {
    // Launch using host waydroid CLI (as requested)
    system("waydroid app launch com.ug.eon.android.tv");
    system("waydroid app launch com.ug.eon.android.tv");
    sleep(9);
    running = true;

    // Navigate to live stream channel 1
    std::cout << "EON: Navigating to live TV channel 1" << std::endl;
    system("adb shell input keyevent KEYCODE_DPAD_CENTER && sleep 3");
    system("adb shell input keyevent KEYCODE_DPAD_DOWN && sleep 2");
    system("adb shell input keyevent KEYCODE_DPAD_CENTER && sleep 1");
    system("adb shell input keyevent KEYCODE_DPAD_CENTER && sleep 1");
    system("adb shell input keyevent KEYCODE_DPAD_CENTER && sleep 1");
    system("adb shell input keyevent KEYCODE_DPAD_DOWN && sleep 1");
    system("adb shell input keyevent KEYCODE_DPAD_CENTER && sleep 1");
    system("adb shell input keyevent KEYCODE_BACK && sleep 1");
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
    switch (ch) {
        // EON app listing uses inconsistent numbering, so these numbers are not displayed
        case Channels::EON_RTS_1: return 1;
        case Channels::EON_PINK: return 3;
        case Channels::EON_PRVA: return 4;
        case Channels::EON_HAPPY: return 5;
        case Channels::EON_BN: return 200;
        case Channels::EON_BN_MUZIKA: return 223;
        case Channels::EON_NATURE: return 335;
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
    std::cout << "Changing channel from " << current << " to " << target << " (delta " << delta << ")\n";
    if (delta == 0) {
        std::cout << "Already on target channel\n";
        return;
    }

    system("adb shell input keyevent KEYCODE_BACK && sleep 1");
    for (int i = 0; i < std::abs(delta); ++i) {
        if (delta > 0) {
            system("adb shell input keyevent KEYCODE_DPAD_DOWN && sleep 0.5");
        } else {
            system("adb shell input keyevent KEYCODE_DPAD_UP && sleep 0.5");
        }
    }
    system("adb shell input keyevent KEYCODE_DPAD_CENTER && sleep 10");

    currentChannel = ch;
}

Channels EON::getChannel() const {
    return currentChannel;
}
