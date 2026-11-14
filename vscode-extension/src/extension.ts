import * as vscode from 'vscode';
import { runFridgeFile } from './runner';
import { setupDiagnostics } from './diagnostics';
import { registerProviders } from './providers';

export function activate(context: vscode.ExtensionContext): void {
    const runCommand = vscode.commands.registerCommand('fridge.runFile', () => {
        const editor = vscode.window.activeTextEditor;
        if (!editor) {
            vscode.window.showWarningMessage('Open a .frg file first to run FRIDGE.');
            return;
        }
        runFridgeFile(editor.document);
    });

    context.subscriptions.push(runCommand);

    setupDiagnostics(context);
    registerProviders(context);
}

export function deactivate(): void {}
