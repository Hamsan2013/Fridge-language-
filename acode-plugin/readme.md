# FRIDGE Language Support for Acode

FRIDGE language mode and basic Run / Check actions for `.frg` files.

## Features

- Registers `.frg` files as **FRIDGE**.
- Adds a **▶ Run** button when a `.frg` file is open.
- Adds a **✓ Check** button when a `.frg` file is open.
- Adds FRIDGE Run and Check commands where Acode's command API is available.
- Sends the current editor contents to Acode's built-in terminal, including unsaved changes.

## Important: terminal environments

This plugin currently uses Acode's built-in terminal. It does **not** execute commands inside the separate Termux app. A FRIDGE binary installed in Termux is therefore not automatically available to this plugin.

Open Acode's menu → Terminal and complete terminal setup. Then make the `fridge` command available inside that terminal environment. When the command is missing, the plugin prints a clear message in the terminal.

## Use

1. Open Acode's built-in Terminal once and complete setup.
2. Open a `.frg` file.
3. Tap **▶** to run or **✓** to check.
4. Read the output in the terminal tab.

## Build

GitHub Actions packages this directory into `FRIDGE-Acode-Plugin.zip`.
