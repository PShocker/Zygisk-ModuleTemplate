// Copyright (c) 2016 avs333
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
//		of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
//		to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//		copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
//		The above copyright notice and this permission notice shall be included in all
//		copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// 		AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

//fork from https://github.com/avs333/Nougat_dlfunctions
//do some modify
//support all cpu abi such as x86, x86_64
//support filename search if filename is not start with '/'

#include "dlfcn_nougat.h"
#include <sys/system_properties.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <dlfcn.h>

#define ANDROID_K 19
#define ANDROID_L 21
#define ANDROID_L2 22
#define ANDROID_M 23
#define ANDROID_N 24
#define ANDROID_N2 25


//Android 8.0
#define ANDROID_O 26


//Android 8.1
#define ANDROID_O2 27

//Android 9.0
#define ANDROID_P 28


//Android 10.0
#define ANDROID_Q 29

//Android 11.0
#define ANDROID_R 30



#define TAG_NAME    "Camel"

#define log_info(fmt, args...) __android_log_print(ANDROID_LOG_INFO, TAG_NAME, (const char *) fmt, ##args)
#define log_err(...) __android_log_print(ANDROID_LOG_ERROR, TAG_NAME, __VA_ARGS__)

#ifdef LOG_DBG
#define log_dbg log_info
#else
#define log_dbg(...)
#endif

#ifdef __LP64__
#define Elf_Ehdr Elf64_Ehdr
#define Elf_Shdr Elf64_Shdr
#define Elf_Sym  Elf64_Sym
#else
#define Elf_Ehdr Elf32_Ehdr
#define Elf_Shdr Elf32_Shdr
#define Elf_Sym  Elf32_Sym
#endif


struct ctx {
    void *load_addr;
    void *dynstr;
    void *dynsym;
    int nsyms;
    off_t bias;
};

static vector<string> Split(const string &str, const string &pattern);

//extern "C" {

int fake_dlclose(void *handle) {
    if (handle) {
        struct ctx *ctx = (struct ctx *) handle;
        if (ctx->dynsym) free(ctx->dynsym);    /* we're saving dynsym and dynstr */
        if (ctx->dynstr) free(ctx->dynstr);    /* from library file just in case */
        free(ctx);
    }
    return 0;
}

#define fatal(fmt, args...) do { log_err(fmt,##args); goto err_exit; } while(0)

/* flags are ignored */
void *fake_dlopen_with_path(const char *libpath, int flags) {
    log_info("fake_dlopen_with_path path  %s", libpath);
    FILE *maps;
    char buff[256];
    struct ctx *ctx = 0;
    off_t load_addr, size;
    int k, fd = -1, found = 0;
    char *shoff;
    auto *elf = (Elf_Ehdr *) MAP_FAILED;


    maps = fopen("/proc/self/maps", "r");

    if (!maps) {
        log_err("打开 maps文件失败 ");
        if (fd >= 0) close(fd);
        if (elf != MAP_FAILED) munmap(elf, size);
        fake_dlclose(ctx);
    }

    const vector<string> &vector = Split(libpath, "|");
    if (vector.size() == 2) {
        while (!found && fgets(buff, sizeof(buff), maps)) {
            if (strstr(buff, "r-xp") ) {

                //如果包含| 并且规则匹配
                if (strstr(buff, vector.at(0).c_str()) && strstr(buff, vector.at(1).c_str())) {
//c0940000-c09b7000 r-xp 00000000 fc:08 1229975                            /data/app/com.kejian.one-Ym849UWJxpgSsC27ctZ8CA==/lib/arm/libTest.so
                    auto *pString = new string(buff);

                    string path = pString->substr(pString->find("/data/app/"));

                    //去掉回车 坑！！！！！
                    size_t n = path.find_last_not_of("\r\n\t");
                    if (n != string::npos){
                        path.erase(n + 1, path.size() - n);
                    }

                    libpath = path.c_str();
                    found = 1;
                }
            }
        }
    } else {
        while (!found && fgets(buff, sizeof(buff), maps)) {

            if ((strstr(buff, "r-xp") || strstr(buff, "r--p")) && strstr(buff, libpath)) {

                found = 1;

                break;

            }

        }
    }

    fclose(maps);

    if (!found) log_err("%s runtime 没找到指定路径 ", libpath);


    if (sscanf(buff, "%lx", &load_addr) != 1) fatal("failed to read load address for %s", libpath);



    /* Now, mmap the same library once again */

    fd = open(libpath, O_RDONLY);
    if (fd < 0) fatal("打开文件失败 %s  %d", libpath, fd);
//    else
//        log_info("打开文件成功 %s", libpath);


    size = lseek(fd, 0, SEEK_END);
    if (size <= 0) fatal("lseek() failed for %s", libpath);



    elf = (Elf_Ehdr *) mmap(0, size, PROT_READ, MAP_SHARED, fd, 0);
    close(fd);
    fd = -1;

    if (elf == MAP_FAILED) fatal("mmap() failed for %s", libpath);


    ctx = (struct ctx *) calloc(1, sizeof(struct ctx));
    if (!ctx) fatal("no memory for %s", libpath);



    ctx->load_addr = (void *) load_addr;
    shoff = ((char *) elf) + elf->e_shoff;

    for (k = 0; k < elf->e_shnum; k++, shoff += elf->e_shentsize) {

        Elf_Shdr *sh = (Elf_Shdr *) shoff;
        log_dbg("%s: k=%d shdr=%p type=%x", __func__, k, sh, sh->sh_type);

        switch (sh->sh_type) {

            case SHT_DYNSYM:
                if (ctx->dynsym) fatal("%s: duplicate DYNSYM sections", libpath); /* .dynsym */
                ctx->dynsym = malloc(sh->sh_size);
                if (!ctx->dynsym) fatal("%s: no memory for .dynsym", libpath);
                memcpy(ctx->dynsym, ((char *) elf) + sh->sh_offset, sh->sh_size);
                ctx->nsyms = (sh->sh_size / sizeof(Elf_Sym));
                break;

            case SHT_STRTAB:
                if (ctx->dynstr) break;    /* .dynstr is guaranteed to be the first STRTAB */
                ctx->dynstr = malloc(sh->sh_size);
                if (!ctx->dynstr) fatal("%s: no memory for .dynstr", libpath);
                memcpy(ctx->dynstr, ((char *) elf) + sh->sh_offset, sh->sh_size);
                break;

            case SHT_PROGBITS:
                if (!ctx->dynstr || !ctx->dynsym) break;
                /* won't even bother checking against the section name */
                ctx->bias = (off_t) sh->sh_addr - (off_t) sh->sh_offset;
                k = elf->e_shnum;  /* exit for */
                break;
        }
    }

    munmap(elf, size);
    elf = 0;

    if (!ctx->dynstr || !ctx->dynsym) fatal("dynamic sections not found in %s", libpath);

#undef fatal

    log_dbg("%s: ok, dynsym = %p, dynstr = %p", libpath, ctx->dynsym, ctx->dynstr);

    return ctx;

    err_exit:
    if (fd >= 0) close(fd);
    if (elf != MAP_FAILED) munmap(elf, size);
    fake_dlclose(ctx);
    return 0;
}


#if defined(__LP64__)
static const char *const kSystemLibDir = "/system/lib64/";
static const char *const kOdmLibDir = "/odm/lib64/";
static const char *const kVendorLibDir = "/vendor/lib64/";
#else
static const char *const kSystemLibDir = "/system/lib/";
static const char *const kOdmLibDir = "/odm/lib/";
static const char *const kVendorLibDir = "/vendor/lib/";


#endif




void *fake_dlopen(const char *filename, int flags) {
    //log_info("start fake_dlopen");
    if ((strlen(filename) > 0 && filename[0] == '/') || (strstr(filename, "|") != nullptr)) {
        return fake_dlopen_with_path(filename, flags);
    } else {

        //尝试对系统So路径进行拼接
        char buf[512] = {0};
        void *handle = NULL;
        // Android 11 fix
        if(strstr(filename, "libart.so")){
            char * art_lib_path = nullptr;
            if (sizeof(void*) == 8) {
                if(android_get_device_api_level()>=ANDROID_R){
                    art_lib_path = "/apex/com.android.art/lib64/libart.so";
                }
                else if (android_get_device_api_level() >= ANDROID_Q) {
                    art_lib_path = "/apex/com.android.runtime/lib64/libart.so";
                } else {
                    art_lib_path = "/system/lib64/libart.so";
                }
            } else {
                //兼容Android 11
                if(android_get_device_api_level()>=ANDROID_R){
                    art_lib_path = "/apex/com.android.art/lib/libart.so";
                }else if (android_get_device_api_level() >= ANDROID_Q) {
                    art_lib_path = "/apex/com.android.runtime/lib/libart.so";
                } else {
                    art_lib_path = "/system/lib/libart.so";
                }
            }

            handle = fake_dlopen_with_path(buf, flags);
            if (handle) {
                return handle;
            }
        }

        memset(buf, 0, sizeof(buf));
        ///system/lib/com.kejian.one|libTest.so
        strcpy(buf, kSystemLibDir);
        strcat(buf, filename);
        //  log_err("路径变为 %s", buf);
        handle = fake_dlopen_with_path(buf, flags);
        if (handle) {
            return handle;
        }

        //odm
        memset(buf, 0, sizeof(buf));
        strcpy(buf, kOdmLibDir);
        strcat(buf, filename);
        handle = fake_dlopen_with_path(buf, flags);
        if (handle) {
            return handle;
        }

        //vendor
        memset(buf, 0, sizeof(buf));
        strcpy(buf, kVendorLibDir);
        strcat(buf, filename);
        handle = fake_dlopen_with_path(buf, flags);
        if (handle) {
            return handle;
        }

        return fake_dlopen_with_path(filename, flags);
    }


}

void *fake_dlsym(void *handle, const char *name) {

    if(name == NULL){
        log_err("Camel fake_dlsym name nullptr  ");
        return NULL;
    }
    if(handle== NULL){
        log_err("Camel fake_dlsym handle nullptr %s ", name);
        return NULL;
    }

    auto *ctx = (struct ctx *) handle;
    if(ctx== NULL){
        log_err("Camel fake_dlsym lib handle ->ctx nullptr  %s ", name);
        return NULL;
    }

    //指向函数符号的
    auto *sym = (Elf_Sym *) ctx->dynsym;
    if(sym == NULL){
        log_err("Camel fake_dlsym sym nullptr %s ", name);
        return NULL;
    }
    //指向字符串表的指针
    char *strings = (char *) ctx->dynstr;
    if(strings == NULL){
        log_err("Camel fake_dlsym strings table nullptr %s ",name);
        return NULL;
    }

    int k;
    for (k = 0; k < ctx->nsyms; k++, sym++){

        char *dysym = strings + sym->st_name;
        //log_err(" %s ", (strings + sym->st_name));

        if (dysym!=nullptr&&(strcmp(dysym, name) == 0)) {
            void *ret = (char *) ctx->load_addr + sym->st_value - ctx->bias;
            log_info("Camel fake_dlsym get dlsym sucess %s ", name);

            return ret;
        }
    }
    log_err("Camel fake_dlsym get dlsym fail %s ", name);

    return NULL;
}
void *getSymCompat(const char *filename, const char *name) {
    void *handle = fake_dlopen(filename, RTLD_NOW);
    if (handle == nullptr) {
        log_err("Camel  getSymCompat getHandle null %s",filename);
        return nullptr;
    }
    void *dlsym = fake_dlsym(handle, name);
    if (dlsym == nullptr) {
        log_err("Camel getSymCompat fake_dlsym null %s ",name);
        return nullptr;
    }

    fake_dlclose(handle);
    return dlsym;
}

const char *fake_dlerror() {
    return nullptr;
}

static vector<string> Split(const string &str, const string &pattern){
    vector<string> retList ;
    if (pattern.empty())
        return retList;
    size_t start = 0, index = str.find_first_of(pattern, 0);
    while (index != std::string::npos) {
        if (start != index)
            retList.push_back(str.substr(start, index - start));
        start = index + 1;
        index = str.find_first_of(pattern, start);
    }
    if (!str.substr(start).empty())
        retList.push_back(str.substr(start));
    return retList;
}