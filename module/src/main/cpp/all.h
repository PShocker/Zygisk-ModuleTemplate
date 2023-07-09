#include "bytehook.h"
#include <android/log.h>


#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, "Magisk-hexl", __VA_ARGS__)


void my_hook(char* package_name);