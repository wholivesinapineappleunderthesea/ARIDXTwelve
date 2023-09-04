cmake_minimum_required (VERSION 3.24)

project ("aridxtwelve")

# source files
add_executable (aridxtwelve "src/engine/main.cc" "src/engine/renderer/D3DRI.cc" "src/engine/Log.cc" "src/engine/Memory.cc" "src/Windows/MinCRT.cc" "src/engine/renderer/D3DRIResource.cc" "src/engine/renderer/D3DRIResourceManager.cc" "src/engine/renderer/D3DRIRenderFrameContext.cc" "src/engine/World/World.cc" "src/engine/World/World1.cc" "src/engine/Entity/Entity.cc" "src/engine/Entity/Component/StaticMeshD3DRIRendererComponent.cc" "src/engine/Entity/Component/BasePhysicsComponent.cc" "src/engine/Entity/Component/BoundingBoxComponent.cc" "src/engine/Entity/Component/CameraComponent.cc")

# cmake compiler options
set_property(TARGET aridxtwelve PROPERTY CXX_STANDARD 23)
set_property(TARGET aridxtwelve PROPERTY INTERPROCEDURAL_OPTIMIZATION)


# safety warnings etc
target_compile_options(aridxtwelve PRIVATE /W4 /WX /permissive- /w14640 /w14242 /w14254 /w14263 /w14265 /w14287 /we4289 /w14296 /w14311 /w14545 /w14546 /w14547 /w14549 /w14555 /w14619 /w14640 /w14826 /w14905 /w14906 /w14928)

# disable C4996
target_compile_definitions(aridxtwelve PRIVATE _CRT_SECURE_NO_WARNINGS)

# msvc disable exceptions
target_compile_options(aridxtwelve PRIVATE /EHs-)
target_compile_options(aridxtwelve PRIVATE /EHc-)
target_compile_definitions(aridxtwelve PRIVATE _HAS_EXCEPTIONS=0)

# msvc disable rtti
target_compile_options(aridxtwelve PRIVATE /GR-)

# disable security cookie
target_compile_options(aridxtwelve PRIVATE /GS-)

# linker options
target_link_libraries(aridxtwelve PRIVATE "kernel32")
target_link_libraries(aridxtwelve PRIVATE "d3d12")
target_link_libraries(aridxtwelve PRIVATE "dxgi")
target_link_libraries(aridxtwelve PRIVATE "d3dcompiler")

# linker options
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS")


# resources
function(embed_resource resource_file_name source_file_name variable_name)
    if(EXISTS "${source_file_name}")
        if("${source_file_name}" IS_NEWER_THAN "${resource_file_name}")
            return()
        endif()
    endif()
    file(READ "${resource_file_name}" hex_content HEX)
    string(REPEAT "[0-9a-f]" 32 pattern)
    string(REGEX REPLACE "(${pattern})" "\\1\n" content "${hex_content}")
    string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1, " content "${content}")
    string(REGEX REPLACE ", $" "" content "${content}")
    set(array_definition "static constexpr unsigned char ${variable_name}[]{\n${content}\n};")
    file(WRITE "${source_file_name}" "${array_definition}")
endfunction()

# embed shaders
embed_resource("resources/shaders/Primary3D.vs.hlsl" "resources/embed/Primary3DVS.h" "Primary3DVS")
embed_resource("resources/shaders/Primary3D.ps.hlsl" "resources/embed/Primary3DPS.h" "Primary3DPS")

# embed meshes
embed_resource("resources/meshes/World1.bin" "resources/embed/World1.h" "World1Mesh")

# combine all resource headers into one
file(GLOB_RECURSE resource_headers "resources/embed/*.h")
file(WRITE "src/embed/resources.h" "#pragma once\n\n")
foreach(resource_header ${resource_headers})
	file(READ "${resource_header}" content)
	file(APPEND "src/embed/resources.h" "${content}\n")
endforeach()