# Beacon Object File Library

A library of Beacon Object Files (BOFs) for Cobalt Strike. This solution uses a modified version of [Cobalt Strike/bof-vs](https://github.com/Cobalt-Strike/bof-vs) to support multiple BOF projects, unit testing, and debug builds within a single Visual Studio solution.

## BOFs

| Project | Description |
|---------|-------------|
| EnumDeviceDrivers | Enumerates all loaded device drivers |
| FileExfiltrationUrlEncoded | Exfiltrates a file via chunked URL-encoded GET requests |
| GetSystemDirectory | Retrieves the Windows system directory path |
| Ipconfig | Displays network adapter IP, subnet mask, and gateway |
| RegistryPersistence | Installs or removes a Run key for persistence |
| TimeStomp | Copies file timestamps from one file to another |
| WhoAmI | Returns the current username |

## Build Configurations

| Configuration | Output | Purpose |
|--------------|--------|---------|
| **Release** | `<Project>.x64.o` / `.x86.o` | BOF for use with `inline-execute` in Cobalt Strike |
| **Debug** | `<Project>.exe` | Standalone executable with mocked Beacon APIs |
| **UnitTest** | `<Project>.exe` | Google Test executable with mocked functions |

## Project Structure

Each BOF project follows the same layout:

```
<ProjectName>/
  bof.cpp              # BOF source code
  Makefile             # nmake build file (used by Release builds)
  <ProjectName>.vcxproj
```

The `Core/` folder contains shared headers and the mock framework:

- `beacon.h` - Cobalt Strike Beacon API declarations
- `base/helpers.h` - `DFR` macro for dynamic function resolution
- `base/mock.h` / `mock.cpp` - Debug/test mock framework
- `utils/boflint.py` - BOF object file linter (runs automatically during Release builds)

## Unit Testing

Each BOF separates its logic into a standalone function outside of the `go()` entry point. In unit test builds, these functions are replaced with assignable function pointers, allowing tests to control return values without mocking Windows APIs directly.

```cpp
// Unit test build: function pointer that can be overridden per test
using m_MyFunction = bool(*)();
m_MyFunction MyFunction = nullptr;

// In a test:
MyFunction = []() -> bool { return false; };  // Simulate failure
```

## Creating a New BOF

A `Template/` folder is included with all the boilerplate files needed for a new BOF project.

### Steps

1. **Copy the Template folder** and rename it to your project name:
   ```
   cp -r Template/ MyNewBOF/
   ```

2. **Rename the project files** inside the new folder:
   ```
   mv MyNewBOF/Template.vcxproj        MyNewBOF/MyNewBOF.vcxproj
   mv MyNewBOF/Template.vcxproj.filters MyNewBOF/MyNewBOF.vcxproj.filters
   mv MyNewBOF/Template.vcxproj.user    MyNewBOF/MyNewBOF.vcxproj.user
   ```

3. **Replace the placeholders** across all files:

   | Placeholder | Replace With | Example |
   |-------------|-------------|---------|
   | `__PROJECT_NAME__` | Your project name | `MyNewBOF` |
   | `__PROJECT_GUID__` | A new GUID | `A1B2C3D4-E5F6-7890-ABCD-EF1234567890` |
   | `__FUNCTION_NAME__` | Your main function name | `MyFunction` |

   You can generate a GUID in PowerShell: `[guid]::NewGuid()`

4. **Add the project to the solution** in Visual Studio:
   - Open `Beacon-Object-File-Library.sln` in Visual Studio
   - Right-click the solution in Solution Explorer -> **Add** -> **Existing Project...**
   - Browse to `MyNewBOF/MyNewBOF.vcxproj` and select it
   - The project will be added with all configurations (Debug, Release, UnitTest) already set up

5. **Implement your BOF** in `bof.cpp` — the template has TODO comments marking where to add your logic.

## Dynamic Function Resolution (DFR)

BOFs cannot link against DLLs normally. Win32 API imports must be declared using the `DFR` macro so that Beacon (or a COFF loader) resolves them at runtime.

**Always declare DFR symbols at global scope inside `extern "C"`**, then `#define` an alias. Do **not** use `DFR_LOCAL` inside function bodies — MSVC produces C++ name-mangled symbols that COFF loaders cannot resolve.

```cpp
#include "..\Core\beacon.h"

extern "C" {
    DFR(ADVAPI32, GetUserNameA);
    DFR(KERNEL32, GetLastError);
}
#define GetUserNameA ADVAPI32$GetUserNameA
#define GetLastError KERNEL32$GetLastError
```

## Aggressor Scripts (CNA)

Each BOF project includes a `.cna` Aggressor Script for loading the BOF into Cobalt Strike. Load them via **Cobalt Strike > Script Manager**.

## Testing with COFFLoader

[TrustedSec/COFFLoader](https://github.com/trustedsec/COFFLoader) is included as a git submodule at `Tools/COFFLoader` for testing BOFs outside of Cobalt Strike.

### Building COFFLoader

Run the build script from the `Tools/` directory:

```
cd Tools
build_coffloader.bat
```

This produces `Tools/COFFLoader/COFFLoader64.exe`.

### Running a BOF

```
Tools\COFFLoader\COFFLoader64.exe go x64\Release\WhoAmI.x64.o
```

For BOFs that take arguments, use the included `beacon_generate.py` to pack them into hex:

```
python3 Tools\COFFLoader\beacon_generate.py
Beacon>addString C:\Windows\system32\kernel32.dll
Beacon>addString C:\Temp\test.txt
Beacon>generate
b'...'
Beacon>exit

Tools\COFFLoader\COFFLoader64.exe go x64\Release\TimeStomp.x64.o <hex_args>
```
