import * as vscode from 'vscode';

export function setupDiagnostics(context: vscode.ExtensionContext): void {
    const collection = vscode.languages.createDiagnosticCollection('fridge');
    context.subscriptions.push(collection);

    if (vscode.window.activeTextEditor) {
        const document = vscode.window.activeTextEditor.document;
        if (document.languageId === 'fridge') {
            validateDocument(document, collection);
        }
    }

    context.subscriptions.push(
        vscode.workspace.onDidChangeTextDocument(event => {
            if (event.document.languageId === 'fridge') {
                validateDocument(event.document, collection);
            } else {
                collection.delete(event.document.uri);
            }
        }),
        vscode.window.onDidChangeActiveTextEditor(editor => {
            if (!editor) return;
            if (editor.document.languageId === 'fridge') {
                validateDocument(editor.document, collection);
            } else {
                collection.delete(editor.document.uri);
            }
        }),
        vscode.workspace.onDidCloseTextDocument(document => {
            collection.delete(document.uri);
        })
    );
}

function validateDocument(doc: vscode.TextDocument, collection: vscode.DiagnosticCollection): void {
    if (doc.languageId !== 'fridge') {
        collection.delete(doc.uri);
        return;
    }

    const diagnostics: vscode.Diagnostic[] = [];
    const lines = doc.getText().split(/\r?\n/);
    const braceStack: number[] = [];

    for (let lineNumber = 0; lineNumber < lines.length; lineNumber++) {
        const rawLine = lines[lineNumber];
        const trimmed = rawLine.trim();
        if (!trimmed || trimmed.startsWith('//') || trimmed.toLowerCase().startsWith('note ')) {
            continue;
        }

        // Scan lexical state once. Braces and comment markers inside quoted
        // text are data, not syntax. Escaped quotes do not end string literals.
        let inString = false;
        let escaped = false;
        for (let column = 0; column < rawLine.length; column++) {
            const ch = rawLine[column];
            if (inString) {
                if (escaped) {
                    escaped = false;
                } else if (ch === '\\') {
                    escaped = true;
                } else if (ch === '"') {
                    inString = false;
                }
                continue;
            }

            if (ch === '/' && rawLine[column + 1] === '/') {
                break;
            }
            if (ch === '"') {
                inString = true;
            } else if (ch === '{') {
                braceStack.push(lineNumber);
            } else if (ch === '}') {
                if (braceStack.length === 0) {
                    diagnostics.push(new vscode.Diagnostic(
                        new vscode.Range(lineNumber, column, lineNumber, column + 1),
                        'Syntax Error: Unexpected closing brace "}" with no matching "{".',
                        vscode.DiagnosticSeverity.Error
                    ));
                } else {
                    braceStack.pop();
                }
            }
        }

        if (inString) {
            diagnostics.push(new vscode.Diagnostic(
                new vscode.Range(lineNumber, 0, lineNumber, rawLine.length),
                'Syntax Error: Unclosed double quote in string literal.',
                vscode.DiagnosticSeverity.Error
            ));
        }

        // Catch-block syntax. The "error" word inside strings, comments,
        // variable names and stop-with-error statements is not a catch block.
        if (/^error\b/i.test(trimmed) && !/^error\s*\(\s*\)\s*\{?\s*$/i.test(trimmed)) {
            diagnostics.push(new vscode.Diagnostic(
                new vscode.Range(lineNumber, 0, lineNumber, rawLine.length),
                'Syntax Error: Catch blocks must use the form "error() {".',
                vscode.DiagnosticSeverity.Error
            ));
        }

        if (/^fnc\b/i.test(trimmed) && !/^fnc\s+[A-Za-z_][A-Za-z0-9_]*\s*\([^)]*\)\s*\{?\s*$/i.test(trimmed)) {
            diagnostics.push(new vscode.Diagnostic(
                new vscode.Range(lineNumber, 0, lineNumber, rawLine.length),
                'Syntax Error: Function definitions must use "fnc name(parameters) {".',
                vscode.DiagnosticSeverity.Error
            ));
        }

        if (/^(rem|remember)\b/i.test(trimmed) &&
            !/^(rem|remember)\s+[A-Za-z_][A-Za-z0-9_]*\s+as\s+.+$/i.test(trimmed)) {
            diagnostics.push(new vscode.Diagnostic(
                new vscode.Range(lineNumber, 0, lineNumber, rawLine.length),
                'Syntax Error: Variable declarations must use "rem name as value".',
                vscode.DiagnosticSeverity.Error
            ));
        }
    }

    for (const openLine of braceStack) {
        diagnostics.push(new vscode.Diagnostic(
            new vscode.Range(openLine, 0, openLine, lines[openLine].length),
            'Syntax Error: Unclosed block brace. Add a matching "}".',
            vscode.DiagnosticSeverity.Error
        ));
    }

    collection.set(doc.uri, diagnostics);
}
