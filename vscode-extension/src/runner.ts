import * as vscode from 'vscode';
import * as path from 'path';
import * as fs from 'fs';
import { execSync, spawn, ChildProcessWithoutNullStreams } from 'child_process';

let outputPanel: vscode.WebviewPanel | undefined;
let activeProcess: ChildProcessWithoutNullStreams | undefined;

export function findFridgeExecutable(): string | null {
    const config = vscode.workspace.getConfiguration('fridge');
    const customPath = config.get<string>('executablePath');
    if (customPath && fs.existsSync(customPath)) {
        return customPath;
    }

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
        commonPaths.push('/usr/local/bin/fridge', '/usr/bin/fridge');
    }

    for (const testPath of commonPaths) {
        if (fs.existsSync(testPath)) {
            return testPath;
        }
    }

    try {
        const cmd = process.platform === 'win32' ? 'where fridge' : 'which fridge';
        const result = execSync(cmd, { stdio: ['pipe', 'pipe', 'ignore'] })
            .toString()
            .trim()
            .split(/\r?\n/)[0];
        if (result && fs.existsSync(result)) {
            return result;
        }
    } catch (_) {}

    return null;
}

export function runFridgeFile(document: vscode.TextDocument): void {
    if (document.isDirty) {
        document.save();
    }

    const exe = findFridgeExecutable();
    if (!exe) {
        vscode.window.showErrorMessage(
            'FRIDGE executable not found! Install FRIDGE or set "fridge.executablePath" in VS Code Settings.',
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
    const fileContent = document.getText();

    if (activeProcess) {
        try {
            activeProcess.kill();
        } catch (_) {}
        activeProcess = undefined;
    }

    if (!outputPanel) {
        outputPanel = vscode.window.createWebviewPanel(
            'fridgeStudioOutput',
            'FRIDGE v0.5.0 Engine & Output',
            vscode.ViewColumn.Beside,
            { enableScripts: true, retainContextWhenHidden: true }
        );
        outputPanel.onDidDispose(() => {
            outputPanel = undefined;
        });
    } else {
        outputPanel.reveal(vscode.ViewColumn.Beside, true);
    }

    outputPanel.webview.html = getStudioHtml(path.basename(filePath));

    if (/\bask\b/i.test(fileContent)) {
        const terminal = vscode.window.terminals.find(t => t.name === 'FRIDGE Terminal')
            || vscode.window.createTerminal('FRIDGE Terminal');
        terminal.show(true);
        terminal.sendText('cd "' + workingDir + '"');
        const cmd = process.platform === 'win32'
            ? '& "' + exe + '" run "' + filePath + '"'
            : '"' + exe + '" run "' + filePath + '"';
        terminal.sendText(cmd);
    }

    const precommands = extractVisualConstructs(fileContent, workingDir);
    for (const cmd of precommands) {
        if (outputPanel) {
            outputPanel.webview.postMessage(cmd);
        }
    }

    try {
        activeProcess = spawn(exe, ['run', filePath], { cwd: workingDir });

        activeProcess.stdout.on('data', (data: Buffer) => {
            const lines = data.toString().split(/\r?\n/);
            for (const line of lines) {
                if (!line.trim()) {
                    continue;
                }
                parseAndDispatchLine(line, workingDir);
            }
        });

        activeProcess.stderr.on('data', (data: Buffer) => {
            const lines = data.toString().split(/\r?\n/);
            for (const line of lines) {
                if (!line.trim()) {
                    continue;
                }
                if (outputPanel) {
                    outputPanel.webview.postMessage({ type: 'stderr', text: line });
                }
            }
        });

        activeProcess.on('close', (code: number | null) => {
            if (outputPanel) {
                outputPanel.webview.postMessage({ type: 'exit', code: code ?? 0 });
            }
            activeProcess = undefined;
        });
    } catch (err: any) {
        vscode.window.showErrorMessage('Error running FRIDGE file: ' + (err?.message || String(err)));
    }
}

function loadObjGeometry(fullPath: string): { vertices: number[][]; edges: number[][] } | null {
    if (!fs.existsSync(fullPath)) {
        return null;
    }
    try {
        const content = fs.readFileSync(fullPath, 'utf-8');
        const vertices: number[][] = [];
        const edges: number[][] = [];
        const lines = content.split(/\r?\n/);
        for (const line of lines) {
            const trimmed = line.trim();
            if (trimmed.startsWith('v ')) {
                const parts = trimmed.split(/\s+/);
                vertices.push([Number(parts[1]) || 0, Number(parts[2]) || 0, Number(parts[3]) || 0]);
            } else if (trimmed.startsWith('f ')) {
                const parts = trimmed.split(/\s+/).slice(1);
                const indices = parts.map(p => parseInt(p.split('/')[0], 10) - 1).filter(n => !isNaN(n));
                for (let i = 0; i < indices.length; i++) {
                    const a = indices[i];
                    const b = indices[(i + 1) % indices.length];
                    if (a >= 0 && b >= 0) {
                        edges.push([a, b]);
                    }
                }
            }
        }
        return vertices.length > 0 ? { vertices, edges } : null;
    } catch (_) {
        return null;
    }
}

function parseAndDispatchLine(line: string, workingDir: string): void {
    if (!outputPanel) {
        return;
    }

    const match3D = line.match(/Instantiated 3D scene node:\s*'(.*?)'/i);
    if (match3D) {
        outputPanel.webview.postMessage({ type: 'spawn3D', name: match3D[1] });
    }

    const matchModel = line.match(/\[FRIDGE 3D Asset\] Loaded model '(.*?)' as '(.*?)'/i);
    if (matchModel) {
        const fullPath = path.resolve(workingDir, matchModel[1]);
        const meshData = loadObjGeometry(fullPath);
        outputPanel.webview.postMessage({
            type: 'spawn3D',
            name: matchModel[2] + ' (' + path.basename(matchModel[1]) + ')',
            mesh: meshData
        });
    }

    outputPanel.webview.postMessage({ type: 'stdout', text: line });
}

function extractVisualConstructs(source: string, workingDir: string): Array<Record<string, any>> {
    const messages: Array<Record<string, any>> = [];
    const lines = source.split(/\r?\n/);

    for (const rawLine of lines) {
        const line = rawLine.trim();
        if (!line || line.startsWith('//') || line.toLowerCase().startsWith('note ')) {
            continue;
        }

        const objMatch = line.match(/^add\s+OBJ\s+([a-zA-Z0-9_]+)/i);
        if (objMatch) {
            messages.push({ type: 'spawn3D', name: objMatch[1] });
            continue;
        }

        const modelMatch = line.match(/^load\s+3d\s+model\s+from\s+"([^"]+)"\s+as\s+([a-zA-Z0-9_]+)/i);
        if (modelMatch) {
            const fullPath = path.resolve(workingDir, modelMatch[1]);
            const meshData = loadObjGeometry(fullPath);
            messages.push({
                type: 'spawn3D',
                name: modelMatch[2] + ' (' + path.basename(modelMatch[1]) + ')',
                mesh: meshData
            });
            continue;
        }

        const rectMatch = line.match(/^draw\s+rectangle\s+at\s+(-?\d+)\s*,\s*(-?\d+)\s+with\s+width\s+(\d+)\s+height\s+(\d+)(?:\s+color\s+"([^"]+)")?/i);
        if (rectMatch) {
            messages.push({
                type: 'draw2D',
                shape: 'rect',
                x: Number(rectMatch[1]),
                y: Number(rectMatch[2]),
                w: Number(rectMatch[3]),
                h: Number(rectMatch[4]),
                color: rectMatch[5] || '#58a6ff'
            });
            continue;
        }

        const circleMatch = line.match(/^draw\s+circle\s+at\s+(-?\d+)\s*,\s*(-?\d+)\s+with\s+radius\s+(\d+)(?:\s+color\s+"([^"]+)")?/i);
        if (circleMatch) {
            messages.push({
                type: 'draw2D',
                shape: 'circle',
                x: Number(circleMatch[1]),
                y: Number(circleMatch[2]),
                r: Number(circleMatch[3]),
                color: circleMatch[4] || '#ff7b72'
            });
            continue;
        }

        const spriteMatch = line.match(/^add\s+SPRITE\s+([a-zA-Z0-9_]+)(?:\s+at\s+(-?\d+)\s*,\s*(-?\d+))?/i);
        if (spriteMatch) {
            messages.push({
                type: 'draw2D',
                shape: 'sprite',
                id: spriteMatch[1],
                x: Number(spriteMatch[2] || 150),
                y: Number(spriteMatch[3] || 150),
                w: 48,
                h: 48,
                color: '#3fb950'
            });
            continue;
        }
    }

    return messages;
}

function getStudioHtml(fileName: string): string {
    return [
        '<!DOCTYPE html>',
        '<html lang="en">',
        '<head>',
        '<meta charset="UTF-8">',
        '<style>',
        '  * { box-sizing: border-box; }',
        '  body { margin: 0; padding: 0; background: #0d1117; color: #e6edf3; font-family: "Segoe UI", Consolas, monospace; display: flex; flex-direction: column; height: 100vh; overflow: hidden; }',
        '  header { background: #161b22; padding: 8px 14px; border-bottom: 1px solid #30363d; display: flex; justify-content: space-between; align-items: center; }',
        '  .brand { font-weight: 700; color: #58a6ff; font-size: 13px; }',
        '  .tabs { display: flex; gap: 6px; }',
        '  .tab-btn { background: #21262d; color: #c9d1d9; border: 1px solid #30363d; padding: 4px 12px; border-radius: 6px; cursor: pointer; font-size: 12px; font-weight: 600; }',
        '  .tab-btn.active { background: #1f6feb; color: #ffffff; border-color: #388bfd; }',
        '  .badge { background: #238636; color: #ffffff; padding: 3px 9px; border-radius: 12px; font-size: 11px; font-weight: 600; }',
        '  #stage-area { flex: 1; position: relative; display: none; background: radial-gradient(circle at center, #1b2432 0%, #090d16 100%); overflow: hidden; }',
        '  #stage-area.visible { display: block; }',
        '  canvas { width: 100%; height: 100%; display: none; }',
        '  canvas.active { display: block; }',
        '  #hud { position: absolute; top: 10px; left: 10px; background: rgba(13, 17, 23, 0.82); padding: 8px 12px; border: 1px solid #30363d; border-radius: 6px; font-size: 12px; pointer-events: none; }',
        '  #console-container { flex: 1; background: #010409; border-top: 1px solid #30363d; padding: 12px; overflow-y: auto; font-family: Consolas, monospace; font-size: 13px; }',
        '  #console-container.docked { flex: 0 0 190px; }',
        '  .log-line { margin: 4px 0; line-height: 1.45; white-space: pre-wrap; word-break: break-word; }',
        '  .log-3d { color: #d2a8ff; font-weight: 600; }',
        '  .log-2d { color: #3fb950; font-weight: 600; }',
        '  .log-err { color: #ff7b72; font-weight: 600; }',
        '  .log-sys { color: #79c0ff; font-style: italic; }',
        '</style>',
        '</head>',
        '<body>',
        '  <header>',
        '    <div class="brand">FRIDGE v0.5.0 Runtime — ' + fileName + '</div>',
        '    <div class="tabs">',
        '      <button class="tab-btn active" id="tab-console" onclick="setMode(\'console\')">Console</button>',
        '      <button class="tab-btn" id="tab-2d" onclick="setMode(\'2d\')">2D Canvas</button>',
        '      <button class="tab-btn" id="tab-3d" onclick="setMode(\'3d\')">3D Viewport</button>',
        '    </div>',
        '    <div class="badge" id="status-badge">Running</div>',
        '  </header>',
        '  <div id="stage-area">',
        '    <canvas id="canvas2d"></canvas>',
        '    <canvas id="canvas3d"></canvas>',
        '    <div id="hud">',
        '      <div><b>Scene Hierarchy</b></div>',
        '      <div id="hud-list">Ready</div>',
        '    </div>',
        '  </div>',
        '  <div id="console-container">',
        '    <div class="log-line log-sys">>>> Executing ' + fileName + ' in FRIDGE v0.5.0 Engine...</div>',
        '  </div>',
        '<script>',
        '  const stageArea = document.getElementById("stage-area");',
        '  const consoleContainer = document.getElementById("console-container");',
        '  const canvas2d = document.getElementById("canvas2d");',
        '  const canvas3d = document.getElementById("canvas3d");',
        '  const ctx2d = canvas2d.getContext("2d");',
        '  const ctx3d = canvas3d.getContext("2d");',
        '  const hudList = document.getElementById("hud-list");',
        '  const statusBadge = document.getElementById("status-badge");',
        '',
        '  let currentMode = "console";',
        '  const shapes2D = [];',
        '  const nodes3D = [];',
        '  const palette = ["#58a6ff", "#3fb950", "#d2a8ff", "#ffa657", "#ff7b72"];',
        '',
        '  function setMode(mode) {',
        '    currentMode = mode;',
        '    document.getElementById("tab-console").classList.toggle("active", mode === "console");',
        '    document.getElementById("tab-2d").classList.toggle("active", mode === "2d");',
        '    document.getElementById("tab-3d").classList.toggle("active", mode === "3d");',
        '    if (mode === "console") {',
        '      stageArea.classList.remove("visible");',
        '      consoleContainer.classList.remove("docked");',
        '    } else {',
        '      stageArea.classList.add("visible");',
        '      consoleContainer.classList.add("docked");',
        '      canvas2d.classList.toggle("active", mode === "2d");',
        '      canvas3d.classList.toggle("active", mode === "3d");',
        '      resizeCanvases();',
        '      updateHud();',
        '    }',
        '  }',
        '',
        '  function resizeCanvases() {',
        '    const w = stageArea.clientWidth || 640;',
        '    const h = stageArea.clientHeight || 380;',
        '    canvas2d.width = w; canvas2d.height = h;',
        '    canvas3d.width = w; canvas3d.height = h;',
        '  }',
        '  window.addEventListener("resize", resizeCanvases);',
        '',
        '  function updateHud() {',
        '    if (currentMode === "2d") {',
        '      hudList.innerHTML = shapes2D.length ? shapes2D.map((s, i) => "• [" + (i+1) + "] " + s.shape + " (" + s.x + "," + s.y + ")").join("<br>") : "2D Stage Ready";',
        '    } else if (currentMode === "3d") {',
        '      hudList.innerHTML = nodes3D.length ? nodes3D.map((n, i) => "• [" + (i+1) + "] " + n.name).join("<br>") : "Drag mouse to orbit 3D camera";',
        '    }',
        '  }',
        '',
        '  function appendLog(text, cls) {',
        '    const div = document.createElement("div");',
        '    div.className = "log-line" + (cls ? " " + cls : "");',
        '    div.textContent = text;',
        '    consoleContainer.appendChild(div);',
        '    consoleContainer.scrollTop = consoleContainer.scrollHeight;',
        '  }',
        '',
        '  function render2D() {',
        '    const w = canvas2d.width, h = canvas2d.height;',
        '    ctx2d.clearRect(0, 0, w, h);',
        '    ctx2d.strokeStyle = "rgba(88, 166, 255, 0.12)";',
        '    for (let x = 0; x < w; x += 40) { ctx2d.beginPath(); ctx2d.moveTo(x, 0); ctx2d.lineTo(x, h); ctx2d.stroke(); }',
        '    for (let y = 0; y < h; y += 40) { ctx2d.beginPath(); ctx2d.moveTo(0, y); ctx2d.lineTo(w, y); ctx2d.stroke(); }',
        '    for (const s of shapes2D) {',
        '      ctx2d.fillStyle = s.color || "#58a6ff";',
        '      if (s.shape === "rect") {',
        '        ctx2d.fillRect(s.x, s.y, s.w, s.h);',
        '      } else if (s.shape === "circle") {',
        '        ctx2d.beginPath(); ctx2d.arc(s.x, s.y, s.r, 0, Math.PI * 2); ctx2d.fill();',
        '      } else if (s.shape === "sprite") {',
        '        ctx2d.fillRect(s.x, s.y, s.w, s.h);',
        '        ctx2d.strokeStyle = "#ffffff"; ctx2d.strokeRect(s.x, s.y, s.w, s.h);',
        '        ctx2d.fillStyle = "#ffffff"; ctx2d.font = "12px Consolas"; ctx2d.fillText(s.id, s.x + 4, s.y - 6);',
        '      }',
        '    }',
        '  }',
        '',
        '  let camYaw = 0.6, camPitch = 0.35, isDragging = false, lastX = 0, lastY = 0, tick = 0;',
        '  canvas3d.addEventListener("mousedown", e => { isDragging = true; lastX = e.clientX; lastY = e.clientY; });',
        '  window.addEventListener("mouseup", () => { isDragging = false; });',
        '  window.addEventListener("mousemove", e => {',
        '    if (!isDragging) return;',
        '    camYaw += (e.clientX - lastX) * 0.01;',
        '    camPitch = Math.max(-1.2, Math.min(1.2, camPitch + (e.clientY - lastY) * 0.01));',
        '    lastX = e.clientX; lastY = e.clientY;',
        '  });',
        '',
        '  function project3D(x, y, z, w, h) {',
        '    const cosY = Math.cos(camYaw), sinY = Math.sin(camYaw);',
        '    const x1 = x * cosY - z * sinY;',
        '    const z1 = z * cosY + x * sinY;',
        '    const cosP = Math.cos(camPitch), sinP = Math.sin(camPitch);',
        '    const y2 = y * cosP - z1 * sinP;',
        '    const z2 = z1 * cosP + y * sinP + 9;',
        '    if (z2 <= 0.2) return null;',
        '    const fov = Math.min(w, h) * 0.85;',
        '    return { sx: w / 2 + (x1 / z2) * fov, sy: h / 2 - (y2 / z2) * fov };',
        '  }',
        '',
        '  function drawLine3D(p1, p2, w, h, color, width) {',
        '    const a = project3D(p1[0], p1[1], p1[2], w, h);',
        '    const b = project3D(p2[0], p2[1], p2[2], w, h);',
        '    if (!a || !b) return;',
        '    ctx3d.strokeStyle = color; ctx3d.lineWidth = width || 1.5;',
        '    ctx3d.beginPath(); ctx3d.moveTo(a.sx, a.sy); ctx3d.lineTo(b.sx, b.sy); ctx3d.stroke();',
        '  }',
        '',
        '  function render3D() {',
        '    const w = canvas3d.width, h = canvas3d.height;',
        '    ctx3d.clearRect(0, 0, w, h);',
        '    tick += 0.02;',
        '    for (let i = -5; i <= 5; i++) {',
        '      drawLine3D([i, -1, -5], [i, -1, 5], w, h, "rgba(88,166,255,0.22)", 1);',
        '      drawLine3D([-5, -1, i], [5, -1, i], w, h, "rgba(88,166,255,0.22)", 1);',
        '    }',
        '    nodes3D.forEach((node, idx) => {',
        '      const cx = (idx - (nodes3D.length - 1) / 2) * 2.4;',
        '      const cy = 0.2 + Math.sin(tick + idx) * 0.25;',
        '      const cz = 0;',
        '      const rot = tick + idx * 0.7;',
        '      if (node.mesh && node.mesh.vertices && node.mesh.vertices.length > 0) {',
        '        const pts = node.mesh.vertices.map(v => {',
        '          const rx = v[0] * Math.cos(rot) - v[2] * Math.sin(rot);',
        '          const rz = v[2] * Math.cos(rot) + v[0] * Math.sin(rot);',
        '          return [cx + rx, cy + v[1], cz + rz];',
        '        });',
        '        for (const e of node.mesh.edges) {',
        '          if (pts[e[0]] && pts[e[1]]) drawLine3D(pts[e[0]], pts[e[1]], w, h, node.color, 1.8);',
        '        }',
        '      } else {',
        '        const r = 0.75;',
        '        const pts = [];',
        '        for (let i = 0; i < 8; i++) {',
        '          const vx = (i & 1 ? r : -r), vy = (i & 2 ? r : -r), vz = (i & 4 ? r : -r);',
        '          const rx = vx * Math.cos(rot) - vz * Math.sin(rot);',
        '          const rz = vz * Math.cos(rot) + vx * Math.sin(rot);',
        '          pts.push([cx + rx, cy + vy, cz + rz]);',
        '        }',
        '        const edges = [[0,1],[1,3],[3,2],[2,0],[4,5],[5,7],[7,6],[6,4],[0,4],[1,5],[2,6],[3,7]];',
        '        for (const e of edges) { drawLine3D(pts[e[0]], pts[e[1]], w, h, node.color, 2.2); }',
        '      }',
        '      const labelPos = project3D(cx, cy + 1.25, cz, w, h);',
        '      if (labelPos) {',
        '        ctx3d.fillStyle = "#ffffff"; ctx3d.font = "bold 12px Segoe UI";',
        '        ctx3d.fillText(node.name, labelPos.sx - 28, labelPos.sy);',
        '      }',
        '    });',
        '  }',
        '',
        '  function animate() {',
        '    if (currentMode === "2d") render2D();',
        '    if (currentMode === "3d") render3D();',
        '    requestAnimationFrame(animate);',
        '  }',
        '  animate();',
        '',
        '  window.addEventListener("message", event => {',
        '    const msg = event.data;',
        '    if (msg.type === "stdout") {',
        '      const cls = msg.text.includes("[FRIDGE 3D") ? "log-3d" : (msg.text.includes("[FRIDGE 2D") ? "log-2d" : "");',
        '      appendLog(msg.text, cls);',
        '    } else if (msg.type === "stderr") {',
        '      appendLog(msg.text, "log-err");',
        '    } else if (msg.type === "spawn3D") {',
        '      if (!nodes3D.some(n => n.name === msg.name)) {',
        '        nodes3D.push({ name: msg.name, color: palette[nodes3D.length % palette.length], mesh: msg.mesh || null });',
        '      }',
        '      if (currentMode === "console") setMode("3d");',
        '      updateHud();',
        '    } else if (msg.type === "draw2D") {',
        '      shapes2D.push(msg);',
        '      if (currentMode === "console") setMode("2d");',
        '      updateHud();',
        '    } else if (msg.type === "exit") {',
        '      statusBadge.textContent = msg.code === 0 ? "Finished (0)" : "Exit (" + msg.code + ")";',
        '      statusBadge.style.background = msg.code === 0 ? "#1f6feb" : "#da3633";',
        '    }',
        '  });',
        '</script>',
        '</body>',
        '</html>'
    ].join('\n');
}
