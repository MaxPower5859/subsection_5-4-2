#include "smart_home_system.h"

int main()
{
    smartHomeSystemInit();
    while (true) {
        smartHomeSystemUpdate();
    }
}

/* El problema del codigo bloqueante se encuentra en pc_serial_com.cpp
 */