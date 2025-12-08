{
    depfiles_format = "gcc",
    depfiles = "build/.objs/paralleled_yosys/macosx/arm64/release/src/__cpp_main.cc.cc:   src/main.cc src/./verilog2hgraph/reader.hh   src/./verilog2hgraph/config.hh src/./global/debug.hh\
",
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
    files = {
        "src/main.cc"
    }
}