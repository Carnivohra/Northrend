# Northrend

Northrend is a modern reimplementation of a World of Warcraft game engine, designed with a strong focus on performance, modularity, and a lightweight runtime.

The project provides reusable engine systems for building compatible game clients while keeping game-specific behavior, content, and assets outside the engine. Northrend is developed as a library and is intended to be integrated into a separate game project. It does not provide a standalone executable or a playable game by itself.

## Design Goals

- **Performance** — Efficient systems and compile-time composition keep runtime overhead low.
- **Modularity** — Platform backends, renderers, and game-specific components remain independently selectable.
- **Lightweight architecture** — Games include only the systems and dependencies required by their target platform.
- **Multiplatform support** — The architecture is designed to support desktop and mobile platforms through platform-specific backends and renderers.
- **Clear separation** — Engine functionality remains independent from expansion-specific logic, assets, and content.

## Architecture

Northrend supplies the shared engine foundation. A game repository integrates the engine, selects the required platform and rendering components, and provides its own application logic and assets.

This separation allows multiple games or expansion-specific clients to share the same engine without introducing their individual rules or content into Northrend.

## Project Status

Northrend is under active development. APIs, platform integrations, and supported features may change as the architecture evolves.

## License

Northrend is available under either the
[Apache License 2.0](LICENSE-APACHE) or the [MIT License](LICENSE-MIT), at your
option.

Unless explicitly stated otherwise, any contribution intentionally submitted
for inclusion in Northrend is dual-licensed under the same terms, without
additional terms or conditions.

Third-party components remain subject to their respective license terms. See
[Third-Party Notices](dep/THIRD_PARTY_NOTICES.md) for details.

## Legal Notice

This repository contains no source code, assets, or other proprietary material from Blizzard Entertainment. Northrend is an independent project and is not affiliated with, endorsed by, or sponsored by Blizzard Entertainment.

World of Warcraft and Blizzard Entertainment are trademarks or registered trademarks of Blizzard Entertainment, Inc. Their names are used only to describe the project's intended technical compatibility and scope.
