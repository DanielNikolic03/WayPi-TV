#include "MOVE.h"
#include <unistd.h>

MOVE::MOVE() = default;

MOVE::~MOVE() {
    stop();
}

void MOVE::start() {
    // Launch using host waydroid CLI (as requested)
    system("waydroid app launch com.mtssi.supernovabih");
    system("waydroid app launch com.mtssi.supernovabih");
    sleep(8);
    running = true;

    // Navigate to live stream channel RTS 1
    std::cout << "MOVE: Navigating to live TV channel RTS 1" << std::endl;
    system("adb shell input keyevent KEYCODE_BACK && sleep 2");
    system("adb shell input keyevent KEYCODE_DPAD_LEFT && sleep 2");
    system("adb shell input keyevent KEYCODE_DPAD_LEFT && sleep 2");
    system("adb shell input keyevent KEYCODE_DPAD_DOWN && sleep 2");
    system("adb shell input keyevent KEYCODE_DPAD_CENTER && sleep 2");
    system("adb shell input keyevent KEYCODE_DPAD_CENTER && sleep 2");
    system("adb shell input keyevent KEYCODE_BACK && sleep 2");
}

void MOVE::stop() {
    int rc = system("adb shell am force-stop com.mtssi.supernovabih");
    (void)rc;
    running = false;
}

bool MOVE::isRunning() const {
    return running; 
}

int MOVE::channelToAlt(Channels ch) {
    switch (ch) {
        // MOVE app listing uses inconsistent numbering, so these numbers are not displayed
        case Channels::MOVE_RTS_1: return 1;
        case Channels::MOVE_PINK: return 2;
        case Channels::MOVE_PRVA: return 3;
        case Channels::MOVE_HAPPY: return 4;
        case Channels::MOVE_BN: return 5;
        case Channels::MOVE_BN_MUZIKA: return 6;
        default: return -1;
    }
}

void MOVE::setChannel(Channels ch) {
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

    system("adb shell input keyevent KEYCODE_DPAD_LEFT && sleep 2");
    for (int i = 0; i < std::abs(delta); ++i) {
        if (delta > 0) {
            system("adb shell input keyevent KEYCODE_DPAD_DOWN && sleep 1");
        } else {
            system("adb shell input keyevent KEYCODE_DPAD_UP && sleep 1");
        }
    }
    system("adb shell input keyevent KEYCODE_DPAD_CENTER && sleep 2");
    system("adb shell input keyevent KEYCODE_BACK && sleep 2");

    currentChannel = ch;
}

Channels MOVE::getChannel() const {
    return currentChannel;
}
