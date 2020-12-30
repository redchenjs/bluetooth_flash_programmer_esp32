set(COMPONENT_SRCDIRS port src)
set(COMPONENT_ADD_INCLUDEDIRS inc)

set(COMPONENT_REQUIRES main)

register_component()

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-format)
