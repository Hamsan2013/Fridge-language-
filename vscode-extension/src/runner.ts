import * as vscode from 'vscode';
import * as path from 'path';
import * as fs from 'fs';
import { execSync } from 'child_process';

export function findFridgeExecutable(): string | null {
    const config = vscode.workspace.getConfiguration('fridge');
    const customPath = config.get<string>('executablePath');
    if (customPath && fs.existsSync(customPath)) {
        return customPath;
    }

    // Check PATH environment
    try {
        const cmd = process.platform === 'win32' ? 'where fridge' : 'which fridge';
        const result = execSync(cmd).toString().trim().split('\n')[0];
        if (fs.existsSync(result)) return result;
    } catch (_) {}

    return null;
}

export function runFridgeFile(document: vscode.TextDocument) {
    const exe = findFridgeExecutable();
    if (!exe) {
        vscode.window.showErrorMessage(
            'FRIDGE executable not found! Install FRIDGE or specify "fridge.executablePath" in settings.',
            'Open Settings'
        ).then(item => {
            if (item) vscode.commands.executeCommand('workbench.action.openSettings', 'fridge');
        });
        return;
    }

    const filePath = document.fileName;
    const workingDir = path.dirname(filePath);
    const terminal = vscode.window.terminals.find(t => t.name === 'FRIDGE Terminal') 
                     || vscode.window.createTerminal('FRIDGE Terminal');

    terminal.show();
    terminal.sendText(`cd "${workingDir}"`);
    terminal.sendText(`"${exe}" run "${filePath}"`);
}
