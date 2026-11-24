# Folder Organizer

A Qt 6 desktop application that sorts files in a selected folder into
type-based subfolders. It previews planned moves before changing anything and
never overwrites an existing file.

[![Release](https://github.com/sanjeevmaharjan/organizer/actions/workflows/release.yml/badge.svg)](https://github.com/sanjeevmaharjan/organizer/actions/workflows/release.yml)

## AI acknowledgment

AI assistance was used in the development of this project.

## Requirements

- CMake 3.21 or newer
- A C++17 compiler
- Qt 6.2 or newer (Widgets and Test modules)

## Build

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

## Run

```sh
./build/folder-organizer
```

Choose a folder, select **Preview**, and review the proposed destinations.
Select **Organize** to move the files. Existing destination names are preserved;
the organizer adds a numbered suffix to avoid replacing them.

Files are categorized as `Documents`, `Images`, `Audio`, `Video`, `Archives`,
`Code`, or `Other`. Only regular files directly inside the selected folder are
processed. Existing subfolders and symbolic links are left untouched.

## Disclaimer

Folder Organizer is provided "as is", without warranties to the extent permitted
by law. It moves files on your behalf, so review the preview carefully and keep
backups of important data. The authors are not liable for loss or damage
resulting from use of the software to the extent permitted by applicable law.
This disclaimer does not limit rights that cannot be excluded by law.

## Test

```sh
ctest --test-dir build --output-on-failure
```

## GitHub releases

Releases are built and published by GitHub Actions when a version tag is pushed:

```sh
git tag v1.0.0
git push origin v1.0.0
```

The workflow builds and tests Linux, Windows, and macOS packages, then attaches
them to the tag's GitHub Release. Linux releases are AppImages; Windows and
macOS releases are ZIP archives. Repository Actions must be enabled, and the
workflow's `GITHUB_TOKEN` needs permission to write releases.

## Project layout

```text
.github/
  workflows/ release automation
packaging/
  folder-organizer.desktop
src/
  app/       Application entry point
  core/      File classification, planning, and organizing
  ui/        Qt Widgets user interface
tests/
  core/      Core behavior tests
```

