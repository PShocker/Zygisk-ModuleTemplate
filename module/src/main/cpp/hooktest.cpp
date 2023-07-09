//
// Created by admin on 2023-06-12.
//

#include <cstdlib>
#include <string.h>
#include <dlfcn.h>
#include "all.h"



static char* getStackInfo(void *lr) {
    Dl_info info;
    memset(&info, 0, sizeof(info));
    dladdr(lr, &info);

    char result[512];
    sprintf(result, ", called from: %s (%s)", info.dli_fname, info.dli_sname);
    return result;
}


FILE *new_fopen(const char *path, const char *mode) {
    // 执行 stack 清理（不可省略）
    BYTEHOOK_STACK_SCOPE();

    // 在调用原函数之前，做点什么....
    LOGD("pre fopen: %s  %s", path, getStackInfo(BYTEHOOK_RETURN_ADDRESS()));

    if (strstr(path, "file.txt")){
        return nullptr;
    } else{
        // 调用真正的 fopen 函数
        FILE *fp = BYTEHOOK_CALL_PREV(new_fopen, path, mode);

        // 返回 fopen 的结果
        return fp;
    }
}


void hooked_callback(bytehook_stub_t task_stub, int status_code, const char *caller_path_name,
                          const char *sym_name, void *new_func, void *prev_func, void *arg){
    LOGD("sym_name=%s, status_code=%d, arg=%s", sym_name, status_code, reinterpret_cast<char *>(arg));
}


void pre_dlopen(const char *filename, void *data){
    LOGD("pre_dlopen=%s", filename);
}
void post_dlopen(const char *filename,
                               int result,  // 0: OK  -1: Failed
                               void *data) {
    LOGD("post_dlopen=%s, result=%d", filename, result);
}


void my_hook(char* package_name)
{
    bytehook_init(BYTEHOOK_MODE_AUTOMATIC, true);

    bytehook_add_dlopen_callback(pre_dlopen, post_dlopen, nullptr);

    char *arg = "yuanrenxue-gebilaohua";

    size_t size = strlen(package_name) + 1; // 获取字符串的长度（包括空字符）
    char* p = static_cast<char*>(malloc(size)); // 动态分配内存,记得free
    if (p) {
        strncpy(p, package_name, size);
    }
    bytehook_hook_single(
        "liblessontest.so",
        nullptr,
        "fopen",
        reinterpret_cast<void *>(new_fopen),
        hooked_callback,
        p);
}
