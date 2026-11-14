import * as vscode from 'vscode';

const KEYWORD_DOCS: Record<string, { title: string; summary: string; example: string }> = {
    rem: {
        title: 'rem (Shorthand for remember)',
        summary: 'Creates and stores a variable in memory.',
        example: 'rem score as 100\nrem player_name as "Hamsan"'
    },
    remember: {
        title: 'remember (Variable Declaration)',
        summary: 'Creates and stores a variable in memory (alias: rem).',
        example: 'remember health as 100'
    },
    fnc: {
        title: 'fnc (Function Declaration)',
        summary: 'Defines a reusable function with parameters inside ( ) and a { } block.',
        example: 'fnc take_damage(amount) {\n    set health to health - amount\n}'
    },
    del: {
        title: 'del() (Delete File)',
        summary: 'Deletes a file from disk using parenthesized target path.',
        example: 'del("assets/data/temp.txt")'
    },
    delete: {
        title: 'delete (Delete File)',
        summary: 'Deletes a file from the project directory.',
        example: 'delete file "assets/data/save.txt"'
    },
    attempt: {
        title: 'attempt { ... } error() { ... }',
        summary: 'Executes code safely and catches runtime or file errors in the mandatory error() block.',
        example: 'attempt {\n    read file "save.txt" into data\n} error() {\n    show "Save file not found!"\n}'
    },
    error: {
        title: 'error() (Error Catch Block / Stop Error)',
        summary: 'Catches errors after an attempt { } block, or halts execution via stop with error.',
        example: 'attempt {\n    read file "data.txt" into info\n} error() {\n    show "Handled safely!"\n}'
    },
    show: {
        title: 'show (Console Output)',
        summary: 'Prints text, variables, or combined expressions to the FRIDGE Output Console.',
        example: 'show "Score: " joined with score'
    },
    load: {
        title: 'load 3d model',
        summary: 'Loads a 3D .obj model from your assets folder into the FRIDGE 3D Engine.',
        example: 'load 3d model from "assets/models/car.obj" as player_car'
    }
};

export function registerProviders(context: vscode.ExtensionContext): void {
    // 1. Hover Documentation Preview Provider
    const hoverProvider = vscode.languages.registerHoverProvider('fridge', {
        provideHover(document, position) {
            const range = document.getWordRangeAtPosition(position);
            if (!range) {
                return null;
            }
            const word = document.getText(range).toLowerCase();
            const doc = KEYWORD_DOCS[word];
            if (doc) {
                const md = new vscode.MarkdownString();
                md.appendMarkdown(`### ⚡ ${doc.title}\n\n`);
                md.appendMarkdown(`${doc.summary}\n\n`);
                md.appendCodeblock(doc.example, 'fridge');
                return new vscode.Hover(md, range);
            }
            return null;
        }
    });

    // 2. Auto-Complete Snippets Provider
    const completionProvider = vscode.languages.registerCompletionItemProvider('fridge', {
        provideCompletionItems() {
            const items: vscode.CompletionItem[] = [];

            const addSnippet = (label: string, insert: string, detail: string) => {
                const item = new vscode.CompletionItem(label, vscode.CompletionItemKind.Snippet);
                item.insertText = new vscode.SnippetString(insert);
                item.detail = detail;
                items.push(item);
            };

            addSnippet('rem', 'rem ${1:variable_name} as ${2:100}', 'Create a variable (v0.5.0 shorthand)');
            addSnippet('fnc', 'fnc ${1:name}(${2:param}) {\n\t$0\n}', 'Create a function with parameters');
            addSnippet('attempt', 'attempt {\n\t$1\n} error() {\n\tshow "${2:An error occurred}"\n}', 'Safe try/catch block with error()');
            addSnippet('if', 'if ${1:condition} {\n\t$0\n}', 'Conditional if block');
            addSnippet('load 3d model', 'load 3d model from "assets/${1:model.obj}" as ${2:my_model}', 'Load a 3D model from assets');
            addSnippet('create file', 'create file "${1:assets/data.txt}" with "${2:Hello FRIDGE}"', 'Create and write to a file');
            addSnippet('read file', 'read file "${1:assets/data.txt}" into ${2:content}', 'Read file contents into variable');
            addSnippet('del', 'del("${1:assets/data.txt}")', 'Delete a file');

            return items;
        }
    });

    context.subscriptions.push(hoverProvider, completionProvider);
}
