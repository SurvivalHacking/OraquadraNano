#ifdef EFFECT_BTTF
// ================== WEB SERVER PER CONFIGURAZIONE BTTF ==================
// Interfaccia web per modificare:
// - Date iconiche (DESTINATION TIME e LAST TIME DEPARTED)
// - Abilitazione sveglie per suonare il buzzer
// SALVATAGGIO: File JSON su SD card (/bttf_config.json)

// Funzione esterna per aggiornamento immediato del display (definita in 12_BTTF.ino)
extern void forceBTTFRedraw();

// ================== FUNZIONI SALVATAGGIO/CARICAMENTO SU SD ==================

// Salva configurazione BTTF su SD card in formato JSON
bool saveBTTFConfigToSD() {
  File configFile = SD.open("/bttf_config.json", FILE_WRITE);
  if (!configFile) {
    Serial.println("[BTTF] Errore apertura file config per scrittura");
    return false;
  }

  // Crea JSON manualmente
  String json = "{\n";

  json += "  \"destination\": {\n";
  json += "    \"month\": " + String(destinationTime.month) + ",\n";
  json += "    \"day\": " + String(destinationTime.day) + ",\n";
  json += "    \"year\": " + String(destinationTime.year) + ",\n";
  json += "    \"hour\": " + String(destinationTime.hour) + ",\n";
  json += "    \"minute\": " + String(destinationTime.minute) + ",\n";
  json += "    \"ampm\": \"" + String(destinationTime.ampm) + "\",\n";
  json += "    \"alarmEnabled\": " + String(alarmDestinationEnabled ? "true" : "false") + "\n";
  json += "  },\n";

  json += "  \"lastDeparted\": {\n";
  json += "    \"month\": " + String(lastDeparted.month) + ",\n";
  json += "    \"day\": " + String(lastDeparted.day) + ",\n";
  json += "    \"year\": " + String(lastDeparted.year) + ",\n";
  json += "    \"hour\": " + String(lastDeparted.hour) + ",\n";
  json += "    \"minute\": " + String(lastDeparted.minute) + ",\n";
  json += "    \"ampm\": \"" + String(lastDeparted.ampm) + "\",\n";
  json += "    \"alarmEnabled\": " + String(alarmLastDepartedEnabled ? "true" : "false") + "\n";
  json += "  }\n";
  json += "}\n";

  configFile.print(json);
  configFile.close();

  Serial.println("[BTTF] Configurazione salvata su /bttf_config.json");
  return true;
}

// Carica la configurazione BTTF da SD card
bool loadBTTFConfigFromSD() {
  if (!SD.exists("/bttf_config.json")) {
    Serial.println("[BTTF] File config non trovato, uso valori di default");
    // Valori già impostati nello sketch
    saveBTTFConfigToSD();
    return true;
  }

  File configFile = SD.open("/bttf_config.json", FILE_READ);
  if (!configFile) {
    Serial.println("[BTTF] Errore apertura file config");
    return false;
  }

  String jsonStr = "";
  while (configFile.available()) {
    jsonStr += (char)configFile.read();
  }
  configFile.close();

  // Parse JSON manualmente
  auto extractInt = [](const String& json, const String& key) -> int {
    int idx = json.indexOf("\"" + key + "\":");
    if (idx == -1) return 0;
    idx = json.indexOf(":", idx) + 1;
    while (json.charAt(idx) == ' ' || json.charAt(idx) == '\n') idx++;
    int endIdx = idx;
    while (json.charAt(endIdx) >= '0' && json.charAt(endIdx) <= '9') endIdx++;
    return json.substring(idx, endIdx).toInt();
  };

  auto extractString = [](const String& json, const String& key) -> String {
    int idx = json.indexOf("\"" + key + "\":");
    if (idx == -1) return "";
    idx = json.indexOf("\"", idx + key.length() + 3) + 1;
    int endIdx = json.indexOf("\"", idx);
    return json.substring(idx, endIdx);
  };

  auto extractBool = [](const String& json, const String& key) -> bool {
    int idx = json.indexOf("\"" + key + "\":");
    if (idx == -1) return false;
    return json.indexOf("true", idx) > 0 && json.indexOf("true", idx) < idx + 20;
  };

  // Carica DESTINATION TIME
  int destIdx = jsonStr.indexOf("\"destination\"");
  String destSection = jsonStr.substring(destIdx, jsonStr.indexOf("\"lastDeparted\""));
  destinationTime.month = extractInt(destSection, "month");
  destinationTime.day = extractInt(destSection, "day");
  destinationTime.year = extractInt(destSection, "year");
  destinationTime.hour = extractInt(destSection, "hour");
  destinationTime.minute = extractInt(destSection, "minute");
  String destAmpm = extractString(destSection, "ampm");
  destinationTime.ampm = (destAmpm == "PM") ? "PM" : "AM";
  alarmDestinationEnabled = extractBool(destSection, "alarmEnabled");

  // Carica LAST TIME DEPARTED
  int lastIdx = jsonStr.indexOf("\"lastDeparted\"");
  String lastSection = jsonStr.substring(lastIdx);
  lastDeparted.month = extractInt(lastSection, "month");
  lastDeparted.day = extractInt(lastSection, "day");
  lastDeparted.year = extractInt(lastSection, "year");
  lastDeparted.hour = extractInt(lastSection, "hour");
  lastDeparted.minute = extractInt(lastSection, "minute");
  String lastAmpm = extractString(lastSection, "ampm");
  lastDeparted.ampm = (lastAmpm == "PM") ? "PM" : "AM";
  alarmLastDepartedEnabled = extractBool(lastSection, "alarmEnabled");

  Serial.println("[BTTF] Configurazione caricata da /bttf_config.json");
  Serial.printf("[BTTF] DEST: %d/%d/%d %d:%02d %s (alarm=%d)\n",
                destinationTime.month, destinationTime.day, destinationTime.year,
                destinationTime.hour, destinationTime.minute, destinationTime.ampm,
                alarmDestinationEnabled);
  Serial.printf("[BTTF] LAST: %d/%d/%d %d:%02d %s (alarm=%d)\n",
                lastDeparted.month, lastDeparted.day, lastDeparted.year,
                lastDeparted.hour, lastDeparted.minute, lastDeparted.ampm,
                alarmLastDepartedEnabled);
  return true;
}

// ================== ENDPOINT WEB SERVER ==================

// GET /bttf - Pagina HTML principale
void handleBTTFEditor(AsyncWebServerRequest *request) {
  String html = R"=====(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>⚡ BTTF Time Circuits ⚡</title>
<link rel="preconnect" href="https://fonts.googleapis.com">
<link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
<link href="https://fonts.googleapis.com/css2?family=Orbitron:wght@400;700;900&display=swap" rel="stylesheet">
<style>
* { margin: 0; padding: 0; box-sizing: border-box; }

body {
  font-family: 'Orbitron', 'Courier New', monospace;
  background: linear-gradient(135deg, #0a0a0a 0%, #1a1a2e 50%, #0a0a0a 100%);
  min-height: 100vh;
  padding: 20px;
  position: relative;
  overflow-x: hidden;
}

body::before {
  content: '';
  position: fixed;
  top: 0; left: 0;
  width: 100%; height: 100%;
  background:
    repeating-linear-gradient(0deg, rgba(0,255,255,0.03) 0px, transparent 1px, transparent 2px, rgba(0,255,255,0.03) 3px),
    repeating-linear-gradient(90deg, rgba(255,0,0,0.03) 0px, transparent 1px, transparent 2px, rgba(255,0,0,0.03) 3px);
  pointer-events: none;
  z-index: 0;
}

.container {
  max-width: 900px;
  margin: 0 auto;
  position: relative;
  z-index: 1;
}

h1 {
  text-align: center;
  font-size: clamp(24px, 5vw, 48px);
  font-weight: 900;
  margin-bottom: 40px;
  background: linear-gradient(45deg, #ff0000, #ff6600, #ffff00);
  -webkit-background-clip: text;
  -webkit-text-fill-color: transparent;
  background-clip: text;
  text-shadow: 0 0 30px rgba(255,0,0,0.5);
  animation: glow 2s ease-in-out infinite alternate;
  letter-spacing: 4px;
}

@keyframes glow {
  from { filter: drop-shadow(0 0 20px rgba(255,0,0,0.8)); }
  to { filter: drop-shadow(0 0 40px rgba(255,100,0,1)); }
}

.panel {
  background: rgba(10,10,10,0.8);
  border: 3px solid;
  border-radius: 15px;
  padding: 25px;
  margin: 30px 0;
  position: relative;
  backdrop-filter: blur(10px);
  transition: all 0.3s ease;
  box-shadow: 0 8px 32px rgba(0,0,0,0.5);
}

.panel::before {
  content: '';
  position: absolute;
  top: -3px; left: -3px; right: -3px; bottom: -3px;
  background: inherit;
  border-radius: 15px;
  filter: blur(15px);
  opacity: 0.5;
  z-index: -1;
}

.panel:hover {
  transform: translateY(-5px);
  box-shadow: 0 12px 48px rgba(0,0,0,0.7);
}

.panel-red {
  border-color: #ff0000;
  box-shadow: 0 0 30px rgba(255,0,0,0.3), inset 0 0 20px rgba(255,0,0,0.1);
}

.panel-green {
  border-color: #00ff00;
  box-shadow: 0 0 30px rgba(0,255,0,0.3), inset 0 0 20px rgba(0,255,0,0.1);
}

.panel-amber {
  border-color: #ffaa00;
  box-shadow: 0 0 30px rgba(255,170,0,0.3), inset 0 0 20px rgba(255,170,0,0.1);
}

.panel h2 {
  font-size: clamp(18px, 3vw, 28px);
  font-weight: 700;
  margin-bottom: 20px;
  padding-bottom: 15px;
  border-bottom: 2px solid;
  text-transform: uppercase;
  letter-spacing: 3px;
  display: flex;
  align-items: center;
  gap: 10px;
}

.panel-red h2 {
  color: #ff0000;
  border-color: #ff0000;
  text-shadow: 0 0 10px rgba(255,0,0,0.8);
}

.panel-green h2 {
  color: #00ff00;
  border-color: #00ff00;
  text-shadow: 0 0 10px rgba(0,255,0,0.8);
}

.panel-amber h2 {
  color: #ffaa00;
  border-color: #ffaa00;
  text-shadow: 0 0 10px rgba(255,170,0,0.8);
}

.time-display {
  background: #000;
  border: 2px solid;
  border-radius: 10px;
  padding: 20px;
  margin: 15px 0;
  font-size: clamp(24px, 4vw, 36px);
  font-weight: 700;
  text-align: center;
  letter-spacing: 8px;
  font-family: 'Courier New', monospace;
  box-shadow: inset 0 0 20px rgba(0,0,0,0.8);
}

.panel-red .time-display {
  border-color: #ff0000;
  color: #ff0000;
  text-shadow: 0 0 20px rgba(255,0,0,1);
}

.panel-green .time-display {
  border-color: #00ff00;
  color: #00ff00;
  text-shadow: 0 0 20px rgba(0,255,0,1);
  animation: pulse 2s ease-in-out infinite;
}

@keyframes pulse {
  0%, 100% { opacity: 1; }
  50% { opacity: 0.8; }
}

.panel-amber .time-display {
  border-color: #ffaa00;
  color: #ffaa00;
  text-shadow: 0 0 20px rgba(255,170,0,1);
}

.form-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
  gap: 15px;
  margin: 15px 0;
}

.form-row {
  display: flex;
  flex-direction: column;
  gap: 8px;
}

label {
  color: #00ddff;
  font-size: 12px;
  font-weight: 700;
  text-transform: uppercase;
  letter-spacing: 1px;
}

input, select {
  background: #000;
  color: #00ff00;
  border: 2px solid #00ff00;
  border-radius: 8px;
  padding: 12px;
  font-family: 'Orbitron', monospace;
  font-size: 16px;
  font-weight: 700;
  transition: all 0.3s ease;
  box-shadow: inset 0 0 10px rgba(0,0,0,0.5);
}

input:focus, select:focus {
  outline: none;
  border-color: #00ffff;
  box-shadow: 0 0 15px rgba(0,255,255,0.5), inset 0 0 10px rgba(0,0,0,0.5);
}

input[readonly] {
  cursor: not-allowed;
  opacity: 0.8;
}

.alarm-toggle {
  display: flex;
  align-items: center;
  gap: 15px;
  margin-top: 20px;
  padding: 15px;
  background: rgba(255,255,0,0.1);
  border: 2px dashed #ffff00;
  border-radius: 10px;
  transition: all 0.3s ease;
}

.alarm-toggle:hover {
  background: rgba(255,255,0,0.2);
  border-color: #ffff00;
  box-shadow: 0 0 20px rgba(255,255,0,0.3);
}

.alarm-toggle label {
  flex: 1;
  color: #ffff00;
  font-size: 14px;
  cursor: pointer;
}

input[type="checkbox"] {
  width: 24px;
  height: 24px;
  cursor: pointer;
  accent-color: #ffff00;
}

button {
  background: linear-gradient(135deg, #ff0000 0%, #ff3300 100%);
  color: #fff;
  border: 3px solid #ff6600;
  border-radius: 12px;
  padding: 18px 40px;
  cursor: pointer;
  font-family: 'Orbitron', monospace;
  font-weight: 900;
  font-size: 18px;
  width: 100%;
  margin-top: 30px;
  text-transform: uppercase;
  letter-spacing: 3px;
  box-shadow: 0 8px 30px rgba(255,0,0,0.4);
  transition: all 0.3s ease;
  position: relative;
  overflow: hidden;
}

button::before {
  content: '';
  position: absolute;
  top: 50%;
  left: 50%;
  width: 0;
  height: 0;
  border-radius: 50%;
  background: rgba(255,255,255,0.3);
  transform: translate(-50%, -50%);
  transition: width 0.6s, height 0.6s;
}

button:hover::before {
  width: 300px;
  height: 300px;
}

button:hover {
  transform: translateY(-3px);
  box-shadow: 0 12px 40px rgba(255,0,0,0.6);
  border-color: #ffaa00;
}

button:active {
  transform: translateY(0);
}

.status {
  color: #ffff00;
  font-weight: 700;
  text-align: center;
  margin: 20px 0;
  padding: 15px;
  background: rgba(255,255,0,0.1);
  border: 2px solid #ffff00;
  border-radius: 10px;
  font-size: 16px;
  letter-spacing: 2px;
  box-shadow: 0 0 20px rgba(255,255,0,0.3);
  transition: all 0.3s ease;
}

.status.success {
  color: #00ff00;
  border-color: #00ff00;
  background: rgba(0,255,0,0.1);
  animation: flash 0.5s ease;
}

.status.error {
  color: #ff0000;
  border-color: #ff0000;
  background: rgba(255,0,0,0.1);
  animation: shake 0.5s ease;
}

@keyframes flash {
  0%, 100% { opacity: 1; }
  50% { opacity: 0.5; }
}

@keyframes shake {
  0%, 100% { transform: translateX(0); }
  25% { transform: translateX(-10px); }
  75% { transform: translateX(10px); }
}

@media (max-width: 600px) {
  .form-grid {
    grid-template-columns: 1fr;
  }
  h1 { font-size: 24px; }
  .panel { padding: 15px; }
}
</style>
</head>
<body>
<div class="container">
<h1>⚡ BTTF TIME CIRCUITS ⚡</h1>

<div class="panel panel-red">
  <h2>🎯 DESTINATION TIME</h2>
  <div class="time-display" id="dest_display">-- --- ---- --:-- --</div>
  <div class="form-grid">
    <div class="form-row">
      <label>Month</label>
      <input type="number" id="dest_month" min="1" max="12" value="10">
    </div>
    <div class="form-row">
      <label>Day</label>
      <input type="number" id="dest_day" min="1" max="31" value="26">
    </div>
    <div class="form-row">
      <label>Year</label>
      <input type="number" id="dest_year" min="1900" max="2100" value="1985">
    </div>
    <div class="form-row">
      <label>Hour (1-12)</label>
      <input type="number" id="dest_hour" min="1" max="12" value="1">
    </div>
    <div class="form-row">
      <label>Minute</label>
      <input type="number" id="dest_minute" min="0" max="59" value="20">
    </div>
    <div class="form-row">
      <label>AM/PM</label>
      <select id="dest_ampm">
        <option value="AM">AM</option>
        <option value="PM">PM</option>
      </select>
    </div>
  </div>
  <div class="alarm-toggle">
    <label for="dest_alarm">🔔 Attiva sveglia</label>
    <input type="checkbox" id="dest_alarm">
  </div>
</div>

<div class="panel panel-green">
  <h2>⏱️ PRESENT TIME</h2>
  <div class="time-display" id="pres_display">-- --- ---- --:--:-- --</div>
</div>

<div class="panel panel-amber">
  <h2>🕰️ LAST TIME DEPARTED</h2>
  <div class="time-display" id="last_display">-- --- ---- --:-- --</div>
  <div class="form-grid">
    <div class="form-row">
      <label>Month</label>
      <input type="number" id="last_month" min="1" max="12" value="11">
    </div>
    <div class="form-row">
      <label>Day</label>
      <input type="number" id="last_day" min="1" max="31" value="5">
    </div>
    <div class="form-row">
      <label>Year</label>
      <input type="number" id="last_year" min="1900" max="2100" value="1955">
    </div>
    <div class="form-row">
      <label>Hour (1-12)</label>
      <input type="number" id="last_hour" min="1" max="12" value="6">
    </div>
    <div class="form-row">
      <label>Minute</label>
      <input type="number" id="last_minute" min="0" max="59" value="0">
    </div>
    <div class="form-row">
      <label>AM/PM</label>
      <select id="last_ampm">
        <option value="AM">AM</option>
        <option value="PM">PM</option>
      </select>
    </div>
  </div>
  <div class="alarm-toggle">
    <label for="last_alarm">🔔 Attiva sveglia</label>
    <input type="checkbox" id="last_alarm">
  </div>
</div>

<button onclick="saveConfig()">💾 SALVA CONFIGURAZIONE</button>

<div class="status" id="status">⚡ Sistema pronto ⚡</div>

</div>

<script>
const monthNames = ['', 'JAN', 'FEB', 'MAR', 'APR', 'MAY', 'JUN', 'JUL', 'AUG', 'SEP', 'OCT', 'NOV', 'DEC'];

// Aggiorna il display visivo di un pannello
function updateDisplay(prefix, month, day, year, hour, minute, second = null, ampmOverride = null) {
  const monthName = monthNames[month] || '---';
  const dayStr = String(day).padStart(2, '0');
  const yearStr = String(year);
  const hourStr = String(hour).padStart(2, '0');
  const minStr = String(minute).padStart(2, '0');

  let timeStr = `${hourStr}:${minStr}`;
  if (second !== null) {
    const secStr = String(second).padStart(2, '0');
    timeStr += `:${secStr}`;
  }

  // Ottieni AM/PM: usa override se fornito, altrimenti dal DOM
  let ampm = ampmOverride;
  if (!ampm) {
    const ampmElement = document.getElementById(`${prefix}_ampm`);
    if (ampmElement) {
      ampm = ampmElement.value || ampmElement.textContent || 'AM';
    } else {
      ampm = 'AM';
    }
  }

  const display = `${monthName} ${dayStr} ${yearStr} ${timeStr} ${ampm}`;
  const displayElement = document.getElementById(`${prefix}_display`);
  if (displayElement) {
    displayElement.textContent = display;
  }
}

// Aggiorna i display quando cambiano i valori
function setupInputListeners(prefix) {
  ['month', 'day', 'year', 'hour', 'minute', 'ampm'].forEach(field => {
    const element = document.getElementById(`${prefix}_${field}`);
    if (element) {
      element.addEventListener('change', () => updateDisplayFromInputs(prefix));
      element.addEventListener('input', () => updateDisplayFromInputs(prefix));
    }
  });
}

function updateDisplayFromInputs(prefix) {
  const month = parseInt(document.getElementById(`${prefix}_month`).value) || 1;
  const day = parseInt(document.getElementById(`${prefix}_day`).value) || 1;
  const year = parseInt(document.getElementById(`${prefix}_year`).value) || 2000;
  const hour = parseInt(document.getElementById(`${prefix}_hour`).value) || 12;
  const minute = parseInt(document.getElementById(`${prefix}_minute`).value) || 0;
  updateDisplay(prefix, month, day, year, hour, minute);
}

// Carica configurazione corrente
async function loadConfig() {
  try {
    const resp = await fetch('/bttf/config');
    const data = await resp.json();

    // DESTINATION TIME
    document.getElementById('dest_month').value = data.destination.month;
    document.getElementById('dest_day').value = data.destination.day;
    document.getElementById('dest_year').value = data.destination.year;
    document.getElementById('dest_hour').value = data.destination.hour;
    document.getElementById('dest_minute').value = data.destination.minute;
    document.getElementById('dest_ampm').value = data.destination.ampm;
    document.getElementById('dest_alarm').checked = data.destination.alarmEnabled;
    updateDisplay('dest', data.destination.month, data.destination.day, data.destination.year,
                  data.destination.hour, data.destination.minute);

    // LAST TIME DEPARTED
    document.getElementById('last_month').value = data.lastDeparted.month;
    document.getElementById('last_day').value = data.lastDeparted.day;
    document.getElementById('last_year').value = data.lastDeparted.year;
    document.getElementById('last_hour').value = data.lastDeparted.hour;
    document.getElementById('last_minute').value = data.lastDeparted.minute;
    document.getElementById('last_ampm').value = data.lastDeparted.ampm;
    document.getElementById('last_alarm').checked = data.lastDeparted.alarmEnabled;
    updateDisplay('last', data.lastDeparted.month, data.lastDeparted.day, data.lastDeparted.year,
                  data.lastDeparted.hour, data.lastDeparted.minute);

    const statusEl = document.getElementById('status');
    statusEl.textContent = '✅ Configurazione caricata';
    statusEl.className = 'status success';
    setTimeout(() => { statusEl.className = 'status'; }, 2000);
  } catch(err) {
    const statusEl = document.getElementById('status');
    statusEl.textContent = '❌ Errore caricamento';
    statusEl.className = 'status error';
  }
}

// Salva configurazione
async function saveConfig() {
  console.log('=== SAVE CONFIG CALLED ===');

  const data = {
    dest_month: document.getElementById('dest_month').value,
    dest_day: document.getElementById('dest_day').value,
    dest_year: document.getElementById('dest_year').value,
    dest_hour: document.getElementById('dest_hour').value,
    dest_minute: document.getElementById('dest_minute').value,
    dest_ampm: document.getElementById('dest_ampm').value,
    dest_alarm: document.getElementById('dest_alarm').checked ? 1 : 0,
    last_month: document.getElementById('last_month').value,
    last_day: document.getElementById('last_day').value,
    last_year: document.getElementById('last_year').value,
    last_hour: document.getElementById('last_hour').value,
    last_minute: document.getElementById('last_minute').value,
    last_ampm: document.getElementById('last_ampm').value,
    last_alarm: document.getElementById('last_alarm').checked ? 1 : 0
  };

  console.log('Data to save:', data);

  const params = new URLSearchParams(data);
  const url = `/bttf/save?${params}`;
  console.log('Request URL:', url);

  const statusEl = document.getElementById('status');

  try {
    statusEl.textContent = '⏳ Salvataggio in corso...';
    statusEl.className = 'status';

    console.log('Sending fetch request...');
    const resp = await fetch(url);
    console.log('Response received:', resp.status, resp.statusText);

    if (resp.ok) {
      const responseText = await resp.text();
      console.log('Response text:', responseText);

      statusEl.textContent = '✅ Salvato con successo!';
      statusEl.className = 'status success';

      // Aggiorna i display dopo il salvataggio
      updateDisplayFromInputs('dest');
      updateDisplayFromInputs('last');

      setTimeout(() => { statusEl.className = 'status'; }, 3000);
    } else {
      console.error('Response not OK:', resp.status);
      statusEl.textContent = '❌ Errore salvataggio';
      statusEl.className = 'status error';
    }
  } catch(err) {
    console.error('Fetch error:', err);
    statusEl.textContent = '❌ Errore connessione: ' + err.message;
    statusEl.className = 'status error';
  }
  console.log('=== SAVE CONFIG COMPLETED ===');
}

// Aggiorna PRESENT TIME in tempo reale
async function updatePresentTime() {
  try {
    const resp = await fetch('/bttf/presenttime');
    const data = await resp.json();

    updateDisplay('pres', data.month, data.day, data.year,
                  data.hour, data.minute, data.second, data.ampm);
  } catch(err) {
    console.error('Errore aggiornamento present time:', err);
  }
}

// Setup listeners per aggiornare i display in tempo reale
setupInputListeners('dest');
setupInputListeners('last');

// Carica al caricamento pagina
loadConfig();

// Aggiorna present time subito e poi ogni secondo
updatePresentTime();
setInterval(updatePresentTime, 1000);
</script>
</body>
</html>
)=====";

  request->send(200, "text/html", html);
}

// GET /bttf/config - Restituisce configurazione corrente in JSON
void handleGetConfig(AsyncWebServerRequest *request) {
  String json = "{\n";

  json += "  \"destination\": {\n";
  json += "    \"month\": " + String(destinationTime.month) + ",\n";
  json += "    \"day\": " + String(destinationTime.day) + ",\n";
  json += "    \"year\": " + String(destinationTime.year) + ",\n";
  json += "    \"hour\": " + String(destinationTime.hour) + ",\n";
  json += "    \"minute\": " + String(destinationTime.minute) + ",\n";
  json += "    \"ampm\": \"" + String(destinationTime.ampm) + "\",\n";
  json += "    \"alarmEnabled\": " + String(alarmDestinationEnabled ? "true" : "false") + "\n";
  json += "  },\n";

  json += "  \"lastDeparted\": {\n";
  json += "    \"month\": " + String(lastDeparted.month) + ",\n";
  json += "    \"day\": " + String(lastDeparted.day) + ",\n";
  json += "    \"year\": " + String(lastDeparted.year) + ",\n";
  json += "    \"hour\": " + String(lastDeparted.hour) + ",\n";
  json += "    \"minute\": " + String(lastDeparted.minute) + ",\n";
  json += "    \"ampm\": \"" + String(lastDeparted.ampm) + "\",\n";
  json += "    \"alarmEnabled\": " + String(alarmLastDepartedEnabled ? "true" : "false") + "\n";
  json += "  }\n";
  json += "}\n";

  request->send(200, "application/json", json);
}

// GET /bttf/presenttime - Restituisce l'orario corrente in tempo reale
void handleGetPresentTime(AsyncWebServerRequest *request) {
  // Ottieni l'ora corrente da ezTime
  int hour24 = myTZ.hour();
  int minute = myTZ.minute();
  int second = myTZ.second();
  int day = myTZ.day();
  int month = myTZ.month();
  int year = myTZ.year();

  // Converti da formato 24h a 12h con AM/PM
  String ampm = (hour24 >= 12) ? "PM" : "AM";
  int hour12 = hour24 % 12;
  if (hour12 == 0) hour12 = 12;  // 00:xx diventa 12:xx AM

  String json = "{\n";
  json += "  \"month\": " + String(month) + ",\n";
  json += "  \"day\": " + String(day) + ",\n";
  json += "  \"year\": " + String(year) + ",\n";
  json += "  \"hour\": " + String(hour12) + ",\n";
  json += "  \"minute\": " + String(minute) + ",\n";
  json += "  \"second\": " + String(second) + ",\n";
  json += "  \"ampm\": \"" + ampm + "\"\n";
  json += "}\n";

  request->send(200, "application/json", json);
}

// GET /bttf/save - Salva configurazione
void handleSaveConfig(AsyncWebServerRequest *request) {
  Serial.println("=====================================");
  Serial.println("[BTTF WEB] *** RICEVUTA RICHIESTA SALVATAGGIO ***");

  // DESTINATION TIME
  if (request->hasParam("dest_month")) {
    destinationTime.month = request->getParam("dest_month")->value().toInt();
  }
  if (request->hasParam("dest_day")) {
    destinationTime.day = request->getParam("dest_day")->value().toInt();
  }
  if (request->hasParam("dest_year")) {
    destinationTime.year = request->getParam("dest_year")->value().toInt();
  }
  if (request->hasParam("dest_hour")) {
    destinationTime.hour = request->getParam("dest_hour")->value().toInt();
  }
  if (request->hasParam("dest_minute")) {
    destinationTime.minute = request->getParam("dest_minute")->value().toInt();
  }
  if (request->hasParam("dest_ampm")) {
    String ampm = request->getParam("dest_ampm")->value();
    destinationTime.ampm = (ampm == "PM") ? "PM" : "AM";
  }
  if (request->hasParam("dest_alarm")) {
    alarmDestinationEnabled = (request->getParam("dest_alarm")->value().toInt() == 1);
  }

  Serial.printf("[BTTF WEB] DEST ricevuto: %d/%d/%d %d:%02d %s (alarm=%d)\n",
                destinationTime.month, destinationTime.day, destinationTime.year,
                destinationTime.hour, destinationTime.minute, destinationTime.ampm,
                alarmDestinationEnabled);

  // LAST TIME DEPARTED
  if (request->hasParam("last_month")) {
    lastDeparted.month = request->getParam("last_month")->value().toInt();
  }
  if (request->hasParam("last_day")) {
    lastDeparted.day = request->getParam("last_day")->value().toInt();
  }
  if (request->hasParam("last_year")) {
    lastDeparted.year = request->getParam("last_year")->value().toInt();
  }
  if (request->hasParam("last_hour")) {
    lastDeparted.hour = request->getParam("last_hour")->value().toInt();
  }
  if (request->hasParam("last_minute")) {
    lastDeparted.minute = request->getParam("last_minute")->value().toInt();
  }
  if (request->hasParam("last_ampm")) {
    String ampm = request->getParam("last_ampm")->value();
    lastDeparted.ampm = (ampm == "PM") ? "PM" : "AM";
  }
  if (request->hasParam("last_alarm")) {
    alarmLastDepartedEnabled = (request->getParam("last_alarm")->value().toInt() == 1);
  }

  Serial.printf("[BTTF WEB] LAST ricevuto: %d/%d/%d %d:%02d %s (alarm=%d)\n",
                lastDeparted.month, lastDeparted.day, lastDeparted.year,
                lastDeparted.hour, lastDeparted.minute, lastDeparted.ampm,
                alarmLastDepartedEnabled);

  // Salva su SD card
  Serial.println("[BTTF WEB] Salvataggio su SD card...");
  bool saved = saveBTTFConfigToSD();

  if (saved) {
    Serial.println("[BTTF WEB] ✓ Salvato con successo su SD");
    // Reset flag allarmi per permettere riattivazione
    alarmDestinationTriggered = false;
    alarmLastDepartedTriggered = false;
  } else {
    Serial.println("[BTTF WEB] ✗ ERRORE salvataggio su SD!");
  }

  // Aggiorna display
  Serial.println("[BTTF WEB] Chiamata forceBTTFRedraw()...");
  forceBTTFRedraw();
  Serial.println("[BTTF WEB] forceBTTFRedraw() eseguita");
  Serial.println("=====================================");

  request->send(200, "text/plain", saved ? "OK" : "ERROR");
}

// GET /settime - Imposta manualmente data e ora per test
// Esempio: http://192.168.1.X:8080/settime?year=2025&month=1&day=1&hour=0&minute=0&second=0
void handleSetTime(AsyncWebServerRequest *request) {
  if (!request->hasParam("year") || !request->hasParam("month") || !request->hasParam("day") ||
      !request->hasParam("hour") || !request->hasParam("minute") || !request->hasParam("second")) {
    request->send(400, "text/plain", "Parametri mancanti. Usa: /settime?year=2025&month=1&day=1&hour=0&minute=0&second=0");
    return;
  }

  int year = request->getParam("year")->value().toInt();
  int month = request->getParam("month")->value().toInt();
  int day = request->getParam("day")->value().toInt();
  int hour = request->getParam("hour")->value().toInt();
  int minute = request->getParam("minute")->value().toInt();
  int second = request->getParam("second")->value().toInt();

  // Costruisci la stringa di data/ora per logging
  char dateTimeStr[30];
  sprintf(dateTimeStr, "%04d-%02d-%02d %02d:%02d:%02d", year, month, day, hour, minute, second);

  // Converti in timestamp Unix (secondi dal 1970-01-01)
  struct tm timeinfo;
  timeinfo.tm_year = year - 1900;  // tm_year è anni dal 1900
  timeinfo.tm_mon = month - 1;     // tm_mon è 0-11
  timeinfo.tm_mday = day;
  timeinfo.tm_hour = hour;
  timeinfo.tm_min = minute;
  timeinfo.tm_sec = second;
  timeinfo.tm_isdst = -1;          // Auto-determina DST

  time_t timestamp = mktime(&timeinfo);

  // Imposta l'ora manualmente usando ezTime
  myTZ.setTime(timestamp);

  Serial.printf("[TEST] Ora impostata manualmente: %s\n", dateTimeStr);
  Serial.printf("[TEST] Verifica: %02d/%02d/%d %02d:%02d:%02d\n",
                myTZ.day(), myTZ.month(), myTZ.year(),
                myTZ.hour(), myTZ.minute(), myTZ.second());

  String response = "OK - Ora impostata: " + String(dateTimeStr);
  response += "\nVerifica: " + myTZ.dateTime("d/m/Y H:i:s");
  request->send(200, "text/plain", response);
}

// Registra gli endpoint sul server (chiamato da setup)
void setup_bttf_webserver(AsyncWebServer* server) {
  Serial.println("=====================================");
  Serial.println("[BTTF WEB] Registrazione endpoints...");
  Serial.println("[BTTF WEB] IMPORTANTE: endpoints specifici prima di quelli generici!");

  // IMPORTANTE: Registrare gli endpoint più specifici PRIMA di quelli generici
  // Altrimenti /bttf cattura tutto (incluso /bttf/save, /bttf/config, ecc.)

  server->on("/bttf/presenttime", HTTP_GET, handleGetPresentTime);
  Serial.println("[BTTF WEB] ✓ Registrato: GET /bttf/presenttime");

  server->on("/bttf/save", HTTP_GET, handleSaveConfig);
  Serial.println("[BTTF WEB] ✓ Registrato: GET /bttf/save");

  server->on("/bttf/config", HTTP_GET, handleGetConfig);
  Serial.println("[BTTF WEB] ✓ Registrato: GET /bttf/config");

  server->on("/settime", HTTP_GET, handleSetTime);
  Serial.println("[BTTF WEB] ✓ Registrato: GET /settime");

  // /bttf DEVE essere registrato per ULTIMO per non catturare gli altri endpoint
  server->on("/bttf", HTTP_GET, handleBTTFEditor);
  Serial.println("[BTTF WEB] ✓ Registrato: GET /bttf (pagina principale)");

  Serial.println("[BTTF WEB] Tutti gli endpoints registrati con successo!");
  Serial.println("=====================================");
}

#endif
