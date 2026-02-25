# mdpeek

[![Copr build status](https://copr.fedorainfracloud.org/coprs/guillermodotn/mdpeek/package/mdpeek/status_image/last_build.png)](https://copr.fedorainfracloud.org/coprs/guillermodotn/mdpeek/package/mdpeek/)


Lightweight CLI markdown previewer with GitHub-style rendering and live reload.

Renders GitHub Flavored Markdown in a native GTK4 window and automatically
refreshes when the file changes on disk.

## Features

- **GFM support** — tables, strikethrough, autolinks, task lists, tag filter
  (via cmark-gfm)
- **Live reload** — watches the file for changes and re-renders automatically
- **GitHub-style rendering** — pixel-perfect GitHub CSS via WebKitGTK
- **Lightweight** — WebKitGTK (~30MB) instead of Chromium (~261MB)
- **Scroll preservation** — maintains scroll position across reloads
- **Atomic save handling** — correctly handles editors that save via
  write-tmp + rename

## Prerequisites

**Container build (recommended):**

- [Podman](https://podman.io/) (or Docker)

**Local build:**

- CMake >= 3.16
- GCC with C99 support
- GTK4 + WebKitGTK development headers
  - Fedora: `sudo dnf install gtk4-devel webkitgtk6.0-devel`
  - Ubuntu/Debian: `sudo apt install libgtk-4-dev libwebkitgtk-6.0-dev`
  - Arch: `sudo pacman -S gtk4 webkitgtk-6.0`

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
> needed beyond Podman and GTK4/WebKitGTK runtime libraries.
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
- **WebKitGTK** renders the HTML with full GitHub CSS styling in a GTK4 window
- **GFileMonitor** monitors the file for changes (uses inotify on
  Linux) with a 150ms debounce to handle rapid saves
