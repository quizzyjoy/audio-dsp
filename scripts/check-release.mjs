#!/usr/bin/env node
/**
 * Release consistency checks that do not require the HarmonyOS SDK.
 *
 *   node scripts/check-release.mjs
 *
 * Verifies:
 *  1. library / root / entry / AppScope carry the same version
 *  2. the package root contains the required OHPM files
 *  3. README contains the official install command
 *  4. CHANGELOG has an entry for the current version
 */
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const here = path.dirname(fileURLToPath(import.meta.url));
const root = path.join(here, '..');
const harmony = path.join(root, 'harmony');
const library = path.join(harmony, 'library');

const failures = [];
const checks = [];

function readJson5Loose(file) {
  const text = fs.readFileSync(file, 'utf8');
  // These manifests are JSON5, but the only extension we use is trailing commas.
  return JSON.parse(text.replace(/,\s*([}\]])/g, '$1'));
}

function check(name, fn) {
  try {
    fn();
    checks.push(`ok   ${name}`);
  } catch (error) {
    failures.push(`${name}: ${error.message}`);
    checks.push(`FAIL ${name}`);
  }
}

const libraryManifest = readJson5Loose(path.join(library, 'oh-package.json5'));
const rootManifest = readJson5Loose(path.join(harmony, 'oh-package.json5'));
const entryManifest = readJson5Loose(path.join(harmony, 'entry', 'oh-package.json5'));
const appScope = readJson5Loose(path.join(harmony, 'AppScope', 'app.json5'));
const version = libraryManifest.version;

check('package name is scoped', () => {
  if (!libraryManifest.name.startsWith('@quizzyjoy/')) {
    throw new Error(`unexpected name ${libraryManifest.name}`);
  }
});

check('versions are in sync', () => {
  const pairs = [
    ['harmony/oh-package.json5', rootManifest.version],
    ['harmony/entry/oh-package.json5', entryManifest.version],
    ['harmony/AppScope/app.json5 versionName', appScope.app.versionName]
  ];
  for (const [file, value] of pairs) {
    if (value !== version) throw new Error(`${file} is ${value}, expected ${version}`);
  }
});

check('required package files exist', () => {
  const required = ['Index.ets', 'README.md', 'CHANGELOG.md', 'LICENSE', 'THIRD_PARTY_NOTICES.md', 'oh-package.json5'];
  for (const file of required) {
    const target = path.join(library, file);
    if (!fs.existsSync(target) || fs.statSync(target).size === 0) {
      throw new Error(`missing or empty: library/${file}`);
    }
  }
});

check('README has the install command', () => {
  const readme = fs.readFileSync(path.join(library, 'README.md'), 'utf8');
  const wanted = `ohpm install ${libraryManifest.name}`;
  if (!readme.includes(wanted)) throw new Error(`README must contain "${wanted}"`);
});

check('README covers the nine sections', () => {
  const readme = fs.readFileSync(path.join(library, 'README.md'), 'utf8');
  const sections = ['安装', '所需权限', '接口', '示例', '约束与限制', '目录结构', '贡献', '许可'];
  const missing = sections.filter((section) => !readme.includes(section));
  if (missing.length > 0) throw new Error(`missing sections: ${missing.join(', ')}`);
});

check('CHANGELOG mentions the current version', () => {
  const changelog = fs.readFileSync(path.join(library, 'CHANGELOG.md'), 'utf8');
  if (!changelog.includes(version)) throw new Error(`CHANGELOG has no entry for ${version}`);
});

check('native sources are present', () => {
  const cpp = path.join(library, 'src', 'main', 'cpp');
  const files = ['CMakeLists.txt', 'audio_dsp.cpp', 'audio_dsp.h', 'napi_init.cpp', 'third_party/kissfft/kiss_fft.c'];
  for (const file of files) {
    if (!fs.existsSync(path.join(cpp, file))) throw new Error(`missing cpp/${file}`);
  }
});

console.log(checks.join('\n'));
if (failures.length > 0) {
  console.error('\ncheck-release failed:');
  for (const failure of failures) console.error(' - ' + failure);
  process.exit(1);
}
console.log(`\nall checks passed for ${libraryManifest.name}@${version}`);
