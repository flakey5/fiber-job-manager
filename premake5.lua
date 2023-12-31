workspace 'fiber-jobs-manager'
    cppdialect 'c++20'
    location '.pmk'
    objdir '.obj'
    targetdir '.bin'

    platforms { 'Win32', 'x64' }
    configurations { 'Debug', 'Release' }

    project 'fiber-jobs-manager'
        kind 'ConsoleApp'
        language 'c++'

        files {
            'src/**.cpp',
            'src/**.hpp',
            'include/**.hpp',
            'premake5.lua',
            'README.md'
        }

        includedirs {
            'include'
        }

        filter { 'system:windows' }
            defines { 'PLATFORM_WINDOWS' }
        filter {}
