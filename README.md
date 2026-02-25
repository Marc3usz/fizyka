# Fizyka - Gravity Simulator

A minimalistic 2D N-body gravity simulator written in C using [Raylib](https://www.raylib.com/). It focuses on accurate orbital mechanics using real-world SI units and custom arena-based memory management.

## Features

- **Real-scale Simulation**: Uses real masses (kg) and stores world space as integer millimeters (`__int128`) for high-precision distances.
- **Solar System Seed**: Includes Sun, 8 planets, and major moons initialized with elliptical orbits.
- **Custom Tech Stack**:
  - Arena allocator for predictable memory usage.
  - Custom dynamic arrays.
  - `__int128` world coordinates and velocities for large-range, 1 mm precision simulation.
- **Tools**:
  - Dynamic orbital trails.
  - Waypoints system with distance measurement lines.
  - Smart label culling (prioritizes larger bodies).
  - Variable time scale (speed up/slow down time).

## Building

The project uses a standard Makefile and targets Windows (MinGW/GCC).

1. Ensure you have `gcc` and `make` installed (e.g., via MinGW-w64).
2. Run the build command:
   ```sh
   make
   ```

_Note: Raylib headers and libraries are included in `./include` and `./lib`._

## Running

1. Ensure `raylib.dll` is in the same directory as the executable (or in your PATH).
2. Run the simulation:
   ```ps1
   .\fizyka.exe
   ```

## Controls

| Action                   | Key / Mouse       |
| :----------------------- | :---------------- |
| **Pan Camera**           | Middle Mouse Drag |
| **Zoom**                 | Mouse Wheel       |
| **Pause / Resume**       | `Space`           |
| **Simulate Single Step** | `N` (when paused) |
| **Increase Speed**       | `+` / `Numpad +`  |
| **Decrease Speed**       | `-` / `Numpad -`  |
| **Toggle Timer**         | `T`               |
| **Reset Timer**          | `R`               |

### Measurement Tools

- **Add Waypoint**: `W` (at mouse cursor)
- **Remove Waypoint**: `E` (hover over waypoint)
- **Draw Distance Line**: Left Click first point -> Left Click second point
- **Remove Line**: Right Click on line
