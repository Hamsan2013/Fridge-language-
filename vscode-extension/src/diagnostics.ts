import * as vscode from 'vscode';

export function setupDiagnostics(context: vscode.ExtensionContext): void {
    const collection = vscode.languages.createDiagnosticCollection('fridge');
    context.subscriptions.push(collection);

    if (vscode.window.activeTextEditor) {
        validateDocument(vscode.window.activeTextEditor.document, collection);
    }

    context.subscriptions.push(
        vscode.workspace.onDidChangeTextDocument(e => {
            if (e.document.languageId === 'fridge') {
                validateDocument(e.document, collection);
            }
        }),
        vscode.window.onDidChangeActiveTextEditor(editor => {
            if (editor && editor.document.languageId === 'fridge') {
                validateDocument(editor.document, collection);
            }
        })
    );
}

function validateDocument(doc: vscode.TextDocument, collection: vscode.DiagnosticCollection): void {
    if (doc.languageId !== 'fridge') {
        return;
    }

    const diagnostics: vscode.Diagnostic[] = [];
    const text = doc.getText();
    const lines = text.split(/\r?\n/);

    const braceStack: number[] = [];

    for (let i = 0; i < lines.length; i++) {
        const rawLine = lines[i];
        const trimmed = rawLine.trim();

        if (!trimmed || trimmed.startsWith('//') || trimmed.toLowerCase().startsWith('note ')) {
            continue;
        }

        // 1. Check for unclosed string quotes
        const quoteCount = (rawLine.match(/(?<!\\)"/g) || []).length;
        if (quoteCount % 2 !== 0) {
            diagnostics.push(
                new vscode.Diagnostic(
                    new vscode.Range(i, 0, i, rawLine.length),
                    'Syntax Error: Unclosed double quote (") in string literal.',
                    vscode.DiagnosticSeverity.Error
                )
            );
        }

        // 2. Track curly braces { }
        for (let charIdx = 0; charIdx < rawLine.length; charIdx++) {
            const ch = rawLine[charIdx];
            if (ch === '{') {
                braceStack.push(i);
            } else if (ch === '}') {
                if (braceStack.length === 0) {
                    diagnostics.push(
                        new vscode.Diagnostic(
                            new vscode.Range(i, charIdx, i, charIdx + 1),
                            'Syntax Error: Unexpected closing bracket "}" with no matching "{".',
                            vscode.DiagnosticSeverity.Error
                        )
                    );
                } else {
                    braceStack.pop();
                }
            }
        }

        // 3. Enforce v0.5.0 rule: error() must have empty parentheses
        if (/\berror\b/.test(trimmed) && !/\bstop\s+with\s+error\b/i.test(trimmed)) {
            if (!/\berror\s*\(\s*\)\s*\{?/.test(trimmed)) {
                diagnostics.push(
                    new vscode.Diagnostic(
                        new vscode.Range(i, 0, i, rawLine.length),
                        'FRIDGE v0.5.0 Rule: Error catch blocks must use empty small brackets: "error() {".',
                        vscode.DiagnosticSeverity.Error
                    )
                );
            }
        }

        // 4. Enforce v0.5.0 rule: fnc must use ( ) for parameters
        if (/^fnc\b/i.test(trimmed) && !/^fnc\s+[a-zA-Z_][a-zA-Z0-9_]*\s*\(.*\)/i.test(trimmed)) {
            diagnostics.push(
                new vscode.Diagnostic(
                    new vscode.Range(i, 0, i, rawLine.length),
                    'FRIDGE v0.5.0 Rule: Function shorthand "fnc" requires parameter brackets: fnc name(params) {',
                    vscode.DiagnosticSeverity.Error
                )
            );
        }

        // 5. Enforce variable declaration syntax for rem / remember
        if (/^(rem|remember)\b/i.test(trimmed) && !/^(rem|remember)\s+[a-zA-Z_][a-zA-Z0-9_]*\s+as\s+.+/i.test(trimmed)) {
            diagnostics.push(
                new vscode.Diagnostic(
                    new vscode.Range(i, 0, i, rawLine.length),
                    'Syntax Error: Variable declaration must follow "rem <name> as <value>".',
                    vscode.DiagnosticSeverity.Error
                )
            );
        }

        // 6. Warn on legacy 'end if' / 'end repeat' blocks in v0.5.0
        if (/^end\s+(if|repeat|while|function|attempt)\b/i.test(trimmed)) {
            diagnostics.push(
                new vscode.Diagnostic(
                    new vscode.Range(i, 0, i, rawLine.length),
                    'Deprecated in FRIDGE v0.5.0: Use curly brackets "{ }" instead of "end ..." statements.',
                    vscode.DiagnosticSeverity.Warning
                )
            );
        }
    }

    // Report any unclosed '{' blocks
    for (const openLine of braceStack) {
        diagnostics.push(
            new vscode.Diagnostic(
                new vscode.Range(openLine, 0, openLine, lines[openLine].length),
                'Syntax Error: Unclosed curly bracket "{". Add a matching "}" to close the block.',
                vscode.DiagnosticSeverity.Error
            )
        );
    }

    collection.set(doc.uri, diagnostics);
}
