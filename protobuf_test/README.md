
# Basic Protobuf Example

This example shows:
+ how to use the protbuf-c library component.
+ how to integrate the proxy-stub generation associated to the .proto files within the project:
  - At generation time. Inconvenient but the only option the platformio & ESP-IDF cmake model constraints allow.
  - At build time. Using a [toolchain approach](https://docs.espressif.com/projects/esp-idf/en/v5.1.1/esp32/api-guides/build-system.html#using-esp-idf-in-custom-cmake-projects) possible in the ESP-IDF framework but not on platformio.

Basically:
    + the example populates with random data the structure defined in gps.proto 
    + serializes this structure into a buffer using protobuf.
    + deserializes the buffer payload into a new structure.

## How to use example

### Dependencies

In order to build the proxy-stub is necesary to have a local installation of [protobuf](https://github.com/protocolbuffers/protobuf) and [protobuf-c](https://github.com/protobuf-c/protobuf-c):
In Ubuntu via apt packages:
```bash
sudo apt-get install -y protobuf-compiler libprotobuf-dev libprotoc-dev
```
On Windows downloading the binaries from [here](https://github.com/MiguelBarro/protobuf-c/actions/runs/6196616447).
Favour the version using static libraries for simplicity. 

### Serialization Code generation

The basic procedure to generate the proxy-stub is callig the generator `protoc` with the plugin `protoc-gen-c`. In order
to work the protoc must be able to find the plugin in a path from the PATH environmental variable. An example call:
```bash
protoc.exe --cpp_out <DESTINATION_PATH> -I<protobuf bin dir>/include -I<.proto files dir> <proto files>
```
Here we need to include the own protobuf headers because the ubuntu version won't work otherwise.

CMake will take care of running the above generation code. In order to do so it must be able to find both the
`protobuf` and `protobuf-c` binaries. CMake resources to use:
```cmake
find_package(protobuf CONFIG) 
```
in order to make them available:
+ On Ubuntu success is guaranteed because the apt-packages will deployed to the system binary dirs.
+ On Windows hinting is necessary, two flavours:
  - call cmake specifing
    [*<package>_ROOT*](https://cmake.org/cmake/help/latest/variable/PackageName_ROOT.html#variable:%3CPackageName%3E_ROOT)
    CMake variables or environment variables.
  - using [CMake package registry](https://cmake.org/cmake/help/book/mastering-cmake/chapter/Finding%20Packages.html#cmake-package-registry).
    Platformio only will work with the environment variables or registry approaches.

Note that the cmake `FindProtobuf.cmake` module defines a function `protobuf_generate_cpp()` which cannot be used here
due to the use of the plugin for c source generation. Though, there is an undocumented function, which the aforementioned
relies on, that can be used `protobuf_generate()`. Unfortunately both rely on the `add_custom_command()` that cannot be
used with the ESP-IDF simplified CMake framework.

### Project generation examples
+ From CMake (using toolchain approach). The `.proto` files can be modified without regenerate the cmake binary dir:
```pwsh
esp32-demos> cmake -DCMAKE_BUILD_TYPE=Release `
                   -Dprotobuf_ROOT=/temp/install/protobuf `
                   -Dprotobuf-c_ROOT=/temp/install/protobuf-c `
                   -B build/proto -G Ninja protobuf_test
esp32-demos> cmake --build build/proto --target all
````

+ From Visual Studio Code:
```pwsh
> $Env:PLATFORMIO_IDE=TRUE
> ${Env:protobuf-c_ROOT}="/temp/install/protobuf-c"
> $Env:protobuf_ROOT="/temp/install/protobuf"
> code <whatever>/esp32-demos/protobuf_test
```

+ From platformio. We can use directly the [platformio CLI](https://docs.platformio.org/en/stable/core/userguide/index.html#usage).
We must define a PLATFORMIO_IDE environment variable (as vscode plugin does).
```pwsh
> $Env:PLATFORMIO_IDE="TRUE"
> ${Env:protobuf-c_ROOT}="/temp/install/protobuf-c"
> $Env:protobuf_ROOT="/temp/install/protobuf"
> ~/.platformio/penv/Scripts/pio run -d <whatever>/esp32-demos/protobuf_test
```

### Example Structure

Note that the .proto files are introduced in a specific *proto* folder.

    esp32-demos\protobuf_test
    |   CMakeLists.txt
    |   CMakeToolchain.cmake
    |   platformio.ini
    |   README.md
    |   sdkconfig
    |   sdkconfig.defaults
    |   sdkconfig.esp32dev
    |   version.txt
    |   
    +---proto
    |       gps.proto
    |       
    \---src
            CMakeLists.txt
            protobuf_example_main.cpp
        
