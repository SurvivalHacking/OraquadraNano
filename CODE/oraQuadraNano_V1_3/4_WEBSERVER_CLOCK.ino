// ================== WEB SERVER PER CONFIGURAZIONE OROLOGIO ANALOGICO ==================
//
// Questo file implementa un web server con interfaccia grafica per configurare
// l'orologio analogico in tempo reale, includendo:
// - Colori delle lancette (ore, minuti, secondi)
// - Posizione della data
// - Visibilità della data
// - Upload di nuove skin (immagini di sfondo)
//
// Accesso: http://<IP_ESP32>/clock
//

#ifdef EFFECT_ANALOG_CLOCK

// Funzione helper per convertire RGB888 in RGB565
uint16_t rgb888to565(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

// Funzione helper per convertire RGB565 in componenti RGB888
void rgb565to888(uint16_t color, uint8_t &r, uint8_t &g, uint8_t &b) {
  r = (color >> 8) & 0xF8;
  g = (color >> 3) & 0xFC;
  b = (color << 3) & 0xF8;
}

// Funzione helper per convertire byte in hex string con padding (es: 15 -> "0f")
String byteToHex(uint8_t value) {
  String hex = String(value, HEX);
  if (hex.length() < 2) {
    hex = "0" + hex;
  }
  return hex;
}

// Funzione per salvare visibilità data in EEPROM (unica impostazione globale)
void saveClockDateVisibility() {
  // Salva visibilità data (impostazione globale non legata alla skin)
  EEPROM.write(EEPROM_CLOCK_DATE_ADDR, showClockDate ? 1 : 0);
  EEPROM.commit();

  Serial.println("[WEBSERVER] Configurazione orologio salvata in EEPROM");
}

// Pagina HTML per la configurazione dell'orologio - Parte 1 (HTML + CSS)
const char CLOCK_CONFIG_HTML_PART1[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="it">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Configurazione Orologio Analogico</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }

        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            padding: 20px;
            color: #333;
        }

        .container {
            max-width: 900px;
            margin: 0 auto;
            background: white;
            border-radius: 20px;
            box-shadow: 0 20px 60px rgba(0,0,0,0.3);
            overflow: hidden;
        }

        .header {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            padding: 30px;
            text-align: center;
        }

        .header h1 {
            font-size: 2rem;
            margin-bottom: 10px;
        }

        .header p {
            opacity: 0.9;
        }

        .content {
            padding: 30px;
        }

        .config-section {
            margin-bottom: 20px;
            padding: 15px;
            background: #f8f9fa;
            border-radius: 10px;
        }

        .config-section h2 {
            font-size: 1.2rem;
            margin-bottom: 15px;
            color: #667eea;
            border-bottom: 2px solid #667eea;
            padding-bottom: 8px;
        }

        .config-group {
            margin-bottom: 20px;
        }

        .config-group label {
            display: block;
            font-weight: 600;
            margin-bottom: 8px;
            color: #555;
        }

        .color-input-group {
            display: flex;
            align-items: center;
            gap: 15px;
        }

        input[type="color"] {
            width: 80px;
            height: 40px;
            border: 2px solid #ddd;
            border-radius: 8px;
            cursor: pointer;
            transition: transform 0.2s;
        }

        input[type="color"]:hover {
            transform: scale(1.1);
        }

        .toggle-switch {
            position: relative;
            display: inline-block;
            width: 60px;
            height: 34px;
        }

        .toggle-switch input {
            opacity: 0;
            width: 0;
            height: 0;
        }

        .slider {
            position: absolute;
            cursor: pointer;
            top: 0;
            left: 0;
            right: 0;
            bottom: 0;
            background-color: #ccc;
            transition: .4s;
            border-radius: 34px;
        }

        .slider:before {
            position: absolute;
            content: "";
            height: 26px;
            width: 26px;
            left: 4px;
            bottom: 4px;
            background-color: white;
            transition: .4s;
            border-radius: 50%;
        }

        input:checked + .slider {
            background-color: #667eea;
        }

        input:checked + .slider:before {
            transform: translateX(26px);
        }

        .btn {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            border: none;
            padding: 15px 30px;
            border-radius: 10px;
            font-size: 1rem;
            font-weight: 600;
            cursor: pointer;
            transition: transform 0.2s, box-shadow 0.2s;
            margin-right: 10px;
        }

        .btn:hover {
            transform: translateY(-2px);
            box-shadow: 0 10px 20px rgba(102, 126, 234, 0.4);
        }

        .btn:active {
            transform: translateY(0);
        }

        .btn-secondary {
            background: #6c757d;
        }

        .upload-area {
            border: 2px dashed #667eea;
            border-radius: 10px;
            padding: 30px;
            text-align: center;
            transition: background 0.3s;
        }

        .upload-area:hover {
            background: #f0f4ff;
        }

        .upload-area input[type="file"] {
            display: none;
        }

        .upload-label {
            cursor: pointer;
            color: #667eea;
            font-weight: 600;
        }

        .status-message {
            padding: 15px;
            border-radius: 10px;
            margin-top: 20px;
            display: none;
        }

        .status-message.success {
            background: #d4edda;
            color: #155724;
            border: 1px solid #c3e6cb;
        }

        .status-message.error {
            background: #f8d7da;
            color: #721c24;
            border: 1px solid #f5c6cb;
        }

        .skin-card {
            background: white;
            border-radius: 10px;
            overflow: hidden;
            box-shadow: 0 4px 6px rgba(0,0,0,0.1);
            transition: transform 0.2s, box-shadow 0.2s;
            cursor: pointer;
        }

        .skin-card:hover {
            transform: translateY(-5px);
            box-shadow: 0 8px 15px rgba(102, 126, 234, 0.3);
        }

        .skin-card.active {
            border: 3px solid #667eea;
            box-shadow: 0 8px 20px rgba(102, 126, 234, 0.5);
        }

        .skin-card img {
            width: 100%;
            height: 150px;
            object-fit: cover;
            display: block;
        }

        .skin-card .skin-info {
            padding: 10px;
            text-align: center;
        }

        .skin-card .skin-name {
            font-weight: 600;
            color: #333;
            font-size: 0.9rem;
            margin-bottom: 5px;
            word-break: break-all;
        }

        .skin-card .skin-size {
            font-size: 0.75rem;
            color: #999;
        }

        .skin-card .btn-select {
            width: 100%;
            padding: 8px;
            background: #667eea;
            color: white;
            border: none;
            border-radius: 0;
            cursor: pointer;
            font-weight: 600;
            transition: background 0.2s;
        }

        .skin-card .btn-select:hover {
            background: #5568d3;
        }

        .skin-card.active .btn-select {
            background: #28a745;
        }

        /* Area preview interattiva drag & drop */
        .interactive-preview {
            margin: 20px auto;
            padding: 15px;
            background: #f8f9fa;
            border-radius: 15px;
            max-width: 600px;
        }

        .interactive-preview h3 {
            text-align: center;
            color: #667eea;
            margin-bottom: 15px;
            font-size: 1.2rem;
        }

        .clock-preview-container {
            position: relative;
            width: 480px;
            height: 480px;
            margin: 0 auto;
            background-color: #333;
            background-size: 480px 480px;
            background-position: center center;
            background-repeat: no-repeat;
            border-radius: 50%;
            box-shadow:
                0 10px 40px rgba(0,0,0,0.4),
                inset 0 0 0 8px rgba(85, 85, 85, 0.5);
            overflow: hidden;
        }

        .draggable-field {
            position: absolute;
            padding: 0;
            background: transparent;
            border: 2px solid;
            border-radius: 4px;
            cursor: move;
            font-weight: bold;
            font-size: 16px;
            user-select: none;
            box-shadow: 0 2px 8px rgba(0,0,0,0.5);
            transition: transform 0.1s ease, box-shadow 0.1s ease;
            z-index: 10;
            line-height: 1;
        }

        .draggable-field:hover {
            transform: scale(1.05);
            box-shadow: 0 6px 20px rgba(0,0,0,0.5);
            z-index: 10;
        }

        .draggable-field.dragging {
            opacity: 0.7;
            transform: scale(1.1);
            box-shadow: 0 8px 30px rgba(102, 126, 234, 0.6);
            z-index: 100;
        }

        .draggable-field.weekday {
            border-color: #667eea;
            background: rgba(102, 126, 234, 0.9);
            color: white;
        }

        .draggable-field.day {
            border-color: #28a745;
            background: rgba(40, 167, 69, 0.9);
            color: white;
        }

        .draggable-field.month {
            border-color: #ffc107;
            background: rgba(255, 193, 7, 0.9);
            color: #333;
        }

        .draggable-field.year {
            border-color: #dc3545;
            background: rgba(220, 53, 69, 0.9);
            color: white;
        }

        .preview-instructions {
            text-align: center;
            padding: 15px;
            background: linear-gradient(135deg, #667eea15, #764ba215);
            border-radius: 10px;
            margin-top: 15px;
            font-size: 0.95rem;
            color: #555;
        }

        .preview-instructions strong {
            color: #667eea;
        }

        .preview-loading {
            position: absolute;
            top: 50%;
            left: 50%;
            transform: translate(-50%, -50%);
            background: rgba(0, 0, 0, 0.7);
            color: white;
            padding: 15px 30px;
            border-radius: 10px;
            font-weight: bold;
            z-index: 1000;
            display: none;
        }

        .preview-loading.show {
            display: block;
        }

        /* Griglia di debug (opzionale) */
        .debug-grid {
            position: absolute;
            top: 0;
            left: 0;
            width: 480px;
            height: 480px;
            pointer-events: none;
            z-index: 5;
            display: none;
        }

        .debug-grid.show {
            display: block;
        }

        .debug-grid line {
            stroke: red;
            stroke-width: 1;
            opacity: 0.5;
        }

        .debug-grid text {
            fill: red;
            font-size: 10px;
            font-weight: bold;
        }

        .auto-save-indicator {
            position: fixed;
            top: 20px;
            right: 20px;
            padding: 15px 25px;
            background: #28a745;
            color: white;
            border-radius: 30px;
            box-shadow: 0 4px 15px rgba(40, 167, 69, 0.4);
            font-weight: bold;
            opacity: 0;
            transition: opacity 0.3s ease;
            z-index: 1000;
        }

        .auto-save-indicator.show {
            opacity: 1;
        }

        .auto-save-indicator.saving {
            background: #ffc107;
            color: #333;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>Configurazione Orologio Analogico</h1>
            <p>Personalizza colori, posizione e skin in tempo reale</p>
        </div>

        <div class="content">
            <!-- Indicatore Auto-Save -->
            <div id="autoSaveIndicator" class="auto-save-indicator">
                Salvato automaticamente!
            </div>

            <!-- Messaggio informativo compatto -->
            <div style="background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white; padding: 12px 15px; border-radius: 10px; margin-bottom: 15px; text-align: center; font-size: 0.9rem;">
                <strong>Ogni skin ha configurazione personalizzata!</strong> Trascina i campi data dove preferisci.
            </div>

            <!-- Area Preview Interattiva Drag & Drop -->
            <div class="interactive-preview">
                <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 15px;">
                    <h3 style="margin: 0;">Posiziona i Campi sull'Orologio</h3>
                    <button onclick="toggleDebugGrid()" style="padding: 8px 15px; background: #667eea; color: white; border: none; border-radius: 5px; cursor: pointer; font-size: 0.85rem;">
                        Griglia Debug
                    </button>
                </div>
                <div class="clock-preview-container" id="clockPreview">
                    <!-- Indicatore caricamento -->
                    <div class="preview-loading" id="previewLoading">Caricamento skin...</div>

                    <!-- SVG Lancette Orologio (sempre visibile) -->
                    <svg style="position: absolute; top: 0; left: 0; width: 480px; height: 480px; pointer-events: none; z-index: 4;" viewBox="0 0 480 480" xmlns="http://www.w3.org/2000/svg">
                        <!-- Lancette orologio (centro: 240,240) -->
                        <!-- Lancetta ore con contorno -->
                        <line id="hourHandOutline" x1="240" y1="240" x2="240" y2="160"
                              stroke="#FFFFFF" stroke-width="8" stroke-linecap="round" />
                        <line id="hourHand" x1="240" y1="240" x2="240" y2="160"
                              stroke="#000000" stroke-width="6" stroke-linecap="round" />

                        <!-- Lancetta minuti con contorno -->
                        <line id="minuteHandOutline" x1="240" y1="240" x2="240" y2="130"
                              stroke="#FFFFFF" stroke-width="6" stroke-linecap="round" />
                        <line id="minuteHand" x1="240" y1="240" x2="240" y2="130"
                              stroke="#000000" stroke-width="4" stroke-linecap="round" />

                        <!-- Lancetta secondi con contorno -->
                        <line id="secondHandOutline" x1="240" y1="240" x2="240" y2="120"
                              stroke="#FFFFFF" stroke-width="4" stroke-linecap="round" />
                        <line id="secondHand" x1="240" y1="240" x2="240" y2="120"
                              stroke="#FF0000" stroke-width="2" stroke-linecap="round" />

                        <!-- Perno centrale con contorno -->
                        <circle id="centerPinOutline" cx="240" cy="240" r="8" fill="#FFFFFF" />
                        <circle id="centerPin" cx="240" cy="240" r="6" fill="#000000" />
                    </svg>

                    <!-- Griglia di debug coordinate MILLIMETRATA -->
                    <svg class="debug-grid" id="debugGrid" viewBox="0 0 480 480" xmlns="http://www.w3.org/2000/svg">
                        <!-- Linee verticali sottili ogni 10px -->
                        <g id="gridThinV" stroke-width="0.3" opacity="0.4">
                            <line x1="10" y1="0" x2="10" y2="480" />
                            <line x1="20" y1="0" x2="20" y2="480" />
                            <line x1="30" y1="0" x2="30" y2="480" />
                            <line x1="40" y1="0" x2="40" y2="480" />
                            <line x1="60" y1="0" x2="60" y2="480" />
                            <line x1="70" y1="0" x2="70" y2="480" />
                            <line x1="80" y1="0" x2="80" y2="480" />
                            <line x1="90" y1="0" x2="90" y2="480" />
                            <line x1="110" y1="0" x2="110" y2="480" />
                            <line x1="120" y1="0" x2="120" y2="480" />
                            <line x1="130" y1="0" x2="130" y2="480" />
                            <line x1="140" y1="0" x2="140" y2="480" />
                            <line x1="160" y1="0" x2="160" y2="480" />
                            <line x1="170" y1="0" x2="170" y2="480" />
                            <line x1="180" y1="0" x2="180" y2="480" />
                            <line x1="190" y1="0" x2="190" y2="480" />
                            <line x1="210" y1="0" x2="210" y2="480" />
                            <line x1="220" y1="0" x2="220" y2="480" />
                            <line x1="230" y1="0" x2="230" y2="480" />
                            <line x1="260" y1="0" x2="260" y2="480" />
                            <line x1="270" y1="0" x2="270" y2="480" />
                            <line x1="280" y1="0" x2="280" y2="480" />
                            <line x1="290" y1="0" x2="290" y2="480" />
                            <line x1="310" y1="0" x2="310" y2="480" />
                            <line x1="320" y1="0" x2="320" y2="480" />
                            <line x1="330" y1="0" x2="330" y2="480" />
                            <line x1="340" y1="0" x2="340" y2="480" />
                            <line x1="360" y1="0" x2="360" y2="480" />
                            <line x1="370" y1="0" x2="370" y2="480" />
                            <line x1="380" y1="0" x2="380" y2="480" />
                            <line x1="390" y1="0" x2="390" y2="480" />
                            <line x1="410" y1="0" x2="410" y2="480" />
                            <line x1="420" y1="0" x2="420" y2="480" />
                            <line x1="430" y1="0" x2="430" y2="480" />
                            <line x1="440" y1="0" x2="440" y2="480" />
                            <line x1="460" y1="0" x2="460" y2="480" />
                            <line x1="470" y1="0" x2="470" y2="480" />
                        </g>

                        <!-- Linee orizzontali sottili ogni 10px -->
                        <g id="gridThinH" stroke-width="0.3" opacity="0.4">
                            <line x1="0" y1="10" x2="480" y2="10" />
                            <line x1="0" y1="20" x2="480" y2="20" />
                            <line x1="0" y1="30" x2="480" y2="30" />
                            <line x1="0" y1="40" x2="480" y2="40" />
                            <line x1="0" y1="60" x2="480" y2="60" />
                            <line x1="0" y1="70" x2="480" y2="70" />
                            <line x1="0" y1="80" x2="480" y2="80" />
                            <line x1="0" y1="90" x2="480" y2="90" />
                            <line x1="0" y1="110" x2="480" y2="110" />
                            <line x1="0" y1="120" x2="480" y2="120" />
                            <line x1="0" y1="130" x2="480" y2="130" />
                            <line x1="0" y1="140" x2="480" y2="140" />
                            <line x1="0" y1="160" x2="480" y2="160" />
                            <line x1="0" y1="170" x2="480" y2="170" />
                            <line x1="0" y1="180" x2="480" y2="180" />
                            <line x1="0" y1="190" x2="480" y2="190" />
                            <line x1="0" y1="210" x2="480" y2="210" />
                            <line x1="0" y1="220" x2="480" y2="220" />
                            <line x1="0" y1="230" x2="480" y2="230" />
                            <line x1="0" y1="260" x2="480" y2="260" />
                            <line x1="0" y1="270" x2="480" y2="270" />
                            <line x1="0" y1="280" x2="480" y2="280" />
                            <line x1="0" y1="290" x2="480" y2="290" />
                            <line x1="0" y1="310" x2="480" y2="310" />
                            <line x1="0" y1="320" x2="480" y2="320" />
                            <line x1="0" y1="330" x2="480" y2="330" />
                            <line x1="0" y1="340" x2="480" y2="340" />
                            <line x1="0" y1="360" x2="480" y2="360" />
                            <line x1="0" y1="370" x2="480" y2="370" />
                            <line x1="0" y1="380" x2="480" y2="380" />
                            <line x1="0" y1="390" x2="480" y2="390" />
                            <line x1="0" y1="410" x2="480" y2="410" />
                            <line x1="0" y1="420" x2="480" y2="420" />
                            <line x1="0" y1="430" x2="480" y2="430" />
                            <line x1="0" y1="440" x2="480" y2="440" />
                            <line x1="0" y1="460" x2="480" y2="460" />
                            <line x1="0" y1="470" x2="480" y2="470" />
                        </g>

                        <!-- Linee verticali principali ogni 50px (più spesse) -->
                        <g id="gridThickV" stroke-width="0.8" opacity="0.8">
                            <line x1="0" y1="0" x2="0" y2="480" />
                            <line x1="50" y1="0" x2="50" y2="480" />
                            <line x1="100" y1="0" x2="100" y2="480" />
                            <line x1="150" y1="0" x2="150" y2="480" />
                            <line x1="200" y1="0" x2="200" y2="480" />
                            <line x1="250" y1="0" x2="250" y2="480" />
                            <line x1="300" y1="0" x2="300" y2="480" />
                            <line x1="350" y1="0" x2="350" y2="480" />
                            <line x1="400" y1="0" x2="400" y2="480" />
                            <line x1="450" y1="0" x2="450" y2="480" />
                            <line x1="479" y1="0" x2="479" y2="480" />
                        </g>

                        <!-- Linee orizzontali principali ogni 50px (più spesse) -->
                        <g id="gridThickH" stroke-width="0.8" opacity="0.8">
                            <line x1="0" y1="0" x2="480" y2="0" />
                            <line x1="0" y1="50" x2="480" y2="50" />
                            <line x1="0" y1="100" x2="480" y2="100" />
                            <line x1="0" y1="150" x2="480" y2="150" />
                            <line x1="0" y1="200" x2="480" y2="200" />
                            <line x1="0" y1="250" x2="480" y2="250" />
                            <line x1="0" y1="300" x2="480" y2="300" />
                            <line x1="0" y1="350" x2="480" y2="350" />
                            <line x1="0" y1="400" x2="480" y2="400" />
                            <line x1="0" y1="450" x2="480" y2="450" />
                            <line x1="0" y1="479" x2="480" y2="479" />
                        </g>

                        <!-- Etichette coordinate principali -->
                        <g id="gridText" font-size="10" font-weight="bold">
                            <text x="3" y="12">0</text>
                            <text x="53" y="12">50</text>
                            <text x="98" y="12">100</text>
                            <text x="148" y="12">150</text>
                            <text x="198" y="12">200</text>
                            <text x="248" y="12">250</text>
                            <text x="298" y="12">300</text>
                            <text x="348" y="12">350</text>
                            <text x="398" y="12">400</text>
                            <text x="448" y="12">450</text>

                            <text x="3" y="55">50</text>
                            <text x="3" y="105">100</text>
                            <text x="3" y="155">150</text>
                            <text x="3" y="205">200</text>
                            <text x="3" y="255">250</text>
                            <text x="3" y="305">300</text>
                            <text x="3" y="355">350</text>
                            <text x="3" y="405">400</text>
                            <text x="3" y="455">450</text>
                            <text x="460" y="475">480</text>
                        </g>
                    </svg>

                    <!-- Elementi draggable per ogni campo -->
                    <div class="draggable-field weekday" id="dragWeekday" data-field="weekday">
                        Lun
                    </div>
                    <div class="draggable-field day" id="dragDay" data-field="day">
                        15
                    </div>
                    <div class="draggable-field month" id="dragMonth" data-field="month">
                        Gen
                    </div>
                    <div class="draggable-field year" id="dragYear" data-field="year">
                        2025
                    </div>
                </div>
                <div class="preview-instructions">
                    <strong>Trascina</strong> i campi sulla skin dell'orologio.<br>
                    <strong>Salvataggio automatico</strong> istantaneo al rilascio del mouse.
                </div>
            </div>

            <!-- Sezione Colori Lancette - Compatta -->
            <div class="config-section">
                <h2>Colori Lancette</h2>
                <div style="display: grid; grid-template-columns: repeat(3, 1fr); gap: 15px;">
                    <div style="text-align: center;">
                        <label style="display: block; margin-bottom: 8px; font-weight: 600; color: #555;">Ore</label>
                        <input type="color" id="hourColor" value="#000000" style="width: 100%; height: 50px;">
                    </div>
                    <div style="text-align: center;">
                        <label style="display: block; margin-bottom: 8px; font-weight: 600; color: #555;">Minuti</label>
                        <input type="color" id="minuteColor" value="#000000" style="width: 100%; height: 50px;">
                    </div>
                    <div style="text-align: center;">
                        <label style="display: block; margin-bottom: 8px; font-weight: 600; color: #555;">Secondi</label>
                        <input type="color" id="secondColor" value="#FF0000" style="width: 100%; height: 50px;">
                    </div>
                </div>
            </div>

            <!-- Sezione Lunghezze Lancette -->
            <div class="config-section">
                <h2>Lunghezze Lancette</h2>
                <div style="display: grid; grid-template-columns: repeat(3, 1fr); gap: 20px;">
                    <div>
                        <label style="display: block; margin-bottom: 8px; font-weight: 600; color: #555;">Ore (px)</label>
                        <div style="display: flex; align-items: center; gap: 10px;">
                            <input type="range" id="hourLength" min="30" max="240" value="80" style="flex: 1;">
                            <span id="hourLengthLabel" style="font-weight: bold; color: #667eea; min-width: 40px;">80</span>
                        </div>
                    </div>
                    <div>
                        <label style="display: block; margin-bottom: 8px; font-weight: 600; color: #555;">Minuti (px)</label>
                        <div style="display: flex; align-items: center; gap: 10px;">
                            <input type="range" id="minuteLength" min="30" max="240" value="110" style="flex: 1;">
                            <span id="minuteLengthLabel" style="font-weight: bold; color: #667eea; min-width: 40px;">110</span>
                        </div>
                    </div>
                    <div>
                        <label style="display: block; margin-bottom: 8px; font-weight: 600; color: #555;">Secondi (px)</label>
                        <div style="display: flex; align-items: center; gap: 10px;">
                            <input type="range" id="secondLength" min="30" max="240" value="120" style="flex: 1;">
                            <span id="secondLengthLabel" style="font-weight: bold; color: #dc3545; min-width: 40px;">120</span>
                        </div>
                    </div>
                </div>
            </div>

            <!-- Sezione Configurazione Data Compatta -->
            <div class="config-section">
                <h2>Configurazione Campi Data</h2>

                <div class="config-group">
                    <label>
                        Mostra Data Globalmente
                        <label class="toggle-switch">
                            <input type="checkbox" id="showDate" checked>
                            <span class="slider"></span>
                        </label>
                    </label>
                </div>

                <!-- Griglia compatta per i campi -->
                <div style="display: grid; grid-template-columns: repeat(2, 1fr); gap: 15px; margin-top: 20px;">
                    <!-- Campo: Giorno Settimana -->
                    <div style="border: 2px solid #667eea; border-radius: 10px; padding: 12px;">
                        <h4 style="margin: 0 0 10px 0; color: #667eea; font-size: 0.9rem;">Giorno Settimana</h4>
                        <div style="display: flex; align-items: center; gap: 10px; margin-bottom: 8px;">
                            <label class="toggle-switch" style="margin: 0;">
                                <input type="checkbox" id="weekdayEnabled" checked>
                                <span class="slider"></span>
                            </label>
                            <input type="color" id="weekdayColor" value="#FFFFFF" style="width: 50px; height: 30px;">
                        </div>
                        <div style="display: flex; align-items: center; gap: 8px; margin-top: 8px;">
                            <label style="font-size: 0.85rem; color: #666;">Dimensione:</label>
                            <input type="range" id="weekdayFontSize" min="1" max="5" value="2" style="flex: 1;">
                            <span id="weekdayFontSizeLabel" style="font-size: 0.85rem; color: #667eea; font-weight: bold; min-width: 20px;">2</span>
                        </div>
                        <!-- Hidden inputs per memorizzare X/Y -->
                        <input type="hidden" id="weekdayX" value="190">
                        <input type="hidden" id="weekdayY" value="280">
                    </div>

                    <!-- Campo: Giorno Numero -->
                    <div style="border: 2px solid #28a745; border-radius: 10px; padding: 12px;">
                        <h4 style="margin: 0 0 10px 0; color: #28a745; font-size: 0.9rem;">Giorno Numero</h4>
                        <div style="display: flex; align-items: center; gap: 10px; margin-bottom: 8px;">
                            <label class="toggle-switch" style="margin: 0;">
                                <input type="checkbox" id="dayEnabled" checked>
                                <span class="slider"></span>
                            </label>
                            <input type="color" id="dayColor" value="#FFFFFF" style="width: 50px; height: 30px;">
                        </div>
                        <div style="display: flex; align-items: center; gap: 8px; margin-top: 8px;">
                            <label style="font-size: 0.85rem; color: #666;">Dimensione:</label>
                            <input type="range" id="dayFontSize" min="1" max="5" value="3" style="flex: 1;">
                            <span id="dayFontSizeLabel" style="font-size: 0.85rem; color: #28a745; font-weight: bold; min-width: 20px;">3</span>
                        </div>
                        <input type="hidden" id="dayX" value="210">
                        <input type="hidden" id="dayY" value="320">
                    </div>

                    <!-- Campo: Mese -->
                    <div style="border: 2px solid #ffc107; border-radius: 10px; padding: 12px;">
                        <h4 style="margin: 0 0 10px 0; color: #ffc107; font-size: 0.9rem;">Mese</h4>
                        <div style="display: flex; align-items: center; gap: 10px; margin-bottom: 8px;">
                            <label class="toggle-switch" style="margin: 0;">
                                <input type="checkbox" id="monthEnabled" checked>
                                <span class="slider"></span>
                            </label>
                            <input type="color" id="monthColor" value="#FFFFFF" style="width: 50px; height: 30px;">
                        </div>
                        <div style="display: flex; align-items: center; gap: 8px; margin-top: 8px;">
                            <label style="font-size: 0.85rem; color: #666;">Dimensione:</label>
                            <input type="range" id="monthFontSize" min="1" max="5" value="2" style="flex: 1;">
                            <span id="monthFontSizeLabel" style="font-size: 0.85rem; color: #ffc107; font-weight: bold; min-width: 20px;">2</span>
                        </div>
                        <input type="hidden" id="monthX" value="200">
                        <input type="hidden" id="monthY" value="360">
                    </div>

                    <!-- Campo: Anno -->
                    <div style="border: 2px solid #dc3545; border-radius: 10px; padding: 12px;">
                        <h4 style="margin: 0 0 10px 0; color: #dc3545; font-size: 0.9rem;">Anno</h4>
                        <div style="display: flex; align-items: center; gap: 10px; margin-bottom: 8px;">
                            <label class="toggle-switch" style="margin: 0;">
                                <input type="checkbox" id="yearEnabled" checked>
                                <span class="slider"></span>
                            </label>
                            <input type="color" id="yearColor" value="#FFFFFF" style="width: 50px; height: 30px;">
                        </div>
                        <div style="display: flex; align-items: center; gap: 8px; margin-top: 8px;">
                            <label style="font-size: 0.85rem; color: #666;">Dimensione:</label>
                            <input type="range" id="yearFontSize" min="1" max="5" value="2" style="flex: 1;">
                            <span id="yearFontSizeLabel" style="font-size: 0.85rem; color: #dc3545; font-weight: bold; min-width: 20px;">2</span>
                        </div>
                        <input type="hidden" id="yearX" value="195">
                        <input type="hidden" id="yearY" value="400">
                    </div>
                </div>
            </div>

            <!-- Sezione Galleria Skin -->
            <div class="config-section">
                <h2>Galleria Skin Orologio</h2>

                <div id="skinGallery" style="display: grid; grid-template-columns: repeat(auto-fill, minmax(150px, 1fr)); gap: 15px; margin-bottom: 20px;">
                    <p style="text-align: center; color: #999;">Caricamento galleria...</p>
                </div>

                <div class="upload-area">
                    <p>Carica una nuova skin (JPG, 480x480px)</p>
                    <input type="file" id="skinFile" accept=".jpg,.jpeg">
                    <label for="skinFile" class="upload-label">Scegli File</label>
                    <p id="fileName" style="margin-top: 10px; color: #666;"></p>
                </div>

                <button class="btn" onclick="uploadSkin()" style="margin-top: 15px;">
                    Upload Nuova Skin
                </button>
            </div>

            <!-- Messaggio di stato -->
            <div id="statusMessage" class="status-message"></div>
        </div>
    </div>
)rawliteral";

// Pagina HTML per la configurazione dell'orologio - Parte 2 (JavaScript)
const char CLOCK_CONFIG_HTML_PART2[] PROGMEM = R"rawliteral(
    <script>
        // Variabili globali per drag & drop
        let isDragging = false;
        let currentDragElement = null;
        let offsetX = 0;
        let offsetY = 0;

        // Carica configurazione e galleria all'avvio
        window.onload = function() {
            loadConfig();
            loadSkinGallery();
            initDragAndDrop();
            loadPreviewImage();
            // Aggiorna le lancette con valori di default
            updateClockHands();
        };

        // Adatta i colori della griglia al colore di sfondo dell'immagine
        function adaptGridToBackground(img) {
            try {
                // Crea canvas temporaneo per analizzare i pixel
                const canvas = document.createElement('canvas');
                canvas.width = img.width;
                canvas.height = img.height;
                const ctx = canvas.getContext('2d');
                ctx.drawImage(img, 0, 0);

                // Campiona pixel da varie posizioni dell'immagine
                const samples = [];
                const step = 40; // Campiona ogni 40 pixel

                for (let x = step; x < img.width; x += step) {
                    for (let y = step; y < img.height; y += step) {
                        try {
                            const pixel = ctx.getImageData(x, y, 1, 1).data;
                            samples.push({
                                r: pixel[0],
                                g: pixel[1],
                                b: pixel[2]
                            });
                        } catch(e) {
                            // Ignora errori di accesso pixel
                        }
                    }
                }

                if (samples.length === 0) {
                    console.warn('[GRID] Impossibile analizzare immagine, uso colori di default');
                    return;
                }

                // Calcola luminosità media (formula standard: 0.299*R + 0.587*G + 0.114*B)
                let totalLuminance = 0;
                samples.forEach(pixel => {
                    const luminance = 0.299 * pixel.r + 0.587 * pixel.g + 0.114 * pixel.b;
                    totalLuminance += luminance;
                });
                const avgLuminance = totalLuminance / samples.length;

                console.log('[GRID] Luminosità media sfondo:', avgLuminance.toFixed(2));

                // Determina colori contrastati
                let thinLineColor, thickLineColor, textColor;

                if (avgLuminance > 128) {
                    // Sfondo CHIARO -> usa linee SCURE
                    thinLineColor = '#000000';   // Nero per linee sottili
                    thickLineColor = '#0000FF';  // Blu per linee principali
                    textColor = '#0000FF';       // Blu per testo
                    console.log('[GRID] Sfondo chiaro -> griglia scura');
                } else {
                    // Sfondo SCURO -> usa linee CHIARE
                    thinLineColor = '#FFFFFF';   // Bianco per linee sottili
                    thickLineColor = '#00FF00';  // Verde per linee principali
                    textColor = '#00FF00';       // Verde per testo
                    console.log('[GRID] Sfondo scuro -> griglia chiara');
                }

                // Applica i colori alla griglia SVG usando gli ID dei gruppi
                const gridThinV = document.getElementById('gridThinV');
                const gridThinH = document.getElementById('gridThinH');
                const gridThickV = document.getElementById('gridThickV');
                const gridThickH = document.getElementById('gridThickH');
                const gridText = document.getElementById('gridText');

                if (gridThinV && gridThinH && gridThickV && gridThickH && gridText) {
                    // Imposta colore linee sottili (applica sul gruppo <g>)
                    gridThinV.setAttribute('stroke', thinLineColor);
                    gridThinH.setAttribute('stroke', thinLineColor);

                    // Imposta colore linee principali (applica sul gruppo <g>)
                    gridThickV.setAttribute('stroke', thickLineColor);
                    gridThickH.setAttribute('stroke', thickLineColor);

                    // Imposta colore testo (applica sul gruppo <g>)
                    gridText.setAttribute('fill', textColor);

                    console.log('[GRID] Colori griglia aggiornati:');
                    console.log('  - Linee sottili:', thinLineColor);
                    console.log('  - Linee principali:', thickLineColor);
                    console.log('  - Testo:', textColor);
                } else {
                    console.error('[GRID] Impossibile trovare gruppi SVG griglia');
                }
            } catch (error) {
                console.error('[GRID] Errore analisi immagine:', error);
                // In caso di errore, mantieni i colori di default
            }
        }

        // Carica l'immagine della skin attiva nella preview
        async function loadPreviewImage() {
            const loadingIndicator = document.getElementById('previewLoading');
            const previewContainer = document.getElementById('clockPreview');

            try {
                // Mostra indicatore di caricamento
                loadingIndicator.classList.add('show');
                loadingIndicator.textContent = 'Caricamento skin...';

                console.log('[DEBUG] Richiesta skin attiva...');
                const activeResponse = await fetch('/clock/activeSkin');
                const activeData = await activeResponse.json();
                const activeSkin = activeData.activeSkin;

                console.log('[DEBUG] Skin attiva ricevuta:', activeSkin);

                if (activeSkin && activeSkin !== '') {
                    // Aggiungi timestamp per evitare cache
                    const timestamp = new Date().getTime();
                    const imageUrl = '/clock/preview?file=' + encodeURIComponent(activeSkin) + '&t=' + timestamp;

                    console.log('[DEBUG] URL immagine:', imageUrl);

                    // Precarica l'immagine prima di impostarla come sfondo
                    const img = new Image();
                    img.crossOrigin = "Anonymous"; // Permetti analisi pixel
                    img.onload = function() {
                        const bgUrl = "url('" + imageUrl + "')";
                        previewContainer.style.backgroundImage = bgUrl;
                        loadingIndicator.classList.remove('show');
                        console.log('[DEBUG] Immagine preview caricata con successo!');
                        console.log('[DEBUG] Background impostato:', bgUrl);

                        // Analizza il colore di sfondo e adatta la griglia
                        adaptGridToBackground(img);
                    };
                    img.onerror = function() {
                        console.error('[ERROR] Impossibile caricare immagine:', activeSkin);
                        console.error('[ERROR] URL tentato:', imageUrl);
                        loadingIndicator.textContent = 'Errore caricamento immagine';
                        setTimeout(() => {
                            loadingIndicator.classList.remove('show');
                        }, 3000);
                    };
                    img.src = imageUrl;
                } else {
                    console.warn('[WARN] Nessuna skin attiva trovata');
                    loadingIndicator.textContent = 'Nessuna skin caricata';
                    setTimeout(() => {
                        loadingIndicator.classList.remove('show');
                    }, 2000);
                }
            } catch (error) {
                console.error('[ERROR] Errore durante caricamento preview:', error);
                loadingIndicator.textContent = 'Errore connessione';
                setTimeout(() => {
                    loadingIndicator.classList.remove('show');
                }, 3000);
            }
        }


        // Event listeners per i colori (aggiorna preview in tempo reale, salva solo al rilascio)
        document.getElementById('hourColor').addEventListener('input', function(e) {
            updateClockHands();
        });
        document.getElementById('hourColor').addEventListener('change', function(e) {
            saveConfig();
        });

        document.getElementById('minuteColor').addEventListener('input', function(e) {
            updateClockHands();
        });
        document.getElementById('minuteColor').addEventListener('change', function(e) {
            saveConfig();
        });

        document.getElementById('secondColor').addEventListener('input', function(e) {
            updateClockHands();
        });
        document.getElementById('secondColor').addEventListener('change', function(e) {
            saveConfig();
        });

        // Event listeners per i colori dei campi data (aggiorna preview, salva solo al rilascio)
        document.getElementById('weekdayColor').addEventListener('input', function(e) {
            document.getElementById('dragWeekday').style.color = e.target.value;
        });
        document.getElementById('weekdayColor').addEventListener('change', function(e) {
            saveConfig();
        });

        document.getElementById('dayColor').addEventListener('input', function(e) {
            document.getElementById('dragDay').style.color = e.target.value;
        });
        document.getElementById('dayColor').addEventListener('change', function(e) {
            saveConfig();
        });

        document.getElementById('monthColor').addEventListener('input', function(e) {
            document.getElementById('dragMonth').style.color = e.target.value;
        });
        document.getElementById('monthColor').addEventListener('change', function(e) {
            saveConfig();
        });

        document.getElementById('yearColor').addEventListener('input', function(e) {
            document.getElementById('dragYear').style.color = e.target.value;
        });
        document.getElementById('yearColor').addEventListener('change', function(e) {
            saveConfig();
        });

        // Event listener per fontSize sliders - aggiorna preview in tempo reale, salva solo al rilascio
        document.getElementById('weekdayFontSize').addEventListener('input', function(e) {
            document.getElementById('weekdayFontSizeLabel').textContent = e.target.value;
            document.getElementById('dragWeekday').style.fontSize = getFontSizeFromSlider(parseInt(e.target.value));
        });
        document.getElementById('weekdayFontSize').addEventListener('change', function(e) {
            saveConfig();
        });

        document.getElementById('dayFontSize').addEventListener('input', function(e) {
            document.getElementById('dayFontSizeLabel').textContent = e.target.value;
            document.getElementById('dragDay').style.fontSize = getFontSizeFromSlider(parseInt(e.target.value));
        });
        document.getElementById('dayFontSize').addEventListener('change', function(e) {
            saveConfig();
        });

        document.getElementById('monthFontSize').addEventListener('input', function(e) {
            document.getElementById('monthFontSizeLabel').textContent = e.target.value;
            document.getElementById('dragMonth').style.fontSize = getFontSizeFromSlider(parseInt(e.target.value));
        });
        document.getElementById('monthFontSize').addEventListener('change', function(e) {
            saveConfig();
        });

        document.getElementById('yearFontSize').addEventListener('input', function(e) {
            document.getElementById('yearFontSizeLabel').textContent = e.target.value;
            document.getElementById('dragYear').style.fontSize = getFontSizeFromSlider(parseInt(e.target.value));
        });
        document.getElementById('yearFontSize').addEventListener('change', function(e) {
            saveConfig();
        });

        // Event listener per lunghezze lancette - aggiorna preview in tempo reale, salva solo al rilascio
        document.getElementById('hourLength').addEventListener('input', function(e) {
            document.getElementById('hourLengthLabel').textContent = e.target.value;
            updateClockHands();
        });
        document.getElementById('hourLength').addEventListener('change', function(e) {
            saveConfig();
        });

        document.getElementById('minuteLength').addEventListener('input', function(e) {
            document.getElementById('minuteLengthLabel').textContent = e.target.value;
            updateClockHands();
        });
        document.getElementById('minuteLength').addEventListener('change', function(e) {
            saveConfig();
        });

        document.getElementById('secondLength').addEventListener('input', function(e) {
            document.getElementById('secondLengthLabel').textContent = e.target.value;
            updateClockHands();
        });
        document.getElementById('secondLength').addEventListener('change', function(e) {
            saveConfig();
        });

        document.getElementById('skinFile').addEventListener('change', function(e) {
            if (e.target.files.length > 0) {
                document.getElementById('fileName').textContent = e.target.files[0].name;
            }
        });

        // Funzione per convertire valore slider (1-5) in dimensione CSS
        function getFontSizeFromSlider(value) {
            const fontSizes = {
                1: '12px',
                2: '16px',
                3: '20px',
                4: '24px',
                5: '28px'
            };
            return fontSizes[value] || '16px';
        }

        // Funzione per calcolare luminosità di un colore hex
        function getColorBrightness(hexColor) {
            const r = parseInt(hexColor.substr(1, 2), 16);
            const g = parseInt(hexColor.substr(3, 2), 16);
            const b = parseInt(hexColor.substr(5, 2), 16);
            return (r + g + b) / 3;
        }

        // Funzione per aggiornare le lancette SVG nella preview
        function updateClockHands() {
            try {
                const centerX = 240;
                const centerY = 240;

                // Leggi colori e lunghezze
                const hourColor = document.getElementById('hourColor').value;
                const minuteColor = document.getElementById('minuteColor').value;
                const secondColor = document.getElementById('secondColor').value;
                const hourLength = parseInt(document.getElementById('hourLength').value);
                const minuteLength = parseInt(document.getElementById('minuteLength').value);
                const secondLength = parseInt(document.getElementById('secondLength').value);

                console.log('[DEBUG] Aggiornamento lancette:', {hourColor, minuteColor, secondColor, hourLength, minuteLength, secondLength});

            // Calcola colori contorno (bianco se scuro, nero se chiaro)
            const hourOutline = getColorBrightness(hourColor) < 128 ? '#FFFFFF' : '#000000';
            const minuteOutline = getColorBrightness(minuteColor) < 128 ? '#FFFFFF' : '#000000';
            const secondOutline = getColorBrightness(secondColor) < 128 ? '#FFFFFF' : '#000000';

            // Aggiorna lancetta ore (10:10 = 300° per ore, 60° per minuti)
            const hourAngle = 300; // 10 ore
            const hourRad = (hourAngle - 90) * Math.PI / 180;
            document.getElementById('hourHandOutline').setAttribute('x2', centerX + Math.cos(hourRad) * hourLength);
            document.getElementById('hourHandOutline').setAttribute('y2', centerY + Math.sin(hourRad) * hourLength);
            document.getElementById('hourHandOutline').setAttribute('stroke', hourOutline);
            document.getElementById('hourHand').setAttribute('x2', centerX + Math.cos(hourRad) * hourLength);
            document.getElementById('hourHand').setAttribute('y2', centerY + Math.sin(hourRad) * hourLength);
            document.getElementById('hourHand').setAttribute('stroke', hourColor);

            // Aggiorna lancetta minuti (10:10 = 60° per minuti)
            const minuteAngle = 60; // 10 minuti
            const minuteRad = (minuteAngle - 90) * Math.PI / 180;
            document.getElementById('minuteHandOutline').setAttribute('x2', centerX + Math.cos(minuteRad) * minuteLength);
            document.getElementById('minuteHandOutline').setAttribute('y2', centerY + Math.sin(minuteRad) * minuteLength);
            document.getElementById('minuteHandOutline').setAttribute('stroke', minuteOutline);
            document.getElementById('minuteHand').setAttribute('x2', centerX + Math.cos(minuteRad) * minuteLength);
            document.getElementById('minuteHand').setAttribute('y2', centerY + Math.sin(minuteRad) * minuteLength);
            document.getElementById('minuteHand').setAttribute('stroke', minuteColor);

            // Aggiorna lancetta secondi (30 secondi = 180°)
            const secondAngle = 180; // 30 secondi
            const secondRad = (secondAngle - 90) * Math.PI / 180;
            document.getElementById('secondHandOutline').setAttribute('x2', centerX + Math.cos(secondRad) * secondLength);
            document.getElementById('secondHandOutline').setAttribute('y2', centerY + Math.sin(secondRad) * secondLength);
            document.getElementById('secondHandOutline').setAttribute('stroke', secondOutline);
            document.getElementById('secondHand').setAttribute('x2', centerX + Math.cos(secondRad) * secondLength);
            document.getElementById('secondHand').setAttribute('y2', centerY + Math.sin(secondRad) * secondLength);
            document.getElementById('secondHand').setAttribute('stroke', secondColor);

                // Aggiorna perno centrale (usa colore lancetta ore)
                document.getElementById('centerPinOutline').setAttribute('fill', hourOutline);
                document.getElementById('centerPin').setAttribute('fill', hourColor);

                console.log('[DEBUG] Lancette aggiornate con successo');
            } catch (error) {
                console.error('[ERROR] Errore aggiornamento lancette:', error);
            }
        }

        // Carica configurazione dal server
        async function loadConfig() {
            try {
                console.log('[DEBUG] Caricamento configurazione...');
                const response = await fetch('/clock/config');

                // Debug: controlla content-type
                const contentType = response.headers.get('content-type');
                console.log('[DEBUG] Content-Type ricevuto:', contentType);

                if (!contentType || !contentType.includes('application/json')) {
                    const text = await response.text();
                    console.error('[ERROR] Risposta non JSON:', text.substring(0, 200));
                    throw new Error('Server ha restituito HTML invece di JSON');
                }

                const config = await response.json();
                console.log('[DEBUG] Configurazione ricevuta:', config);

                // Colori lancette
                document.getElementById('hourColor').value = config.hourColor;
                document.getElementById('minuteColor').value = config.minuteColor;
                document.getElementById('secondColor').value = config.secondColor;
                document.getElementById('showDate').checked = config.showDate;

                // Lunghezze lancette
                if (config.hourLength !== undefined) {
                    document.getElementById('hourLength').value = config.hourLength;
                    document.getElementById('hourLengthLabel').textContent = config.hourLength;
                }
                if (config.minuteLength !== undefined) {
                    document.getElementById('minuteLength').value = config.minuteLength;
                    document.getElementById('minuteLengthLabel').textContent = config.minuteLength;
                }
                if (config.secondLength !== undefined) {
                    document.getElementById('secondLength').value = config.secondLength;
                    document.getElementById('secondLengthLabel').textContent = config.secondLength;
                }

                // Campo Weekday
                if (config.weekday) {
                    document.getElementById('weekdayEnabled').checked = config.weekday.enabled;
                    document.getElementById('weekdayX').value = config.weekday.x;
                    document.getElementById('weekdayY').value = config.weekday.y;
                    document.getElementById('weekdayColor').value = config.weekday.color;
                    document.getElementById('weekdayFontSize').value = config.weekday.fontSize || 2;
                    document.getElementById('weekdayFontSizeLabel').textContent = config.weekday.fontSize || 2;
                }

                // Campo Day
                if (config.day) {
                    document.getElementById('dayEnabled').checked = config.day.enabled;
                    document.getElementById('dayX').value = config.day.x;
                    document.getElementById('dayY').value = config.day.y;
                    document.getElementById('dayColor').value = config.day.color;
                    document.getElementById('dayFontSize').value = config.day.fontSize || 3;
                    document.getElementById('dayFontSizeLabel').textContent = config.day.fontSize || 3;
                }

                // Campo Month
                if (config.month) {
                    document.getElementById('monthEnabled').checked = config.month.enabled;
                    document.getElementById('monthX').value = config.month.x;
                    document.getElementById('monthY').value = config.month.y;
                    document.getElementById('monthColor').value = config.month.color;
                    document.getElementById('monthFontSize').value = config.month.fontSize || 2;
                    document.getElementById('monthFontSizeLabel').textContent = config.month.fontSize || 2;
                }

                // Campo Year
                if (config.year) {
                    document.getElementById('yearEnabled').checked = config.year.enabled;
                    document.getElementById('yearX').value = config.year.x;
                    document.getElementById('yearY').value = config.year.y;
                    document.getElementById('yearColor').value = config.year.color;
                    document.getElementById('yearFontSize').value = config.year.fontSize || 2;
                    document.getElementById('yearFontSizeLabel').textContent = config.year.fontSize || 2;
                }

                // Aggiorna i colori e le dimensioni degli elementi draggable
                if (config.weekday) {
                    document.getElementById('dragWeekday').style.color = config.weekday.color;
                    document.getElementById('dragWeekday').style.fontSize = getFontSizeFromSlider(config.weekday.fontSize || 2);
                }
                if (config.day) {
                    document.getElementById('dragDay').style.color = config.day.color;
                    document.getElementById('dragDay').style.fontSize = getFontSizeFromSlider(config.day.fontSize || 3);
                }
                if (config.month) {
                    document.getElementById('dragMonth').style.color = config.month.color;
                    document.getElementById('dragMonth').style.fontSize = getFontSizeFromSlider(config.month.fontSize || 2);
                }
                if (config.year) {
                    document.getElementById('dragYear').style.color = config.year.color;
                    document.getElementById('dragYear').style.fontSize = getFontSizeFromSlider(config.year.fontSize || 2);
                }

                // Aggiorna le posizioni degli elementi draggable
                updateDraggablePositions();

                // Aggiorna le lancette nella preview
                updateClockHands();

                showMessage('Configurazione caricata!', 'success');
            } catch (error) {
                showMessage('Errore nel caricamento: ' + error, 'error');
            }
        }

        // Salva configurazione sul server
        async function saveConfig() {
            const config = {
                hourColor: document.getElementById('hourColor').value,
                minuteColor: document.getElementById('minuteColor').value,
                secondColor: document.getElementById('secondColor').value,
                showDate: document.getElementById('showDate').checked,
                hourLength: parseInt(document.getElementById('hourLength').value),
                minuteLength: parseInt(document.getElementById('minuteLength').value),
                secondLength: parseInt(document.getElementById('secondLength').value),
                weekday: {
                    enabled: document.getElementById('weekdayEnabled').checked,
                    x: parseInt(document.getElementById('weekdayX').value),
                    y: parseInt(document.getElementById('weekdayY').value),
                    color: document.getElementById('weekdayColor').value,
                    fontSize: parseInt(document.getElementById('weekdayFontSize').value)
                },
                day: {
                    enabled: document.getElementById('dayEnabled').checked,
                    x: parseInt(document.getElementById('dayX').value),
                    y: parseInt(document.getElementById('dayY').value),
                    color: document.getElementById('dayColor').value,
                    fontSize: parseInt(document.getElementById('dayFontSize').value)
                },
                month: {
                    enabled: document.getElementById('monthEnabled').checked,
                    x: parseInt(document.getElementById('monthX').value),
                    y: parseInt(document.getElementById('monthY').value),
                    color: document.getElementById('monthColor').value,
                    fontSize: parseInt(document.getElementById('monthFontSize').value)
                },
                year: {
                    enabled: document.getElementById('yearEnabled').checked,
                    x: parseInt(document.getElementById('yearX').value),
                    y: parseInt(document.getElementById('yearY').value),
                    color: document.getElementById('yearColor').value,
                    fontSize: parseInt(document.getElementById('yearFontSize').value)
                }
            };

            console.log('[DEBUG] Invio configurazione:', config);

            // Mostra indicatore di salvataggio
            const indicator = document.getElementById('autoSaveIndicator');
            indicator.textContent = 'Salvataggio...';
            indicator.className = 'auto-save-indicator saving show';

            try {
                const response = await fetch('/clock/config', {
                    method: 'POST',
                    headers: {
                        'Content-Type': 'application/json',
                    },
                    body: JSON.stringify(config)
                });

                if (response.ok) {
                    // Mostra conferma salvataggio
                    indicator.textContent = 'Salvato!';
                    indicator.className = 'auto-save-indicator show';
                    setTimeout(() => {
                        indicator.classList.remove('show');
                    }, 2000);
                } else {
                    indicator.classList.remove('show');
                    showMessage('Errore nel salvataggio', 'error');
                }
            } catch (error) {
                indicator.classList.remove('show');
                showMessage('Errore: ' + error, 'error');
            }
        }

        // Carica galleria skin dalla SD card
        async function loadSkinGallery() {
            try {
                // Carica lista skin
                const skinsResponse = await fetch('/clock/skins');
                const skins = await skinsResponse.json();

                // Carica skin attiva
                const activeResponse = await fetch('/clock/activeSkin');
                const activeData = await activeResponse.json();
                const activeSkin = activeData.activeSkin;

                console.log('[DEBUG] Skin attiva:', activeSkin);

                const gallery = document.getElementById('skinGallery');

                if (skins.length === 0) {
                    gallery.innerHTML = '<p style="text-align: center; color: #999;">Nessuna skin trovata sulla SD card</p>';
                    return;
                }

                gallery.innerHTML = '';

                skins.forEach(skin => {
                    const card = document.createElement('div');
                    card.className = 'skin-card';

                    // Evidenzia la skin attiva
                    if (skin.name === activeSkin) {
                        card.classList.add('active');
                    }

                    const sizeKB = (skin.size / 1024).toFixed(1);

                    card.innerHTML = `
                        <img src="/clock/preview?file=${encodeURIComponent(skin.name)}"
                             alt="${skin.name}"
                             onerror="this.src='data:image/svg+xml,%3Csvg xmlns=%22http://www.w3.org/2000/svg%22 viewBox=%220 0 100 100%22%3E%3Crect fill=%22%23ddd%22 width=%22100%22 height=%22100%22/%3E%3Ctext x=%2250%22 y=%2250%22 text-anchor=%22middle%22 fill=%22%23999%22%3ENo Preview%3C/text%3E%3C/svg%3E'">
                        <div class="skin-info">
                            <div class="skin-name">${skin.name}</div>
                            <div class="skin-size">${sizeKB} KB</div>
                        </div>
                        <button class="btn-select" onclick="selectSkin('${skin.name}')">
                            ${skin.name === activeSkin ? '✓ Attiva' : '▶ Usa Questa'}
                        </button>
                        <button class="btn-delete" onclick="deleteSkin('${skin.name}', ${skin.name === activeSkin})" style="margin-top: 5px; background: #dc3545; color: white; border: none; padding: 8px 12px; border-radius: 5px; cursor: pointer; font-size: 0.85rem; width: 100%;">
                            🗑️ Elimina
                        </button>
                    `;

                    gallery.appendChild(card);
                });

            } catch (error) {
                console.error('Errore caricamento galleria:', error);
                document.getElementById('skinGallery').innerHTML =
                    '<p style="text-align: center; color: red;">Errore caricamento galleria</p>';
            }
        }

        // Seleziona una skin dalla galleria
        async function selectSkin(filename) {
            try {
                showMessage('🔄 Cambio skin in corso...', 'success');

                const response = await fetch('/clock/selectSkin', {
                    method: 'POST',
                    headers: {
                        'Content-Type': 'application/json',
                    },
                    body: JSON.stringify({ filename: filename })
                });

                if (response.ok) {
                    showMessage('Skin cambiata! Aggiornamento display...', 'success');
                    // Ricarica la configurazione e preview per mostrare i colori e posizioni di questa skin
                    setTimeout(() => {
                        loadConfig();
                        loadSkinGallery();
                        loadPreviewImage();
                    }, 1000);
                } else {
                    const error = await response.json();
                    showMessage('Errore: ' + (error.message || 'Cambio skin fallito'), 'error');
                }
            } catch (error) {
                showMessage('Errore: ' + error, 'error');
            }
        }

        // Elimina una skin dalla SD card
        async function deleteSkin(filename, isActive) {
            // Conferma eliminazione
            const confirmMessage = isActive
                ? `⚠️ ATTENZIONE: Stai per eliminare la skin ATTUALMENTE IN USO!\n\n"${filename}"\n\nVerranno eliminati:\n- File immagine (.jpg)\n- File configurazione (.cfg)\n\nSei sicuro di voler continuare?`
                : `Sei sicuro di voler eliminare la skin?\n\n"${filename}"\n\nVerranno eliminati:\n- File immagine (.jpg)\n- File configurazione (.cfg)\n\nQuesta operazione non può essere annullata.`;

            if (!confirm(confirmMessage)) {
                return;
            }

            try {
                showMessage('🗑️ Eliminazione in corso...', 'success');

                const response = await fetch('/clock/deleteSkin', {
                    method: 'POST',
                    headers: {
                        'Content-Type': 'application/json',
                    },
                    body: JSON.stringify({ filename: filename })
                });

                if (response.ok) {
                    showMessage('✅ Skin e configurazione eliminate: ' + filename, 'success');

                    // Se era la skin attiva, ricarica anche la preview
                    if (isActive) {
                        setTimeout(() => {
                            loadConfig();
                            loadSkinGallery();
                            loadPreviewImage();
                        }, 1000);
                    } else {
                        // Altrimenti ricarica solo la galleria
                        setTimeout(() => {
                            loadSkinGallery();
                        }, 1000);
                    }
                } else {
                    const error = await response.json();
                    showMessage('❌ Errore eliminazione: ' + (error.message || 'Eliminazione fallita'), 'error');
                }
            } catch (error) {
                showMessage('❌ Errore: ' + error.message, 'error');
                console.error('[DELETE ERROR]', error);
            }
        }

        // Upload skin
        async function uploadSkin() {
            const fileInput = document.getElementById('skinFile');
            if (fileInput.files.length === 0) {
                showMessage('⚠️ Seleziona prima un file!', 'error');
                return;
            }

            const file = fileInput.files[0];

            // Verifica che sia un JPG
            if (!file.name.toLowerCase().endsWith('.jpg') && !file.name.toLowerCase().endsWith('.jpeg')) {
                showMessage('⚠️ Solo file JPG/JPEG sono accettati!', 'error');
                return;
            }

            // Verifica dimensione file (max 5MB)
            if (file.size > 5 * 1024 * 1024) {
                showMessage('⚠️ Il file è troppo grande (max 5MB)', 'error');
                return;
            }

            const formData = new FormData();
            formData.append('file', file);

            try {
                showMessage('📤 Upload in corso... (' + Math.round(file.size / 1024) + ' KB)', 'success');

                const response = await fetch('/clock/upload', {
                    method: 'POST',
                    body: formData
                });

                if (response.ok) {
                    const result = await response.json();
                    console.log('[DEBUG] Upload completato:', result);

                    showMessage('✅ Skin caricata: ' + result.filename, 'success');
                    fileInput.value = '';
                    document.getElementById('fileName').textContent = '';

                    // Ricarica la galleria dopo un delay per dare tempo alla SD
                    setTimeout(() => {
                        console.log('[DEBUG] Ricaricando galleria dopo upload...');
                        loadSkinGallery();

                        // Opzionale: seleziona automaticamente la skin appena caricata
                        // Decommentare la riga seguente per attivare la selezione automatica
                        // selectSkin(result.filename);
                    }, 2000);
                } else {
                    const errorText = await response.text();
                    showMessage('❌ Errore upload: ' + errorText, 'error');
                }
            } catch (error) {
                showMessage('❌ Errore connessione: ' + error.message, 'error');
                console.error('[UPLOAD ERROR]', error);
            }
        }

        // Mostra messaggio di stato
        function showMessage(message, type) {
            const msgEl = document.getElementById('statusMessage');
            msgEl.textContent = message;
            msgEl.className = 'status-message ' + type;
            msgEl.style.display = 'block';

            setTimeout(() => {
                msgEl.style.display = 'none';
            }, 5000);
        }

        // ==================== DRAG AND DROP FUNCTIONALITY ====================

        // Inizializza drag and drop per tutti i campi
        function initDragAndDrop() {
            const draggableElements = document.querySelectorAll('.draggable-field');
            const container = document.getElementById('clockPreview');

            draggableElements.forEach(element => {
                element.addEventListener('mousedown', startDrag);
                element.addEventListener('touchstart', startDrag);
            });

            document.addEventListener('mousemove', drag);
            document.addEventListener('touchmove', drag);
            document.addEventListener('mouseup', stopDrag);
            document.addEventListener('touchend', stopDrag);

            // Posiziona inizialmente gli elementi in base ai valori degli slider
            updateDraggablePositions();
        }

        // Inizia il trascinamento
        function startDrag(e) {
            e.preventDefault();
            currentDragElement = e.target.closest('.draggable-field');
            if (!currentDragElement) return;

            isDragging = true;
            currentDragElement.classList.add('dragging');

            const container = document.getElementById('clockPreview');
            const rect = container.getBoundingClientRect();
            const elementRect = currentDragElement.getBoundingClientRect();

            // Calcola l'offset del mouse rispetto all'elemento
            if (e.type === 'touchstart') {
                offsetX = e.touches[0].clientX - elementRect.left;
                offsetY = e.touches[0].clientY - elementRect.top;
            } else {
                offsetX = e.clientX - elementRect.left;
                offsetY = e.clientY - elementRect.top;
            }
        }

        // Trascina l'elemento
        function drag(e) {
            if (!isDragging || !currentDragElement) return;
            e.preventDefault();

            const container = document.getElementById('clockPreview');
            const rect = container.getBoundingClientRect();

            let clientX, clientY;
            if (e.type === 'touchmove') {
                clientX = e.touches[0].clientX;
                clientY = e.touches[0].clientY;
            } else {
                clientX = e.clientX;
                clientY = e.clientY;
            }

            // Calcola la nuova posizione relativa al container
            let newX = clientX - rect.left - offsetX;
            let newY = clientY - rect.top - offsetY;

            // Limita i valori entro i confini (0-480px)
            const elementWidth = currentDragElement.offsetWidth;
            const elementHeight = currentDragElement.offsetHeight;

            newX = Math.max(0, Math.min(480 - elementWidth, newX));
            newY = Math.max(0, Math.min(480 - elementHeight, newY));

            // Applica la posizione
            currentDragElement.style.left = newX + 'px';
            currentDragElement.style.top = newY + 'px';

            // Aggiorna gli slider in tempo reale
            const fieldType = currentDragElement.dataset.field;
            updateSlidersFromDrag(fieldType, Math.round(newX), Math.round(newY));

            // Mostra coordinate in tempo reale (per debug)
            showCoordinates(Math.round(newX), Math.round(newY));
        }

        // Ferma il trascinamento
        function stopDrag(e) {
            if (!isDragging) return;

            isDragging = false;
            if (currentDragElement) {
                currentDragElement.classList.remove('dragging');
                currentDragElement = null;
            }

            // Nascondi indicatore coordinate
            hideCoordinates();

            // Salva immediatamente al rilascio del mouse
            saveConfig();
        }

        // Aggiorna la posizione degli elementi draggable in base agli slider
        function updateDraggablePositions() {
            // Offset calibrati dall'utente - PERFETTO!
            const offsetX = 0;
            const offsetY = -15;

            // Weekday
            const weekdayX = parseInt(document.getElementById('weekdayX').value) + offsetX;
            const weekdayY = parseInt(document.getElementById('weekdayY').value) + offsetY;
            const weekdayEl = document.getElementById('dragWeekday');
            weekdayEl.style.left = weekdayX + 'px';
            weekdayEl.style.top = weekdayY + 'px';
            weekdayEl.style.display = document.getElementById('weekdayEnabled').checked ? 'block' : 'none';

            // Day
            const dayX = parseInt(document.getElementById('dayX').value) + offsetX;
            const dayY = parseInt(document.getElementById('dayY').value) + offsetY;
            const dayEl = document.getElementById('dragDay');
            dayEl.style.left = dayX + 'px';
            dayEl.style.top = dayY + 'px';
            dayEl.style.display = document.getElementById('dayEnabled').checked ? 'block' : 'none';

            // Month
            const monthX = parseInt(document.getElementById('monthX').value) + offsetX;
            const monthY = parseInt(document.getElementById('monthY').value) + offsetY;
            const monthEl = document.getElementById('dragMonth');
            monthEl.style.left = monthX + 'px';
            monthEl.style.top = monthY + 'px';
            monthEl.style.display = document.getElementById('monthEnabled').checked ? 'block' : 'none';

            // Year
            const yearX = parseInt(document.getElementById('yearX').value) + offsetX;
            const yearY = parseInt(document.getElementById('yearY').value) + offsetY;
            const yearEl = document.getElementById('dragYear');
            yearEl.style.left = yearX + 'px';
            yearEl.style.top = yearY + 'px';
            yearEl.style.display = document.getElementById('yearEnabled').checked ? 'block' : 'none';
        }

        // Aggiorna i valori X/Y nascosti quando si trascina un elemento
        // IMPORTANTE: Applica offset per compensare differenza web <-> display
        function updateSlidersFromDrag(fieldType, x, y) {
            // Offset calibrati dall'utente - PERFETTO!
            const offsetX = 0;
            const offsetY = -15;

            // Calcola posizione display
            const displayX = Math.max(0, Math.round(x - offsetX));
            const displayY = Math.max(0, Math.round(y - offsetY));

            if (fieldType === 'weekday') {
                document.getElementById('weekdayX').value = displayX;
                document.getElementById('weekdayY').value = displayY;
            } else if (fieldType === 'day') {
                document.getElementById('dayX').value = displayX;
                document.getElementById('dayY').value = displayY;
            } else if (fieldType === 'month') {
                document.getElementById('monthX').value = displayX;
                document.getElementById('monthY').value = displayY;
            } else if (fieldType === 'year') {
                document.getElementById('yearX').value = displayX;
                document.getElementById('yearY').value = displayY;
            }
        }

        // ==================== SINCRONIZZAZIONE TOGGLE -> DRAG ====================

        // Aggiungi listeners ai toggle per mostrare/nascondere campi e salvataggio immediato
        function handleToggleChange() {
            updateDraggablePositions();
            saveConfig();
        }

        document.getElementById('showDate').addEventListener('change', function() {
            saveConfig();
        });

        document.getElementById('weekdayEnabled').addEventListener('change', handleToggleChange);
        document.getElementById('dayEnabled').addEventListener('change', handleToggleChange);
        document.getElementById('monthEnabled').addEventListener('change', handleToggleChange);
        document.getElementById('yearEnabled').addEventListener('change', handleToggleChange);

        // ==================== DEBUG GRID ====================

        // Toggle griglia di debug
        function toggleDebugGrid() {
            const grid = document.getElementById('debugGrid');
            grid.classList.toggle('show');
        }

        // Mostra coordinate in tempo reale durante il drag (per debug)
        let debugCoordDisplay = null;

        // Crea indicatore coordinate se non esiste
        function showCoordinates(x, y) {
            if (!debugCoordDisplay) {
                debugCoordDisplay = document.createElement('div');
                debugCoordDisplay.style.position = 'fixed';
                debugCoordDisplay.style.top = '80px';
                debugCoordDisplay.style.right = '20px';
                debugCoordDisplay.style.background = 'rgba(0,0,0,0.8)';
                debugCoordDisplay.style.color = 'white';
                debugCoordDisplay.style.padding = '10px 15px';
                debugCoordDisplay.style.borderRadius = '8px';
                debugCoordDisplay.style.fontFamily = 'monospace';
                debugCoordDisplay.style.fontSize = '14px';
                debugCoordDisplay.style.zIndex = '10000';
                document.body.appendChild(debugCoordDisplay);
            }
            debugCoordDisplay.textContent = `X: ${x} | Y: ${y}`;
            debugCoordDisplay.style.display = 'block';
        }

        function hideCoordinates() {
            if (debugCoordDisplay) {
                debugCoordDisplay.style.display = 'none';
            }
        }
    </script>
</body>
</html>
)rawliteral";

// Setup del web server per la configurazione dell'orologio
void setup_clock_webserver(AsyncWebServer* server) {
  Serial.println("[WEBSERVER] Registrazione endpoints orologio...");

  // IMPORTANTE: Registra prima gli endpoints più specifici, poi quelli generici

  // API: GET configurazione corrente
  server->on("/clock/config", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("[WEBSERVER] GET /clock/config richiesto");

    // Converti RGB565 in RGB888 per il web - Lancette
    uint8_t hr, hg, hb, mr, mg, mb, sr, sg, sb;
    rgb565to888(clockHourHandColor, hr, hg, hb);
    rgb565to888(clockMinuteHandColor, mr, mg, mb);
    rgb565to888(clockSecondHandColor, sr, sg, sb);

    // Converti RGB565 in RGB888 per campi data
    uint8_t wr, wg, wb, dr, dg, db, mor, mog, mob, yr, yg, yb;
    rgb565to888(clockWeekdayField.color, wr, wg, wb);
    rgb565to888(clockDayField.color, dr, dg, db);
    rgb565to888(clockMonthField.color, mor, mog, mob);
    rgb565to888(clockYearField.color, yr, yg, yb);

    // Crea JSON con la configurazione
    String json = "{";
    json += "\"hourColor\":\"#" + byteToHex(hr) + byteToHex(hg) + byteToHex(hb) + "\",";
    json += "\"minuteColor\":\"#" + byteToHex(mr) + byteToHex(mg) + byteToHex(mb) + "\",";
    json += "\"secondColor\":\"#" + byteToHex(sr) + byteToHex(sg) + byteToHex(sb) + "\",";
    json += "\"showDate\":" + String(showClockDate ? "true" : "false") + ",";
    json += "\"hourLength\":" + String(clockHourHandLength) + ",";
    json += "\"minuteLength\":" + String(clockMinuteHandLength) + ",";
    json += "\"secondLength\":" + String(clockSecondHandLength) + ",";

    // Weekday field
    json += "\"weekday\":{";
    json += "\"enabled\":" + String(clockWeekdayField.enabled ? "true" : "false") + ",";
    json += "\"x\":" + String(clockWeekdayField.x) + ",";
    json += "\"y\":" + String(clockWeekdayField.y) + ",";
    json += "\"color\":\"#" + byteToHex(wr) + byteToHex(wg) + byteToHex(wb) + "\",";
    json += "\"fontSize\":" + String(clockWeekdayField.fontSize);
    json += "},";

    // Day field
    json += "\"day\":{";
    json += "\"enabled\":" + String(clockDayField.enabled ? "true" : "false") + ",";
    json += "\"x\":" + String(clockDayField.x) + ",";
    json += "\"y\":" + String(clockDayField.y) + ",";
    json += "\"color\":\"#" + byteToHex(dr) + byteToHex(dg) + byteToHex(db) + "\",";
    json += "\"fontSize\":" + String(clockDayField.fontSize);
    json += "},";

    // Month field
    json += "\"month\":{";
    json += "\"enabled\":" + String(clockMonthField.enabled ? "true" : "false") + ",";
    json += "\"x\":" + String(clockMonthField.x) + ",";
    json += "\"y\":" + String(clockMonthField.y) + ",";
    json += "\"color\":\"#" + byteToHex(mor) + byteToHex(mog) + byteToHex(mob) + "\",";
    json += "\"fontSize\":" + String(clockMonthField.fontSize);
    json += "},";

    // Year field
    json += "\"year\":{";
    json += "\"enabled\":" + String(clockYearField.enabled ? "true" : "false") + ",";
    json += "\"x\":" + String(clockYearField.x) + ",";
    json += "\"y\":" + String(clockYearField.y) + ",";
    json += "\"color\":\"#" + byteToHex(yr) + byteToHex(yg) + byteToHex(yb) + "\",";
    json += "\"fontSize\":" + String(clockYearField.fontSize);
    json += "}";

    json += "}";

    Serial.printf("[WEBSERVER] Risposta JSON: %s\n", json.c_str());
    request->send(200, "application/json", json);
  });

  // API: POST salva nuova configurazione
  server->on("/clock/config", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
      // Parse JSON body
      String body = "";
      for (size_t i = 0; i < len; i++) {
        body += (char)data[i];
      }

      Serial.println("[WEBSERVER] Ricevuta configurazione: " + body);

      // Funzione helper per estrarre colore
      auto parseColor = [&](const String& fieldName) -> uint16_t {
        // Cerca il pattern: "fieldName":"#
        String searchPattern = "\"" + fieldName + "\":\"#";
        int startIdx = body.indexOf(searchPattern);

        if (startIdx >= 0) {
          // Salta fino al carattere dopo il #
          // "fieldName":"# = lunghezza della pattern
          int colorStartIdx = startIdx + searchPattern.length();
          String colorStr = body.substring(colorStartIdx, colorStartIdx + 6);

          Serial.printf("[DEBUG parseColor] %s = #%s\n", fieldName.c_str(), colorStr.c_str());

          long color = strtol(colorStr.c_str(), NULL, 16);
          uint8_t r = (color >> 16) & 0xFF;
          uint8_t g = (color >> 8) & 0xFF;
          uint8_t b = color & 0xFF;
          uint16_t rgb565 = rgb888to565(r, g, b);

          Serial.printf("[DEBUG parseColor] RGB(%d,%d,%d) -> 0x%04X\n", r, g, b, rgb565);
          return rgb565;
        }

        Serial.printf("[DEBUG parseColor] %s NOT FOUND, using WHITE\n", fieldName.c_str());
        return WHITE;
      };

      // Funzione helper per estrarre intero
      auto parseInt = [&](const String& fieldName) -> int {
        int idx = body.indexOf("\"" + fieldName + "\":") + fieldName.length() + 3;
        if (idx > fieldName.length() + 3) {
          int endIdx = body.indexOf(",", idx);
          if (endIdx == -1) endIdx = body.indexOf("}", idx);
          String valStr = body.substring(idx, endIdx);
          return valStr.toInt();
        }
        return 0;
      };

      // Funzione helper per estrarre booleano
      auto parseBool = [&](const String& fieldName) -> bool {
        int idx = body.indexOf("\"" + fieldName + "\":") + fieldName.length() + 3;
        if (idx > fieldName.length() + 3) {
          return body.substring(idx, idx + 4) == "true";
        }
        return false;
      };

      // Parse colori lancette
      clockHourHandColor = parseColor("hourColor");
      clockMinuteHandColor = parseColor("minuteColor");
      clockSecondHandColor = parseColor("secondColor");

      Serial.printf("[DEBUG] Colori lancette ricevuti:\n");
      Serial.printf("  Hour: 0x%04X\n", clockHourHandColor);
      Serial.printf("  Minute: 0x%04X\n", clockMinuteHandColor);
      Serial.printf("  Second: 0x%04X\n", clockSecondHandColor);

      // Parse visibilità globale data
      showClockDate = parseBool("showDate");

      // Parse lunghezze lancette
      clockHourHandLength = parseInt("hourLength");
      clockMinuteHandLength = parseInt("minuteLength");
      clockSecondHandLength = parseInt("secondLength");

      Serial.printf("[DEBUG] Lunghezze lancette ricevute:\n");
      Serial.printf("  Hour: %dpx\n", clockHourHandLength);
      Serial.printf("  Minute: %dpx\n", clockMinuteHandLength);
      Serial.printf("  Second: %dpx\n", clockSecondHandLength);

      // Parse campo weekday
      int weekdayIdx = body.indexOf("\"weekday\":{");
      if (weekdayIdx >= 0) {
        String weekdaySection = body.substring(weekdayIdx, body.indexOf("}", weekdayIdx) + 1);
        clockWeekdayField.enabled = weekdaySection.indexOf("\"enabled\":true") > 0;
        clockWeekdayField.x = parseInt("x");
        clockWeekdayField.y = parseInt("y");
        clockWeekdayField.color = parseColor("color");
        // Parse fontSize dal weekdaySection
        int fontSizeIdx = weekdaySection.indexOf("\"fontSize\":");
        if (fontSizeIdx > 0) {
          clockWeekdayField.fontSize = weekdaySection.substring(fontSizeIdx + 11, weekdaySection.indexOf(",", fontSizeIdx) > 0 ? weekdaySection.indexOf(",", fontSizeIdx) : weekdaySection.indexOf("}", fontSizeIdx)).toInt();
        } else {
          clockWeekdayField.fontSize = 2; // default
        }
      }

      // Parse campo day
      int dayIdx = body.indexOf("\"day\":{");
      if (dayIdx >= 0) {
        String daySection = body.substring(dayIdx, body.indexOf("}", dayIdx) + 1);
        clockDayField.enabled = daySection.indexOf("\"enabled\":true") > 0;
        // Parsing specifico per il secondo campo (offset dopo weekday)
        int dayXIdx = body.indexOf("\"x\":", weekdayIdx + 50);
        int dayYIdx = body.indexOf("\"y\":", weekdayIdx + 50);
        int dayColorIdx = body.indexOf("\"color\":\"#", weekdayIdx + 50);
        if (dayXIdx > 0) {
          int endIdx = body.indexOf(",", dayXIdx);
          clockDayField.x = body.substring(dayXIdx + 4, endIdx).toInt();
        }
        if (dayYIdx > 0) {
          int endIdx = body.indexOf(",", dayYIdx);
          clockDayField.y = body.substring(dayYIdx + 4, endIdx).toInt();
        }
        if (dayColorIdx > 0) {
          String colorStr = body.substring(dayColorIdx + 10, dayColorIdx + 16);
          long color = strtol(colorStr.c_str(), NULL, 16);
          clockDayField.color = rgb888to565((color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF);
        }
        // Parse fontSize
        int dayFontSizeIdx = daySection.indexOf("\"fontSize\":");
        if (dayFontSizeIdx > 0) {
          clockDayField.fontSize = daySection.substring(dayFontSizeIdx + 11, daySection.indexOf(",", dayFontSizeIdx) > 0 ? daySection.indexOf(",", dayFontSizeIdx) : daySection.indexOf("}", dayFontSizeIdx)).toInt();
        } else {
          clockDayField.fontSize = 3; // default
        }
      }

      // Parse campo month
      int monthIdx = body.indexOf("\"month\":{");
      if (monthIdx >= 0) {
        String monthSection = body.substring(monthIdx, body.indexOf("}", monthIdx) + 1);
        clockMonthField.enabled = monthSection.indexOf("\"enabled\":true") > 0;
        int monthXIdx = body.indexOf("\"x\":", dayIdx + 50);
        int monthYIdx = body.indexOf("\"y\":", dayIdx + 50);
        int monthColorIdx = body.indexOf("\"color\":\"#", dayIdx + 50);
        if (monthXIdx > 0) {
          int endIdx = body.indexOf(",", monthXIdx);
          clockMonthField.x = body.substring(monthXIdx + 4, endIdx).toInt();
        }
        if (monthYIdx > 0) {
          int endIdx = body.indexOf(",", monthYIdx);
          clockMonthField.y = body.substring(monthYIdx + 4, endIdx).toInt();
        }
        if (monthColorIdx > 0) {
          String colorStr = body.substring(monthColorIdx + 10, monthColorIdx + 16);
          long color = strtol(colorStr.c_str(), NULL, 16);
          clockMonthField.color = rgb888to565((color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF);
        }
        // Parse fontSize
        int monthFontSizeIdx = monthSection.indexOf("\"fontSize\":");
        if (monthFontSizeIdx > 0) {
          clockMonthField.fontSize = monthSection.substring(monthFontSizeIdx + 11, monthSection.indexOf(",", monthFontSizeIdx) > 0 ? monthSection.indexOf(",", monthFontSizeIdx) : monthSection.indexOf("}", monthFontSizeIdx)).toInt();
        } else {
          clockMonthField.fontSize = 2; // default
        }
      }

      // Parse campo year
      int yearIdx = body.indexOf("\"year\":{");
      if (yearIdx >= 0) {
        String yearSection = body.substring(yearIdx, body.indexOf("}", yearIdx) + 1);
        clockYearField.enabled = yearSection.indexOf("\"enabled\":true") > 0;
        int yearXIdx = body.indexOf("\"x\":", monthIdx + 50);
        int yearYIdx = body.indexOf("\"y\":", monthIdx + 50);
        int yearColorIdx = body.indexOf("\"color\":\"#", monthIdx + 50);
        if (yearXIdx > 0) {
          int endIdx = body.indexOf(",", yearXIdx);
          clockYearField.x = body.substring(yearXIdx + 4, endIdx).toInt();
        }
        if (yearYIdx > 0) {
          int endIdx = body.indexOf(",", yearYIdx);
          clockYearField.y = body.substring(yearYIdx + 4, endIdx).toInt();
        }
        if (yearColorIdx > 0) {
          String colorStr = body.substring(yearColorIdx + 10, yearColorIdx + 16);
          long color = strtol(colorStr.c_str(), NULL, 16);
          clockYearField.color = rgb888to565((color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF);
        }
        // Parse fontSize
        int yearFontSizeIdx = yearSection.indexOf("\"fontSize\":");
        if (yearFontSizeIdx > 0) {
          clockYearField.fontSize = yearSection.substring(yearFontSizeIdx + 11, yearSection.indexOf(",", yearFontSizeIdx) > 0 ? yearSection.indexOf(",", yearFontSizeIdx) : yearSection.indexOf("}", yearFontSizeIdx)).toInt();
        } else {
          clockYearField.fontSize = 2; // default
        }
      }

      // Salva la configurazione nel file .cfg specifico della skin corrente
      saveClockConfig(clockActiveSkin);

      // Salva la visibilità della data in EEPROM (impostazione globale)
      saveClockDateVisibility();

      // Forza refresh dell'orologio
      analogClockInitNeeded = true;

      request->send(200, "application/json", "{\"status\":\"ok\"}");
    }
  );

  // API: GET lista file sulla SD card
  server->on("/clock/skins", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("[WEBSERVER] GET /clock/skins - Elencando file JPG dalla SD...");
    String json = "[";

    File root = SD.open("/");
    if (root) {
      Serial.println("[WEBSERVER] SD card aperta, lettura file...");
      File file = root.openNextFile();
      bool first = true;
      int fileCount = 0;

      while (file) {
        if (!file.isDirectory()) {
          String filename = String(file.name());

          // Rimuovi il path se presente (alcuni SD returnano "/filename.jpg")
          int lastSlash = filename.lastIndexOf('/');
          if (lastSlash >= 0) {
            filename = filename.substring(lastSlash + 1);
          }

          Serial.printf("[WEBSERVER]   File trovato: '%s' (size: %d bytes)\n", filename.c_str(), file.size());

          // Filtra solo file JPG/JPEG
          if (filename.endsWith(".jpg") || filename.endsWith(".JPG") ||
              filename.endsWith(".jpeg") || filename.endsWith(".JPEG")) {

            if (!first) json += ",";
            first = false;

            json += "{";
            json += "\"name\":\"" + filename + "\",";
            json += "\"size\":" + String(file.size());
            json += "}";

            fileCount++;
            Serial.printf("[WEBSERVER]     -> Aggiunto a lista (totale JPG: %d)\n", fileCount);
          }
        }
        file.close();
        file = root.openNextFile();
      }
      root.close();
      Serial.printf("[WEBSERVER] Lettura completata. Trovati %d file JPG\n", fileCount);
    } else {
      Serial.println("[WEBSERVER] ERRORE: Impossibile aprire root SD card!");
    }

    json += "]";
    Serial.printf("[WEBSERVER] JSON risposta: %s\n", json.c_str());
    request->send(200, "application/json", json);
  });

  // API: GET preview immagine dalla SD
  server->on("/clock/preview", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("file")) {
      String filename = "/" + request->getParam("file")->value();

      Serial.printf("[PREVIEW] Richiesta file: %s\n", filename.c_str());

      if (SD.exists(filename)) {
        File imageFile = SD.open(filename, FILE_READ);
        if (imageFile) {
          size_t fileSize = imageFile.size();
          Serial.printf("[PREVIEW] File aperto: %s, dimensione: %d bytes\n", filename.c_str(), fileSize);

          // Verifica memoria disponibile
          size_t freeHeap = ESP.getFreeHeap();
          size_t freePSRAM = ESP.getFreePsram();
          Serial.printf("[PREVIEW] Memoria libera - Heap: %d bytes, PSRAM: %d bytes\n", freeHeap, freePSRAM);

          // Prova ad allocare in PSRAM se disponibile, altrimenti usa heap
          uint8_t* buffer = nullptr;

          if (freePSRAM > fileSize + 10000) {
            // Usa PSRAM per file grandi
            buffer = (uint8_t*)ps_malloc(fileSize);
            if (buffer) {
              Serial.printf("[PREVIEW] Buffer allocato in PSRAM\n");
            }
          }

          if (!buffer && freeHeap > fileSize + 10000) {
            // Fallback su heap normale
            buffer = (uint8_t*)malloc(fileSize);
            if (buffer) {
              Serial.printf("[PREVIEW] Buffer allocato in Heap\n");
            }
          }

          if (buffer) {
            size_t bytesRead = imageFile.read(buffer, fileSize);
            imageFile.close(); // CHIUDI SUBITO IL FILE!

            Serial.printf("[PREVIEW] Letti %d bytes, file chiuso\n", bytesRead);

            // Crea la risposta con un callback di cleanup che libera il buffer
            AsyncWebServerResponse *response = request->beginResponse(
              "image/jpeg",
              fileSize,
              [buffer, fileSize](uint8_t *outputBuffer, size_t maxLen, size_t index) -> size_t {
                // Calcola quanti byte copiare
                size_t remaining = fileSize - index;
                size_t toCopy = (remaining < maxLen) ? remaining : maxLen;

                if (toCopy > 0) {
                  memcpy(outputBuffer, buffer + index, toCopy);
                }

                // Se abbiamo finito, libera il buffer
                if (index + toCopy >= fileSize) {
                  free((void*)buffer);
                  Serial.printf("[PREVIEW] Buffer liberato dopo invio completo\n");
                }

                return toCopy;
              }
            );

            response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
            response->addHeader("Pragma", "no-cache");
            response->addHeader("Expires", "0");
            response->addHeader("Access-Control-Allow-Origin", "*");
            response->addHeader("Access-Control-Allow-Methods", "GET");
            request->send(response);

            Serial.printf("[PREVIEW] Risposta inviata (buffer verra' liberato al termine)\n");
          } else {
            imageFile.close();
            Serial.printf("[PREVIEW] ERRORE: malloc/ps_malloc fallito per %d bytes\n", fileSize);
            request->send(500, "text/plain", "Memoria insufficiente");
          }
        } else {
          Serial.printf("[PREVIEW] ERRORE: Impossibile aprire file %s\n", filename.c_str());
          request->send(500, "text/plain", "Impossibile aprire file");
        }
      } else {
        Serial.printf("[PREVIEW] ERRORE: File non trovato: %s\n", filename.c_str());
        request->send(404, "text/plain", "File non trovato");
      }
    } else {
      Serial.printf("[PREVIEW] ERRORE: Parametro 'file' mancante\n");
      request->send(400, "text/plain", "Parametro 'file' mancante");
    }
  });

  // API: GET skin attiva
  server->on("/clock/activeSkin", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.printf("[WEBSERVER] GET /clock/activeSkin -> %s\n", clockActiveSkin);

    char json[64];
    snprintf(json, sizeof(json), "{\"activeSkin\":\"%s\"}", clockActiveSkin);

    request->send(200, "application/json", json);
  });

  // API: POST seleziona skin attiva
  server->on("/clock/selectSkin", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
      String body = "";
      for (size_t i = 0; i < len; i++) {
        body += (char)data[i];
      }

      // Estrai nome file dal JSON {"filename":"nome.jpg"}
      int filenameIdx = body.indexOf("\"filename\":\"") + 12;
      int endIdx = body.indexOf("\"", filenameIdx);

      if (filenameIdx > 12 && endIdx > filenameIdx) {
        String newFilename = body.substring(filenameIdx, endIdx);
        String filepath = "/" + newFilename;

        // Verifica che il file esista sulla SD
        if (SD.exists(filepath)) {
          // Salva il nome della skin in EEPROM (max 31 caratteri + null terminator)
          strncpy(clockActiveSkin, newFilename.c_str(), 31);
          clockActiveSkin[31] = 0; // Assicura terminazione

          // Salva in EEPROM
          for (int i = 0; i < 32; i++) {
            EEPROM.write(EEPROM_CLOCK_SKIN_NAME_ADDR + i, clockActiveSkin[i]);
          }
          EEPROM.commit();

          Serial.printf("[WEBSERVER] Skin cambiata: %s\n", clockActiveSkin);

          // Forza ricaricamento dell'immagine
          analogClockInitNeeded = true;
          clockImageLoaded = false;

          request->send(200, "application/json", "{\"status\":\"ok\"}");
        } else {
          request->send(404, "application/json", "{\"status\":\"error\",\"message\":\"File non trovato\"}");
        }
      } else {
        request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Filename mancante\"}");
      }
    }
  );

  // API: POST upload skin
  static String lastUploadedFilename = "";  // Variabile statica per memorizzare il nome del file

  server->on("/clock/upload", HTTP_POST,
    [](AsyncWebServerRequest *request){
      // Risposta inviata solo dopo il completamento dell'upload
      // Restituisce il nome del file caricato per permettere selezione automatica
      String response = "{\"success\":true,\"filename\":\"" + lastUploadedFilename + "\"}";
      request->send(200, "application/json", response);
    },
    [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
      static File uploadFile;
      static String uploadFilename;

      if (index == 0) {
        Serial.printf("[WEBSERVER] Upload Start: %s\n", filename.c_str());

        // Verifica che il file sia un JPG
        if (!filename.endsWith(".jpg") && !filename.endsWith(".jpeg") && !filename.endsWith(".JPG") && !filename.endsWith(".JPEG")) {
          Serial.println("[WEBSERVER] Errore: solo file JPG/JPEG sono accettati!");
          request->send(400, "text/plain", "Solo file JPG/JPEG sono accettati");
          return;
        }

        // Costruisci il percorso del file usando il nome originale
        uploadFilename = "/" + filename;
        Serial.printf("[WEBSERVER] Salvataggio in: %s\n", uploadFilename.c_str());

        // Apri file sulla SD card per la scrittura
        uploadFile = SD.open(uploadFilename.c_str(), FILE_WRITE);
        if (!uploadFile) {
          Serial.println("[WEBSERVER] Errore apertura file per upload!");
          request->send(500, "text/plain", "Errore apertura file sulla SD");
          return;
        }
      }

      // Scrivi chunk di dati
      if (uploadFile && len) {
        size_t written = uploadFile.write(data, len);
        if (written != len) {
          Serial.printf("[WEBSERVER] ATTENZIONE: scritti solo %d bytes su %d\n", written, len);
        }
      }

      // Finalizza upload
      if (final) {
        if (uploadFile) {
          uploadFile.flush();  // Forza scrittura su SD
          Serial.println("[WEBSERVER] File flushed su SD");
          uploadFile.close();
          Serial.println("[WEBSERVER] File chiuso");
        }
        Serial.printf("[WEBSERVER] Upload Complete: %s (%u bytes)\n", uploadFilename.c_str(), index + len);

        // Verifica dimensione file
        if (SD.exists(uploadFilename.c_str())) {
          File checkFile = SD.open(uploadFilename.c_str(), FILE_READ);
          if (checkFile) {
            size_t fileSize = checkFile.size();
            checkFile.close();
            Serial.printf("[WEBSERVER] File salvato, dimensione: %u bytes\n", fileSize);

            if (fileSize == 0) {
              Serial.println("[WEBSERVER] ERRORE: File vuoto!");
              SD.remove(uploadFilename.c_str());
              lastUploadedFilename = "";  // Reset su errore
            } else {
              // Salva il nome del file (senza /) per la risposta
              lastUploadedFilename = uploadFilename.substring(1);  // Rimuovi lo slash iniziale
              Serial.printf("[WEBSERVER] File caricato con successo: %s\n", lastUploadedFilename.c_str());
            }
          }
        }

        // Forza ricaricamento immagine solo se era la skin attiva
        analogClockInitNeeded = true;
        clockImageLoaded = false;
      }
    }
  );

  // API: POST elimina skin dalla SD card
  server->on("/clock/deleteSkin", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
      String body = "";
      for (size_t i = 0; i < len; i++) {
        body += (char)data[i];
      }

      Serial.printf("[WEBSERVER] DELETE skin request: %s\n", body.c_str());

      // Estrai il filename dal JSON
      int filenameIdx = body.indexOf("\"filename\":\"") + 12;
      if (filenameIdx > 12) {
        int endIdx = body.indexOf("\"", filenameIdx);
        String filename = body.substring(filenameIdx, endIdx);
        String fullPath = "/" + filename;

        Serial.printf("[WEBSERVER] Tentativo eliminazione: %s\n", fullPath.c_str());

        // Verifica che il file esista
        if (SD.exists(fullPath.c_str())) {
          // Costruisci il nome del file .cfg associato (es: "orologio.jpg" -> "/orologio.cfg")
          String cfgFilename = filename;
          int dotIndex = cfgFilename.lastIndexOf('.');
          if (dotIndex > 0) {
            cfgFilename = cfgFilename.substring(0, dotIndex);
          }
          cfgFilename += ".cfg";
          String cfgPath = "/" + cfgFilename;

          Serial.printf("[WEBSERVER] File .cfg associato: %s\n", cfgPath.c_str());

          // Elimina il file JPG
          if (SD.remove(fullPath.c_str())) {
            Serial.printf("[WEBSERVER] File JPG eliminato con successo: %s\n", fullPath.c_str());

            // Elimina anche il file .cfg se esiste
            if (SD.exists(cfgPath.c_str())) {
              if (SD.remove(cfgPath.c_str())) {
                Serial.printf("[WEBSERVER] File CFG eliminato con successo: %s\n", cfgPath.c_str());
              } else {
                Serial.printf("[WEBSERVER] ATTENZIONE: Impossibile eliminare file CFG: %s\n", cfgPath.c_str());
              }
            } else {
              Serial.printf("[WEBSERVER] Nessun file CFG trovato per questa skin\n");
            }

            // Se era la skin attiva, resetta a default (se esiste)
            if (strcmp(clockActiveSkin, filename.c_str()) == 0) {
              Serial.println("[WEBSERVER] Skin attiva eliminata, cercando default...");

              // Cerca un'altra skin da impostare come default
              File root = SD.open("/");
              if (root) {
                File file = root.openNextFile();
                bool foundAlternative = false;

                while (file && !foundAlternative) {
                  if (!file.isDirectory()) {
                    String altFilename = String(file.name());
                    int lastSlash = altFilename.lastIndexOf('/');
                    if (lastSlash >= 0) {
                      altFilename = altFilename.substring(lastSlash + 1);
                    }

                    if (altFilename.endsWith(".jpg") || altFilename.endsWith(".JPG") ||
                        altFilename.endsWith(".jpeg") || altFilename.endsWith(".JPEG")) {
                      strncpy(clockActiveSkin, altFilename.c_str(), sizeof(clockActiveSkin) - 1);
                      clockActiveSkin[sizeof(clockActiveSkin) - 1] = '\0';
                      Serial.printf("[WEBSERVER] Nuova skin attiva: %s\n", clockActiveSkin);
                      foundAlternative = true;

                      // Forza ricaricamento
                      analogClockInitNeeded = true;
                      clockImageLoaded = false;
                    }
                  }
                  file.close();
                  file = root.openNextFile();
                }
                root.close();

                if (!foundAlternative) {
                  Serial.println("[WEBSERVER] Nessuna skin alternativa trovata");
                  strcpy(clockActiveSkin, "");
                }
              }
            }

            request->send(200, "application/json", "{\"success\":true,\"message\":\"Skin e configurazione eliminate\"}");
          } else {
            Serial.printf("[WEBSERVER] ERRORE: Impossibile eliminare il file %s\n", fullPath.c_str());
            request->send(500, "application/json", "{\"success\":false,\"message\":\"Impossibile eliminare il file dalla SD\"}");
          }
        } else {
          Serial.printf("[WEBSERVER] ERRORE: File non trovato: %s\n", fullPath.c_str());
          request->send(404, "application/json", "{\"success\":false,\"message\":\"File non trovato\"}");
        }
      } else {
        Serial.println("[WEBSERVER] ERRORE: Filename mancante nella richiesta");
        request->send(400, "application/json", "{\"success\":false,\"message\":\"Parametro filename mancante\"}");
      }
    }
  );

  // Endpoint per la pagina HTML di configurazione (DOPO tutti gli endpoint API)
  server->on("/clock", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("[WEBSERVER] GET /clock (pagina HTML) richiesto");
    // Concatena le due parti dell'HTML
    String fullHTML = String(FPSTR(CLOCK_CONFIG_HTML_PART1)) + String(FPSTR(CLOCK_CONFIG_HTML_PART2));
    request->send(200, "text/html", fullHTML);
  });

  // Handler per 404
  server->onNotFound([](AsyncWebServerRequest *request){
    Serial.printf("[WEBSERVER] 404 - %s non trovato\n", request->url().c_str());
    request->send(404, "text/plain", "Endpoint non trovato");
  });

  Serial.println("[WEBSERVER] Endpoints registrati:");
  Serial.println("  GET  /clock             -> Pagina HTML configurazione");
  Serial.println("  GET  /clock/config      -> JSON configurazione corrente");
  Serial.println("  POST /clock/config      -> Salva configurazione");
  Serial.println("  GET  /clock/skins       -> Lista skin sulla SD");
  Serial.println("  GET  /clock/activeSkin  -> Nome skin attualmente attiva");
  Serial.println("  GET  /clock/preview     -> Preview immagine");
  Serial.println("  POST /clock/selectSkin  -> Seleziona skin attiva");
  Serial.println("  POST /clock/upload      -> Upload nuova skin");
  Serial.println("  POST /clock/deleteSkin  -> Elimina skin dalla SD");
}

#endif // EFFECT_ANALOG_CLOCK
