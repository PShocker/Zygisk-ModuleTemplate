#include <cstdlib>
#include <unistd.h>
#include <fcntl.h>
#include <android/log.h>
#include <string.h>

#include "zygisk.hpp"
#include "all.h"
#include "frida.h"
#include "pine.h"

using zygisk::Api;
using zygisk::AppSpecializeArgs;
using zygisk::ServerSpecializeArgs;


class MyModule : public zygisk::ModuleBase {
public:
    void onLoad(Api *api, JNIEnv *env) override {
        this->api = api;
        this->env = env;
    }

    void postAppSpecialize(const AppSpecializeArgs *args) override {
        // Use JNI to fetch our process name
        const char *process = env->GetStringUTFChars(args->nice_name, nullptr);
        preSpecialize(process);
        env->ReleaseStringUTFChars(args->nice_name, process);
    }

    void preServerSpecialize(ServerSpecializeArgs *args) override {
        preSpecialize("system_server");
    }

private:
    Api *api;
    JNIEnv *env;

    void preSpecialize(const char *process) {
        LOGD("pine_start, %s", process);
        bool start_result;
        if(strstr("system_server", process) || strstr(process, "com.android") ||
        strstr(process, "com.google") || strstr(process, "com.qualcomm") ||
        strstr(process, "android.process")){
//            start_result = pine_start(env, true);
        } else{
//            setDebuggable(false);
            start_result = pine_start(env, false);
        }
        LOGD("pine_start result: %d , %s", start_result, process);

        // Demonstrate connecting to to companion process
        // We ask the companion for a random number
        unsigned r = 0;
        int fd = api->connectCompanion();
        read(fd, &r, sizeof(r));
        close(fd);
//        LOGD("example: process=[%s], r=[%u]\n", process, r);
        if (strstr("com.hexl.lessontest", process)){
//            startHook("/data/local/tmp/hook.js");

            char* package_name = const_cast<char *>(process);
//            my_hook(package_name);
        } else{
            // Since we do not hook any functions, we should let Zygisk dlclose ourselves
            api->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);
        }
    }

};

static int urandom = -1;

static void companion_handler(int i) {
    if (urandom < 0) {
        urandom = open("/dev/urandom", O_RDONLY);
    }
    unsigned r;
    read(urandom, &r, sizeof(r));
//    LOGD("example: companion r=[%u]\n", r);
    write(i, &r, sizeof(r));
}

REGISTER_ZYGISK_MODULE(MyModule)
REGISTER_ZYGISK_COMPANION(companion_handler)
