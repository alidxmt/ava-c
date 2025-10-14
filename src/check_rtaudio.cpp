#include <iostream>
#include "RtAudio.h"

int main() {
    RtAudio dac;
    unsigned int count = dac.getDeviceCount();
    std::cout << "RtAudio OK. Device count = " << count << "\n";
    std::cout << "Default output device id = " << dac.getDefaultOutputDevice() << "\n";
    return 0;
}
