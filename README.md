# Examples pick up from expressif/esp-idf repos and modify to work both on platformio and sdk v5.0.2

In order to avoid installing the SDK twice we can reuse the platformio deployed version.

+ Install the deployed sdk by running:

```
PS> C:\Users\<your user>\.platformio\packages\framework-espidf\install.ps1
```
note that python.exe must be accessible from the `$Env:PATH`.

+ To load the ESP32-IDF environment in a terminal run:

```
PS> C:\Users\<your user>\.espressif\python_env\idf5.0_py3.11_env\Scripts\Activate.ps1
PS> C:\Users\<your user>\.platformio\packages\framework-espidf\export.ps1
```

## Building a project

For example the hello_world. Using CMake:

```
esp32-demos> cmake -B build/hello_world -G Ninja .\hello_world\
esp32-demos> cmake --build build/hello_world --target menuconfig
esp32-demos> cmake --build build/hello_world --target all
esp32-demos> cmake --build build/hello_world --target flash

esp32-demos> [System.IO.Ports.SerialPort]::getportnames()
    COM3
esp32-demos> $ENV:ESPPORT = "COM3"
esp32-demos> cmake --build build/hello_world --target monitor
```

Using python:

```
esp32-demos> $src = "-C hello_world", "-B build/hello_world"
esp32-demos> idf.py $src menuconfig
esp32-demos> idf.py $src all
esp32-demos> idf.py $src flash
esp32-demos> idf.py $src monitor
```
