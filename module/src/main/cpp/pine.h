#include "all.h"
#include <jni.h>
#include "dlfcn_compat.h"

#ifndef ZYGISK_MODULETEMPLATE_PINE_H
#define ZYGISK_MODULETEMPLATE_PINE_H
bool pine_start(JNIEnv *env, bool isSystem);
void setDebuggable(bool debug);
#endif //ZYGISK_MODULETEMPLATE_PINE_H
