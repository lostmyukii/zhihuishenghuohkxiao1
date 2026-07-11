(function (root, factory) {
  const api = factory();
  if (typeof module === "object" && module.exports) {
    module.exports = api;
  }
  root.SmartLifeAlertCore = api;
})(typeof globalThis !== "undefined" ? globalThis : window, function () {
  const MODE_SENSOR_KEYS = {
    study: ["light", "temperature", "humidity", "sound"],
    rest: [],
    away: ["pir"],
    energy: ["light", "pir"],
  };

  const ALERT_SENSOR_KEYS = {
    noise: "sound",
    temperature: "temperature",
    intrusion: "pir",
  };

  const SAFETY_CODES = new Set(["intrusion", "mq2", "gas", "water", "flame"]);
  const REMINDER_CODES = new Set(["noise", "temperature"]);

  function normalizeAlerts(alerts) {
    if (!Array.isArray(alerts)) return [];
    return [...new Set(alerts.map((alert) => String(alert || "").trim()).filter(Boolean))];
  }

  function sensorsForMode(mode) {
    return [...(MODE_SENSOR_KEYS[String(mode || "")] || [])];
  }

  function numericValue(value) {
    if (value === null || value === undefined || value === "") return null;
    const numeric = Number(value);
    return Number.isFinite(numeric) ? numeric : null;
  }

  function formatValue(value, suffix, decimals = 0) {
    const numeric = numericValue(value);
    if (numeric === null) return null;
    return `${numeric.toFixed(decimals)}${suffix}`;
  }

  function thresholdReason(prefix, value, threshold, suffix, decimals, fallback) {
    const currentText = formatValue(value, suffix, decimals);
    const thresholdText = formatValue(threshold, suffix, decimals);
    if (!currentText || !thresholdText) return fallback;
    return `${prefix}当前 ${currentText}，达到或超过页面阈值 ${thresholdText}。`;
  }

  function describeAlert(code, context = {}) {
    const normalized = String(code || "unknown").trim() || "unknown";
    const sensors = context.sensors || {};
    const thresholds = context.thresholds || {};

    if (normalized === "noise") {
      return {
        code: normalized,
        sensorKey: "sound",
        severity: "reminder",
        title: "学习噪声提醒",
        reason: thresholdReason(
          "学习模式环境声音",
          sensors.sound,
          thresholds.soundThreshold,
          "%",
          0,
          "学习模式环境声音达到或超过提醒阈值。",
        ),
        source: "声音 · GPIO4",
      };
    }

    if (normalized === "temperature") {
      return {
        code: normalized,
        sensorKey: "temperature",
        severity: "reminder",
        title: "学习温度提醒",
        reason: thresholdReason(
          "学习模式温度",
          sensors.temperature,
          thresholds.temperatureThreshold,
          " C",
          1,
          "学习模式温度达到或超过舒适阈值。",
        ),
        source: "DHT11 · GPIO14",
      };
    }

    if (normalized === "intrusion") {
      return {
        code: normalized,
        sensorKey: "pir",
        severity: "alarm",
        title: "离家人体报警",
        reason: "离家模式下，玄关 PIR 检测到人体活动。",
        source: "PIR · GPIO5",
      };
    }

    if (normalized === "mq2" || normalized === "gas") {
      return {
        code: normalized,
        sensorKey: "",
        severity: "alarm",
        title: "烟雾／燃气报警",
        reason: thresholdReason(
          "可选 MQ-2",
          sensors.mq2,
          thresholds.mq2Threshold,
          "%",
          0,
          "可选 MQ-2 烟雾／燃气传感器上报安全异常。",
        ),
        source: "MQ-2 · GPIO2",
      };
    }

    if (normalized === "water") {
      return {
        code: normalized,
        sensorKey: "",
        severity: "alarm",
        title: "漏水报警",
        reason: "可选水滴传感器检测到漏水。",
        source: "水滴 · GPIO8",
      };
    }

    if (normalized === "flame") {
      return {
        code: normalized,
        sensorKey: "",
        severity: "alarm",
        title: "火源报警",
        reason: "可选火焰传感器检测到火源信号。",
        source: "火焰 · GPIO6",
      };
    }

    return {
      code: normalized,
      sensorKey: ALERT_SENSOR_KEYS[normalized] || "",
      severity: SAFETY_CODES.has(normalized) ? "alarm" : REMINDER_CODES.has(normalized) ? "reminder" : "alarm",
      title: "设备异常提醒",
      reason: `设备上报异常：${normalized}。`,
      source: "N16R8 实时数据",
    };
  }

  function actionMessages(alerts, context = {}) {
    const codes = normalizeAlerts(alerts);
    const actuators = context.actuators || {};
    const messages = [];
    const buzzerText = actuators.buzzer === true ? "蜂鸣器正在提醒" : "蜂鸣器当前静音";

    if (codes.some((code) => code === "noise" || code === "temperature")) {
      messages.push(`学习舒适提醒：${buzzerText}，OLED/网页同步显示。`);
    }

    if (codes.includes("intrusion")) {
      const lampText = actuators.lamp === false ? "学习灯已关闭" : "学习灯状态以实时数据为准";
      const alarmBuzzerText = actuators.buzzer === true ? "蜂鸣器正在报警" : "蜂鸣器当前静音";
      messages.push(`离家安防联动：${lampText}，${alarmBuzzerText}，OLED/网页同步显示。`);
    }

    if (codes.some((code) => ["mq2", "gas", "water", "flame"].includes(code))) {
      const safetyBuzzerText = actuators.buzzer === true ? "蜂鸣器正在报警" : "蜂鸣器当前静音";
      messages.push(`可选安全扩展：网页正在显示安全报警，${safetyBuzzerText}。`);
    }

    return messages;
  }

  function buildPresentation(alerts, context = {}) {
    if (context.fresh === false) {
      return {
        state: "idle",
        title: "等待实时数据",
        summary: "连接开发板后显示报警与提醒原因",
        items: [],
        actions: [],
        sensorStates: {},
      };
    }

    const normalized = normalizeAlerts(alerts);
    const items = normalized.map((code) => describeAlert(code, context));
    const alarmCount = items.filter((item) => item.severity === "alarm").length;
    const reminderCount = items.filter((item) => item.severity === "reminder").length;
    const state = alarmCount > 0 ? "alarm" : reminderCount > 0 ? "reminder" : "normal";
    const sensorStates = {};

    items.forEach((item) => {
      if (!item.sensorKey) return;
      if (!sensorStates[item.sensorKey] || item.severity === "alarm") {
        sensorStates[item.sensorKey] = item.severity;
      }
    });

    let title = "当前无报警或提醒";
    let summary = "核心传感器持续监测";
    if (state === "alarm") {
      title = `检测到 ${alarmCount} 项报警`;
      summary = items.map((item) => item.title).join("、");
    } else if (state === "reminder") {
      title = `检测到 ${reminderCount} 项提醒`;
      summary = items.map((item) => item.title).join("、");
    }

    return {
      state,
      title,
      summary,
      items,
      actions: actionMessages(normalized, context),
      sensorStates,
    };
  }

  return {
    MODE_SENSOR_KEYS,
    ALERT_SENSOR_KEYS,
    normalizeAlerts,
    sensorsForMode,
    describeAlert,
    actionMessages,
    buildPresentation,
  };
});
