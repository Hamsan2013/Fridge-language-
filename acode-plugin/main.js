/*
 * FRIDGE Language Support for Acode
 * Registers the .frg file extension with Acode's editor language system.
 */

(function () {
  "use strict";

  const PLUGIN_ID = "com.hamsan.fridge";
  const LANGUAGE_ID = "fridge";

  let editorLanguages = null;
  let registered = false;

  function notify(message, type) {
    if (window.acode && typeof acode.pushNotification === "function") {
      acode.pushNotification("FRIDGE", message, {
        type: type || "info",
        autoClose: true
      });
    }
  }

  async function init() {
    editorLanguages = acode.require("editorLanguages");

    if (!editorLanguages || typeof editorLanguages.register !== "function") {
      throw new Error("Acode editorLanguages API is not available.");
    }

    editorLanguages.register(
      LANGUAGE_ID,
      ["frg"],
      "FRIDGE",
      async function () {
        // A valid empty CodeMirror extension keeps FRIDGE files associated
        // with their own language mode. Token highlighting can be added next.
        return [];
      }
    );

    registered = true;
    notify("FRIDGE file support is enabled. Open a .frg file to use it.", "success");
  }

  function destroy() {
    if (registered && editorLanguages &&
        typeof editorLanguages.unregister === "function") {
      editorLanguages.unregister(LANGUAGE_ID);
    }

    registered = false;
    editorLanguages = null;
  }

  if (window.acode) {
    acode.setPluginInit(PLUGIN_ID, async function () {
      await init();
    });

    acode.setPluginUnmount(PLUGIN_ID, function () {
      destroy();
    });
  }
})();
