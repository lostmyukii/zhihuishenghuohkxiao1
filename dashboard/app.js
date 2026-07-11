(function () {
  const FRESH_FRAME_MS = 3500;
  const MAX_LOGS = 18;
  const MODE_LABELS = {
    study: "学习模式",
    rest: "休息模式",
    away: "离家模式",
    energy: "节能模式",
  };

  const state = {
    port: null,
    reader: null,
    writer: null,
    connected: false,
    readBuffer: "",
    frameCount: 0,
    lastFrameAt: 0,
    hello: null,
    telemetry: null,
    lastAck: null,
    lastVoiceIntent: null,
    logs: [],
  };

  const voiceIntentApi = window.SmartLifeVoiceIntent;
  const energyApi = window.SmartLifeEnergy;

  const el = {
    serialStatus: document.querySelector("#serialStatus"),
    boardStatus: document.querySelector("#boardStatus"),
    frameStatus: document.querySelector("#frameStatus"),
    baudRate: document.querySelector("#baudRate"),
    connectButton: document.querySelector("#connectButton"),
    disconnectButton: document.querySelector("#disconnectButton"),
    deviceName: document.querySelector("#deviceName"),
    firmwareVersion: document.querySelector("#firmwareVersion"),
    lastFrameAt: document.querySelector("#lastFrameAt"),
    supportNotice: document.querySelector("#supportNotice"),
    modeChip: document.querySelector("#modeChip"),
    ackStatus: document.querySelector("#ackStatus"),
    thresholdSummary: document.querySelector("#thresholdSummary"),
    oledState: document.querySelector("#oledState"),
    oledLines: document.querySelector("#oledLines"),
    thresholdForm: document.querySelector("#thresholdForm"),
    eventLog: document.querySelector("#eventLog"),
    clearLogButton: document.querySelector("#clearLogButton"),
    voiceStatus: document.querySelector("#voiceStatus"),
    voiceQuickGrid: document.querySelector("#voiceQuickGrid"),
    voiceForm: document.querySelector("#voiceForm"),
    voiceTextInput: document.querySelector("#voiceTextInput"),
    voiceResult: document.querySelector("#voiceResult"),
    lampOnButton: document.querySelector("#lampOnButton"),
    lampOffButton: document.querySelector("#lampOffButton"),
    energyScore: document.querySelector("#energyScore"),
    energyReason: document.querySelector("#energyReason"),
    studyRoomState: document.querySelector("#studyRoomState"),
    livingRoomState: document.querySelector("#livingRoomState"),
    entryRoomState: document.querySelector("#entryRoomState"),
    metrics: {
      light: document.querySelector("#lightValue"),
      temperature: document.querySelector("#temperatureValue"),
      humidity: document.querySelector("#humidityValue"),
      sound: document.querySelector("#soundValue"),
      pir: document.querySelector("#pirValue"),
      lamp: document.querySelector("#lampValue"),
      buzzer: document.querySelector("#buzzerValue"),
    },
  };

  function isSerialSupported() {
    return "serial" in navigator;
  }

  function isFreshOnline() {
    return state.connected && state.lastFrameAt > 0 && Date.now() - state.lastFrameAt <= FRESH_FRAME_MS;
  }

  function formatTime(timestamp) {
    if (!timestamp) return "无";
    return new Intl.DateTimeFormat("zh-CN", {
      hour: "2-digit",
      minute: "2-digit",
      second: "2-digit",
    }).format(timestamp);
  }

  function valueOrWaiting(value, suffix = "") {
    if (value === null || value === undefined || Number.isNaN(value)) return "等待";
    return `${value}${suffix}`;
  }

  function booleanText(value, trueText, falseText) {
    if (value === null || value === undefined) return "等待";
    return value ? trueText : falseText;
  }

  function addLog(kind, message) {
    const entry = {
      at: Date.now(),
      kind,
      message,
    };
    state.logs.unshift(entry);
    state.logs = state.logs.slice(0, MAX_LOGS);
    renderLog();
  }

  function renderLog() {
    el.eventLog.innerHTML = "";
    if (state.logs.length === 0) {
      const empty = document.createElement("li");
      empty.className = "muted-log";
      empty.textContent = "等待串口事件";
      el.eventLog.appendChild(empty);
      return;
    }

    state.logs.forEach((entry) => {
      const item = document.createElement("li");
      item.className = `log-${entry.kind}`;
      item.innerHTML = `<time>${formatTime(entry.at)}</time><span>${entry.message}</span>`;
      el.eventLog.appendChild(item);
    });
  }

  function setPill(element, text, status) {
    element.textContent = text;
    element.dataset.status = status;
  }

  function updateCommandButtons() {
    document.querySelectorAll("[data-mode]").forEach((button) => {
      button.disabled = !state.connected;
    });
    el.thresholdForm.querySelectorAll("button, input").forEach((control) => {
      control.disabled = !state.connected;
    });
    el.voiceForm.querySelectorAll("button, input").forEach((control) => {
      control.disabled = !state.connected;
    });
    el.voiceQuickGrid.querySelectorAll("button").forEach((button) => {
      button.disabled = !state.connected;
    });
    el.lampOnButton.disabled = !state.connected;
    el.lampOffButton.disabled = !state.connected;
  }

  function renderStatus() {
    const fresh = isFreshOnline();
    setPill(el.serialStatus, state.connected ? "串口已连接" : "串口未连接", state.connected ? "ok" : "idle");
    setPill(el.boardStatus, fresh ? "开发板在线" : "开发板离线", fresh ? "ok" : "offline");
    setPill(el.frameStatus, `${state.frameCount} 帧`, state.frameCount > 0 ? "ok" : "idle");

    el.connectButton.disabled = state.connected || !isSerialSupported();
    el.disconnectButton.disabled = !state.connected;
    el.lastFrameAt.textContent = formatTime(state.lastFrameAt);
    el.supportNotice.dataset.status = isSerialSupported() ? "ok" : "offline";
    el.supportNotice.textContent = isSerialSupported()
      ? "Chrome 或 Edge 在 HTTPS/localhost 页面中可使用 Web Serial。"
      : "当前浏览器不支持 Web Serial，请使用 Chrome 或 Edge。";

    updateCommandButtons();
  }

  function renderHello() {
    const hello = state.hello;
    el.deviceName.textContent = hello?.deviceName || hello?.board || "等待 hello";
    el.firmwareVersion.textContent = hello?.firmware || "等待 hello";
    el.baudRate.textContent = `${hello?.baud || 115200}`;
  }

  function renderOledLines(lines) {
    const rows = Array.isArray(lines) && lines.length > 0 ? lines.slice(0, 6) : ["等待 OLED 状态"];
    el.oledLines.innerHTML = "";
    rows.forEach((line) => {
      const row = document.createElement("span");
      row.textContent = line || " ";
      el.oledLines.appendChild(row);
    });
  }

  function renderTelemetry() {
    const telemetry = isFreshOnline() ? state.telemetry : null;
    const sensors = telemetry?.sensors || {};
    const actuators = telemetry?.actuators || {};
    const health = telemetry?.health || {};
    const display = telemetry?.display || {};

    const activeMode = telemetry?.mode || null;
    el.modeChip.textContent = activeMode ? MODE_LABELS[activeMode] || activeMode : "等待模式";
    el.modeChip.dataset.status = activeMode ? "active" : "idle";
    document.querySelectorAll("[data-mode]").forEach((button) => {
      const isActive = activeMode === button.dataset.mode;
      button.dataset.active = String(isActive);
      button.setAttribute("aria-pressed", String(isActive));
    });
    el.thresholdSummary.textContent = telemetry
      ? `光 ${valueOrWaiting(sensors.light, "%")} / 噪 ${valueOrWaiting(sensors.sound, "%")}`
      : "等待阈值";
    el.oledState.textContent =
      health.oled === "ready" ? "OLED 已连接" : health.oled === "missing" ? "OLED 未检测" : "等待 OLED";
    el.oledState.dataset.status = health.oled === "ready" ? "ok" : health.oled === "missing" ? "offline" : "idle";
    renderOledLines(display.lines);

    el.metrics.light.textContent = valueOrWaiting(sensors.light, "%");
    el.metrics.temperature.textContent = valueOrWaiting(sensors.temperature, " C");
    el.metrics.humidity.textContent = valueOrWaiting(sensors.humidity, "%");
    el.metrics.sound.textContent = valueOrWaiting(sensors.sound, "%");
    el.metrics.pir.textContent = booleanText(sensors.pir, "有人", "无人");

    el.metrics.lamp.textContent = booleanText(actuators.lamp, "开启", "关闭");
    el.metrics.buzzer.textContent = booleanText(actuators.buzzer, "响铃", "静音");

    const freshEnergy = telemetry?.energy || null;
    el.energyScore.textContent = freshEnergy && energyApi
      ? energyApi.formatEnergyScore(freshEnergy.score)
      : "等待实时数据";
    el.energyReason.textContent = freshEnergy && energyApi
      ? energyApi.describeEnergyReason(freshEnergy.reason)
      : "等待实时数据";

    el.studyRoomState.textContent = telemetry
      ? `光照 ${valueOrWaiting(sensors.light, "%")} / 噪声 ${valueOrWaiting(sensors.sound, "%")}`
      : "等待数据";
    el.livingRoomState.textContent = telemetry
      ? `温度 ${valueOrWaiting(sensors.temperature, " C")} / 提醒 ${booleanText(actuators.buzzer, "开启", "静音")}`
      : "等待数据";
    el.entryRoomState.textContent = telemetry
      ? `人体 ${booleanText(sensors.pir, "有人", "无人")} / 告警 ${(telemetry.alerts || []).length}`
      : "等待数据";
  }

  function renderAck() {
    if (!state.lastAck) {
      el.ackStatus.textContent = "等待 ack";
      el.ackStatus.dataset.status = "idle";
      return;
    }
    el.ackStatus.textContent = state.lastAck.ok ? `ack ${state.lastAck.message || "ok"}` : "ack failed";
    el.ackStatus.dataset.status = state.lastAck.ok ? "ok" : "offline";
  }

  function renderVoiceIntent() {
    const lastVoiceIntent = state.lastVoiceIntent;
    if (!lastVoiceIntent) {
      el.voiceStatus.textContent = "文本模式";
      el.voiceStatus.dataset.status = "idle";
      el.voiceResult.textContent = "等待语音文字";
      return;
    }

    el.voiceStatus.textContent = lastVoiceIntent.ok ? lastVoiceIntent.intent : "unknown";
    el.voiceStatus.dataset.status = lastVoiceIntent.ok ? "ok" : "offline";
    el.voiceResult.textContent = lastVoiceIntent.ok
      ? `${lastVoiceIntent.label}: ${lastVoiceIntent.text}`
      : `${lastVoiceIntent.message}: ${lastVoiceIntent.text || ""}`;
  }

  function render() {
    renderStatus();
    renderHello();
    renderTelemetry();
    renderAck();
    renderVoiceIntent();
  }

  function handleFrame(frame) {
    state.frameCount += 1;
    state.lastFrameAt = Date.now();

    if (frame.type === "hello") {
      state.hello = frame;
      addLog("hello", `收到 hello: ${frame.profileId || frame.board || "n16r8"}`);
    } else if (frame.type === "telemetry") {
      state.telemetry = frame;
    } else if (frame.type === "ack") {
      state.lastAck = frame;
      addLog(frame.ok ? "ack" : "error", `ack: ${frame.message || frame.ok}`);
    } else {
      addLog("frame", `收到 ${frame.type || "unknown"} 帧`);
    }

    render();
  }

  function processLine(line) {
    const text = line.trim();
    if (!text) return;
    if (!text.startsWith("{")) {
      const jsonStart = text.indexOf("{");
      const jsonEnd = text.lastIndexOf("}");
      if (jsonStart >= 0 && jsonEnd > jsonStart) {
        processLine(text.slice(jsonStart, jsonEnd + 1));
      } else {
        addLog("frame", "忽略启动日志");
      }
      return;
    }
    try {
      handleFrame(JSON.parse(text));
    } catch (error) {
      addLog("error", `JSON 解析失败: ${error.message}`);
    }
  }

  async function readLoop() {
    const decoder = new TextDecoder();
    try {
      while (state.connected && state.port?.readable) {
        const { value, done } = await state.reader.read();
        if (done) break;
        state.readBuffer += decoder.decode(value, { stream: true });
        const lines = state.readBuffer.split("\n");
        state.readBuffer = lines.pop() || "";
        lines.forEach(processLine);
      }
    } catch (error) {
      if (state.connected) addLog("error", `串口读取中断: ${error.message}`);
    } finally {
      try {
        state.reader?.releaseLock();
      } catch (error) {
        addLog("error", `释放读取锁失败: ${error.message}`);
      }
    }
  }

  async function connectSerial() {
    if (!isSerialSupported()) {
      addLog("error", "当前浏览器不支持 Web Serial");
      render();
      return;
    }

    try {
      state.port = await navigator.serial.requestPort();
      await state.port.open({ baudRate: 115200 });
      await state.port.setSignals?.({ dataTerminalReady: false, requestToSend: false });
      state.reader = state.port.readable.getReader();
      state.writer = state.port.writable.getWriter();
      state.connected = true;
      state.readBuffer = "";
      addLog("frame", "串口已打开，等待 hello");
      render();
      readLoop();
    } catch (error) {
      addLog("error", `连接失败: ${error.message}`);
      await disconnectSerial(false);
    }
  }

  async function disconnectSerial(showLog = true) {
    state.connected = false;

    try {
      await state.reader?.cancel();
    } catch (error) {
      if (showLog) addLog("error", `停止读取失败: ${error.message}`);
    }

    try {
      state.reader?.releaseLock();
    } catch (_) {
      // The reader may already be released by readLoop.
    }

    try {
      state.writer?.releaseLock();
    } catch (_) {
      // Writer may not have been created if connection failed early.
    }

    try {
      await state.port?.close();
    } catch (error) {
      if (showLog) addLog("error", `关闭串口失败: ${error.message}`);
    }

    state.port = null;
    state.reader = null;
    state.writer = null;
    if (showLog) addLog("frame", "串口已断开");
    render();
  }

  async function sendCommand(command) {
    if (!state.connected || !state.writer) {
      addLog("error", "未连接串口，命令未发送");
      render();
      return;
    }

    try {
      const payload = `${JSON.stringify(command)}\n`;
      await state.writer.write(new TextEncoder().encode(payload));
      addLog("command", `发送 ${command.type}${command.mode ? `:${command.mode}` : ""}`);
    } catch (error) {
      addLog("error", `发送失败: ${error.message}`);
    }
  }

  async function submitVoiceText(text) {
    if (!voiceIntentApi) {
      addLog("error", "语音意图模块未加载");
      return;
    }

    const result = voiceIntentApi.resolveVoiceIntent(text);
    state.lastVoiceIntent = result;
    render();

    if (!result.ok) {
      addLog("error", `语音未发送: ${result.message}`);
      return;
    }

    addLog("command", `语音意图 ${result.intent}: ${result.label}`);
    await sendCommand(result.command);
  }

  function renderVoiceQuickButtons() {
    if (!voiceIntentApi) {
      el.voiceQuickGrid.textContent = "语音意图模块未加载";
      return;
    }

    el.voiceQuickGrid.innerHTML = "";
    voiceIntentApi.quickIntents.forEach((item) => {
      const button = document.createElement("button");
      button.type = "button";
      button.textContent = item.label;
      button.dataset.voiceText = item.text;
      button.addEventListener("click", () => {
        el.voiceTextInput.value = item.text;
        submitVoiceText(item.text);
      });
      el.voiceQuickGrid.appendChild(button);
    });
  }

  function bindEvents() {
    el.connectButton.addEventListener("click", connectSerial);
    el.disconnectButton.addEventListener("click", () => disconnectSerial(true));
    el.clearLogButton.addEventListener("click", () => {
      state.logs = [];
      renderLog();
    });

    document.querySelectorAll("[data-mode]").forEach((button) => {
      button.addEventListener("click", () => {
        sendCommand({ type: "command", mode: button.dataset.mode });
      });
    });

    el.lampOnButton.addEventListener("click", () => {
      sendCommand({ type: "command", actuator: { lamp: true } });
    });

    el.lampOffButton.addEventListener("click", () => {
      sendCommand({ type: "command", actuator: { lamp: false } });
    });

    el.thresholdForm.addEventListener("submit", (event) => {
      event.preventDefault();
      const data = new FormData(el.thresholdForm);
      const set = {};
      ["lightThreshold", "temperatureThreshold", "soundThreshold"].forEach((key) => {
        const rawValue = Number(data.get(key));
        if (!Number.isNaN(rawValue)) set[key] = rawValue;
      });
      sendCommand({ type: "command", set });
    });

    el.voiceForm.addEventListener("submit", (event) => {
      event.preventDefault();
      submitVoiceText(el.voiceTextInput.value);
    });

    navigator.serial?.addEventListener("disconnect", () => {
      addLog("error", "USB 设备已断开");
      disconnectSerial(false);
    });
  }

  renderVoiceQuickButtons();
  bindEvents();
  renderLog();
  render();
  window.setInterval(render, 1000);
})();
