const STORAGE_KEY = "tic-tac-settings-v3";
const API_BASE = window.location.protocol === "file:" ? "http://127.0.0.1:18080" : "";

const modeDescriptions = {
  classic: "Классическая партия на поле 3x3. Если выбран ИИ, игрок сам выбирает свою фигуру.",
  ultimate: "Ultimate режим с реальным ИИ и ограниченным поиском для серверной части.",
  multi: "Режим на 3-4 игроков с увеличенным полем и тактическим ИИ для свободных слотов.",
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
  resultShownFor: null,
  modalTimer: null,
  previousGame: null,
  selectedModeConfig: {
    classicMatchType: "pvp",
    classicUserSymbolIndex: 0,
    ultimateMatchType: "pvp",
    ultimateUserSymbolIndex: 0,
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
  classicSymbolWrap: document.getElementById("classicSymbolWrap"),
  classicUserSymbolIndex: document.getElementById("classicUserSymbolIndex"),
  ultimateMatchType: document.getElementById("ultimateMatchType"),
  ultimateSymbolWrap: document.getElementById("ultimateSymbolWrap"),
  ultimateUserSymbolIndex: document.getElementById("ultimateUserSymbolIndex"),
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
  resultModal: document.getElementById("resultModal"),
  modalTitle: document.getElementById("modalTitle"),
  modalMessage: document.getElementById("modalMessage"),
  modalScore: document.getElementById("modalScore"),
  modalRestartButton: document.getElementById("modalRestartButton"),
  modalMenuButton: document.getElementById("modalMenuButton"),
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

  const settingsButton = document.getElementById("settingsButton");
  const closeSettingsButton = document.getElementById("closeSettingsButton");
  const themeToggle = document.getElementById("themeToggle");
  const homeButton = document.getElementById("homeButton");

  settingsButton?.addEventListener("click", () => {
    els.settingsView?.classList.toggle("hidden");
  });
  closeSettingsButton?.addEventListener("click", () => {
    els.settingsView?.classList.add("hidden");
  });

  themeToggle?.addEventListener("click", async () => {
    const order = ["light", "dark", "inferno"];
    const next = order[(order.indexOf(state.settings.theme) + 1) % order.length];
    await updateSettings({ theme: next });
  });

  homeButton?.addEventListener("click", endGameAndShowMenu);
  els.startGameButton?.addEventListener("click", startSelectedMode);
  els.restartButton?.addEventListener("click", restartCurrentGame);
  els.endGameButton?.addEventListener("click", endGameAndShowMenu);
  els.modalRestartButton?.addEventListener("click", async () => {
    hideResultModal();
    await restartCurrentGame();
  });
  els.modalMenuButton?.addEventListener("click", async () => {
    hideResultModal();
    await endGameAndShowMenu();
  });

  [
    ["classicMatchType", els.classicMatchType],
    ["classicUserSymbolIndex", els.classicUserSymbolIndex],
    ["ultimateMatchType", els.ultimateMatchType],
    ["ultimateUserSymbolIndex", els.ultimateUserSymbolIndex],
    ["multiPlayersCount", els.multiPlayersCount],
    ["multiHumansCount", els.multiHumansCount],
    ["multiBoardSize", els.multiBoardSize],
    ["multiWinLength", els.multiWinLength],
  ].forEach(([key, element]) => {
    if (!element) {
      return;
    }
    element.addEventListener("change", () => {
      state.selectedModeConfig[key] = Number.isNaN(Number(element.value)) ? element.value : Number(element.value);
      syncConfigVisibility();
    });
  });

  els.themeSelect?.addEventListener("change", async () => updateSettings({ theme: els.themeSelect.value }));
  els.soundEnabled?.addEventListener("change", async () => updateSettings({ soundEnabled: els.soundEnabled.checked }));
  els.animationsEnabled?.addEventListener("change", async () => updateSettings({ animationsEnabled: els.animationsEnabled.checked }));
  els.aiDifficulty?.addEventListener("change", async () => updateSettings({ aiDifficulty: els.aiDifficulty.value }));
  els.symbolSet?.addEventListener("change", async () => updateSettings({ symbolSet: els.symbolSet.value }));
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
    renderStatus("Сервер недоступен. Запусти backend и обнови страницу.");
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
  syncConfigVisibility();
}

function syncConfigVisibility() {
  els.classicSymbolWrap?.classList.toggle("hidden", els.classicMatchType?.value !== "ai");
  els.ultimateSymbolWrap?.classList.toggle("hidden", els.ultimateMatchType?.value !== "ai");

  const players = Number(els.multiPlayersCount?.value || 3);
  Array.from(els.multiHumansCount?.options || []).forEach((option) => {
    option.hidden = Number(option.value) > players;
  });
  if (els.multiHumansCount && Number(els.multiHumansCount.value) > players) {
    els.multiHumansCount.value = String(players);
    state.selectedModeConfig.multiHumansCount = players;
  }

  const boardSize = Number(els.multiBoardSize?.value || 5);
  Array.from(els.multiWinLength?.options || []).forEach((option) => {
    option.hidden = Number(option.value) > boardSize;
  });
  if (els.multiWinLength && Number(els.multiWinLength.value) > boardSize) {
    const nextValue = Math.min(3, boardSize);
    els.multiWinLength.value = String(nextValue);
    state.selectedModeConfig.multiWinLength = nextValue;
  }
}

function setBusy(next) {
  state.busy = next;
  if (els.startGameButton) {
    els.startGameButton.disabled = next;
  }
  if (els.restartButton) {
    els.restartButton.disabled = next;
  }
}

function showMenu() {
  els.menuView.classList.remove("hidden");
  els.gameView.classList.add("hidden");
  state.game = null;
  state.resultShownFor = null;
  renderStatus();
}

function showGame() {
  els.menuView.classList.add("hidden");
  els.gameView.classList.remove("hidden");
}

async function startSelectedMode() {
  setBusy(true);
  hideResultModal();
  try {
    const data = await api("/startGame", "POST", {
      mode: state.currentMode,
      config: state.selectedModeConfig,
      settings: state.settings,
    });
    state.game = data.game;
    state.resultShownFor = null;
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
  hideResultModal();
  try {
    const data = await api("/restart", "POST");
    state.game = data.game;
    state.resultShownFor = null;
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

async function endGameAndShowMenu() {
  try {
    await api("/endGame", "POST");
  } catch (error) {
    renderStatus(error.message);
  }
  hideResultModal();
  showMenu();
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
  maybeShowResultModal();
  state.previousGame = JSON.parse(JSON.stringify(state.game));
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
      if (isFreshClassicMove(x, y)) {
        button.classList.add("fresh-move");
      }
      if (isGameOver()) {
        button.classList.add("match-end");
      }
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
      if (isFreshClassicMove(x, y)) {
        button.classList.add("fresh-move");
      }
      if (isGameOver()) {
        button.classList.add("match-end");
      }
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
    if (isGameOver()) {
      macro.classList.add("match-end");
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
        if (isFreshUltimateMove(macroIndex, x, y)) {
          button.classList.add("fresh-move");
        }
        if (isGameOver()) {
          button.classList.add("match-end");
        }
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
    const score = Array.isArray(state.game.scores) ? state.game.scores[index] ?? 0 : 0;
    chip.innerHTML = `
      <strong>${player.label}</strong><br>
      <span>${player.symbol}${player.isAI ? " • ИИ" : ""}</span><br>
      <span class="player-chip-score">Счёт: ${score}</span>
    `;
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

  els.hintIndicator.textContent = isGameOver() ? "Раунд завершён. Можно начать новый." : "Выберите клетку.";
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

function maybeShowResultModal() {
  if (!isGameOver()) {
    return;
  }
  const signature = JSON.stringify({
    mode: state.game.mode,
    winner: state.game.winner,
    draw: state.game.isDraw,
    scores: state.game.scores,
  });
  if (state.resultShownFor === signature) {
    return;
  }
  state.resultShownFor = signature;

  if (state.game.winner !== null) {
    const winner = state.game.players[state.game.winner];
    els.modalTitle.textContent = "Есть победитель";
    els.modalMessage.textContent = `${winner.label} выиграл этот матч.`;
  } else {
    els.modalTitle.textContent = "Ничья";
    els.modalMessage.textContent = "Матч завершён без победителя.";
  }

  els.modalScore.innerHTML = "";
  state.game.players.forEach((player, index) => {
    const row = document.createElement("div");
    row.className = "modal-score-row";
    row.innerHTML = `<span>${player.label} ${player.symbol}</span><strong>${state.game.scores?.[index] ?? 0}</strong>`;
    els.modalScore.appendChild(row);
  });
  if (state.modalTimer) {
    clearTimeout(state.modalTimer);
  }
  const delay = state.settings.animationsEnabled ? 950 : 260;
  state.modalTimer = window.setTimeout(() => {
    els.resultModal.classList.remove("hidden");
    state.modalTimer = null;
  }, delay);
}

function hideResultModal() {
  if (state.modalTimer) {
    clearTimeout(state.modalTimer);
    state.modalTimer = null;
  }
  els.resultModal.classList.add("hidden");
}

function isFreshClassicMove(x, y) {
  const previousBoard = state.previousGame?.board;
  const currentValue = state.game?.board?.[y]?.[x];
  if (!currentValue || !previousBoard) {
    return false;
  }
  return !previousBoard[y]?.[x];
}

function isFreshUltimateMove(macroIndex, x, y) {
  const previousBoard = state.previousGame?.miniBoards?.[macroIndex]?.board;
  const currentValue = state.game?.miniBoards?.[macroIndex]?.board?.[y]?.[x];
  if (!currentValue || !previousBoard) {
    return false;
  }
  return !previousBoard[y]?.[x];
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
  if (els.themeSelect) els.themeSelect.value = state.settings.theme;
  if (els.soundEnabled) els.soundEnabled.checked = state.settings.soundEnabled;
  if (els.animationsEnabled) els.animationsEnabled.checked = state.settings.animationsEnabled;
  if (els.aiDifficulty) els.aiDifficulty.value = state.settings.aiDifficulty;
  if (els.symbolSet) els.symbolSet.value = state.settings.symbolSet;

  if (els.classicMatchType) els.classicMatchType.value = state.selectedModeConfig.classicMatchType;
  if (els.classicUserSymbolIndex) els.classicUserSymbolIndex.value = String(state.selectedModeConfig.classicUserSymbolIndex);
  if (els.ultimateMatchType) els.ultimateMatchType.value = state.selectedModeConfig.ultimateMatchType;
  if (els.ultimateUserSymbolIndex) els.ultimateUserSymbolIndex.value = String(state.selectedModeConfig.ultimateUserSymbolIndex);
  if (els.multiPlayersCount) els.multiPlayersCount.value = String(state.selectedModeConfig.multiPlayersCount);
  if (els.multiHumansCount) els.multiHumansCount.value = String(state.selectedModeConfig.multiHumansCount);
  if (els.multiBoardSize) els.multiBoardSize.value = String(state.selectedModeConfig.multiBoardSize);
  if (els.multiWinLength) els.multiWinLength.value = String(state.selectedModeConfig.multiWinLength);
  syncConfigVisibility();
}

function loadSettings() {
  try {
    return { ...defaults, ...JSON.parse(localStorage.getItem(STORAGE_KEY) || "{}") };
  } catch {
    return { ...defaults };
  }
}

function persistSettings() {
  localStorage.setItem(STORAGE_KEY, JSON.stringify(state.settings));
}
