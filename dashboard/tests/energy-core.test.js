#!/usr/bin/env node

const assert = require("assert/strict");
const {
  WAITING_TEXT,
  UNKNOWN_REASON_TEXT,
  describeEnergyReason,
  formatEnergyScore,
} = require("../energy-core.js");

assert.equal(describeEnergyReason("empty-room-light-off"), "无人，已自动关闭学习灯");
assert.equal(
  describeEnergyReason("energy-mode-balances-light-and-comfort"),
  "节能模式正在根据光照和人体状态控制学习灯",
);
assert.equal(
  describeEnergyReason("study-mode-keeps-comfort-when-needed"),
  "学习模式按需保持照明与舒适提醒",
);
assert.equal(describeEnergyReason("scene-rules-active"), "当前模式的智能规则正在运行");
assert.equal(describeEnergyReason("无人，学习灯已关闭"), "无人，学习灯已关闭");
assert.equal(describeEnergyReason("future-energy-rule"), UNKNOWN_REASON_TEXT);
assert.equal(describeEnergyReason(null), WAITING_TEXT);

assert.equal(formatEnergyScore(90), "90 分");
assert.equal(formatEnergyScore("82.4"), "82 分");
assert.equal(formatEnergyScore(0), "0 分");
assert.equal(formatEnergyScore(null), WAITING_TEXT);
assert.equal(formatEnergyScore(101), WAITING_TEXT);
assert.equal(formatEnergyScore("not-a-number"), WAITING_TEXT);

console.log("energy core tests passed");
