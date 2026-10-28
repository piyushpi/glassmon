# glassmon

Minimal translucent hardware monitor widget for Linux. Shows CPU/GPU temperature, usage, and fan speed with a real-time temperature graph. Always on top, draggable frameless window.

![glassmon screenshot](screenshot.png)

## For users (no build needed)

1. Download the latest release: [releases page](https://github.com/piyushpi/glassmon/releases).
2. Extract and run:
```bash
tar -xzf glassmon-linux-x86_64.tar.gz
./glassmon
```
Requires Qt6 runtime installed.

Right-click the widget to toggle always-on-top or exit. Autostart can be set up with `./install.sh` if you cloned the repo.

## Build from source

Requires CMake 3.16+, Qt 6, and a C++20 compiler.

```bash
./build.sh
./build/glassmon
```

To install and enable autostart:
```bash
./install.sh
```

## License

AGPL-3.0
