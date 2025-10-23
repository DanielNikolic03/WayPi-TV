#ifndef Waydroid_H
#define Waydroid_H

#include <string>
#include <memory>
#include <iostream>
#include <string>
#include <sstream>
#include <memory>
#include <stdexcept>
#include <array>
#include <algorithm>
#include <cctype>
#include <termios.h>
#include <unistd.h>
#include <cstdio>
#include <sys/types.h> // pid_t

#include "channels.h"
#include "App.h"
#include "Apps/SVT.h"
#include "Apps/EON.h"

class Waydroid {
private:
    std::string sessionStatus = "UNKNOWN";
    std::string containerStatus = "UNKNOWN";
    std::string ipAddress = "UNKNOWN";
    bool adbConnected = false;

    std::unique_ptr<App> runningApp = nullptr;
    Channels currentChannel{Channels::SVT1};
    
    void parseStatus();
    std::string executeCommand(const std::string& command);
    std::string trim(const std::string& str);

public:
    Waydroid();
    ~Waydroid();

    bool start();
    bool stop();
    bool isRunning();
    void connectAdb();
    void disconnectAdb();
    bool isConnectedAdb();
    
    void setChannel(Channels ch);
    Channels getChannel();

    // [DEBUG] Keyboard input handling
    void handleKeyboardInput();
    
    // Getters for the parsed status information
    const std::string& getSessionStatus() const { return sessionStatus; }
    const std::string& getContainerStatus() const { return containerStatus; }
    const std::string& getIpAddress() const { return ipAddress; }

private:
    pid_t uiPid = -1;

    bool showUI();   // open the UI window (full UI)
};

#endif
