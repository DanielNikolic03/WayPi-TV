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

// Function to find keyboard devices with keypad capability preference
vector<string> findKeyboardDevices() {
    DIR* dir = opendir("/dev/input");
    if (!dir) {
        cerr << "Cannot open /dev/input" << endl;
        return {};
    }
    
    struct dirent* entry;
    vector<string> devices;
    string fallback;
    
    while ((entry = readdir(dir)) != nullptr) {
        if (strncmp(entry->d_name, "event", 5) == 0) {
            string path = string("/dev/input/") + entry->d_name;
            int fd = open(path.c_str(), O_RDONLY);
            if (fd < 0) continue;
            
            char name[256] = "Unknown";
            ioctl(fd, EVIOCGNAME(sizeof(name)), name);
            
            // Look for keyboard-like devices
            string deviceName = name;
            transform(deviceName.begin(), deviceName.end(), deviceName.begin(), ::tolower);
            
            if (deviceName.find("keyboard") != string::npos || 
                deviceName.find("at translated") != string::npos) {
                // Prefer devices that actually expose keypad keys or KP Enter
                bool hasKeypad = deviceSupportsAnyKeys(fd, {
                    KEY_KP1, KEY_KP2, KEY_KP3, KEY_KP4, KEY_KP5,
                    KEY_KP6, KEY_KP7, KEY_KP8, KEY_KP9, KEY_KP0,
                    KEY_KPENTER, KEY_KPMINUS, KEY_KPPLUS
                });
                if (hasKeypad) {
                    cout << "Keyboard device: " << name << " at " << path << endl;
                    devices.push_back(path);
                } else if (fallback.empty()) {
                    // keep first keyboard-like device as fallback
                    fallback = path;
                }
            }
            close(fd);
        }
    }
    closedir(dir);
    if (devices.empty() && !fallback.empty()) {
        devices.push_back(fallback);
    }
    return devices;
}

// Function to unbind keyboard from kernel
bool unbindKeyboard(const string& devicePath) {
    // Extract event number from path (e.g., /dev/input/event3 -> event3)
    size_t lastSlash = devicePath.find_last_of('/');
    if (lastSlash == string::npos) return false;

    string eventName = devicePath.substr(lastSlash + 1);

    // Resolve the sysfs device backing this event node
    string deviceSyspath = "/sys/class/input/" + eventName + "/device";
    char realPath[PATH_MAX] = {0};
    if (realpath(deviceSyspath.c_str(), realPath) == nullptr) {
        cerr << "Cannot resolve sysfs path for " << deviceSyspath << endl;
        return false;
    }
    string resolvedPath = realPath;

    // Helper lambdas
    auto basenameOf = [](const string& p) {
        size_t pos = p.find_last_of('/');
        return (pos == string::npos) ? p : p.substr(pos + 1);
    };
    auto readLink = [](const string& p) -> string {
        char buf[PATH_MAX];
        ssize_t len = readlink(p.c_str(), buf, sizeof(buf) - 1);
        if (len == -1) return "";
        buf[len] = '\0';
        return string(buf);
    };
    auto exists = [](const string& p) -> bool {
        struct stat st{};
        return lstat(p.c_str(), &st) == 0;
    };

    cout << "Sysfs path for device: " << resolvedPath << endl;

    // 1) Try PS/2 (atkbd via serio) path first
    if (resolvedPath.find("/serio") != string::npos) {
        size_t serioPos = resolvedPath.find("serio");
        string serioName = resolvedPath.substr(serioPos);
        size_t slashPos = serioName.find('/');
        if (slashPos != string::npos) serioName = serioName.substr(0, slashPos);

        string unbindPath = "/sys/bus/serio/drivers/atkbd/unbind";
        ofstream f(unbindPath);
        if (!f) {
            cerr << "Failed to open " << unbindPath << " (need root?)" << endl;
        } else {
            cout << "Unbinding PS/2 device: " << serioName << endl;
            f << serioName;
            f.close();
            cout << "Unbound via atkbd." << endl;
            return true;
        }
    }

    // 2) Try HID device (hid-generic or vendor HID driver) in /sys/bus/hid/drivers
    // Walk up from resolvedPath to find a node that has a 'driver' symlink pointing to hid driver
    {
        string cur = resolvedPath;
        for (int i = 0; i < 10 && !cur.empty(); ++i) { // limit ascent depth
            string driverLink = cur + "/driver";
            if (exists(driverLink)) {
                string target = readLink(driverLink);
                // The driver name is the last component of the target path
                string driverName = basenameOf(target);
                string nodeName = basenameOf(cur); // e.g., 0003:VID:PID.xxxx

                // If this is a HID driver, try unbinding there
                if (driverName.find("hid") != string::npos) {
                    string unbindPath = string("/sys/bus/hid/drivers/") + driverName + "/unbind";
                    ofstream f(unbindPath);
                    if (f) {
                        cout << "Unbinding HID device (" << nodeName << ") from driver '" << driverName << "'" << endl;
                        f << nodeName;
                        f.close();
                        cout << "Unbound via HID driver." << endl;
                        return true;
                    } else {
                        cerr << "Failed to open " << unbindPath << " (need root?)" << endl;
                    }
                }

                // If this is a USB driver (usbhid), unbind using interface id
                if (driverName == "usbhid") {
                    // For USB interface, the directory name is like '1-2:1.0'
                    string ifaceId = basenameOf(cur);
                    string unbindPath = "/sys/bus/usb/drivers/usbhid/unbind";
                    ofstream f(unbindPath);
                    if (f) {
                        cout << "Unbinding USB HID interface " << ifaceId << " from usbhid" << endl;
                        f << ifaceId;
                        f.close();
                        cout << "Unbound via usbhid." << endl;
                        return true;
                    } else {
                        cerr << "Failed to open " << unbindPath << " (need root?)" << endl;
                    }
                }
            }

            // ascend
            size_t slash = cur.find_last_of('/');
            if (slash == string::npos) break;
            cur = cur.substr(0, slash);
        }
    }

    cerr << "Could not determine how to unbind this device automatically."
         << " Falling back to EVIOCGRAB only (kernel may still see keys)." << endl;
    return false;
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

    // Find the keyboard devices
    vector<string> keyboardDevices = findKeyboardDevices();
    if (keyboardDevices.empty()) {
        cerr << "No keyboard device found!" << endl;
        return 1;
    }
    
    // Handle keyboard input from all
    handleKeyboardInput(w, keyboardDevices);
    
    return 0;
}
