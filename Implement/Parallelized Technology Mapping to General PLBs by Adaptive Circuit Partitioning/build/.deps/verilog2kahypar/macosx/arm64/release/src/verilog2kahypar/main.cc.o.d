{
    files = {
        "src/verilog2kahypar/main.cc"
    },
    depfiles = "build/.objs/verilog2kahypar/macosx/arm64/release/src/verilog2kahypar/__cpp_main.cc.cc:   src/verilog2kahypar/main.cc src/verilog2kahypar/./reader.hh   src/verilog2kahypar/./config.hh src/verilog2kahypar/../global/debug.hh\
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
    depfiles_format = "gcc"
}