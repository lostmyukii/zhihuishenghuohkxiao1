#!/usr/bin/env node

const assert = require("assert/strict");
const { quickIntents, resolveVoiceIntent, normalizeVoiceText } = require("../intent-core.js");

function assertMode(text, mode, intent) {
  const result = resolveVoiceIntent(text);
  assert.equal(result.ok, true);
  assert.equal(result.intent, intent);
  assert.deepEqual(result.command.type, "command");
  assert.equal(result.command.mode, mode);
  assert.equal(result.command.source, "voiceText");
}

assert.equal(normalizeVoiceText(" 开启节能！"), "开启节能");
assert.equal(quickIntents.length >= 5, true);

assertMode("开始学习", "study", "startStudy");
assertMode("请进入学习模式", "study", "startStudy");
assertMode("开启节能模式", "energy", "startEnergy");
assertMode("我要休息一下", "rest", "startRest");
assertMode("我要出门", "away", "leaveHome");

const queryLight = resolveVoiceIntent("现在光线怎么样");
assert.equal(queryLight.ok, true);
assert.equal(queryLight.command.type, "voiceIntent");
assert.equal(queryLight.command.intent, "queryLight");

const unsafe = resolveVoiceIntent("打开煤气并持续加热");
assert.equal(unsafe.ok, false);
assert.equal(unsafe.intent, "unknown");

const empty = resolveVoiceIntent("   ");
assert.equal(empty.ok, false);

console.log("voice intent tests passed");
