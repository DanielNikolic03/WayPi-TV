#include "waydroid.h"
#include <sys/select.h>
#include <sys/time.h>
// unistd.h likely already included through headers for usleep/STDIN_FILENO, but include explicitly for clarity
#include <unistd.h>

Waydroid::Waydroid() {
    parseStatus();
}

Waydroid::~Waydroid() {
    stop();
}

std::string Waydroid::executeCommand(const std::string& command) {
    std::array<char, 128> buffer;
    std::string result;
    
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    
    return result;
}

std::string Waydroid::trim(const std::string& str) {
    // Include all whitespace characters: space, tab, newline, carriage return, form feed, vertical tab
    const std::string whitespace = " \t\n\r\f\v";
    
    size_t first = str.find_first_not_of(whitespace);
    if (std::string::npos == first) {
        return "";  // String is all whitespace
    }
    
    size_t last = str.find_last_not_of(whitespace);
    return str.substr(first, (last - first + 1));
}

void Waydroid::parseStatus() {
    try {
        // Execute waydroid status command
        std::string output = executeCommand("waydroid status");
        
        sessionStatus = "UNKNOWN";
        containerStatus = "UNKNOWN";
        ipAddress = "UNKNOWN";

        // Parse the output line by line
        std::istringstream iss(output);
        std::string line;
        
        while (std::getline(iss, line)) {
            // Find the colon separator
            size_t colonPos = line.find(':');
            if (colonPos != std::string::npos) {
                std::string key = trim(line.substr(0, colonPos));
                std::string value = trim(line.substr(colonPos + 1));
                
                // Parse based on the key
                if (key == "Session") {
                    sessionStatus = value;
                } else if (key == "Container") {
                    containerStatus = value;
                } else if (key == "IP address") {
                    ipAddress = value;
                }
            }
        }
        
        // Debug output to show what was parsed
        std::cout << "Parsed Status - Session: " << sessionStatus 
                  << ", Container: " << containerStatus 
                  << ", IP: " << ipAddress << std::endl;
                  
    } catch (const std::exception& e) {
        std::cerr << "Error parsing waydroid status: " << e.what() << std::endl;
        sessionStatus = "UNKNOWN";
        containerStatus = "UNKNOWN";
        ipAddress = "UNKNOWN";
    }
}

/// @brief Starts Waydroid session and connects with adb
/// @return true if already running, false if started successfully
bool Waydroid::start() {
    if (isRunning() && isConnectedAdb()) {
        return true;
    }

    // Start Waydroid
    if (!isRunning()) {
        // Start waydroid session in background using nohup to detach it properly
        int result = system("nohup waydroid session start > /dev/null 2>&1 &");
        
        if (result == 0) {
            // Wait a moment for the session to initialize
            system("sleep 10");
            std::cout << "Waydroid session start command issued..." << std::endl;
            
            // Also start the waydroid app/emulator GUI
            std::cout << "Starting Waydroid app..." << std::endl;
            system("nohup waydroid show-full-ui > /dev/null 2>&1 &");
            system("sleep 10");
        } else {
            std::cerr << "Failed to start waydroid session" << std::endl;
        }
        parseStatus();  // Re-parse status after attempting to start
    }

    // Start Adb
    if (!isConnectedAdb()) {
        connectAdb();
    }
    
    return false;
}

/// @brief Stops Waydroid session
/// @return true if already stopped, false if stopped successfully
bool Waydroid::stop() {
    if (!isRunning() && !isConnectedAdb()) {
        return true;
    }

    // Stop Adb
    if (isConnectedAdb()) {
        disconnectAdb();
    }

    // Stop Waydroid
    if (isRunning()) {
        int result = system("waydroid session stop");
    
        if (result == 0) {
            std::cout << "Waydroid session stop command issued..." << std::endl;
        } else {
            std::cerr << "Failed to stop waydroid session" << std::endl;
        }
        parseStatus();
    }

    return false;
}

bool Waydroid::isRunning() {
    return (sessionStatus == "RUNNING" && containerStatus == "RUNNING");
}

void Waydroid::connectAdb() {
    if (!ipAddress.empty() && ipAddress != "UNKNOWN") {
        std::string adbCommand = "adb connect " + ipAddress + ":5555";
        int adbResult = system(adbCommand.c_str());
        
        if (adbResult == 0) 
            adbConnected = true;

    } else {
        std::cerr << "No valid IP address found for ADB connection" << std::endl;
    }
}

void Waydroid::disconnectAdb() {
    if (!ipAddress.empty() && ipAddress != "UNKNOWN") {
        std::string adbCommand = "adb disconnect " + ipAddress + ":5555";
        int adbResult = system(adbCommand.c_str());
        
        if (adbResult == 0) 
            adbConnected = false;

    } else {
        std::cerr << "No valid IP address found for ADB connection" << std::endl;
    }
}

bool Waydroid::isConnectedAdb() {
    return adbConnected;
}

void Waydroid::setChannel(Channels ch) {
    // Decide which app should own this channel
    auto appId = ChannelUtil::appFor(ch);

    // Create or switch app instance if necessary
    if (appId == ChannelUtil::AppId::SVT) {
        SVT* svtPtr = dynamic_cast<SVT*>(runningApp.get());
        if (!svtPtr) {
            runningApp = std::make_unique<SVT>();
            svtPtr = static_cast<SVT*>(runningApp.get());
            svtPtr->start();
        }
        svtPtr->setChannel(ch);
    } else if (appId == ChannelUtil::AppId::EON) {
        EON* eonPtr = dynamic_cast<EON*>(runningApp.get());
        if (!eonPtr) {
            runningApp = std::make_unique<EON>();
            eonPtr = static_cast<EON*>(runningApp.get());
            eonPtr->start();
        }
        eonPtr->setChannel(ch);
    } else {
        // Unknown channel: stop and reset app
        runningApp.reset();
    }

    currentChannel = ch;
}

Channels Waydroid::getChannel() {
    if (runningApp) {
        if (auto* svt = dynamic_cast<SVT*>(runningApp.get())) return svt->getChannel();
        if (auto* eon = dynamic_cast<EON*>(runningApp.get())) return eon->getChannel();
    }
    return currentChannel; // fallback to last requested channel
}

void Waydroid::handleKeyboardInput() {
    struct termios old_tio, new_tio;
    
    // Get current terminal settings
    tcgetattr(STDIN_FILENO, &old_tio);
    new_tio = old_tio;
    
    // Disable canonical mode and echo
    new_tio.c_lflag &= (~ICANON & ~ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);
    
    std::cout << "Keyboard control active. Use arrow keys, Enter, and Esc for Back. Press 'q' to quit." << std::endl;
    
    char ch;
    while (true) {
        ch = getchar();
        
        // Check for escape (Esc key or arrow key escape sequence)
        if (ch == 27) { // ESC
            // Wait briefly to see if this is an arrow sequence (Esc '[' ...)
            fd_set rfds;
            FD_ZERO(&rfds);
            FD_SET(STDIN_FILENO, &rfds);
            timeval tv{0, 50000}; // 50ms
            int rv = select(STDIN_FILENO + 1, &rfds, nullptr, nullptr, &tv);

            if (rv == 1) {
                int next = getchar();
                if (next == '[') {
                    int arrow = getchar(); // The actual arrow key code
                    switch (arrow) {
                        case 'A': // Up arrow
                            system("adb shell input keyevent KEYCODE_DPAD_UP");
                            break;
                        case 'B': // Down arrow
                            system("adb shell input keyevent KEYCODE_DPAD_DOWN");
                            break;
                        case 'C': // Right arrow
                            system("adb shell input keyevent KEYCODE_DPAD_RIGHT");
                            break;
                        case 'D': // Left arrow
                            system("adb shell input keyevent KEYCODE_DPAD_LEFT");
                            break;
                        default:
                            break;
                    }
                } else {
                    // Not an arrow sequence -> treat as Back; put char back for next loop
                    if (next != EOF) ungetc(next, stdin);
                    system("adb shell input keyevent KEYCODE_BACK");
                }
            } else {
                // Timeout: plain Esc key -> Back
                system("adb shell input keyevent KEYCODE_BACK");
            }
            continue;
        }
        else if (ch == '\n' || ch == '\r') { // Enter key
            system("adb shell input keyevent KEYCODE_DPAD_CENTER"); // Power button
        }
        else if (ch == 'q' || ch == 'Q') { // Quit
            std::cout << "Quitting keyboard control..." << std::endl;
            break;
        }
        
        // Small delay to prevent overwhelming the system
        usleep(50000); // 50ms delay
    }
    
    // Restore original terminal settings
    tcsetattr(STDIN_FILENO, TCSANOW, &old_tio);
}
