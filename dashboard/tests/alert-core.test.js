#!/usr/bin/env node

const assert = require("assert/strict");
const {
  normalizeAlerts,
  sensorsForMode,
  describeAlert,
  actionMessages,
  buildPresentation,
} = require("../alert-core.js");

assert.deepEqual(sensorsForMode("study"), ["light", "temperature", "humidity", "sound"]);
assert.deepEqual(sensorsForMode("rest"), []);
assert.deepEqual(sensorsForMode("away"), ["pir"]);
assert.deepEqual(sensorsForMode("energy"), ["light", "pir"]);
assert.deepEqual(sensorsForMode("unknown"), []);

assert.deepEqual(normalizeAlerts(["noise", "noise", " intrusion ", ""]), ["noise", "intrusion"]);
assert.deepEqual(normalizeAlerts(null), []);

const noise = describeAlert("noise", {
  sensors: { sound: 72 },
  thresholds: { soundThreshold: 65 },
});
assert.equal(noise.severity, "reminder");
assert.equal(noise.sensorKey, "sound");
assert.match(noise.reason, /72%/);
assert.match(noise.reason, /65%/);
assert.equal(noise.source, "声音 · GPIO4");

const temperature = describeAlert("temperature", {
  sensors: { temperature: 30.2 },
  thresholds: { temperatureThreshold: 29 },
});
assert.equal(temperature.severity, "reminder");
assert.equal(temperature.sensorKey, "temperature");
assert.match(temperature.reason, /30\.2 C/);
assert.match(temperature.reason, /29\.0 C/);
assert.equal(temperature.source, "DHT11 · GPIO14");

const intrusion = describeAlert("intrusion");
assert.equal(intrusion.severity, "alarm");
assert.equal(intrusion.sensorKey, "pir");
assert.match(intrusion.reason, /离家模式/);
assert.equal(intrusion.source, "PIR · GPIO5");

assert.equal(describeAlert("mq2").source, "MQ-2 · GPIO2");
assert.equal(describeAlert("water").source, "水滴 · GPIO8");
assert.equal(describeAlert("flame").source, "火焰 · GPIO6");

const unknown = describeAlert("future-code");
assert.equal(unknown.severity, "alarm");
assert.match(unknown.reason, /future-code/);

const reminderPresentation = buildPresentation(["noise", "temperature"], {
  fresh: true,
  sensors: { sound: 72, temperature: 30.2 },
  thresholds: { soundThreshold: 65, temperatureThreshold: 29 },
  actuators: { buzzer: true },
});
assert.equal(reminderPresentation.state, "reminder");
assert.equal(reminderPresentation.title, "检测到 2 项提醒");
assert.deepEqual(reminderPresentation.sensorStates, { sound: "reminder", temperature: "reminder" });
assert.match(reminderPresentation.actions[0], /蜂鸣器正在提醒/);

const alarmPresentation = buildPresentation(["noise", "intrusion"], {
  fresh: true,
  actuators: { lamp: false, buzzer: true },
});
assert.equal(alarmPresentation.state, "alarm");
assert.equal(alarmPresentation.title, "检测到 1 项报警");
assert.deepEqual(alarmPresentation.sensorStates, { sound: "reminder", pir: "alarm" });
assert.equal(alarmPresentation.items.length, 2);
assert.match(alarmPresentation.actions.join(" "), /学习灯已关闭/);
assert.doesNotMatch(alarmPresentation.actions.join(" "), /风扇|RGB|舵机/);

const optionalActions = actionMessages(["mq2", "water"], { actuators: { buzzer: false } });
assert.equal(optionalActions.length, 1);
assert.match(optionalActions[0], /网页正在显示安全报警/);
assert.match(optionalActions[0], /蜂鸣器当前静音/);
assert.doesNotMatch(optionalActions[0], /风扇|RGB|舵机/);

const normalPresentation = buildPresentation([], { fresh: true });
assert.equal(normalPresentation.state, "normal");
assert.equal(normalPresentation.title, "当前无报警或提醒");

const offlinePresentation = buildPresentation(["intrusion"], { fresh: false });
assert.equal(offlinePresentation.state, "idle");
assert.equal(offlinePresentation.title, "等待实时数据");
assert.deepEqual(offlinePresentation.items, []);

console.log("alert core tests passed");
