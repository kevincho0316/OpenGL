{
    files = {
        [[build\.objs\OpenGl\windows\x64\release\src\main.cpp.obj]]
    },
    values = {
        [[C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Tools\MSVC\14.50.35717\bin\HostX64\x64\link.exe]],
        {
            "-nologo",
            "-dynamicbase",
            "-nxcompat",
            "-machine:x64",
            [[-libpath:C:\Users\kevin\AppData\Local\.xmake\packages\g\glad\v0.1.36\e8a7d85f737849599dafae527be6b4c2\lib]],
            [[-libpath:C:\Users\kevin\AppData\Local\.xmake\packages\g\glfw\3.4\d59f6dd71e7647cc95d2c1d35e51a39a\lib]],
            "/opt:ref",
            "/opt:icf",
            "glad.lib",
            "glfw3.lib",
            "opengl32.lib",
            "user32.lib",
            "shell32.lib",
            "gdi32.lib"
        }
    }
}