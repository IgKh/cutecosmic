# CuteCosmic (WIP)

Plugins to the Qt toolkit that help make Qt (and KDE) applications look and feel more at home in the COSMIC™ Desktop environment.

Currently consists of a Qt Platform Theme plugin.

> [!IMPORTANT]
> CuteCosmic is work in progress and is very experimental. Use at your own risk.

> [!NOTE]
> This is an unofficial project and is not in any way affiliated with or endorsed by System76, Inc.

## Features

The following configuration is relayed from COSMIC settings to Qt applications:

- [x] Dark mode
- [x] Per-application dark mode override
- [x] High contrast mode (Qt 6.10+)
- [x] File dialogs
- [x] Icon theme
  - Including symbolic icon re-coloring to avoid dark-on-dark icons
- [x] Fonts
- [x] Color palette[^1]

[^1]: Requires enabling the toolkit theming option in COSMIC settings. Most KDE applications require a restart after theme change.

## Installation

CuteCosmic must currently be built from source. To do so, you'll need a C++ compiler, the most recent Rust stable compiler, CMake and development files (headers, libraries and tools) for Qt 6.

The project aims to support only the last three released minor versions of Qt, as well as the most recent Qt 6 LTS series (if it is not one of the three). Currently this means Qt 6.8, 6.9 and 6.10.

> [!IMPORTANT]
> CuteCosmic must be built and installed separately for each installation of Qt you have. Some applications (like Qt Creator) ship with their own installation of Qt, in addition to the system wide installation.

> [!IMPORTANT]
> CuteCosmic makes extensive use of private Qt APIs that are outside the scope of Qt's normal compatibility guarantees. If you update a Qt installation for which CuteCosmic was built (including patch releases), it **MUST** be re-built to function properly.

To build and install use a regular CMake invocation like this:

```bash
  cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
  cmake --build build
  cmake --build build -t install
```

Add `sudo` to the install command if building against a system-wide Qt installation. For building against a specific Qt installation, use the path to its specific `qt-cmake` wrapper script instead of `cmake`.

## Usage

If installed correctly, CuteCosmic will automatically be loaded and used when working from inside a `cosmic-session`.

You can force its' usage with the `QT_QPA_PLATFORMTHEME` environment variable, e.g:
```bash
QT_QPA_PLATFORMTHEME=cosmic /path/to/a/qt/app
```

If not working, troubleshoot by setting the `QT_DEBUG_PLUGINS` environment variable and watch the log traces to see if `libcutecosmictheme.so` is available and loaded.

## Contributing

Issue reports and code contributions are gratefully accepted. Please do not send unsolicited Pull Requests, please first propose patch ideas and plans in the relevant issue (or open an issue if one doesn't already exists).

## License

Copyright 2025 Igor Khanin.

Made available under the [GPL v3](https://choosealicense.com/licenses/gpl-3.0/) license.
