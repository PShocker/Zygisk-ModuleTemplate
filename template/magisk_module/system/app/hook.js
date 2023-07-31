function main() {
    Java.perform(function () {
        console.log("11111");
        Java.enumerateClassLoaders({
            onMatch: function(loader) {
                let msg = `[loader]:${loader}`;
                console.log(msg);
                Java.use("com.hexl.lessontest.TestClass").print(msg);
            },
            onComplete: function() {
                let msg = `find loader end`;
                console.log(msg);
                Java.use("com.hexl.lessontest.TestClass").print(msg);
            }
        });
    })
}

setImmediate(main);