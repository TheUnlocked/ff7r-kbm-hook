# FF7R Improved Keyboard and Mouse Controls

View details about this mod and download it from [NexusMods](https://www.nexusmods.com/finalfantasy7remake/mods/1552).

## Building

### Dependencies

You will likely need all of the dependencies listed in [PluginTemplate](https://github.com/gottyduke/PluginTemplate?tab=readme-ov-file#-requirements), including:

- [CMake 3.26+](https://cmake.org/)
- [PowerShell](https://github.com/PowerShell/PowerShell/releases/latest)
- [Vcpkg](https://github.com/microsoft/vcpkg)
- [Visual Studio Community 2022](https://visualstudio.microsoft.com/)
    - Desktop development with C++

But view the above link for more details.

### Update submodules

If you haven't already, run `update-submodule.bat` to download the submodules that this relies upon. You can also use the `DKUtilPath` environment variable as described [here](https://github.com/gottyduke/PluginTemplate?tab=readme-ov-file#-dependencies).

### Building

Set the `FF7RGamePath` to a directory to copy the plugin DLL to that location when it finishes building. I would recommend having it copy to your NativeMods folder during development.

Run `build-msvc.bat` to build. The compiled DLL will be in `Plugin/dist` and in the directory indicated by `FF7RGamePath` if applicable.

## Contributing

I use [Ghidra](https://github.com/NationalSecurityAgency/ghidra) and [Cheat Engine](https://github.com/cheat-engine/cheat-engine) for reverse engineering.

Try to follow the existing style and project architecture as much as possible. Memory locations should be found using patterns of surrounding bytes in the binary rather than fixed offset to maximize compatibility with different executable versions and retailers.

## Credits

Based off the incredibly helpful [PluginTemplate](https://github.com/gottyduke/PluginTemplate) by [gottyduke](https://github.com/gottyduke).
