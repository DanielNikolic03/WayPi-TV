#ifndef SVT_H
#define SVT_H

#include <cstdlib>
#include <unistd.h>
#include <iostream>

#include "../App.h"
#include "../channels.h"

class SVT : public App {
private:
    Channels currentChannel{Channels::SVT1};
    bool running{false};

    int channelToAlt(Channels ch);

public:
    SVT();
    ~SVT();

    void start();
    void stop();
    bool isRunning() const;

    void setChannel(Channels ch);
    Channels getChannel() const;
};

#endif
