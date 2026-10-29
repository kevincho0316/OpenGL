{
    files = {
        "src/main.cpp"
    },
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
    depfiles = "main.o: src/main.cpp src/shader.h\
",
    depfiles_format = "gcc"
}