import * as vscode from 'vscode';
import { exec } from 'child_process';
import { findFridgeExecutable } from './runner';

export function updateDiagnostics(document: vscode.TextDocument, collection: vscode.DiagnosticCollection) {
    const exe = findFridgeExecutable();
    if (!exe) return;

    exec(`"${exe}" check "${document.fileName}"`, (error, stdout, stderr) => {
        const diagnostics: vscode.Diagnostic[] = [];
        const output = stderr.toString() + stdout.toString();
        const errorRegex = /\[(.*?)\] Error: (.*?)\n\s+--> .*?:(\d+):(\d+)/g;

        let match;
        while ((match = errorRegex.exec(output)) !== null) {
            const line = Math.max(0, parseInt(match[3]) - 1);
            const col = Math.max(0, parseInt(match[4]) - 1);
            const range = new vscode.Range(line, col, line, col + 10);
            diagnostics.push(new vscode.Diagnostic(range, match[2], vscode.DiagnosticSeverity.Error));
        }

        collection.set(document.uri, diagnostics);
    });
}
