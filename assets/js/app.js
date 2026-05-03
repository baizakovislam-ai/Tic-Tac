const STORAGE_KEY = "tic-tac-settings-v2";
const API_BASE = window.location.protocol === "file:" ? "http://127.0.0.1:18080" : "";

const modeDescriptions = {
  classic: "Классическая партия на поле 3x3. Есть PvP и режим против ИИ.",
  ultimate: "Большое поле состоит из девяти малых досок 3x3 с правилами Ultimate.",
  multi: "Режим на 3-4 игроков с увеличенным полем и настраиваемой длиной линии.",
};

const defaults = {
  theme: "light",
  soundEnabled: false,
  animationsEnabled: true,
  aiDifficulty: "medium",
  symbolSet: "classic",
};

const state = {
  settings: loadSettings(),
  currentMode: "classic",
  game: null,
  busy: false,
  selectedModeConfig: {
    classicMatchType: "pvp",
    classicFirstPlayer: 0,
    multiPlayersCount: 3,
    multiHumansCount: 1,
    multiBoardSize: 5,
    multiWinLength: 3,
  },
};

const els = {
  body: document.body,
  menuView: document.getElementById("menuView"),
  gameView: document.getElementById("gameView"),
  settingsView: document.getElementById("settingsView"),
  modeDescription: document.getElementById("modeDescription"),
  boardContainer: document.getElementById("boardContainer"),
  turnIndicator: document.getElementById("turnIndicator"),
  resultIndicator: document.getElementById("resultIndicator"),
  hintIndicator: document.getElementById("hintIndicator"),
  playersStrip: document.getElementById("playersStrip"),
  gameTitle: document.getElementById("gameTitle"),
  classicMatchType: document.getElementById("classicMatchType"),
  classicFirstPlayer: document.getElementById("classicFirstPlayer"),
  multiPlayersCount: document.getElementById("multiPlayersCount"),
  multiHumansCount: document.getElementById("multiHumansCount"),
  multiBoardSize: document.getElementById("multiBoardSize"),
  multiWinLength: document.getElementById("multiWinLength"),
  themeSelect: document.getElementById("themeSelect"),
  soundEnabled: document.getElementById("soundEnabled"),
  animationsEnabled: document.getElementById("animationsEnabled"),
  aiDifficulty: document.getElementById("aiDifficulty"),
  symbolSet: document.getElementById("symbolSet"),
  startGameButton: document.getElementById("startGameButton"),
  restartButton: document.getElementById("restartButton"),
  endGameButton: document.getElementById("endGameButton"),
};

bootstrap();

async function bootstrap() {
  applySettingsToInputs();
  attachEvents();
  setMode("classic");
  renderStatus();
  await hydrateFromServer();
}

function attachEvents() {
  document.querySelectorAll(".mode-card").forEach((button) => {
    button.addEventListener("click", () => setMode(button.dataset.mode));
  });

  document.getElementById("settingsButton").addEventListener("click", () => {
    els.settingsView.classList.toggle("hidden");
  });

  document.getElementById("closeSettingsButton").addEventListener("click", () => {
    els.settingsView.classList.add("hidden");
  });

  document.getElementById("themeToggle").addEventListener("click", async () => {
    const order = ["light", "dark", "inferno"];
    const next = order[(order.indexOf(state.settings.theme) + 1) % order.length];
    await updateSettings({ theme: next });
  });

  document.getElementById("homeButton").addEventListener("click", endGameAndShowMenu);
  els.startGameButton.addEventListener("click", startSelectedMode);
  els.restartButton.addEventListener("click", restartCurrentGame);
  els.endGameButton.addEventListener("click", endGameAndShowMenu);

  [
    ["classicMatchType", els.classicMatchType],
    ["classicFirstPlayer", els.classicFirstPlayer],
    ["multiPlayersCount", els.multiPlayersCount],
    ["multiHumansCount", els.multiHumansCount],
    ["multiBoardSize", els.multiBoardSize],
    ["multiWinLength", els.multiWinLength],
  ].forEach(([key, element]) => {
    element.addEventListener("change", () => {
      state.selectedModeConfig[key] = Number.isNaN(Number(element.value)) ? element.value : Number(element.value);
      syncMultiConstraints();
    });
  });

  els.themeSelect.addEventListener("change", async () => updateSettings({ theme: els.themeSelect.value }));
  els.soundEnabled.addEventListener("change", async () => updateSettings({ soundEnabled: els.soundEnabled.checked }));
  els.animationsEnabled.addEventListener("change", async () => updateSettings({ animationsEnabled: els.animationsEnabled.checked }));
  els.aiDifficulty.addEventListener("change", async () => updateSettings({ aiDifficulty: els.aiDifficulty.value }));
  els.symbolSet.addEventListener("change", async () => updateSettings({ symbolSet: els.symbolSet.value }));
}

async function hydrateFromServer() {
  try {
    const data = await api("/state", "GET");
    if (data.settings) {
      state.settings = { ...state.settings, ...data.settings };
      persistSettings();
      applySettingsToInputs();
    }
    if (data.game) {
      state.game = data.game;
      state.currentMode = data.game.mode;
      setMode(state.currentMode);
      els.gameTitle.textContent = getModeTitle(state.game.mode);
      showGame();
      renderGame();
    }
  } catch (error) {
    renderStatus("Сервер недоступен. Запусти backend и открой игру снова.");
  }
}

async function api(path, method = "GET", payload) {
  const options = {
    method,
    headers: {
      "Content-Type": "application/json",
    },
  };
  if (payload) {
    options.body = JSON.stringify(payload);
  }
  const response = await fetch(`${API_BASE}${path}`, options);
  const data = await response.json();
  if (!response.ok || data.status === "error") {
    throw new Error(data.message || "Ошибка запроса.");
  }
  return data;
}

function setMode(mode) {
  state.currentMode = mode;
  document.querySelectorAll(".mode-card").forEach((button) => {
    button.classList.toggle("active", button.dataset.mode === mode);
  });
  document.getElementById("classicConfig").classList.toggle("hidden", mode !== "classic");
  document.getElementById("ultimateConfig").classList.toggle("hidden", mode !== "ultimate");
  document.getElementById("multiConfig").classList.toggle("hidden", mode !== "multi");
  els.modeDescription.textContent = modeDescriptions[mode];
  syncMultiConstraints();
}

function syncMultiConstraints() {
  const players = Number(els.multiPlayersCount.value);
  Array.from(els.multiHumansCount.options).forEach((option) => {
    option.hidden = Number(option.value) > players;
  });
  if (Number(els.multiHumansCount.value) > players) {
    els.multiHumansCount.value = String(players);
    state.selectedModeConfig.multiHumansCount = players;
  }

  const boardSize = Number(els.multiBoardSize.value);
  Array.from(els.multiWinLength.options).forEach((option) => {
    option.hidden = Number(option.value) > boardSize;
  });
  if (Number(els.multiWinLength.value) > boardSize) {
    els.multiWinLength.value = String(Math.min(3, boardSize));
    state.selectedModeConfig.multiWinLength = Number(els.multiWinLength.value);
  }
}

function setBusy(next) {
  state.busy = next;
  els.startGameButton.disabled = next;
  els.restartButton.disabled = next;
}

function showMenu() {
  els.menuView.classList.remove("hidden");
  els.gameView.classList.add("hidden");
  state.game = null;
  renderStatus();
}

async function endGameAndShowMenu() {
  try {
    await api("/endGame", "POST");
  } catch (error) {
    renderStatus(error.message);
  }
  showMenu();
}

function showGame() {
  els.menuView.classList.add("hidden");
  els.gameView.classList.remove("hidden");
}

async function startSelectedMode() {
  setBusy(true);
  try {
    const data = await api("/startGame", "POST", {
      mode: state.currentMode,
      config: state.selectedModeConfig,
      settings: state.settings,
    });
    state.game = data.game;
    els.gameTitle.textContent = getModeTitle(state.game.mode);
    showGame();
  } catch (error) {
    renderStatus(error.message);
  } finally {
    setBusy(false);
    if (state.game) {
      renderGame();
    }
  }
}

async function restartCurrentGame() {
  setBusy(true);
  try {
    const data = await api("/restart", "POST");
    state.game = data.game;
    els.gameTitle.textContent = getModeTitle(state.game.mode);
    showGame();
  } catch (error) {
    renderStatus(error.message);
  } finally {
    setBusy(false);
    if (state.game) {
      renderGame();
    }
  }
}

async function updateSettings(next) {
  state.settings = { ...state.settings, ...next };
  persistSettings();
  applySettingsToInputs();
  try {
    await api("/settings", "POST", state.settings);
  } catch (error) {
    renderStatus(error.message);
  }
}

function renderGame() {
  if (!state.game) {
    return;
  }
  renderBoard();
  renderPlayers();
  renderStatus();
}

function renderBoard() {
  els.boardContainer.innerHTML = "";
  if (state.game.mode === "classic") {
    renderClassicBoard();
  } else if (state.game.mode === "ultimate") {
    renderUltimateBoard();
  } else {
    renderMultiBoard();
  }
}

function renderClassicBoard() {
  const board = document.createElement("div");
  board.className = "classic-board";
  state.game.board.forEach((row, y) => {
    row.forEach((cell, x) => {
      const button = document.createElement("button");
      button.className = "cell";
      button.type = "button";
      button.textContent = cell || "";
      button.disabled = Boolean(cell) || isGameOver() || state.busy;
      if (isWinningCell(x, y)) {
        button.classList.add("win-line");
      }
      button.addEventListener("click", () => makeMove({ x, y }));
      board.appendChild(button);
    });
  });
  els.boardContainer.appendChild(board);
}

function renderMultiBoard() {
  const board = document.createElement("div");
  board.className = "multi-board";
  board.style.gridTemplateColumns = `repeat(${state.game.boardSize}, 1fr)`;
  state.game.board.forEach((row, y) => {
    row.forEach((cell, x) => {
      const button = document.createElement("button");
      button.className = "cell";
      button.type = "button";
      button.textContent = cell || "";
      button.disabled = Boolean(cell) || isGameOver() || state.busy;
      if (isWinningCell(x, y)) {
        button.classList.add("win-line");
      }
      button.addEventListener("click", () => makeMove({ x, y }));
      board.appendChild(button);
    });
  });
  els.boardContainer.appendChild(board);
}

function renderUltimateBoard() {
  const wrapper = document.createElement("div");
  wrapper.className = "ultimate-board";

  state.game.miniBoards.forEach((mini, macroIndex) => {
    const macro = document.createElement("div");
    macro.className = "macro-cell";
    const active = state.game.activeMacroIndex === null || state.game.activeMacroIndex === macroIndex;
    if (active && !mini.winner && !mini.draw) {
      macro.classList.add("active-target");
    }
    if (mini.winner || mini.draw) {
      macro.classList.add("locked");
      macro.dataset.owner = mini.winner || "•";
    } else {
      macro.dataset.owner = "";
    }

    mini.board.forEach((row, y) => {
      row.forEach((cell, x) => {
        const button = document.createElement("button");
        button.className = "small-cell";
        button.type = "button";
        button.textContent = cell || "";
        const canPlayHere = (state.game.activeMacroIndex === null || state.game.activeMacroIndex === macroIndex) && !mini.winner && !mini.draw;
        button.disabled = Boolean(cell) || !canPlayHere || isGameOver() || state.busy;
        if (!button.disabled) {
          button.classList.add("target");
        }
        if ((mini.winningCells || []).some(([wx, wy]) => wx === x && wy === y)) {
          button.classList.add("win-line");
        }
        button.addEventListener("click", () => makeMove({ macroIndex, x, y }));
        macro.appendChild(button);
      });
    });
    wrapper.appendChild(macro);
  });
  els.boardContainer.appendChild(wrapper);
}

function renderPlayers() {
  els.playersStrip.innerHTML = "";
  if (!state.game) {
    return;
  }
  state.game.players.forEach((player, index) => {
    const chip = document.createElement("article");
    chip.className = "player-chip";
    if (index === state.game.currentPlayer && !isGameOver()) {
      chip.classList.add("active");
    }
    chip.innerHTML = `<strong>${player.label}</strong><br><span>${player.symbol}${player.isAI ? " • ИИ" : ""}</span>`;
    els.playersStrip.appendChild(chip);
  });
}

function renderStatus(message) {
  if (message) {
    els.turnIndicator.textContent = "Внимание";
    els.resultIndicator.textContent = "Ошибка";
    els.hintIndicator.textContent = message;
    return;
  }

  if (!state.game) {
    els.turnIndicator.textContent = "Игрок не выбран";
    els.resultIndicator.textContent = "Ожидается старт";
    els.hintIndicator.textContent = "Выберите режим и начните игру.";
    return;
  }

  const player = state.game.players[state.game.currentPlayer];
  els.turnIndicator.textContent = player ? `${player.label} (${player.symbol})` : "Партия завершена";

  if (state.game.winner !== null) {
    const winner = state.game.players[state.game.winner];
    els.resultIndicator.textContent = `${winner.label} победил`;
  } else if (state.game.isDraw) {
    els.resultIndicator.textContent = "Ничья";
  } else {
    els.resultIndicator.textContent = "Игра продолжается";
  }

  if (state.game.mode === "ultimate" && !isGameOver()) {
    els.hintIndicator.textContent = state.game.activeMacroIndex === null
      ? "Можно играть в любом доступном секторе."
      : `Активный сектор: ${state.game.activeMacroIndex + 1}.`;
    return;
  }

  if (state.game.mode === "multi" && !isGameOver()) {
    els.hintIndicator.textContent = `Соберите линию длиной ${state.game.winLength}.`;
    return;
  }

  els.hintIndicator.textContent = isGameOver() ? "Можно начать новую игру." : "Выберите клетку.";
}

async function makeMove(payload) {
  if (!state.game || state.busy || isGameOver()) {
    return;
  }
  setBusy(true);
  try {
    const data = await api("/move", "POST", payload);
    state.game = data.game;
  } catch (error) {
    renderStatus(error.message);
  } finally {
    setBusy(false);
    if (state.game) {
      renderGame();
    }
  }
}

function isWinningCell(x, y) {
  return (state.game?.winningCells || []).some(([wx, wy]) => wx === x && wy === y);
}

function isGameOver() {
  return Boolean(state.game && (state.game.winner !== null || state.game.isDraw));
}

function getModeTitle(mode) {
  return {
    classic: "Классический режим",
    ultimate: "Ultimate режим",
    multi: "Режим 3-4 игроков",
  }[mode];
}

function applySettingsToInputs() {
  els.body.dataset.theme = state.settings.theme;
  els.themeSelect.value = state.settings.theme;
  els.soundEnabled.checked = state.settings.soundEnabled;
  els.animationsEnabled.checked = state.settings.animationsEnabled;
  els.aiDifficulty.value = state.settings.aiDifficulty;
  els.symbolSet.value = state.settings.symbolSet;
  els.classicMatchType.value = state.selectedModeConfig.classicMatchType;
  els.classicFirstPlayer.value = String(state.selectedModeConfig.classicFirstPlayer);
  els.multiPlayersCount.value = String(state.selectedModeConfig.multiPlayersCount);
  els.multiHumansCount.value = String(state.selectedModeConfig.multiHumansCount);
  els.multiBoardSize.value = String(state.selectedModeConfig.multiBoardSize);
  els.multiWinLength.value = String(state.selectedModeConfig.multiWinLength);
  syncMultiConstraints();
}

function loadSettings() {
  try {
    return { ...defaults, ...JSON.parse(localStorage.getItem(STORAGE_KEY) || "{}") };
  } catch (error) {
    return { ...defaults };
  }
}

function persistSettings() {
  localStorage.setItem(STORAGE_KEY, JSON.stringify(state.settings));
}
