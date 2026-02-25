# mdpeek

![cmark-gfm](https://copr.fedorainfracloud.org/coprs/guillermodotn/mdpeek/package/cmark-gfm/status_image/last_build.png)

CLI markdown previewer with GitHub-style rendering and live reload.

Renders GitHub Flavored Markdown in a native Qt window and automatically
refreshes when the file changes on disk.

## Features

- **GFM support** — tables, strikethrough, autolinks, task lists, tag filter
  (via cmark-gfm)
- **Live reload** — watches the file for changes and re-renders automatically
- **GitHub-style rendering** — pixel-perfect GitHub CSS via QWebEngineView
- **Scroll preservation** — maintains scroll position across reloads
- **Atomic save handling** — correctly handles editors that save via
  write-tmp + rename

## Prerequisites

**Container build (recommended):**

- [Podman](https://podman.io/) (or Docker)

**Local build:**

- CMake >= 3.16
- GCC/G++ with C++17 support
- Qt6 Widgets + WebEngine development headers
  - Fedora: `sudo dnf install qt6-qtbase-devel qt6-qtwebengine-devel`
  - Ubuntu/Debian: `sudo apt install qt6-base-dev qt6-webengine-dev`
  - Arch: `sudo pacman -S qt6-base qt6-webengine`

## Clone

```bash
git clone --recursive https://github.com/<user>/mdpeek.git
cd mdpeek
```

If you already cloned without `--recursive`:

```bash
git submodule update --init --recursive
```

## Build

> [!TIP]
> ### Container build (recommended)
>
> Compiles everything inside a Fedora 43 container — no host dependencies
> needed beyond Podman and Qt6 runtime libraries.
>
> ```bash
> ./build.sh
> ```
>
> The binary is extracted to `./build/mdpeek`.

### Local build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel $(nproc)
```

## Usage

```bash
./build/mdpeek <file.md>
```

Open a markdown file in a preview window. Edit the file in any editor and
the preview updates automatically.

## How It Works

- **cmark-gfm** (vendored as a Git submodule, statically linked) parses
  Markdown to HTML with GitHub Flavored Markdown extensions
- **QWebEngineView** renders the HTML with full GitHub CSS styling
- **QFileSystemWatcher** monitors the file for changes (uses inotify on
  Linux) with a 150ms debounce timer to handle rapid saves
