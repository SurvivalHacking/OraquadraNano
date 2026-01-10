// ================== WEB SERVER PER GEMINI AI ==================
//
// Questo file implementa un web server con interfaccia per interagire con Gemini AI
// Permette di:
// - Inviare domande a Google Gemini
// - Visualizzare le risposte in tempo reale
// - Configurare parametri (temperature, tokens, ecc.)
//
// Accesso: http://<IP_ESP32>:8080/gemini
//
// By Paolo Sambinello (www.survivalhacking.it)
// ==========================================================

#ifdef EFFECT_GEMINI_AI

#include <mbedtls/base64.h>

// Pagina HTML per l'interfaccia Gemini AI
const char GEMINI_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="it">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Gemini AI - OraQuadraNano</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }

        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #1e3c72 0%, #2a5298 100%);
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
            background: linear-gradient(135deg, #00d2ff 0%, #3a47d5 100%);
            color: white;
            padding: 30px;
            text-align: center;
        }

        .header h1 {
            font-size: 2.5rem;
            margin-bottom: 10px;
        }

        .header p {
            opacity: 0.9;
            font-size: 1.1rem;
        }

        .content {
            padding: 30px;
        }

        .input-section {
            margin-bottom: 25px;
        }

        .input-section label {
            display: block;
            margin-bottom: 8px;
            font-weight: 600;
            color: #2a5298;
        }

        .input-section textarea {
            width: 100%;
            padding: 12px;
            border: 2px solid #ddd;
            border-radius: 10px;
            font-size: 1rem;
            font-family: inherit;
            resize: vertical;
            min-height: 100px;
            transition: border-color 0.3s;
        }

        .input-section textarea:focus {
            outline: none;
            border-color: #3a47d5;
        }

        .btn {
            background: linear-gradient(135deg, #00d2ff 0%, #3a47d5 100%);
            color: white;
            border: none;
            padding: 15px 40px;
            font-size: 1.1rem;
            font-weight: 600;
            border-radius: 25px;
            cursor: pointer;
            transition: transform 0.2s, box-shadow 0.2s;
            box-shadow: 0 4px 15px rgba(58, 71, 213, 0.4);
        }

        .btn:hover {
            transform: translateY(-2px);
            box-shadow: 0 6px 20px rgba(58, 71, 213, 0.6);
        }

        .btn:active {
            transform: translateY(0);
        }

        .btn:disabled {
            background: #ccc;
            cursor: not-allowed;
            box-shadow: none;
        }

        .answer-section {
            margin-top: 30px;
            padding: 20px;
            background: #f8f9fa;
            border-radius: 15px;
            border-left: 4px solid #3a47d5;
            display: none;
        }

        .answer-section.visible {
            display: block;
        }

        .answer-section h3 {
            color: #2a5298;
            margin-bottom: 15px;
            font-size: 1.3rem;
        }

        .answer-text {
            white-space: pre-wrap;
            word-wrap: break-word;
            line-height: 1.6;
            font-size: 1rem;
            color: #333;
        }

        .loading {
            display: none;
            text-align: center;
            padding: 20px;
        }

        .loading.visible {
            display: block;
        }

        .spinner {
            border: 4px solid #f3f3f3;
            border-top: 4px solid #3a47d5;
            border-radius: 50%;
            width: 50px;
            height: 50px;
            animation: spin 1s linear infinite;
            margin: 0 auto 15px;
        }

        @keyframes spin {
            0% { transform: rotate(0deg); }
            100% { transform: rotate(360deg); }
        }

        .status-text {
            color: #666;
            font-style: italic;
        }

        .config-section {
            margin-top: 30px;
            padding: 20px;
            background: #fff8e1;
            border-radius: 15px;
            border-left: 4px solid #ffc107;
        }

        .config-section h3 {
            color: #f57c00;
            margin-bottom: 15px;
        }

        .config-row {
            display: grid;
            grid-template-columns: 1fr 1fr 1fr;
            gap: 15px;
            margin-top: 15px;
        }

        .config-item {
            display: flex;
            flex-direction: column;
        }

        .config-item label {
            font-size: 0.85rem;
            color: #666;
            margin-bottom: 5px;
        }

        .config-item input {
            padding: 8px;
            border: 2px solid #ddd;
            border-radius: 8px;
            font-size: 0.95rem;
        }

        .info-box {
            margin-top: 20px;
            padding: 15px;
            background: #e3f2fd;
            border-radius: 10px;
            border-left: 4px solid #2196f3;
        }

        .info-box p {
            margin: 5px 0;
            font-size: 0.9rem;
            color: #1565c0;
        }

        @media (max-width: 768px) {
            .config-row {
                grid-template-columns: 1fr;
            }

            .header h1 {
                font-size: 2rem;
            }
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🤖 Gemini AI</h1>
            <p>Fai domande all'intelligenza artificiale di Google</p>
        </div>

        <div class="content">
            <div class="input-section">
                <label for="question">La tua domanda:</label>
                <textarea id="question" placeholder="Scrivi qui la tua domanda... (es: Cos'è l'intelligenza artificiale? Come funziona un computer? Raccontami una barzelletta...)"></textarea>
            </div>

            <button class="btn" onclick="sendQuestion()">Invia Domanda</button>

            <div style="margin-top: 20px; text-align: center;">
                <button class="btn" style="background: linear-gradient(135deg, #ff6b6b 0%, #ee5a6f 100%);" onclick="startVoiceRecording()" id="voiceBtn">
                    🎤 Inizia Registrazione Vocale
                </button>
                <p style="font-size: 0.9rem; color: #666; margin-top: 10px;" id="voiceStatus"></p>
            </div>

            <div class="loading" id="loading">
                <div class="spinner"></div>
                <p class="status-text">Gemini sta pensando...</p>
            </div>

            <div class="answer-section" id="answerSection">
                <h3>💡 Risposta di Gemini:</h3>
                <div class="answer-text" id="answerText"></div>
            </div>

            <div class="config-section">
                <h3>⚙️ Configurazione Avanzata</h3>
                <div class="config-row">
                    <div class="config-item">
                        <label>Max Tokens:</label>
                        <input type="number" id="maxTokens" value="1024" min="128" max="4096" step="128">
                    </div>
                    <div class="config-item">
                        <label>Temperature (0.0-2.0):</label>
                        <input type="number" id="temperature" value="0.7" min="0.0" max="2.0" step="0.1">
                    </div>
                    <div class="config-item">
                        <label>Top P (0.0-1.0):</label>
                        <input type="number" id="topP" value="0.9" min="0.0" max="1.0" step="0.05">
                    </div>
                </div>
                <div class="info-box">
                    <p><strong>Max Tokens:</strong> Lunghezza massima della risposta (più alto = risposte più lunghe)</p>
                    <p><strong>Temperature:</strong> Creatività della risposta (0 = precisa, 2 = molto creativa)</p>
                    <p><strong>Top P:</strong> Diversità delle parole usate (0.9 = bilanciato)</p>
                </div>
            </div>
        </div>
    </div>

    <script>
        async function sendQuestion() {
            const questionEl = document.getElementById('question');
            const question = questionEl.value.trim();

            if (!question) {
                alert('Per favore, scrivi una domanda!');
                return;
            }

            // Mostra loading, nascondi risposta precedente
            document.getElementById('loading').classList.add('visible');
            document.getElementById('answerSection').classList.remove('visible');
            document.querySelector('.btn').disabled = true;

            // Leggi parametri di configurazione
            const maxTokens = document.getElementById('maxTokens').value;
            const temperature = document.getElementById('temperature').value;
            const topP = document.getElementById('topP').value;

            try {
                // Prima invia i parametri di configurazione
                await fetch(`/gemini/config?tokens=${maxTokens}&temp=${temperature}&topP=${topP}`);

                // Poi invia la domanda
                const response = await fetch('/gemini/ask', {
                    method: 'POST',
                    headers: {
                        'Content-Type': 'application/x-www-form-urlencoded',
                    },
                    body: 'question=' + encodeURIComponent(question)
                });

                const data = await response.json();

                // Nascondi loading
                document.getElementById('loading').classList.remove('visible');
                document.querySelector('.btn').disabled = false;

                if (data.success) {
                    document.getElementById('answerText').textContent = data.answer;
                    document.getElementById('answerSection').classList.add('visible');
                } else {
                    alert('Errore: ' + data.error);
                }
            } catch (error) {
                document.getElementById('loading').classList.remove('visible');
                document.querySelector('.btn').disabled = false;
                alert('Errore di connessione: ' + error.message);
            }
        }

        // Permetti invio con Ctrl+Enter
        document.getElementById('question').addEventListener('keydown', function(e) {
            if (e.ctrlKey && e.key === 'Enter') {
                sendQuestion();
            }
        });

        // === VOICE RECORDING ===
        let isRecording = false;

        async function startVoiceRecording() {
            const btn = document.getElementById('voiceBtn');
            const status = document.getElementById('voiceStatus');

            if (!isRecording) {
                // Start recording
                try {
                    const response = await fetch('/gemini/voice/start', { method: 'POST' });
                    const data = await response.json();

                    if (data.success) {
                        isRecording = true;
                        btn.textContent = '⏹️ Stop e Invia';
                        btn.style.background = 'linear-gradient(135deg, #4CAF50 0%, #45a049 100%)';
                        status.textContent = '🔴 Registrando... (max 5 secondi)';
                        status.style.color = '#f44336';
                    } else {
                        alert('Errore: ' + data.message);
                    }
                } catch (error) {
                    alert('Errore di connessione: ' + error.message);
                }
            } else {
                // Stop recording and process
                btn.disabled = true;
                status.textContent = '🔄 Processing audio...';

                try {
                    const response = await fetch('/gemini/voice/stop', { method: 'POST' });
                    const data = await response.json();

                    if (data.success) {
                        status.textContent = '✅ Audio in elaborazione! Attendi la risposta...';
                        status.style.color = '#4CAF50';

                        // Reset dopo 3 secondi
                        setTimeout(() => {
                            isRecording = false;
                            btn.disabled = false;
                            btn.textContent = '🎤 Inizia Registrazione Vocale';
                            btn.style.background = 'linear-gradient(135deg, #ff6b6b 0%, #ee5a6f 100%)';
                            status.textContent = '';
                        }, 3000);
                    }
                } catch (error) {
                    alert('Errore: ' + error.message);
                    isRecording = false;
                    btn.disabled = false;
                }
            }
        }
    </script>
</body>
</html>
)rawliteral";

// Handler per la pagina principale Gemini
void handleGeminiPage(AsyncWebServerRequest *request) {
  request->send_P(200, "text/html", GEMINI_HTML);
}

// Handler per configurare i parametri Gemini
void handleGeminiConfig(AsyncWebServerRequest *request) {
  if (request->hasParam("tokens")) {
    int tokens = request->getParam("tokens")->value().toInt();
    if (tokens >= 128 && tokens <= 4096) {
      gemini_maxOutputTokens = tokens;
    }
  }

  if (request->hasParam("temp")) {
    float temp = request->getParam("temp")->value().toFloat();
    if (temp >= 0.0f && temp <= 2.0f) {
      gemini_temperature = temp;
    }
  }

  if (request->hasParam("topP")) {
    float topP = request->getParam("topP")->value().toFloat();
    if (topP >= 0.0f && topP <= 1.0f) {
      gemini_topP = topP;
    }
  }

  request->send(200, "text/plain", "Configurazione aggiornata");
}

// Handler per inviare domanda a Gemini (con callback asincrono)
void handleGeminiAsk(AsyncWebServerRequest *request) {
  if (!request->hasParam("question", true)) {
    request->send(400, "application/json", "{\"success\":false,\"error\":\"Nessuna domanda fornita\"}");
    return;
  }

  String question = request->getParam("question", true)->value();

  if (question.length() == 0) {
    request->send(400, "application/json", "{\"success\":false,\"error\":\"Domanda vuota\"}");
    return;
  }

  // Salva la richiesta per rispondere dopo
  AsyncWebServerRequest* savedRequest = request;

  // Invia la domanda a Gemini in modo sincrono
  // (Il web server deve aspettare la risposta)
  String answer, error;
  bool success = false;

  for (uint8_t attempt = 0; attempt <= GEMINI_MAX_RETRIES; ++attempt) {
    success = sendQuestionToGemini(question, answer, error);
    if (success) break;
    delay(500 + attempt * 1000);
  }

  if (success) {
    // Aggiorna le variabili globali per il display
    geminiQuestion = question;
    geminiAnswer = answer;
    geminiHasNewAnswer = true;
    geminiScrollY = 0;
    geminiNeedsRedraw = true;  // Forza ridisegno display

    // Prepara risposta JSON
    String jsonResponse = "{\"success\":true,\"answer\":\"";

    // Escape delle virgolette e newline nel JSON
    String escapedAnswer = answer;
    escapedAnswer.replace("\\", "\\\\");
    escapedAnswer.replace("\"", "\\\"");
    escapedAnswer.replace("\n", "\\n");
    escapedAnswer.replace("\r", "");

    jsonResponse += escapedAnswer;
    jsonResponse += "\"}";

    savedRequest->send(200, "application/json", jsonResponse);
  } else {
    // Aggiorna anche in caso di errore per mostrarlo sul display
    geminiQuestion = question;
    geminiAnswer = "ERRORE:\n" + error;
    geminiHasNewAnswer = true;
    geminiNeedsRedraw = true;  // Forza ridisegno display

    String jsonResponse = "{\"success\":false,\"error\":\"";
    jsonResponse += error;
    jsonResponse += "\"}";

    savedRequest->send(500, "application/json", jsonResponse);
  }
}

// Handler per avviare registrazione vocale
void handleVoiceStart(AsyncWebServerRequest *request) {
  if (voiceStartRecording()) {
    request->send(200, "application/json", "{\"success\":true,\"message\":\"Registrazione avviata\"}");
  } else {
    request->send(500, "application/json", "{\"success\":false,\"message\":\"Errore avvio registrazione\"}");
  }
}

// Handler per fermare registrazione e processare
void handleVoiceStop(AsyncWebServerRequest *request) {
  // Avvia processing in background (non bloccare la risposta HTTP)
  // La funzione voiceProcessQuestion() gestirà tutto
  request->send(200, "application/json", "{\"success\":true,\"message\":\"Processing audio...\"}");

  // Process in background
  voiceProcessQuestion();
}

// Handler per configurare ESP32-C3 IP
void handleVoiceSetGateway(AsyncWebServerRequest *request) {
  if (!request->hasParam("ip")) {
    request->send(400, "application/json", "{\"success\":false,\"message\":\"Missing ip parameter\"}");
    return;
  }

  String esp32c3IP = request->getParam("ip")->value();
  voiceSetGatewayIP(esp32c3IP);

  request->send(200, "application/json", "{\"success\":true,\"message\":\"ESP32-C3 IP configurato\"}");
}

// Handler per ricevere audio dall'ESP32-C3 esterno
void handleVoiceFromC3(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
  // Questo viene chiamato quando riceviamo il body della richiesta
  static String jsonBuffer = "";
  static unsigned long startTime = 0;

  // Accumula i dati
  if (index == 0) {
    jsonBuffer = "";
    startTime = millis();
    Serial.println("\n╔════════════════════════════════════════════╗");
    Serial.println("║   RICEVUTO AUDIO DA ESP32-C3              ║");
    Serial.println("╚════════════════════════════════════════════╝");
    Serial.printf("📥 Inizio ricezione: %d bytes totali\n", total);
  }

  for (size_t i = 0; i < len; i++) {
    jsonBuffer += (char)data[i];
  }

  // Mostra progresso
  if (total > 10000) {  // Solo per payload grandi
    float progress = ((float)(index + len) / total) * 100;
    Serial.printf("   Ricevuti %d/%d bytes (%.1f%%)\n", index + len, total, progress);
  }

  // Se abbiamo ricevuto tutto il payload
  if (index + len == total) {
    unsigned long elapsedTime = millis() - startTime;
    Serial.printf("✅ Ricezione completata in %lu ms\n", elapsedTime);

    // Parse JSON
    DynamicJsonDocument doc(total + 1024);
    DeserializationError error = deserializeJson(doc, jsonBuffer);

    if (error) {
      Serial.printf("❌ Errore parsing JSON: %s\n", error.c_str());
      request->send(400, "application/json", "{\"success\":false,\"error\":\"Invalid JSON\"}");
      jsonBuffer = "";
      return;
    }

    // Estrai parametri
    String audioBase64 = doc["audioContent"].as<String>();
    int sampleRate = doc["sampleRate"] | 16000;
    String languageCode = doc["languageCode"] | "it-IT";

    if (audioBase64.length() == 0) {
      Serial.println("❌ Nessun audio nel payload");
      request->send(400, "application/json", "{\"success\":false,\"error\":\"No audio content\"}");
      jsonBuffer = "";
      return;
    }

    Serial.printf("🔊 Audio ricevuto: %d bytes (base64)\n", audioBase64.length());

    // Decodifica base64 usando mbedtls (inclusa in ESP32)
    size_t audioSize = 0;

    // Calcola dimensione output (base64 decode riduce di ~25%)
    size_t maxOutputLen = (audioBase64.length() * 3) / 4 + 4;
    uint8_t* audioData = (uint8_t*)malloc(maxOutputLen);

    if (!audioData) {
      Serial.println("❌ Errore allocazione memoria per decode");
      request->send(500, "application/json", "{\"success\":false,\"error\":\"Out of memory\"}");
      jsonBuffer = "";
      return;
    }

    // Decodifica base64
    int ret = mbedtls_base64_decode(audioData, maxOutputLen, &audioSize,
                                      (const unsigned char*)audioBase64.c_str(),
                                      audioBase64.length());

    if (ret != 0) {
      Serial.printf("❌ Errore decodifica base64: %d\n", ret);
      free(audioData);
      request->send(400, "application/json", "{\"success\":false,\"error\":\"Base64 decode failed\"}");
      jsonBuffer = "";
      return;
    }

    Serial.printf("🔊 Audio decodificato: %d bytes\n", audioSize);

    // Rispondi immediatamente al client (processing in background)
    request->send(200, "application/json", "{\"success\":true,\"message\":\"Audio in elaborazione\"}");
    jsonBuffer = "";

    // Processa audio: STT → Gemini → TTS
    // 1. Speech-to-Text
    String transcript = voiceSpeechToText(audioData, audioSize);

    if (transcript.length() == 0) {
      Serial.println("❌ Trascrizione fallita");
      free(audioData);  // Libera memoria
      return;
    }

    // Aggiorna display con la domanda
    geminiQuestion = transcript;
    geminiAnswer = "Pensando...";
    geminiNeedsRedraw = true;

    // 2. Invia a Gemini
    Serial.println("\n╔════════════════════════════════════════════╗");
    Serial.println("║         ELABORAZIONE GEMINI AI            ║");
    Serial.println("╚════════════════════════════════════════════╝");
    Serial.printf("🤖 Domanda: \"%s\"\n", transcript.c_str());
    Serial.println("⏳ Invio a Gemini...");

    String answer, error_msg;
    unsigned long geminiStart = millis();
    bool success = sendQuestionToGemini(transcript, answer, error_msg);
    unsigned long geminiTime = millis() - geminiStart;

    if (!success) {
      Serial.printf("❌ Errore Gemini (%lu ms): %s\n", geminiTime, error_msg.c_str());
      geminiAnswer = "Errore: " + error_msg;
      geminiNeedsRedraw = true;
      voiceSendTTSToGateway("Mi dispiace, si è verificato un errore.");
      free(audioData);  // Libera memoria
      return;
    }

    Serial.printf("✅ Risposta Gemini ricevuta in %lu ms\n", geminiTime);
    Serial.printf("📝 Risposta: \"%s\"\n", answer.substring(0, 100).c_str());
    if (answer.length() > 100) {
      Serial.printf("   ... (totale %d caratteri)\n", answer.length());
    }

    // Aggiorna display con risposta
    geminiAnswer = answer;
    geminiHasNewAnswer = true;
    geminiNeedsRedraw = true;

    // 3. Invia a TTS Gateway
    String ttsText = answer.substring(0, 500);  // Max 500 caratteri per TTS
    Serial.printf("\n🔊 Preparazione TTS (%d caratteri)...\n", ttsText.length());
    voiceSendTTSToGateway(ttsText);

    // Libera memoria audio
    free(audioData);

    Serial.println("\n✅ ═══ PROCESSING COMPLETATO ═══\n");
  }
}

// Registra gli endpoint sul server (chiamato da setup)
void setup_gemini_webserver(AsyncWebServer* server) {
  Serial.println("=====================================");
  Serial.println("[GEMINI WEB] Registrazione endpoints...");

  server->on("/gemini/config", HTTP_GET, handleGeminiConfig);
  Serial.println("[GEMINI WEB] ✓ Registrato: GET /gemini/config");

  server->on("/gemini/ask", HTTP_POST, handleGeminiAsk);
  Serial.println("[GEMINI WEB] ✓ Registrato: POST /gemini/ask");

  // Voice endpoints
  server->on("/gemini/voice/start", HTTP_POST, handleVoiceStart);
  Serial.println("[GEMINI WEB] ✓ Registrato: POST /gemini/voice/start");

  server->on("/gemini/voice/stop", HTTP_POST, handleVoiceStop);
  Serial.println("[GEMINI WEB] ✓ Registrato: POST /gemini/voice/stop");

  server->on("/gemini/voice/gateway", HTTP_GET, handleVoiceSetGateway);
  Serial.println("[GEMINI WEB] ✓ Registrato: GET /gemini/voice/gateway (ESP32-C3 IP)");

  // Endpoint per ricevere audio da ESP32-C3
  server->on("/api/voice", HTTP_POST,
    [](AsyncWebServerRequest *request) {
      // Body handler gestisce la risposta
    },
    NULL,
    handleVoiceFromC3
  );
  Serial.println("[GEMINI WEB] ✓ Registrato: POST /api/voice (ESP32-C3 audio)");

  // /gemini DEVE essere registrato per ULTIMO
  server->on("/gemini", HTTP_GET, handleGeminiPage);
  Serial.println("[GEMINI WEB] ✓ Registrato: GET /gemini (pagina principale)");

  Serial.println("[GEMINI WEB] Tutti gli endpoints registrati con successo!");
  Serial.println("[GEMINI WEB] Accedi a http://<IP_ESP32>:8080/gemini");
  Serial.println("=====================================");
}

#endif // EFFECT_GEMINI_AI
