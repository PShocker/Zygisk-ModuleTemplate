//
// Created by admin on 2023-07-31.
//

#ifndef ZYGISK_MODULETEMPLATE_FRIDA_H
#define ZYGISK_MODULETEMPLATE_FRIDA_H

#if defined(__arm__)
#include "includes/armeabi-v7a/frida-gumjs.h"
#elif defined(__aarch64__)
#include "includes/arm64-v8a/frida-gumjs.h"
#endif

#include "all.h"

static void on_message( const gchar *message, GBytes *data, gpointer user_data);

int gumjsHook(const char *scriptpath);

char *readfile(const char *filepath);

int hookFunc(const char *scriptpath);

int startHook(const char *scriptpath);


#endif //ZYGISK_MODULETEMPLATE_FRIDA_H
