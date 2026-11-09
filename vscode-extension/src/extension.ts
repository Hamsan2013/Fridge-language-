import * as vscode from 'vscode';
import * as path from 'path';
import * as fs from 'fs';
import { runFridgeFile } from './runner';
import { updateDiagnostics } from './diagnostics';

export function activate(context: vscode.ExtensionContext) {
    const diagnosticCollection = vscode.languages.createDiagnosticCollection('fridge');
    context.subscriptions.push(diagnosticCollection);

    // Register Execution Command
    const runDisposable = vscode.commands.registerCommand('fridge.runFile', () => {
        const editor = vscode.window.activeTextEditor;
        if (!editor) {
            vscode.window.showErrorMessage('No active FRIDGE editor tab open.');
            return;
        }
        runFridgeFile(editor.document);
    });
    context.subscriptions.push(runDisposable);

    // Register On-Save Document Validator for Problems Panel
    vscode.workspace.onDidSaveTextDocument((document) => {
        if (document.languageId === 'fridge') {
            updateDiagnostics(document, diagnosticCollection);
        }
    }, null, context.subscriptions);
}

export function deactivate() {}
