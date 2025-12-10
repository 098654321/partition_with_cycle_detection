{
    depfiles_format = "gcc",
    depfiles = "build/.objs/paralleled_yosys/macosx/arm64/release/src/verilog2hgraph/__cpp_config.cc.cc:   src/verilog2hgraph/config.cc src/verilog2hgraph/config.hh   src/verilog2hgraph/../global/debug.hh\
",
    files = {
        "src/verilog2hgraph/config.cc"
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
    }
}