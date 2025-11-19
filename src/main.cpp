#include "waydroid.h"
#include "channels.h"

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>
#include <dirent.h>
#include <cstring>
#include <fstream>
#include <limits.h>
#include <sys/stat.h>
#include <poll.h>

using namespace std;

std::map<int, Channels> channelMap = {
    {1, Channels::SVT1},
    {2, Channels::SVT2},
    {3, Channels::KUNSKAPSKANALEN},
    {4, Channels::SVT24},
    {5, Channels::EON_BN_MUZIKA},
    {6, Channels::EON_BN},
    {7, Channels::EON_HAPPY},
    {8, Channels::EON_PRVA},
    {9, Channels::EON_PINK},
    {0, Channels::EON_RTS_1}
};

// Helpers for evdev key bitmask
static inline bool test_bit(size_t bit, const unsigned long* array) {
    const size_t BITS_PER_LONG = sizeof(unsigned long) * 8;
    return (array[bit / BITS_PER_LONG] >> (bit % BITS_PER_LONG)) & 1UL;
}

static bool deviceSupportsAnyKeys(int fd, const std::vector<int>& keys) {
    const size_t BITS_PER_LONG = sizeof(unsigned long) * 8;
    unsigned long keybits[(KEY_MAX + BITS_PER_LONG) / BITS_PER_LONG];
    memset(keybits, 0, sizeof(keybits));
    if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keybits)), keybits) < 0) {
        return false;
    }
    for (int k : keys) {
        if (k >= 0 && k <= KEY_MAX && test_bit((size_t)k, keybits)) return true;
    }
    return false;
}

// Simplified device discovery: return all /dev/input/event* paths
vector<string> findKeyboardDevices() {
    DIR* dir = opendir("/dev/input");
    if (!dir) {
        cerr << "Cannot open /dev/input" << endl;
        return {};
    }

    struct dirent* entry;
    vector<string> devices;

    while ((entry = readdir(dir)) != nullptr) {
        if (strncmp(entry->d_name, "event", 5) == 0) {
            devices.push_back(string("/dev/input/") + entry->d_name);
        }
    }
    closedir(dir);
    return devices;
}

// Function to handle keyboard input from multiple devices
void handleKeyboardInput(unique_ptr<Waydroid>& w, const vector<string>& devicePaths) {
    if (devicePaths.empty()) {
        cerr << "No keyboard devices found" << endl;
        return;
    }

    vector<int> fds;
    fds.reserve(devicePaths.size());
    vector<pollfd> pfds;
    pfds.reserve(devicePaths.size());

    for (const auto& path : devicePaths) {
        int fd = open(path.c_str(), O_RDONLY);
        if (fd < 0) {
            cerr << "Cannot open device: " << path << endl;
            fd = open(path.c_str(), O_RDWR);
            if (fd < 0) {
                cerr << "Also failed to open with O_RDWR: " << path << endl;
                continue;
            }
        }
        if (ioctl(fd, EVIOCGRAB, 1) == -1) {
            perror((string("EVIOCGRAB failed for ") + path).c_str());
        } else {
            cout << "Grabbed: " << path << endl;
        }
        fds.push_back(fd);
        pfds.push_back(pollfd{fd, POLLIN, 0});
    }

    if (fds.empty()) {
        cerr << "Failed to open any keyboard devices" << endl;
        return;
    }

    cout << "Listening for numpad input..." << endl;
    cout << "Numpad Enter: Start Waydroid" << endl;
    cout << "Numpad Backspace: Stop Waydroid" << endl;
    cout << "Numpad 1-3: Change channels" << endl;
    cout << "Numpad +/-: Next/Previous channel" << endl;
    cout << "ESC: Exit" << endl;
    
    struct input_event ev;
    
    while (true) {
        int pr = poll(pfds.data(), pfds.size(), -1);
        if (pr < 0) {
            perror("poll");
            break;
        }
        for (size_t i = 0; i < pfds.size(); ++i) {
            if (!(pfds[i].revents & POLLIN)) continue;
            ssize_t n = read(pfds[i].fd, &ev, sizeof(ev));
            if (n != sizeof(ev)) {
                continue;
            }
            // Only process key press events (not release or repeat)
            if (ev.type == EV_KEY && ev.value == 1) {
                switch (ev.code) {
                case KEY_KPENTER: // Numpad Enter
                case KEY_ENTER:   // Some keypads send regular Enter
                    if (w->isRunning() && w->isConnectedAdb()) {
                        cout << "Waydroid is already running!" << endl;
                    } else {
                        cout << "Starting Waydroid..." << endl;
                        w->start();
                    }
                    break;
                    
                case KEY_BACKSPACE: // Backspace
                    if (!w->isRunning() && !w->isConnectedAdb()) {
                        cout << "Waydroid is not running!" << endl;
                    } else {
                        cout << "Stopping Waydroid..." << endl;
                        w->stop();
                    }
                    break;
                    
                case KEY_KP0: case KEY_KP1: case KEY_KP2: case KEY_KP3:
                case KEY_KP4: case KEY_KP5: case KEY_KP6: case KEY_KP7:
                case KEY_KP8: case KEY_KP9: {
                    if (!w->isRunning() || !w->isConnectedAdb()) {
                        cout << "Cannot change channels - Waydroid is not running!" << endl;
                        break;
                    }
                    
                    // Map keypad keys to numbers 0-9
                    int num;
                    switch (ev.code) {
                        case KEY_KP0: num = 0; break;
                        case KEY_KP1: num = 1; break;
                        case KEY_KP2: num = 2; break;
                        case KEY_KP3: num = 3; break;
                        case KEY_KP4: num = 4; break;
                        case KEY_KP5: num = 5; break;
                        case KEY_KP6: num = 6; break;
                        case KEY_KP7: num = 7; break;
                        case KEY_KP8: num = 8; break;
                        case KEY_KP9: num = 9; break;
                        default: num = -1; break;
                    }                    
                    cout << "Numpad key: " << num << " (code: " << ev.code << ")" << endl;
                    
                    // Check if this number is in the channelMap
                    auto it = channelMap.find(num);
                    if (it != channelMap.end()) {
                        cout << "Changing to channel " << num << endl;
                        w->setChannel(it->second);
                    } else {
                        cout << "No channel mapped to key " << num << endl;
                    }
                    break;
                }
                
                case KEY_KPPLUS: { // Numpad Plus - next channel
                    if (!w->isRunning() || !w->isConnectedAdb()) {
                        cout << "Cannot change channels - Waydroid is not running!" << endl;
                        break;
                    }
                    
                    // Find current channel number
                    int currentNum = -1;
                    Channels currentChannel = w->getChannel();
                    for (const auto& pair : channelMap) {
                        if (pair.second == currentChannel) {
                            currentNum = pair.first;
                            break;
                        }
                    }
                    
                    // Find next channel in map
                    auto it = channelMap.find(currentNum);
                    if (it != channelMap.end()) {
                        ++it;
                    }
                    
                    // Wrap around to beginning if at end
                    if (it == channelMap.end()) {
                        it = channelMap.begin();
                    }
                    
                    cout << "Channel up: changing to channel " << it->first << endl;
                    w->setChannel(it->second);
                    break;
                }
                
                case KEY_KPMINUS: { // Numpad Minus - previous channel
                    if (!w->isRunning() || !w->isConnectedAdb()) {
                        cout << "Cannot change channels - Waydroid is not running!" << endl;
                        break;
                    }
                    
                    // Find current channel number
                    int currentNum = -1;
                    Channels currentChannel = w->getChannel();
                    for (const auto& pair : channelMap) {
                        if (pair.second == currentChannel) {
                            currentNum = pair.first;
                            break;
                        }
                    }
                    
                    // Find previous channel in map
                    auto it = channelMap.find(currentNum);
                    if (it != channelMap.end() && it != channelMap.begin()) {
                        --it;
                    } else {
                        // Wrap around to end if at beginning
                        it = channelMap.end();
                        --it;
                    }
                    
                    cout << "Channel down: changing to channel " << it->first << endl;
                    w->setChannel(it->second);
                    break;
                }
                
                case KEY_ESC: // ESC to exit
                    cout << "Exiting..." << endl;
                    // Release all and exit
                    for (int fd : fds) {
                        ioctl(fd, EVIOCGRAB, 0);
                        close(fd);
                    }
                    return;
                }
            }
        }
    }
    
    for (int fd : fds) {
        ioctl(fd, EVIOCGRAB, 0);
        close(fd);
    }
}

int main() {
    unique_ptr<Waydroid> w = make_unique<Waydroid>();
    
    // Find the keyboard devices (simplified: list event devices)
    vector<string> keyboardDevices = findKeyboardDevices();
    if (keyboardDevices.empty()) {
        cerr << "No keyboard device found!" << endl;
        return 1;
    }
    
    // Handle keyboard input from all
    handleKeyboardInput(w, keyboardDevices);
    
    return 0;
}
