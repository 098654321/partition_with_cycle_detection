set_languages("c++20")


-- 目标程序
target("paralleled_yosys")
    set_kind("binary")  -- 可执行程序
    set_default(true)   -- 设为默认构建目标
    
    -- 添加源文件
    add_files("src/*.cc")
    add_files("src/**/*.cpp", "src/**/*.cc")  -- 递归添加
    
    -- 头文件目录
    add_includedirs("src")

    set_targetdir("bin")
    
    -- 编译选项
    add_cxxflags("-Wall", "-Wextra", "-O2")
    add_cflags("-std=c20")
    
