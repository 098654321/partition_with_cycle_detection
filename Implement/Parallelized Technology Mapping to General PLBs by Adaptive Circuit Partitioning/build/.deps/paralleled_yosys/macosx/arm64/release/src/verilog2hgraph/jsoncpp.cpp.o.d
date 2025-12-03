{
    files = {
        "src/verilog2hgraph/jsoncpp.cpp"
    },
    depfiles_format = "gcc",
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
    depfiles = "build/.objs/paralleled_yosys/macosx/arm64/release/src/verilog2hgraph/__cpp_jsoncpp.cpp.cpp:   src/verilog2hgraph/jsoncpp.cpp src/verilog2hgraph/json.h\
"
}