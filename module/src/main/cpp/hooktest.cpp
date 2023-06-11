//
// Created by admin on 2023-06-12.
//

#include <cstdlib>
#include <string.h>
#include "all.h"


FILE *new_fopen(const char *path, const char *mode) {
    // 执行 stack 清理（不可省略）
    BYTEHOOK_STACK_SCOPE();

    // 在调用原函数之前，做点什么....
    LOGD("pre fopen: %s", path);

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
    LOGD("sym_name=%s, status_code=%d", sym_name, status_code);
}

void my_hook()
{
    bytehook_init(BYTEHOOK_MODE_AUTOMATIC, true);

    bytehook_hook_single(
            "liblessontest.so",
            nullptr,
            "fopen",
            reinterpret_cast<void *>(new_fopen),
            hooked_callback,
            nullptr);
}
