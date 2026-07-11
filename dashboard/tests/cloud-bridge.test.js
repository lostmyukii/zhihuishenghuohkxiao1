#!/usr/bin/env node

const assert = require("assert/strict");
const cloud = require("../cloud-core.js");

assert.equal(
  cloud.defaultEndpoint(
    { protocol: "https:", hostname: "hongkongxiao.ilelezhan.cn", host: "hongkongxiao.ilelezhan.cn" },
    "",
  ),
  "wss://hongkongxiao.ilelezhan.cn/smartlife-primary-ws",
);
assert.equal(cloud.defaultEndpoint({ protocol: "http:", hostname: "127.0.0.1", host: "127.0.0.1:19168" }, ""), "");
assert.equal(
  cloud.defaultEndpoint(
    { protocol: "http:", hostname: "127.0.0.1", host: "127.0.0.1:19168" },
    "?ws=ws%3A%2F%2F127.0.0.1%3A19266",
  ),
  "ws://127.0.0.1:19266",
);

assert.equal(cloud.classifyPayload({ type: "telemetry" }), "board");
assert.equal(cloud.classifyPayload({ type: "command" }), "command");
assert.equal(cloud.classifyPayload({ type: "relayStatus" }), "status");
assert.equal(cloud.classifyPayload({ type: "demo" }), "unknown");

const decorated = cloud.decoratePayload({ type: "telemetry", sensors: {} }, "client-1", "web-serial-gateway");
assert.equal(decorated.project, "smartlife-primary");
assert.equal(decorated.origin, "web-serial-gateway");
assert.equal(decorated.originClientId, "client-1");
assert.equal(cloud.shouldIgnore(decorated, "client-1"), true);
assert.equal(cloud.shouldIgnore(decorated, "client-2"), false);

assert.deepEqual(
  cloud.stripTransportMeta({
    type: "command",
    mode: "energy",
    origin: "dashboard",
    originClientId: "client-1",
    mqttTopic: "smartlife/primary/n16r8/command",
    _internal: true,
  }),
  { type: "command", mode: "energy" },
);

console.log("cloud bridge tests passed");
