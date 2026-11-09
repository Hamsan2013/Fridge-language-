# FRIDGE Language Support for Acode

A small Acode plugin that registers the `.frg` file extension as the **FRIDGE** language.

## Included in this first build

- Recognizes files ending in `.frg`.
- Registers a dedicated **FRIDGE** language mode in Acode.
- Shows a notification when the plugin is loaded.
- Cleans up the language registration when the plugin is disabled or unloaded.

## Current limitation

This is the first packaging version. It registers the FRIDGE file type, but does not yet provide keyword coloring, autocomplete, diagnostics, or a FRIDGE interpreter. Those can be added in later versions.

## Install

1. Download `FRIDGE-Acode-Plugin.zip` from the GitHub Actions artifact.
2. Open Acode → Settings → Plugins.
3. Tap **+** → **LOCAL**.
4. Select the ZIP file.
5. Open a `.frg` file.

## Build

The repository workflow `.github/workflows/build-acode-plugin.yml` packages this directory into `FRIDGE-Acode-Plugin.zip`.
