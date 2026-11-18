# AssaultCube Research

A small C++ reverse-engineering experiment exploring AssaultCube's client data and rendering path.

## What I explored

- Reading player and entity state through a small SDK.
- Drawing an ImGui overlay through an OpenGL render hook.
- Projecting world positions into screen-space for entity visualisation.
- Basic aim-adjustment experiments alongside the overlay work.

This project is a version-specific prototype. It modifies the running game process and includes ESP and aim-assist behaviour; it is not suitable for online multiplayer or a supported game modification.

## Project layout

- `ACI/` contains the Windows launcher.
- `ACIDLL/` contains the injected DLL and overlay prototype.

The Visual Studio solution is provided for Windows development. The source is here to document the reverse-engineering work, not to provide a maintained release.

## Defensive analysis

See [DEFENSIVE_ANALYSIS.md](DEFENSIVE_ANALYSIS.md) for a threat model and design considerations based on the client-state, overlay, and input experiments in this repository. It does not claim to implement an anti-cheat.
