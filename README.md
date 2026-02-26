# UnityAspectLock

This repository contains a small Win32 DLL that can be used inside a Unity project to lock
the game window to a fixed aspect ratio (e.g. 16:9). The DLL subclasses the main Unity
window and intercepts `WM_SIZING` messages to enforce the desired ratio.

## Building

The code is pure C++ and requires a Windows toolchain. It can be built using CMake:

```sh
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64       # or your VS version/architecture
cmake --build . --config Release
```

On Windows you can use the Visual Studio generator (`-G "Visual Studio 17 2022" -A x64`)
or the `MinGW Makefiles` generator if you have MinGW installed. On macOS/Linux you will need
a cross-compiler targeting Windows (e.g. `mingw-w64`) and may specify \
`-DCMAKE_SYSTEM_NAME=Windows` along with the compiler paths.

The resulting DLL (`UnityAspectLock.dll`) should be copied into your Unity project under
`Assets/Plugins/x86_64/` (or `x86/` for 32-bit builds).

## Usage

Call the exported functions from C# via P/Invoke:

```csharp
[DllImport("UnityAspectLock")]
private static extern int UnityAspectLock_Install(float aspectWidth, float aspectHeight);

[DllImport("UnityAspectLock")]
private static extern void UnityAspectLock_Uninstall();

[DllImport("UnityAspectLock")]
private static extern int UnityAspectLock_IsInstalled();
```

Then install with the desired aspect ratio:

```csharp
if(UnityAspectLock_Install(16,9) != 0) {
    Debug.Log("Aspect lock enabled");
}
```
