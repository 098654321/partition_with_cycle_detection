set_languages("c++20")


-- 目标程序
target("verilog2kahypar")
    set_kind("binary")  -- 可执行程序
    set_default(true)   -- 设为默认构建目标
    
    -- 添加源文件
    add_files("src/verilog2kahypar/*.cc")
    add_files("src/json/jsoncpp.cpp")
    
    -- 头文件目录
    add_includedirs("src")

    set_targetdir("bin")
    
    -- 编译选项
    add_cxxflags("-Wall", "-Wextra", "-O2")
    add_cflags("-std=c20")

target("partition2verilog")
    set_kind("binary")  -- 可执行程序
    set_default(false)   -- 设为默认构建目标
    
    -- 添加源文件
    add_files("src/partition2verilog/*.cc")
    add_files("src/json/jsoncpp.cpp")
    
    -- 头文件目录
    add_includedirs("src")

    set_targetdir("bin")
    
    -- 编译选项
    add_cxxflags("-Wall", "-Wextra", "-O2")
    add_cflags("-std=c20")

    
