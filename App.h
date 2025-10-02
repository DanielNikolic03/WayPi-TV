// Base interface for any controllable TV app
#ifndef app_H
#define app_H

#include "channels.h"

class App {
public:
    virtual ~App() = default; // ensure proper deletion via base pointer
    virtual void setChannel(Channels ch) = 0;
};

#endif