# FRIDGE Programming Language

**FRIDGE v0.5.2 — Simple English. Real Programs.**

FRIDGE is an English-like programming language implemented in C++17. This repository's v0.5.2 command-line runtime supports variables, arithmetic, conditions, loops, functions, console input/output, and project-local file operations.

## Quick start

~~~sh
fridge version
fridge help
fridge check hello.frg
fridge run hello.frg
~~~

A small program:

~~~fridge
rem player as "Hamsan"
rem score as 10

add 5 to score
show "Hello " joined with player
show "Score: " joined with score

if score >= 15 {
    show "Level complete!"
} otherwise {
    show "Keep going!"
}
~~~

Legacy end-if, end-repeat, end-function, and related block closers are normalized to the current curly-brace block syntax.

## CLI commands

| Command | Purpose |
| --- | --- |
| fridge help | Show command help |
| fridge version | Show the installed engine version and platform |
| fridge check FILE.frg | Validate block structure and supported statement forms without running the program |
| fridge run FILE.frg | Run a source file after the mandatory release check |
| fridge update | Download and install the latest compatible official release |

Direct invocation (fridge hello.frg) is retained for older scripts and shortcuts.

## Mandatory update protection

Every fridge run checks the latest stable GitHub Release before executing a program. When a newer engine release exists, execution is blocked until the user runs:

~~~sh
fridge update
~~~

The updater reads the latest version and assets from this repository's GitHub Releases API. It selects only the exact installer name for the running operating system, downloads the asset over HTTPS, and verifies the SHA-256 digest published by GitHub before launching the installer.

Current updater targets:

| Operating system / architecture | Required release asset |
| --- | --- |
| Windows x64 | FRIDGE-Setup-vX.Y.Z.exe |
| Linux x86_64 / amd64 | FRIDGE-Linux-vX.Y.Z.deb |
| macOS x86_64 / Apple Silicon | FRIDGE-macOS-vX.Y.Z.pkg |
| Android arm64 (Termux) | FRIDGE-Android-arm64-vX.Y.Z |

The updater deliberately blocks unsupported platforms and architectures rather than installing the wrong package. Android arm64 is supported for Termux-style command-line installs; ARM Linux and 32-bit Windows are not updater targets. Android needs the Termux curl package (pkg install curl) for release checks and downloads. It updates the running executable atomically in its existing directory, so that directory must be writable by the current user. Internet access is required to run programs because the version gate fails closed when it cannot verify the current official release. fridge check, fridge help, and fridge version remain available without the update check.

The OS installer may request administrator approval. Windows uses the system installer elevation flow; Linux and macOS use their native package installation commands. The VS Code extension retains its own version and update lifecycle; vscode-extension/package.json is not changed by this engine release.

## Language features in the shipped CLI

- Case-insensitive variable names using rem / remember and set.
- Numeric arithmetic with +, -, *, /, %, parentheses, and English operators such as plus, multiplied by, and divided by.
- Numeric comparisons (>, <, >=, <=) and equality (is, is not, ==, !=), plus and, or, and not.
- show and interactive ask.
- if / otherwise, counted repeat, and condition-based while blocks.
- fnc functions, parameters, and return (returned value is stored as FUNCTION_NAME_result).
- attempt { ... } error() { ... } error handling and stop with error.
- create file, add ... to file, read file ... into ..., and delete file / del(...).

### File safety

FRIDGE file commands are restricted to paths under the current project directory. Absolute paths, .. traversal, and symbolic-link traversal are rejected. Reads preserve the file's contents, and writes report open, flush, and close errors. Keep important files backed up; this is a project-level safety boundary, not an operating-system sandbox.

The interpreter also enforces limits for source size, nesting, loop counts, and total executed statements to reduce accidental runaway programs.

## Current implementation boundaries

This release fixes the command-line interpreter and core scripting path. The repository still contains older, incompatible library/parser generations that are not part of the shipped CMake target. 2D/3D statements are currently accepted as informational console placeholders; this CLI build does not contain a graphics renderer. The .uzip packaging flow, lists/maps, event engine, and full visual theme system are not implemented by this shipped runtime yet and should not be treated as available features.

The VS Code extension lives in vscode-extension/. Its package metadata is intentionally independent from the engine version. The Acode plugin lives in acode-plugin/.

## Build from source

Requirements: a C++17 compiler and CMake 3.16 or newer.

~~~sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build --output-on-failure
~~~

The CMake build creates fridge. When BUILD_TESTING is enabled (the default), it also builds a separate fridge_test executable used by the smoke tests; the test binary does not perform the online update gate.

## Release downloads

Official builds and release notes:

https://github.com/Hamsan2013/Fridge-language-/releases

Version 0.5.2 is the engine version. The VS Code extension's own package version is maintained separately.
