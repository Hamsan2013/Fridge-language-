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

    // 1. Check common installation directories
    const commonPaths: string[] = [];
    if (process.platform === 'win32') {
        const userProfile = process.env.USERPROFILE || '';
        const programFiles = process.env['ProgramFiles'] || 'C:\\Program Files';
        const programFilesX86 = process.env['ProgramFiles(x86)'] || 'C:\\Program Files (x86)';

        commonPaths.push(
            path.join(programFiles, 'FRIDGE', 'bin', 'fridge.exe'),
            path.join(programFilesX86, 'FRIDGE', 'bin', 'fridge.exe'),
            path.join(userProfile, 'Downloads', 'FRIDGE-Windows-Installer-and-Plugin', 'fridge.exe'),
            'C:\\FRIDGE\\bin\\fridge.exe'
        );
    } else {
        commonPaths.push(
            '/usr/local/bin/fridge',
            '/usr/bin/fridge'
        );
    }

    for (const testPath of commonPaths) {
        if (fs.existsSync(testPath)) {
            return testPath;
        }
    }

    // 2. Check PATH environment variable via shell
    try {
        const cmd = process.platform === 'win32' ? 'where fridge' : 'which fridge';
        const result = execSync(cmd, { stdio: ['pipe', 'pipe', 'ignore'] }).toString().trim().split(/\r?\n/)[0];
        if (result && fs.existsSync(result)) {
            return result;
        }
    } catch (_) {}

    return null;
}

export function runFridgeFile(document: vscode.TextDocument) {
    const exe = findFridgeExecutable();
    if (!exe) {
        vscode.window.showErrorMessage(
            'FRIDGE executable not found! Please install FRIDGE, place fridge.exe in your PATH, or set "fridge.executablePath" in Settings.',
            'Open Settings'
        ).then(item => {
            if (item) {
                vscode.commands.executeCommand('workbench.action.openSettings', 'fridge.executablePath');
            }
        });
        return;
    }

    const filePath = document.fileName;
    const workingDir = path.dirname(filePath);
    
    // Reuse existing FRIDGE terminal or create a fresh one
    const terminal = vscode.window.terminals.find(t => t.name === 'FRIDGE Terminal') 
                     || vscode.window.createTerminal('FRIDGE Terminal');

    terminal.show();
    terminal.sendText(`cd "${workingDir}"`);

    // In Windows PowerShell, prepend '& ' so quoted paths execute properly
    const command = process.platform === 'win32'
        ? `& "${exe}" run "${filePath}"`
        : `"${exe}" run "${filePath}"`;

    terminal.sendText(command);
}
