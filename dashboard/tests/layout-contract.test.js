#!/usr/bin/env node

const assert = require("assert/strict");
const fs = require("fs");
const path = require("path");

const dashboardDir = path.resolve(__dirname, "..");
const html = fs.readFileSync(path.join(dashboardDir, "index.html"), "utf8");
const css = fs.readFileSync(path.join(dashboardDir, "style.css"), "utf8");

const ids = [...html.matchAll(/\bid="([^"]+)"/g)].map((match) => match[1]);
assert.equal(new Set(ids).size, ids.length, "Dashboard IDs must be unique");

const requiredIds = [
  "serialStatus",
  "boardStatus",
  "frameStatus",
  "modeChip",
  "connectButton",
  "disconnectButton",
  "ackStatus",
  "thresholdForm",
  "lightValue",
  "temperatureValue",
  "humidityValue",
  "soundValue",
  "pirValue",
  "lampValue",
  "buzzerValue",
  "energyScore",
  "energyReason",
  "voiceQuickGrid",
  "voiceForm",
  "oledLines",
  "eventLog",
];

requiredIds.forEach((id) => {
  assert.equal(ids.includes(id), true, `Missing required Dashboard ID: ${id}`);
});

assert.match(html, /class="dashboard-columns"/);
assert.match(html, /class="dashboard-left"/);
assert.match(html, /class="student-console"/);
assert.match(html, /style\.css\?v=20260712-two-column-ui/);
assert.match(html, /app\.js\?v=20260712-two-column-ui/);
assert.doesNotMatch(html, /风扇|RGB|舵机|8键AD/);

assert.match(css, /grid-template-columns:\s*minmax\(0, 1\.55fr\)/);
assert.match(css, /@media \(max-width: 760px\)/);
assert.match(css, /display:\s*contents/);
assert.doesNotMatch(css, /radial-gradient|linear-gradient/);

console.log("layout contract tests passed");
