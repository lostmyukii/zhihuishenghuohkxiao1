(function (root, factory) {
  const api = factory();
  if (typeof module === "object" && module.exports) {
    module.exports = api;
  }
  root.SmartLifeVoiceIntent = api;
})(typeof globalThis !== "undefined" ? globalThis : window, function () {
  const quickIntents = [
    { label: "开始学习", text: "开始学习" },
    { label: "开启节能", text: "开启节能模式" },
    { label: "我要休息", text: "我要休息" },
    { label: "我要出门", text: "我要出门" },
    { label: "光线怎么样", text: "现在光线怎么样" },
  ];

  const rules = [
    {
      intent: "startStudy",
      action: "setMode",
      mode: "study",
      label: "切换到学习模式",
      patterns: ["开始学习", "学习模式", "我要学习", "进入学习"],
    },
    {
      intent: "startEnergy",
      action: "setMode",
      mode: "energy",
      label: "切换到节能模式",
      patterns: ["开启节能", "节能模式", "省电模式", "低碳模式"],
    },
    {
      intent: "startRest",
      action: "setMode",
      mode: "rest",
      label: "切换到休息模式",
      patterns: ["我要休息", "休息模式", "开始休息", "安静模式"],
    },
    {
      intent: "leaveHome",
      action: "setMode",
      mode: "away",
      label: "切换到离家模式",
      patterns: ["我要出门", "离家模式", "外出模式", "家里没人"],
    },
    {
      intent: "queryLight",
      action: "queryLight",
      label: "查询当前光照",
      patterns: ["光线怎么样", "光照怎么样", "现在亮不亮", "现在暗不暗", "查询光线"],
    },
  ];

  function normalizeVoiceText(text) {
    return String(text || "")
      .trim()
      .replace(/[，。！？、,.!?\s]/g, "")
      .toLowerCase();
  }

  function buildCommand(rule, originalText) {
    if (rule.action === "setMode") {
      return {
        type: "command",
        mode: rule.mode,
        source: "voiceText",
        intent: rule.intent,
        text: originalText,
      };
    }
    if (rule.action === "queryLight") {
      return {
        type: "voiceIntent",
        intent: "queryLight",
        source: "voiceText",
        text: originalText,
      };
    }
    return null;
  }

  function resolveVoiceIntent(text) {
    const originalText = String(text || "").trim();
    const normalized = normalizeVoiceText(originalText);
    if (!normalized) {
      return {
        ok: false,
        intent: "unknown",
        message: "请输入语音文字",
      };
    }

    const rule = rules.find((candidate) =>
      candidate.patterns.some((pattern) => normalized.includes(normalizeVoiceText(pattern))),
    );

    if (!rule) {
      return {
        ok: false,
        intent: "unknown",
        text: originalText,
        message: "未匹配到安全白名单意图",
      };
    }

    return {
      ok: true,
      intent: rule.intent,
      action: rule.action,
      label: rule.label,
      text: originalText,
      command: buildCommand(rule, originalText),
    };
  }

  return {
    quickIntents,
    resolveVoiceIntent,
    normalizeVoiceText,
  };
});
