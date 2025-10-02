#ifndef EON_H
#define EON_H

#include <cstdlib>
#include <unistd.h>
#include <iostream>

#include "App.h"
#include "channels.h"

class EON : public App {
private:
    Channels currentChannel{Channels::SVT1};
    bool running{false};

    int channelToAlt(Channels ch);

public:
    EON();
    ~EON();

    void start();
    void stop();
    bool isRunning() const;

    void setChannel(Channels ch);
    Channels getChannel() const;
};

#endif
