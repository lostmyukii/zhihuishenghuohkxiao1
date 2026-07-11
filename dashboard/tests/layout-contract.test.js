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
  "alertNotice",
  "alertNoticeTitle",
  "alertNoticeSummary",
  "alertNoticeItems",
  "alertNoticeActions",
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
assert.match(html, /style\.css\?v=20260712-ios-mode-alerts/);
assert.match(html, /alert-core\.js\?v=20260712-ios-mode-alerts/);
assert.match(html, /app\.js\?v=20260712-ios-mode-alerts/);
assert.equal(html.indexOf("alert-core.js") < html.indexOf("app.js"), true, "alert-core.js must load before app.js");
assert.doesNotMatch(html, /风扇|RGB|舵机|8键AD/);

const sensorKeys = [...html.matchAll(/\bdata-sensor-key="([^"]+)"/g)].map((match) => match[1]);
assert.deepEqual(sensorKeys, ["light", "temperature", "humidity", "sound", "pir"]);
assert.equal((html.match(/class="sensor-icon"/g) || []).length, 5);
assert.equal((html.match(/class="sensor-state-badge"/g) || []).length, 5);
assert.match(html, /role="status" aria-live="polite" aria-atomic="true"/);

assert.match(css, /grid-template-columns:\s*minmax\(0, 1\.55fr\)/);
assert.match(css, /@media \(max-width: 760px\)/);
assert.match(css, /display:\s*contents/);
assert.match(css, /\.sensor-card\.is-mode-active/);
assert.match(css, /\.sensor-card\.is-reminder/);
assert.match(css, /\.sensor-card\.is-alarm/);
assert.doesNotMatch(css, /radial-gradient|linear-gradient/);

console.log("layout contract tests passed");
