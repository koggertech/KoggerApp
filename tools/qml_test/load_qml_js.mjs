// Loads a QML .js logic file into Node.
//
// Why this exists: `.pragma library` / `.import` are QML directives, not JavaScript. A leading
// `.pragma library` makes `require()` fail with `SyntaxError: Unexpected token '.'`, so the
// suites that require()'d such a file aborted on load and silently stopped testing anything.
// Dropping the pragma is not an option either — without it CMake warns and the script is
// re-evaluated in every QML document that imports the module.
//
// So: strip the directives, evaluate the rest. Directive lines are blanked rather than removed,
// and lineOffset compensates for the wrapper, so stack traces keep the file's real line numbers.

import fs from "node:fs";
import path from "node:path";
import vm from "node:vm";
import { fileURLToPath } from "node:url";

const QML_DIRECTIVE = /^\s*\.(?:pragma|import)\b/;

export function loadQmlJs(fromUrl, ...segments) {
    const here = path.dirname(fileURLToPath(fromUrl));
    const file = path.resolve(here, ...segments);
    const source = fs.readFileSync(file, "utf8");
    const body = source.split(/\r?\n/)
                       .map((line) => (QML_DIRECTIVE.test(line) ? "" : line))
                       .join("\n");

    const module = { exports: {} };
    const script = new vm.Script(`(function (module, exports) {\n${body}\n})`,
                                 { filename: file, lineOffset: -1 });
    script.runInThisContext()(module, module.exports);

    if (Object.keys(module.exports).length === 0) {
        throw new Error(`${path.basename(file)} exported nothing — is its module.exports block missing?`);
    }
    return module.exports;
}

export function loadAppJs(fromUrl, name) {
    return loadQmlJs(fromUrl, "..", "..", "qml", "app", name);
}

export function loadTypesJs(fromUrl, name) {
    return loadQmlJs(fromUrl, "..", "..", "qml", "kqml_types", name);
}
