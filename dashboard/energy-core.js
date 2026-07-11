(function (root, factory) {
  const api = factory();
  if (typeof module === "object" && module.exports) {
    module.exports = api;
  }
  root.SmartLifeEnergy = api;
})(typeof globalThis !== "undefined" ? globalThis : window, function () {
  const WAITING_TEXT = "等待实时数据";
  const UNKNOWN_REASON_TEXT = "设备正在执行节能规则";

  const reasonLabels = {
    "empty-room-light-off": "无人，已自动关闭学习灯",
    "energy-mode-balances-light-and-comfort": "节能模式正在根据光照和人体状态控制学习灯",
    "study-mode-keeps-comfort-when-needed": "学习模式按需保持照明与舒适提醒",
    "scene-rules-active": "当前模式的智能规则正在运行",
    "mock-board-frame-for-dashboard-validation": "模拟开发板正在验证节能状态显示",
  };

  function containsChinese(text) {
    return /[\u3400-\u9fff]/.test(text);
  }

  function describeEnergyReason(reason) {
    const normalized = String(reason || "").trim();
    if (!normalized) return WAITING_TEXT;
    if (reasonLabels[normalized]) return reasonLabels[normalized];
    if (containsChinese(normalized)) return normalized;
    return UNKNOWN_REASON_TEXT;
  }

  function formatEnergyScore(score) {
    if (score === null || score === undefined || score === "") return WAITING_TEXT;
    const numericScore = Number(score);
    if (!Number.isFinite(numericScore) || numericScore < 0 || numericScore > 100) {
      return WAITING_TEXT;
    }
    return `${Math.round(numericScore)} 分`;
  }

  return {
    WAITING_TEXT,
    UNKNOWN_REASON_TEXT,
    describeEnergyReason,
    formatEnergyScore,
  };
});
