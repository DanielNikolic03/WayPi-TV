#include "waydroid.h"
#include "channels.h"

#include <iostream>
#include <memory>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <cstdio> // popen, pclose
#include <unistd.h> // access

using namespace std;

std::map<int, Channels> channelMap = {
    {1, Channels::SVT1},
    {2, Channels::SVT2},
    {3, Channels::SVT24}
};

// Execute a shell command and return stdout (and stderr via redirection) as string
static std::string execCmd(const std::string& cmd) {
    std::string result;
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return result;
    char buffer[512];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    pclose(pipe);
    return result;
}

// Trim helpers
static inline std::string ltrim(std::string s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch){ return !std::isspace(ch); }));
    return s;
}
static inline std::string rtrim(std::string s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch){ return !std::isspace(ch); }).base(), s.end());
    return s;
}
static inline std::string trim(std::string s) { return rtrim(ltrim(std::move(s))); }

// Parse a power status from tool output by locating the line containing "power status"
// and extracting the token after the colon. Returns: on|standby|transition|unknown (lowercase)
static std::string parsePowerStatusFromOutput(const std::string& out) {
    std::string line;
    std::istringstream iss(out);
    while (std::getline(iss, line)) {
        std::string low = line;
        for (char& c : low) c = static_cast<char>(::tolower(static_cast<unsigned char>(c)));
        auto pos = low.find("power status");
        if (pos == std::string::npos) continue;

        // Find colon and extract after it
        auto colon = low.find(':', pos);
        std::string status = (colon == std::string::npos) ? std::string() : low.substr(colon + 1);
        status = trim(status);

        // Remove any trailing parenthetical or extra description
        auto paren = status.find('(');
        if (paren != std::string::npos) status = trim(status.substr(0, paren));

        // Normalize some known phrasings
        if (status.rfind("in transition", 0) == 0) return "transition";
        if (status.rfind("standby", 0) == 0) return "standby";
        if (status == "on") return "on";
        if (status.rfind("unknown", 0) == 0) return "unknown";
        // As a fallback, exact word checks to avoid 'on' matching 'unknown'
        if (status == "in transition") return "transition";
    }
    return "unknown";
}

// Query TV power state via cec-client (logical address 0 = TV)
// Returns one of: "on", "standby", "transition", or "unknown"
static std::string getCecPowerStatus() {
    // First try explicit adapters via cec-ctl (lets us pick /dev/cecX when multiple exist)
    const std::vector<std::string> devs = {"/dev/cec0", "/dev/cec1", "/dev/cec2", "/dev/cec3"};
    for (const auto& dev : devs) {
        if (access(dev.c_str(), R_OK) != 0) continue; // skip missing/inaccessible
        std::string out = execCmd("cec-ctl -d " + dev + " --to 0 --get-power-status 2>&1");
        std::string status = parsePowerStatusFromOutput(out);
        if (status != "unknown") return status;
    }

    // Fallback to cec-client auto adapter selection
    const std::string cmd = "sh -c \"echo 'pow 0' | cec-client -s -d 1 2>&1\"";
    std::string out = execCmd(cmd);
    return parsePowerStatusFromOutput(out);
}

bool isHDMIConnected() {
    // Try multiple possible HDMI paths for Raspberry Pi 5
    std::vector<std::string> hdmiPaths = {
        "/sys/class/drm/card0-HDMI-A-1/status",
        "/sys/class/drm/card0-HDMI-A-2/status", 
        "/sys/class/drm/card1-HDMI-A-1/status",
        "/sys/class/drm/card1-HDMI-A-2/status"
    };
    
    for (const auto& path : hdmiPaths) {
        std::ifstream file(path);
        std::string status;
        if (file.is_open() && file >> status) {
            if (status == "connected") {
                return true;
            }
        }
    }
    
    return false;
}

int main() {
    unique_ptr<Waydroid> w = make_unique<Waydroid>();
    w->start();

    cout << "Interactive mode: press 1/2/3 to switch channels, 'm' for manual arrows, 's' to show TV/HDMI status, 'q' to quit." << endl;

    while (true) {
        cout << "> " << flush;
        char ch;
        if (!cin.get(ch)) {
            // EOF or input error
            cout << "\nInput closed. Exiting..." << endl;
            break;
        }

        // Consume trailing newline if present
        if (ch == '\n' || ch == '\r') {
            continue;
        }

        if (ch == 'q' || ch == 'Q') {
            cout << "Quitting..." << endl;
            break;
        }

        if (ch == 'm' || ch == 'M') {
            cout << "Manual keyboard control active (press 'q' inside to return)." << endl;
            w->handleKeyboardInput();
            continue;
        }

        if (ch == 's' || ch == 'S') {
            std::string tv = getCecPowerStatus();
            cout << "TV power (CEC): " << tv;
            if (tv == "unknown") {
                bool hdmi = isHDMIConnected();
                cout << ", HDMI link: " << (hdmi ? "connected" : "disconnected");
            }
            cout << endl;
            continue;
        }

        if (ch >= '0' && ch <= '9') {
            int userInput = ch - '0';
            auto it = channelMap.find(userInput);
            if (it != channelMap.end()) {
                w->setChannel(it->second);
                cout << "Set to channel " << userInput << endl;
            } else {
                cout << "Unknown channel number: " << userInput << " (valid: ";
                bool first = true;
                for (const auto &kv : channelMap) {
                    if (!first) cout << ", ";
                    cout << kv.first;
                    first = false;
                }
                cout << ")" << endl;
            }
            continue;
        }

        // Unrecognized key
        cout << "Unrecognized input '" << ch << "'. Use digits ";
        bool first = true;
        for (const auto &kv : channelMap) {
            if (!first) cout << ", ";
            cout << kv.first;
            first = false;
        }
    cout << ", 'm' for manual, 's' for HDMI status, or 'q' to quit." << endl;
        
        // Clear rest of the line if any leftover
        if (cin.peek() == '\n') cin.get();
    }

    return 0;
}

