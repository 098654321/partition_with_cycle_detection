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
    depfiles = "build/.objs/paralleled_yosys/macosx/arm64/release/src/verilog2hgraph/__cpp_reader.cc.cc:   src/verilog2hgraph/reader.cc src/verilog2hgraph/reader.hh   src/verilog2hgraph/config.hh src/verilog2hgraph/json.h   src/verilog2hgraph/../global/debug.hh\
",
    files = {
        "src/verilog2hgraph/reader.cc"
    },
    depfiles_format = "gcc"
}