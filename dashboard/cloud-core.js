(function (root, factory) {
  const api = factory();
  if (typeof module === "object" && module.exports) {
    module.exports = api;
  }
  root.SmartLifeCloudCore = api;
})(typeof globalThis !== "undefined" ? globalThis : window, function () {
  const BOARD_FRAME_TYPES = new Set(["hello", "telemetry", "health", "event", "ack"]);
  const COMMAND_TYPES = new Set(["command", "config", "voiceIntent"]);
  const TRANSPORT_META_KEYS = new Set([
    "origin",
    "originClientId",
    "source",
    "mqttTopic",
    "relayedAt",
    "usbWritten",
  ]);

  function queryParams(search) {
    return new URLSearchParams(String(search || "").replace(/^\?/, ""));
  }

  function defaultEndpoint(locationLike, search) {
    const params = queryParams(search);
    const explicit = params.get("ws");
    if (explicit) return explicit;

    const hostname = String(locationLike?.hostname || "");
    const isLocal = !hostname || hostname === "localhost" || hostname === "127.0.0.1" || hostname.endsWith(".local");
    if (isLocal) return "";

    const protocol = locationLike?.protocol === "https:" ? "wss:" : "ws:";
    const host = String(locationLike?.host || hostname);
    return `${protocol}//${host}/smartlife-primary-ws`;
  }

  function clientId(randomUuid) {
    if (typeof randomUuid === "function") {
      const generated = randomUuid();
      if (generated) return generated;
    }
    return `primary-${Date.now()}-${Math.random().toString(16).slice(2)}`;
  }

  function classifyPayload(payload) {
    if (!payload || typeof payload !== "object") return "unknown";
    if (payload.type === "relayStatus") return "status";
    if (payload.type === "ping") return "ping";
    if (BOARD_FRAME_TYPES.has(payload.type)) return "board";
    if (COMMAND_TYPES.has(payload.type)) return "command";
    return "unknown";
  }

  function decoratePayload(payload, currentClientId, origin) {
    return {
      ...payload,
      project: payload.project || "smartlife-primary",
      origin: origin || payload.origin || "dashboard",
      originClientId: currentClientId,
    };
  }

  function shouldIgnore(payload, currentClientId) {
    return Boolean(payload?.originClientId && payload.originClientId === currentClientId);
  }

  function stripTransportMeta(payload) {
    return Object.fromEntries(
      Object.entries(payload || {}).filter(([key]) => !TRANSPORT_META_KEYS.has(key) && !key.startsWith("_")),
    );
  }

  return {
    BOARD_FRAME_TYPES,
    COMMAND_TYPES,
    TRANSPORT_META_KEYS,
    defaultEndpoint,
    clientId,
    classifyPayload,
    decoratePayload,
    shouldIgnore,
    stripTransportMeta,
  };
});
