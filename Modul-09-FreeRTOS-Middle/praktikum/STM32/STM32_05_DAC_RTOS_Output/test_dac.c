#include "config.h"
int main() {
#ifdef STM32F103xB
    int x = DAC_CHANNEL_1;
    return x;
#else
    return 0;
#endif
}
