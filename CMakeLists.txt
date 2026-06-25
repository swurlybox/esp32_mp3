cmake_minimum_required(VERSION 3.22)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
idf_build_set_property(MINIMAL_BUILD ON)
project(app)

#[[ Minimum cmake version required for pretty much every CMakeFile. ]]

#[[ Includes ESP-IDF's own cmake file. Whole lotta slop but probably necessary
for an ESP-IDF project. Configures the project, discover all the components
etc. (ESP-IDF docs) ]]

#[[ Essentially the name of our application/project. Creates the project itself,
and specifies the project name. The project name is used for the final
binary output files of the app, i.e. iot_mp3.elf, iot_mp3.bin. Only one
project can be defined per CMakeLists file. ]]

#[[ Use this to override some default build properties. ]]

#[[ There are optional project variables, which have default values that can
be overwritten for custom behavior.

-   COMPONENT_DIRS: Directories to search for components. Defaults to
    IDF_PATH/components, PROJECT_DIR/components, and EXTRA_COMPONENT_DIRS.
    Override this variable if you don't want to search for components in these
    places.

-   EXTRA_COMPONENT_DIRS: Optional list of additional directories to search
    for components. Paths can be relative to the project directory, or
    absolute.

-   COMPONENTS: A list of component names to build into the project. Defaults
    to all components found in the COMPONENT_DIRS directories. Use this
    variable to "trim down" the project for faster build times. Note that
    any component which "requires" another component via the REQUIRES or
    PRIV_REQUIRES arguments on component registration will automatically
    have it added to this list, so the COMPONENTS list can be very short. The
    MINIMAL_BUILD build property can be used as an alternative to specifying
    only the main component in COMPONENTS.

    (basically use this to specify the components you want, and not have
    ESP-IDF include every possible component as part of the build.)

-   BOOTLOADER_IGNORE_EXTRA_COMPONENT: Optional list of components, placed in
    bootloader_components/, that should be ignored by the bootloader
    compilation. Use this variable if a bootloader component needs to be
    included conditionally inside the project.

-   BOOTLOADER_EXTRA_COMPONENT_DIRS: Optional list of additional directories
    to search for components to be compiled as part of the bootloader. Please
    note that this is a build property.

To set these variables, use the cmake set command ie set(VARIABLE "VALUE").
The set() commands should be placed after the cmake_minimum(...) line but
before the include(...) line. ]]

#[[ Renaming main Component
The build system provides special treatment to the main component. It is a
component that gets automatically added to the build provided that it is in
the expected location, PROJECT_DIR/main. All other components in the build
are also added as its dependencies, saving the user from hunting down
dependencies and providing a build that works right out of the box.
Renaming the main component causes the loss of these behind-the-scenes
heavy lifting, requiring the user to specify the location of the newly renamed
component and manually specify its dependencies. Specifically, the steps to
renaming main are as follows:

1. Rename main directory
2. Set EXTRA_COMPONENT_DIRS in the project CMakeLists.txt to include the
    renamed main directory.
3. Specify the dependencies in the renamed component's CMakeLists.txt file via
    REQUIRES or PRIV_REQUIRES arguments on component registration. ]]


