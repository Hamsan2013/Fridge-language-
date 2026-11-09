/*
 * FRIDGE Language Support for Acode
 * FRIDGE .frg language mode + Run and Check actions.
 *
 * Execution uses Acode's built-in Alpine terminal. It is separate from Termux.
 */
(function () {
  "use strict";

  const PLUGIN_ID = "com.hamsan.fridge";
  const LANGUAGE_ID = "fridge";
  const EXTENSION = "frg";

  let editorLanguages = null;
  let terminalApi = null;
  let runButton = null;
  let checkButton = null;
  let commandsApi = null;
  let registered = false;
  let switchHandler = null;
  let renameHandler = null;

  function notify(message, type) {
    if (window.acode && typeof acode.pushNotification === "function") {
      acode.pushNotification("FRIDGE", message, {
        type: type || "info",
        autoClose: true
      });
    } else if (window.toast) {
      window.toast(message);
    }
  }

  function activeFridgeFile() {
    const file = window.editorManager && editorManager.activeFile;
    if (!file || file.type !== "editor" || !/\.frg$/i.test(file.filename || "")) {
      notify("Open a .frg file first.", "warning");
      return null;
    }
    return file;
  }

  async function getTerminal() {
    try {
      terminalApi = terminalApi || acode.require("terminal");
      const all = terminalApi.getAll();

      if (all && typeof all[Symbol.iterator] === "function") {
        for (const [, terminal] of all) {
          if (terminal && !(terminal.name || "").startsWith("Install:")) {
            return terminal;
          }
        }
      }

      const terminal = await terminalApi.createServer({ name: "FRIDGE Runner" });
      // Give the terminal shell a moment to start.
      await new Promise(resolve => setTimeout(resolve, 700));
      return terminal;
    } catch (error) {
      console.error("[FRIDGE] Terminal error:", error);
      notify("Open Acode Menu → Terminal and finish its setup, then try again.", "warning");
      return null;
    }
  }

  function encodeBase64(text) {
    const bytes = new TextEncoder().encode(text);
    let binary = "";
    for (let i = 0; i < bytes.length; i++) {
      binary += String.fromCharCode(bytes[i]);
    }
    return btoa(binary);
  }

  async function execute(action) {
    const file = activeFridgeFile();
    if (!file) return;

    const terminal = await getTerminal();
    if (!terminal) return;

    const source = file.session && typeof file.session.getValue === "function"
      ? file.session.getValue()
      : "";

    const tempPath = "/tmp/fridge_" + Date.now() + ".frg";
    const encoded = encodeBase64(source);
    const operation = action === "check" ? "check" : "run";
    const shellCommand =
      "printf '%s' '" + encoded + "' | base64 -d > '" + tempPath + "' && " +
      "if command -v fridge >/dev/null 2>&1; then " +
      "fridge " + operation + " '" + tempPath + "'; " +
      "else echo ''; " +
      "echo '[FRIDGE] Interpreter not found in Acode terminal.'; " +
      "echo '[FRIDGE] Your Termux installation is separate from Acode Alpine.'; " +
      "echo '[FRIDGE] Set up/build the FRIDGE CLI inside Acode Terminal, then retry.'; " +
      "fi; rm -f '" + tempPath + "'";

    try {
      terminal.file.makeActive();
      terminalApi.write(terminal.id, shellCommand + "\r");
      notify(action === "check" ? "Checking current FRIDGE file…" : "Running current FRIDGE file…", "info");
    } catch (error) {
      console.error("[FRIDGE] Could not send command:", error);
      notify("Could not send the command to Acode Terminal.", "error");
    }
  }

  function createActionButton(iconName, title, action) {
    const button = document.createElement("span");
    button.className = "icon " + iconName;
    button.setAttribute("action", "fridge-" + action);
    button.title = title;
    button.setAttribute("role", "button");
    button.setAttribute("aria-label", title);
    button.style.cursor = "pointer";
    button.style.padding = "0 8px";
    button.addEventListener("click", function (event) {
      event.preventDefault();
      event.stopPropagation();
      execute(action);
    });
    return button;
  }

  function refreshButtons() {
    const file = window.editorManager && editorManager.activeFile;
    const show = !!(file && file.type === "editor" && /\.frg$/i.test(file.filename || ""));
    const header = document.querySelector("#root header");

    if (runButton) runButton.remove();
    if (checkButton) checkButton.remove();

    if (!show || !header) return;

    header.insertBefore(checkButton, header.lastChild);
    header.insertBefore(runButton, header.lastChild);
  }

  async function init() {
    editorLanguages = acode.require("editorLanguages");
    if (!editorLanguages || typeof editorLanguages.register !== "function") {
      throw new Error("Acode editorLanguages API is not available.");
    }

    editorLanguages.register(
      LANGUAGE_ID,
      [EXTENSION],
      "FRIDGE",
      async function () {
        return [];
      }
    );
    registered = true;

    runButton = createActionButton("play_arrow", "Run FRIDGE program", "run");
    checkButton = createActionButton("check_circle", "Check FRIDGE program", "check");

    switchHandler = refreshButtons;
    renameHandler = refreshButtons;
    editorManager.on("switch-file", switchHandler);
    editorManager.on("rename-file", renameHandler);
    refreshButtons();

    // Also expose actions in Acode's command palette.
    try {
      commandsApi = acode.require("commands");
      commandsApi.addCommand({
        name: "fridge.run",
        description: "FRIDGE: Run current file",
        exec: () => { execute("run"); return true; }
      });
      commandsApi.addCommand({
        name: "fridge.check",
        description: "FRIDGE: Check current file",
        exec: () => { execute("check"); return true; }
      });
    } catch (error) {
      console.warn("[FRIDGE] Command palette registration unavailable:", error);
    }

    notify("FRIDGE is ready. Open a .frg file to see Run and Check buttons.", "success");
  }

  function destroy() {
    if (runButton) runButton.remove();
    if (checkButton) checkButton.remove();

    if (window.editorManager) {
      if (switchHandler) editorManager.off("switch-file", switchHandler);
      if (renameHandler) editorManager.off("rename-file", renameHandler);
    }

    if (commandsApi) {
      try {
        commandsApi.removeCommand("fridge.run");
        commandsApi.removeCommand("fridge.check");
      } catch (error) {
        console.warn("[FRIDGE] Command cleanup warning:", error);
      }
    }

    if (registered && editorLanguages && typeof editorLanguages.unregister === "function") {
      editorLanguages.unregister(LANGUAGE_ID);
    }

    editorLanguages = null;
    terminalApi = null;
    commandsApi = null;
    registered = false;
  }

  if (window.acode) {
    acode.setPluginInit(PLUGIN_ID, async function () {
      await init();
    });
    acode.setPluginUnmount(PLUGIN_ID, destroy);
  }
})();
