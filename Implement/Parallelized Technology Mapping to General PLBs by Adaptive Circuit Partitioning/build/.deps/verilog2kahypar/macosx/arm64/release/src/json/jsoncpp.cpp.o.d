{
    depfiles_format = "gcc",
    files = {
        "src/json/jsoncpp.cpp"
    },
    values = {
        "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang++",
        {
            "-Qunused-arguments",
            "-target",
            "arm64-apple-macos26.1",
            "-isysroot",
            "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX26.1.sdk",
            "-std=c++20",
            "-Isrc",
            "-Wall",
            "-Wextra",
            "-O2"
        }
    },
    depfiles = "build/.objs/verilog2kahypar/macosx/arm64/release/src/json/__cpp_jsoncpp.cpp.cpp:   src/json/jsoncpp.cpp src/json/json.h\
"
}