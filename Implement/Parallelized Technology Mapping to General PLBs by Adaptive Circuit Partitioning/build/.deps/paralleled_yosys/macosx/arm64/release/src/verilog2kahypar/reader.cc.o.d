{
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
    depfiles_format = "gcc",
    depfiles = "build/.objs/paralleled_yosys/macosx/arm64/release/src/verilog2kahypar/reader.cc.o:   src/verilog2kahypar/reader.cc src/verilog2kahypar/reader.hh   src/verilog2kahypar/config.hh src/verilog2kahypar/json.h   src/verilog2kahypar/../global/debug.hh\
",
    files = {
        "src/verilog2kahypar/reader.cc"
    }
}