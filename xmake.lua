--xmake f -p linux -c -y && xmake run              # Linux (Termux에서도 가능)
--xmake f -p windows -c -y && xmake run            # Windows
--xmake f -p android -a arm64-v8a --ndk=$ANDROID_NDK --sdk=$ANDROID_SDK_ROOT --ndk_sdkver=24 -c -y && xmake

add_rules("mode.debug", "mode.release")
set_languages("c++17")

add_requires(
    'lua 5.4.x'
)
local termux_inc = "/data/data/com.termux/files/usr/include"

if is_plat("android") then
    if not os.isdir(termux_inc .. "/glm") then add_requires("glm") end
else
    add_requires("glfw", "glm")
    add_requires("glad", {configs = {profile = "core"}})
end

target("app")
    add_files("src/core/*.cpp")
    add_includedirs("src")
    add_packages('lua')
    add_includedirs("third_party/sol2/include")
    
    if is_plat("android") then
        local ndk  = os.getenv("ANDROID_NDK") or ""
        local glue = path.join(ndk, "sources/android/native_app_glue")
        set_kind("shared")
        set_basename("main")
        add_files("src/platform/android/*.cpp")
        add_files(path.join(glue, "android_native_app_glue.c"))
        add_includedirs(glue)
        if os.isdir(termux_inc .. "/glm") then add_includedirs(termux_inc) else add_packages("glm") end
        add_defines("USE_GLES")
        add_syslinks("android", "EGL", "GLESv3", "log")
        add_ldflags("-u ANativeActivity_onCreate", {force = true})
        add_ldflags("-Wl,--no-undefined", {force = true})
    else
        set_kind("binary")
        set_rundir("$(projectdir)")
        add_files("src/platform/desktop/*.cpp")
        add_packages("glfw", "glm", "glad")
        if is_plat("windows") then
            add_links("opengl32")
        elseif is_plat("linux") then
            add_links("GL")
        elseif is_plat("macosx") then
            add_frameworks("OpenGL")
        end
    end
