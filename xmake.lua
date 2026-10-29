add_rules("mode.debug", "mode.release")

add_requires("glad", {configs = {
    profile = "core",
    extensions = "GL_ARB_multi_bind,GL_ARB_multi_draw_indirect"
}})

add_requires("glfw")
add_requires("glm")

target("OpenGl")
    set_kind("binary")
    set_languages("cxx17")
    set_rundir("$(projectdir)")

    add_files("src/*.cpp")

    add_packages("glad", "glfw", "glm")

    if is_plat("windows") then
        add_links("opengl32")
    elseif is_plat("linux") then
        add_links("GL")
    elseif is_plat("macosx") then
        add_frameworks("OpenGL")
    end
