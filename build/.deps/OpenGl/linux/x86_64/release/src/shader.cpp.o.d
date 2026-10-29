{
    values = {
        "/usr/bin/g++",
        {
            "-m64",
            "-fvisibility=hidden",
            "-fvisibility-inlines-hidden",
            "-O3",
            "-std=c++17",
            "-isystem",
            "/home/jjj/.xmake/packages/g/glad/v2.0.8/d5fd379e748349b4856fd6c5b7bed689/include",
            "-DNDEBUG"
        }
    },
    depfiles = "shader.o: src/shader.cpp src/shader.h\
",
    files = {
        "src/shader.cpp"
    },
    depfiles_format = "gcc"
}