#ifndef MOVE_H
#define MOVE_H

#include <cstdlib>
#include <unistd.h>
#include <iostream>

#include "../App.h"
#include "../channels.h"

class MOVE : public App {
private:
    Channels currentChannel{Channels::};
    bool running{false};

    int channelToAlt(Channels ch);

public:
    MOVE();
    ~MOVE();

    void start();
    void stop();
    bool isRunning() const;

    void setChannel(Channels ch);
    Channels getChannel() const;
};

#endif
