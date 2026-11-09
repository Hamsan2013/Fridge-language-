# FRIDGE Programming Language

**Simple English. Real Programs. 2D, 3D, and Cross-Platform Worlds.**

FRIDGE is an independent, English-like, general-purpose programming language created by **Hamsan A**. It is designed to make programming instructions readable and consistent, using clear English words instead of relying on punctuation-heavy syntax.

FRIDGE v0.4.0 uses a native **C++17** toolchain. Its engine tokenizes source code, parses it into an Abstract Syntax Tree (AST), validates the language grammar, and executes programs locally. FRIDGE does not require an external AI service or an internet connection to execute a program.

- **Current specification:** v0.4.0
- **Source file:** `.frg`
- **Application package:** `.uzip`
- **Runtime:** Native C++17 interpreter
- **Creator:** Hamsan A

---

## Contents

- [What is FRIDGE?](#what-is-fridge)
- [Design principles](#design-principles)
- [Highlights](#highlights)
- [Platform targets](#platform-targets)
- [File formats](#file-formats)
- [Installation and downloads](#installation-and-downloads)
- [Command-line interface](#command-line-interface)
- [Editor support](#editor-support)
- [Language overview](#language-overview)
- [Building from source](#building-from-source)
- [Documentation and learning](#documentation-and-learning)
- [Repository layout](#repository-layout)
- [Contributing and feedback](#contributing-and-feedback)
- [Credits](#credits)

## What is FRIDGE?

FRIDGE is a programming language for writing general-purpose programs with readable, English-like instructions. Its syntax is intended to be approachable while still supporting structured programming concepts such as variables, expressions, conditions, loops, functions, collections, input, events, and file operations.

The language also defines constructs for 2D and 3D-oriented work, including spatial vector values and object hierarchies. Projects can be distributed using FRIDGE's `.uzip` application package format.

FRIDGE is an independent language project. It is not a wrapper around an AI model: source files are processed by the native FRIDGE engine.

## Design principles

- **Readable instructions:** Use familiar English words and phrases.
- **Consistent structure:** One statement per line, with explicit English block endings.
- **Clear diagnostics:** Errors report diagnostic codes and source locations.
- **Local execution:** The interpreter runs programs without requiring an online AI service.
- **A native engine:** The language toolchain is implemented in C++17.

## Highlights

The following capabilities are described by the FRIDGE v0.4.0 language specification.

### Language and runtime

- English-like statement syntax.
- Native C++17 interpreter.
- Lexer, AST parser, grammar validation, and runtime execution.
- Pratt-style expression parser with operator precedence.
- Case-insensitive variable and function name lookup.
- Single-line and multi-line comments.
- Structured lexical, parser, name, type, runtime, package, and asset diagnostics.

### Core programming features

- Variables and reassignment.
- Number, text, boolean, and nothing values.
- Ordered, 1-based lists and key-value maps.
- Arithmetic, comparisons, and logical expressions.
- String joining.
- Conditional statements with `if`, `otherwise`, and `end if`.
- Repetition, `while`, and `for each` loops.
- Functions, return values, local scope, and recursion.
- Console input with `ask`.
- Event and key listeners with `listen`.
- File saving and timing delays.

### 2D and 3D-oriented constructs

- `vector2` and `vector3` values for spatial coordinates.
- Object creation using `add OBJ`.
- Parent-child object hierarchy blocks.

These are language-level constructs; the exact behavior available to a program depends on the FRIDGE runtime and the relevant implementation.

### Application packaging

- Package a project directory into a `.uzip` archive.
- Run a `.uzip` package through the FRIDGE CLI.
- A valid application package has a root-level `main.frg` file.
- Optional `assets/` and `scripts/` directories can hold project resources.

## Platform targets

The v0.4.0 specification lists these platform targets:

| Platform | Target |
| --- | --- |
| Windows | x64 |
| Linux | Native executable |
| macOS | Native executable |
| Raspberry Pi | ARM |

The specification identifies these as FRIDGE targets. Actual binary availability may vary by release. Check the [GitHub Releases](https://github.com/Hamsan2013/Fridge-language-/releases) page for the assets published for a particular version.

## File formats

| Extension | Purpose |
| --- | --- |
| `.frg` | UTF-8 FRIDGE source-code file |
| `.uzip` | Universal ZIP application package |

A `.uzip` project must contain `main.frg` at the archive root. It may also contain `assets/` and `scripts/` directories.

## Installation and downloads

### Download FRIDGE

Get published builds and release notes from:

**[FRIDGE GitHub Releases](https://github.com/Hamsan2013/Fridge-language-/releases)**

Choose the asset that matches your operating system and architecture. Follow the installation instructions included with that release.

If no release asset is available for your platform yet, you can build the interpreter from source using the instructions below.

### Windows

1. Open the [Releases page](https://github.com/Hamsan2013/Fridge-language-/releases).
2. Download the Windows x64 package, when available.
3. Run the installer or follow the package-specific instructions.
4. Open a new terminal and check the installation with:

   ```sh
   fridge version
   ```

5. View the available commands with:

   ```sh
   fridge help
   ```

Make sure the FRIDGE executable is available on your system `PATH` if you want to use the `fridge` command from any terminal directory.

### Linux, macOS, and Raspberry Pi

Use a platform-specific release asset if one is published. Otherwise, build from source with a C++17 compiler and CMake. See [Building from source](#building-from-source).

## Command-line interface

The `fridge` command is the main interface for checking, running, packaging, and diagnosing FRIDGE projects.

| Command | Description |
| --- | --- |
| `fridge version` | Displays the installed engine version, C++17 standard revision, and platform target. |
| `fridge help` | Displays CLI commands and usage information. |
| `fridge check <file.frg>` | Checks source syntax and AST grammar without executing the file. |
| `fridge run <file.frg>` | Validates and executes a FRIDGE source file. |
| `fridge run <app.uzip>` | Runs an application package containing `main.frg`. |
| `fridge pack <directory> <output.uzip>` | Packages a project directory into a `.uzip` file. |
| `fridge doctor` | Checks the installation directory, runtime health, and C++17 environment readiness. |

### Package a project

A project directory should contain its entry point, `main.frg`. Package it with:

```sh
fridge pack ./MyGameFolder StarRunner.uzip
```

Run the resulting package with:

```sh
fridge run StarRunner.uzip
```

## Editor support

### Visual Studio Code

The repository contains the official **FRIDGE Language For VS Code** extension.

The extension contributes:

- FRIDGE language mode for `.frg` and `.uzip` files.
- Syntax highlighting.
- FRIDGE file icons.
- Automatic syntax checking when a file is saved, with diagnostics shown in the Problems panel.
- A run command in the editor.
- The `Ctrl+F5` run shortcut on Windows/Linux and `Cmd+F5` on macOS.
- A configurable path to the FRIDGE executable.

To install a packaged extension:

1. Obtain the `.vsix` file from the matching release, if included.
2. Open Visual Studio Code.
3. Open the Extensions view.
4. Open the Extensions view menu and choose **Install from VSIX...**.
5. Select the downloaded FRIDGE `.vsix` file.

If the interpreter is not found automatically, set the `fridge.executablePath` setting to the full path of your FRIDGE executable.

### Acode for Android

The repository also contains an Acode plugin for FRIDGE language mode and basic Run / Check actions.

The plugin uses Acode's built-in terminal. It does **not** directly execute commands inside the separate Termux app. The `fridge` command must be available in the terminal environment used by Acode.

See the [Acode plugin documentation](acode-plugin/readme.md) for setup details.

## Language overview

FRIDGE source files use the `.frg` extension. The language specification defines these core rules:

- One statement per line.
- Indentation is recommended for readability; it does not determine block scope.
- Variable and function name lookup is case-insensitive.
- Articles such as `a`, `an`, and `the` are accepted in designated grammar positions.
- Single-line comments can begin with `//` or `note`.
- Multi-line comments use `/*` and `*/`.
- Blocks end with explicit phrases such as `end if`, `end repeat`, and `end function`.

### Core value types

| Type | Description |
| --- | --- |
| `number` | Double-precision numeric value |
| `text` | UTF-8 text |
| `boolean` | `true` or `false` |
| `nothing` | Null or empty state |
| `list` | Ordered, dynamic collection with 1-based indexing |
| `map` | Key-value associative collection |
| `vector2` | 2D spatial coordinate |
| `vector3` | 3D spatial coordinate |

### Variables

The specification uses `remember` to declare a variable and `set` to assign a new value. It also defines natural arithmetic instructions such as `add`, `subtract`, `multiply`, and `divide`.

### Expressions

Expressions support grouping with parentheses, arithmetic, string joining, comparisons, and logical operators. FRIDGE uses operator precedence so expressions are not evaluated only from left to right.

### Control flow and functions

The language includes conditional blocks, counted repetition, condition-based loops, collection iteration, user-defined functions, return values, and recursion.

For the full grammar, syntax examples, and diagnostic reference, read the official language manual linked below.

## Building from source

### Requirements

- A C++17-compatible compiler.
- CMake 3.20 or newer.
- Git, to clone the repository.

### Build

Clone the repository:

```sh
git clone https://github.com/Hamsan2013/Fridge-language-.git
cd Fridge-language-
```

Configure the build:

```sh
cmake -S . -B build
```

Compile:

```sh
cmake --build build --config Release
```

The project uses CMake to build the FRIDGE core library and command-line executable. The repository also includes a regression test suite configured through CTest.

To run the configured tests after building:

```sh
ctest --test-dir build --output-on-failure
```

Build output locations can differ by generator and platform. Consult your CMake build output for the exact executable path.

## Documentation and learning

- **Official language specification and developer manual (v0.4.0):** [Read the 21-page PDF manual](https://drive.google.com/file/d/1UxayD6Lk-ppNKYWiWrAOa2Y4RBy1bBeb/view?usp=drivesdk)
- **Video tutorials and installation guide:** [Hamsan's YouTube channel](https://youtube.com/@ahamsan-nm9ct?si=-rMD0cmvu01YJAz5)
- **Source code and updates:** [FRIDGE GitHub repository](https://github.com/Hamsan2013/Fridge-language-)
- **Downloads:** [GitHub Releases](https://github.com/Hamsan2013/Fridge-language-/releases)
- **Acode setup:** [Acode plugin README](acode-plugin/readme.md)
- **Extending FRIDGE:** [Developer extension notes](docs/EXTENDING.md)

## Repository layout

| Path | Purpose |
| --- | --- |
| `src/` | FRIDGE engine and implementation source |
| `include/` | Public C++ headers |
| `tests/` | Regression tests |
| `vscode-extension/` | Visual Studio Code language extension |
| `acode-plugin/` | Acode language support plugin |
| `packaging/` | Platform packaging files |
| `resources/` | Application resources |
| `docs/` | Developer documentation |
| `.github/workflows/` | GitHub Actions workflows |
| `CMakeLists.txt` | CMake build configuration |

## Contributing and feedback

Contributions, bug reports, and suggestions are welcome.

- Open an issue for bugs or feature requests.
- Include the FRIDGE version, operating system, a small source example, and the complete diagnostic message when reporting a problem.
- For code contributions, explain the change and include relevant tests where possible.

Please check the repository's contribution guidance and license before submitting changes.

## Credits

**FRIDGE Programming Language** is created and maintained by **Hamsan A**.

Repository: [Hamsan2013/Fridge-language-](https://github.com/Hamsan2013/Fridge-language-)

---

*FRIDGE v0.4.0 — Simple English. Real Programs.*
