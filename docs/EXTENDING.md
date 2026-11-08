# How to Add New Features in Future FRIDGE Versions

FRIDGE uses a modular Segment Registry so you never need to rewrite the Lexer or Parser when adding new commands.

1. Open `src/segments/stdlib_segments.cpp`.
2. Add a new `reg.register_command("your_keyword", ...)` block.
3. Add the keyword to `vscode-extension/syntaxes/fridge.tmLanguage.json` for syntax highlighting.
4. Push a new Git tag (e.g. `v0.4.0`) to GitHub—GitHub Actions will automatically compile `fridge.exe` with your `.ico` icon and publish the new release.
