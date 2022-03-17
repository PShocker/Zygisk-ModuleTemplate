# Zygisk-ModuleTemplate
Zygisk module template. refer to  [Riru-ModuleTemplate](https://github.com/RikkaApps/Riru-ModuleTemplate) and [zygisk-module-sample](https://github.com/topjohnwu/zygisk-module-sample)


## Build
You must modify

local.properties to adapt your SDK path

You can also modify  /module/build.gradle file to match your NDK and CMAKE versions.

Make sure you have a jdk11 environment.

on the command line
run
```
gradlew :module:assembleRelease
```
or click
```
build.bat
```

## Others
build files in /out 
You can install it in Magisk24.0+
after phone reboot
you can find log in LogCat

