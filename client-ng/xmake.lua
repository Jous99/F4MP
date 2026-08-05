-- CommonLibF4 (submodulo en lib/commonlibf4)
includes("lib/commonlibf4")

set_project("F4MP-NG")
set_version("0.1.0")
set_languages("c++23")
set_warnings("allextra")

add_rules("mode.debug", "mode.releasedbg")
add_rules("plugin.vsxmake.autoupdate")

-- Dependencia de red
add_requires("gamenetworkingsockets")

target("F4MPClient")
    add_packages("gamenetworkingsockets")
    -- Esta regla genera automaticamente la version del plugin de F4SE
    -- (adios al "incompatible") y despliega el DLL al juego.
    add_rules("commonlibf4.plugin", {
        name = "F4MP",
        author = "Jous",
        description = "Fallout 4 Multiplayer"
    })

    add_files("src/**.cpp")
    add_headerfiles("src/**.h")
    add_includedirs("src")
    set_pcxxheader("src/pch.h")
