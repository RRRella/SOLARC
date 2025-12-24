# Building Solarc Engine

## Renderer Backend Selection

Solarc supports multiple graphics backends. You select the backend at CMake configuration time.

### Windows

You can build with either DirectX 12 or Vulkan:
```bash
# Build with DirectX 12 (default on Windows)
cmake -B build -DSOLARC_RENDERER=DX12
cmake --build build

# Build with Vulkan
cmake -B build -DSOLARC_RENDERER=Vulkan
cmake --build build
```

**Output binaries:**
- `build/out/Debug/bin/solarc.exe` (or `solarc-dx12.exe` if you rename it)

### Linux

Only Vulkan is supported on Linux:
```bash
# Build with Vulkan (default and only option on Linux)
cmake -B build -DSOLARC_RENDERER=Vulkan
cmake --build build
```

**Requirements:**
- Vulkan SDK must be installed
- Wayland development libraries

### Binary Naming Convention (Optional)

If you want distinct binary names for each backend, modify `Code/Solarc/CMakeLists.txt`:
```cmake
# After add_executable(${PROJECT_NAME} ...)
if(SOLARC_RENDERER STREQUAL "DX12")
    set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "solarc-dx12")
elseif(SOLARC_RENDERER STREQUAL "Vulkan")
    set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "solarc-vulkan")
endif()
```

### Verifying Build Configuration

CMake will print the selected backend during configuration:
```
-- ==============================================
-- Solarc Engine Build Configuration
-- ==============================================
-- Platform: Windows
-- Renderer: DX12
-- Build Type: Debug
-- ==============================================
```

### Switching Backends

To switch backends, reconfigure CMake:
```bash
# Switch from DX12 to Vulkan
cmake -B build -DSOLARC_RENDERER=Vulkan

# Or delete build directory and start fresh
rm -rf build
cmake -B build -DSOLARC_RENDERER=Vulkan
```

## Common Issues

### "Vulkan SDK not found"
**Solution:** Install Vulkan SDK from https://vulkan.lunarg.com/

### "DirectX 12 is not supported on Linux"
**Solution:** Use `-DSOLARC_RENDERER=Vulkan` on Linux

### Link errors with d3d12.lib
**Solution:** Ensure you have Windows SDK installed (comes with Visual Studio)