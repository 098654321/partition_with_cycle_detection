{
    depfiles = "build/.objs/partition2verilog/macosx/arm64/release/src/partition2verilog/__cpp_writer.cc.cc:   src/partition2verilog/writer.cc src/partition2verilog/../json/json.h   src/partition2verilog/writer.hh   src/partition2verilog/../global/debug.hh\
",
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
    files = {
        "src/partition2verilog/writer.cc"
    }
}