idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS ""
    PRIV_REQUIRES input_module esp_driver_gpio 
)


#[[ Minimal Component CMakeLists
Specify the source files to be compiled together as a component (static lib).
Specify where to look for includes.
Specify component dependencies.

The component name is most likely the name of the directory where this
CMake file exists. So in this case "main". ]]

#[[ Code and Data Placements
ESP-IDF has a feature called linker script generation that enables components
to define where its code and data will be placed in memory through linker
fragment files. These files are processed by the build system, and is used to
augment the linker script used for linking app binary. ]]

#[[ Embedding Binary Data
    idf_component_register(...
                        EMBED_FILES server_root_cert.der)
    idf_component_register(...
                        EMBED_TXTFILES server_root_cert_pem)

The file's contents will be added to the .rodata section in flash, and
are available via symbol names as follows:

    extern const uint8_t server_root_cert_pem_start[] 
        asm("_binary_server_root_cert_pem_start");
    extern const uint8_t server_root_cert_pem_end[]
        asm("_binary_server_root_cert_pem_end");

The names are generated form the full name of the file, as given in 
EMBED_FILES. Characters /, ., etc. are replaced with underscores. The _binary
prefix in the symbol name is added by objcopy and is the same for both text
and binary files.

To embed a file into a project, rather than a component, you can call the
function target_add_binary_data like this:

    target_add_binary_data(myproject.elf "main/data.bin" TEXT)

Place this line after the project() line in your project CMakeLists.txt file.
Replace myproject.elf with your project name. The final argument can be TEXT
to embed a null-terminated string, or BINARY to embed the content as-is.

It is also possible to embed a generated file:
    
    add_custom_command(OUTPUT my_processed_file.bin
                        COMMAND my_process_file_cmd my_unprocessed_file.bin)
    target_add_binary_data(my_target "my_processed_file.bin" BINARY)

]]


#[[ Preset Component Variables
The following component-specific variables are available for use inside
component CMakeLists, but should not be modified:

-   COMPONENT_DIR: The component directory. Evaluates to the absolute path
    of the directory containing CMakeLists.txt. The component path cannot
    contain spaces. This is the same as the CMAKE_CURRENT_SOURCE_DIR variable.

-   COMPONENT_NAME: Name of the component. Same as the name of the component
    directory.

-   COMPONENT_ALIAS: Alias of the library created internally by the build
    system for the component.

-   COMPONENT_LIB: Name of the lirbary created internally by the build system
    for the component.

-   COMPONENT_VERSION: Component version specified by idf_component.yml and
    set by IDF Component Manager.

The following variables are set at the project level, but available for use
in component CMakeLists:

-   CONFIG_*: Each value in the project configuration has a corresponding
    variable available in cmake. All names begin with CONFIG_.

-   ESP_PLATFORM: Set to 1 when the CMake file is processed within the ESP-IDF
    build system. ]]

#[[ Build/Project Variables
The following are some project/build variables that are available as build
properties and whose values can be queried using idf_build_get_property
from the component CMakeLists.txt:

-   PROJECT_NAME: Name of the project, as set in project CMakeLists.txt file.

-   PROJECT_DIR: Absolute path of the project directory containing the project
    CMakeLists. Same as the CMAKE_SOURCE_DIR variable.

-   COMPONENTS: Names of all components that are included in this build,
    formatted as a semicolon-delimited CMake list.

-   IDF_VER: Git version of ESP-IDF (produced by git describe)

-   IDF_VERSION_MAJOR, IDF_VERSION_MINOR, IDF_VERSION_PATCH: Components of
    ESP-IDF version, to be used in conditional expressions. Note that this
    information is less precise than that provided by IDF_VER variable.
    v3.0-dev-*, v4.0-beta1, v4.0-rc1 and v4.0 will all have the same values
    of IDF_VERSION_* variables, but different IDF_VER values.

-   IDF_TARGET: Name of the target for which the project is being built.

-   PROJECT_VER: Project version. (various ways to set this, see docs.)

-   EXTRA_PARTITION_SUBTYPES: CMake list of extra partition subtypes. Each
    subtype description is a comma-separated string with type_name,
    subtype_name, numeric_value format. Components may add new subtypes by
    appending them to this list. ]]

#[[ Controlling Component Compilation
To pass compiler options when compiling source files belonging to a particular
component, use the target_compile_options function:

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-unused-variable)

To apply the compilation flags to a single source file, use the CMake
set_source_files_properties command:

set_source_files_properties(mysrc.c
    PROPERTIES COMPILE_FLAGS
    -Wno-unused-variable )

When using these commands, place them after the call to idf_component_register
in the component CMakeLists file. ]]

#[[ Component Configuration
Each component can also have a Kconfig file, alongside CMakeLists.txt. This
contains configuration settings to add to the configuration menu for this
component.

These settings are found under the "Component Settings" menu when menuconfig
is rune.

To create a component Kconfig file, it is easiest to start with one of the
Kconfig files distributed with ESP-IDF. ]]

#[[ Preprocessor Definitions
The ESP-IDF build system adds the following C preprocessor definitions on the
command line.

-   ESP_PLATFORM: Can be used to detect that build happens within ESP-IDF.

-   IDF_VER: Defined to a git version string.

]]

#[[ Component Requirements
When compiling each component, the ESP-IDF build system recursively evaluates
its dependencies. This means each component needs to declare the components
that it depends on ("requires").

When Writing a Component
idf_component_register(...
                        REQUIRES mbedtls
                        PRIV_REQUIRES console spiffs)

-   REQUIRES should be set to all components whose header files are #included
    from the public header files of this component.

-   PRIV_REQUIRES should be set to all components whose header files are
    #included from any source files in this component, unless already listed
    in REQUIRES. Also, any component which is required to be linked in order
    for this component to function correctly.

-   The values of REQUIRES and PRIV_REQUIRES should not depend on any
    configuration options (CONFIG_xxx macros). This is because requirements
    are expanded before the configuration is loaded. Other component variables
    (like include paths or source files) can depend on configuration options.

-   Not setting either or both REQUIRES variables is fine. If the component
    has no requirements except for the Common component requirements needed
    for RTOS, libc, etc.

If a component only supports some target chips (values of IDF_TARGET) then
it can specify REQUIRED_IDF_TARGETS in the idf_component_register call to
express these requirements. In this case, the build system will generate
an error if the component is included in the build, but does not support
the selected target. ]]
