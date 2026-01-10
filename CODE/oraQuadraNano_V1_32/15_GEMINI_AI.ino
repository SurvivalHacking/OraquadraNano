// ================== MODALITÀ GEMINI AI ==================
// By Paolo Sambinello (www.survivalhacking.it)
// Integrazione Google Gemini AI nel clock OraQuadraNano
// Basato sul progetto ESP32-Gemini-AI-Projects di Ingeimaks
//
// Questa modalità permette di:
// - Inviare domande a Google Gemini AI
// - Visualizzare le risposte sul display 480x480
// - Controllare i parametri via web server
// =========================================================

#ifdef EFFECT_GEMINI_AI

#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

// ====== CONFIGURAZIONE GEMINI API ======
const char* GEMINI_API_KEY  = "AIzaSyCNgdSPlSdzlzuNm2h2wfk7A9oH-4aF5vs";  // API Key Google Gemini
const char* GEMINI_MODEL    = "gemini-2.5-flash";  // Modello Gemini da utilizzare

// Parametri di generazione
uint32_t gemini_maxOutputTokens = 1024;  // Numero massimo di token nella risposta (ridotto per display)
float    gemini_temperature     = 0.7;   // Creatività della risposta (0.0-2.0)
float    gemini_topP            = 0.9;   // Probabilità cumulativa (0.0-1.0)

// Timeout e retry
const uint32_t GEMINI_HTTP_TIMEOUT_MS = 25000;  // 25 secondi
const uint8_t  GEMINI_MAX_RETRIES     = 2;      // Numero massimo di tentativi

// ====== VARIABILI GLOBALI GEMINI ======
WiFiClientSecure geminiTLS;
String geminiQuestion = "";           // Domanda da inviare a Gemini
String geminiAnswer = "";             // Risposta ricevuta da Gemini
String geminiError = "";              // Eventuale errore
bool geminiWaitingResponse = false;   // Flag per indicare richiesta in corso
bool geminiHasNewAnswer = false;      // Flag per indicare nuova risposta disponibile
unsigned long geminiLastRequestTime = 0;  // Timestamp ultima richiesta

// Variabili per lo scrolling del testo e rendering
int geminiScrollY = 0;                // Posizione Y di scroll
int geminiMaxScrollY = 0;             // Massima posizione di scroll
unsigned long geminiLastScrollTime = 0; // Timestamp ultimo scroll automatico
const int GEMINI_SCROLL_SPEED = 3000;  // Millisecondi tra ogni scroll automatico (rallentato)
bool geminiNeedsRedraw = true;        // Flag per indicare se serve ridisegnare lo schermo
int geminiPrevScrollY = -1;           // Posizione scroll precedente per rilevare cambiamenti
// geminiInitialized è dichiarata nel file principale oraQuadraNano_V1_3_1.ino

// ====== FUNZIONI UTILITY ======

// Pulisce il testo dalle formattazioni markdown
String cleanGeminiText(const String& in) {
  String out = in;

  // Rimuovi blocchi di codice ```...```
  int start = -1;
  while ((start = out.indexOf("```")) != -1) {
    int end = out.indexOf("```", start + 3);
    if (end == -1) break;
    out.remove(start, (end + 3) - start);
  }

  // Rimuovi backtick singoli
  out.replace("`", "");

  // Rimuovi asterischi (bold/italic markdown)
  while (out.indexOf("**") != -1) out.replace("**", "");
  while (out.indexOf("*") != -1) out.replace("*", "");

  // Rimuovi carriage return
  out.replace("\r", "");

  // Normalizza newline multiple
  while (out.indexOf("\n\n\n") != -1) out.replace("\n\n\n", "\n\n");

  // Trim
  out.trim();

  return out;
}

// Costruisce l'endpoint API Gemini
String buildGeminiEndpoint() {
  String url = "https://generativelanguage.googleapis.com/v1beta/models/";
  url += GEMINI_MODEL;
  url += ":generateContent?key=";
  url += GEMINI_API_KEY;
  return url;
}

// ====== COMUNICAZIONE CON GEMINI API ======

// Invia la domanda a Gemini e riceve la risposta
bool sendQuestionToGemini(const String& question, String& answerOut, String& errorOut) {
  HTTPClient http;
  String endpoint = buildGeminiEndpoint();

  http.begin(geminiTLS, endpoint);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(GEMINI_HTTP_TIMEOUT_MS);

  // Costruisci il payload JSON
  DynamicJsonDocument payload(2048);

  JsonObject gen = payload.createNestedObject("generationConfig");
  gen["maxOutputTokens"] = gemini_maxOutputTokens;
  gen["temperature"]     = gemini_temperature;
  gen["topP"]            = gemini_topP;

  JsonArray contents = payload.createNestedArray("contents");
  JsonObject item    = contents.createNestedObject();
  JsonArray parts    = item.createNestedArray("parts");
  JsonObject p0      = parts.createNestedObject();
  p0["text"]         = question;

  String body;
  serializeJson(payload, body);

  // Invia la richiesta POST
  int httpCode = http.POST(body);

  if (httpCode != 200) {
    errorOut = String("HTTP ") + httpCode + " - " + http.errorToString(httpCode);
    http.end();
    return false;
  }

  // Ricevi la risposta
  String response = http.getString();
  http.end();

  // Parsing della risposta JSON
  DynamicJsonDocument doc(24576);  // Buffer grande per risposte lunghe
  DeserializationError err = deserializeJson(doc, response);

  if (err) {
    errorOut = String("Errore parsing JSON: ") + err.c_str();
    return false;
  }

  // Estrai il testo della risposta
  if (doc.containsKey("candidates")) {
    JsonVariant textVar = doc["candidates"][0]["content"]["parts"][0]["text"];
    if (!textVar.isNull()) {
      answerOut = cleanGeminiText(textVar.as<String>());
      if (answerOut.length() == 0) {
        answerOut = "Risposta vuota ricevuta da Gemini.";
      }
      return true;
    }
  }

  // Gestisci feedback di sicurezza o errori
  if (doc.containsKey("promptFeedback")) {
    String fb = doc["promptFeedback"].as<String>();
    errorOut = "Prompt feedback: " + fb;
    return false;
  }

  errorOut = "Risposta inattesa dal modello Gemini.";
  return false;
}

// Funzione chiamata dal web server o da altre parti del codice per inviare una domanda
void askGemini(const String& question) {
  if (geminiWaitingResponse) {
    Serial.println("Gemini: Richiesta già in corso, attendi...");
    return;
  }

  geminiQuestion = question;
  geminiWaitingResponse = true;
  geminiHasNewAnswer = false;
  geminiAnswer = "";
  geminiError = "";
  geminiLastRequestTime = millis();
  geminiNeedsRedraw = true;  // Forza ridisegno per mostrare "Pensando..."

  Serial.println("Gemini: Invio domanda - " + question);

  // Tentativo di invio con retry
  bool success = false;
  for (uint8_t attempt = 0; attempt <= GEMINI_MAX_RETRIES; ++attempt) {
    String answer, error;
    Serial.printf("Gemini: Tentativo %u/%u\n", attempt + 1, GEMINI_MAX_RETRIES + 1);

    success = sendQuestionToGemini(geminiQuestion, answer, error);

    if (success) {
      geminiAnswer = answer;
      geminiHasNewAnswer = true;
      geminiScrollY = 0;  // Reset scroll quando arriva nuova risposta
      geminiNeedsRedraw = true;  // Forza ridisegno schermo
      geminiPrevScrollY = -1;
      Serial.println("Gemini: Risposta ricevuta!");
      break;
    } else {
      geminiError = error;
      Serial.println("Gemini: Errore - " + error);
      delay(500 + attempt * 1000);  // Backoff incrementale
    }
  }

  if (!success) {
    geminiAnswer = "ERRORE:\n" + geminiError + "\n\nProva di nuovo o controlla la connessione WiFi e l'API key.";
    geminiHasNewAnswer = true;
    geminiNeedsRedraw = true;
  }

  geminiWaitingResponse = false;
}

// ====== INIZIALIZZAZIONE MODALITÀ GEMINI ======

void setupGeminiMode() {
  // Configura il client TLS
  // ATTENZIONE: setInsecure() accetta tutti i certificati SSL
  // Sicuro per test, NON per produzione
  geminiTLS.setInsecure();

  Serial.println("Gemini AI: Modalità inizializzata");

  // Messaggio di benvenuto
  geminiAnswer = "Benvenuto nella modalita' Gemini AI!\n\nPer fare una domanda, apri il browser e vai al link mostrato in alto a destra.\n\nPuoi chiedere qualsiasi cosa: informazioni, consigli, calcoli, traduzioni, spiegazioni e molto altro!\n\nGoogle Gemini e' un'intelligenza artificiale avanzata in grado di rispondere a domande complesse, generare testo creativo, risolvere problemi matematici e aiutarti in molte attivita'.\n\nIn attesa della tua prima domanda...";
  geminiHasNewAnswer = true;
  geminiScrollY = 0;
  geminiNeedsRedraw = true;
  geminiPrevScrollY = -1;
}

// ====== RENDERING MODALITÀ GEMINI ======

void displayGeminiMode() {
  static unsigned long lastAnimationUpdate = 0;

  if (!geminiInitialized) {
    setupGeminiMode();
    geminiInitialized = true;
    geminiNeedsRedraw = true;  // Forza primo disegno
  }

  // Gestione animazione "Pensando..." senza ridisegnare tutto
  if (geminiWaitingResponse && !geminiNeedsRedraw) {
    // Aggiorna solo l'animazione dei puntini ogni 500ms
    if (millis() - lastAnimationUpdate > 500) {
      // Ridisegna solo la zona dell'animazione
      gfx->fillRect(65, 28, 100, 11, BLACK);
      gfx->setFont(u8g2_font_helvB08_tr);
      gfx->setTextColor(YELLOW);
      gfx->setCursor(65, 38);
      int dots = (millis() / 500) % 4;
      for (int i = 0; i < dots; i++) {
        gfx->print(".");
      }
      lastAnimationUpdate = millis();
    }
    return;  // Non ridisegnare altro
  }

  // Ridisegna solo se necessario
  if (!geminiNeedsRedraw) {
    return;
  }

  // FULL REDRAW
  geminiNeedsRedraw = false;

  // Sfondo nero
  gfx->fillScreen(BLACK);

  // Titolo in alto
  gfx->setFont(u8g2_font_helvB14_tr);
  gfx->setTextColor(CYAN);
  gfx->setCursor(10, 18);
  gfx->print("GEMINI AI");

  // Link/IP in alto a destra (più piccolo)
  gfx->setFont(u8g2_font_helvB08_tr);
  gfx->setTextColor(WHITE);
  gfx->setCursor(220, 15);
  gfx->print("http://");
  gfx->print(WiFi.localIP().toString());
  gfx->print(":8080/gemini");

  // Linea separatrice
  gfx->drawLine(0, 25, 480, 25, CYAN);

  // Indica se è in attesa di risposta
  if (geminiWaitingResponse) {
    gfx->setFont(u8g2_font_helvB08_tr);
    gfx->setTextColor(YELLOW);
    gfx->setCursor(10, 38);
    gfx->print("Pensando");

    // Animazione puntini
    int dots = (millis() / 500) % 4;
    for (int i = 0; i < dots; i++) {
      gfx->print(".");
    }
  }

  // Mostra la domanda (se presente)
  if (geminiQuestion.length() > 0) {
    gfx->setFont(u8g2_font_helvB08_tr);
    gfx->setTextColor(WHITE);
    gfx->setCursor(5, 38);
    gfx->print("Q: ");

    // Word wrap per la domanda con font U8g2
    String q = geminiQuestion;
    int xPos = 20;
    int yPos = 38;
    int maxWidth = 455;
    int charWidth = 5;  // Larghezza media carattere helvB08
    int lineHeight = 11; // Altezza font helvB08 + spaziatura
    int charsPerLine = maxWidth / charWidth;  // ~91 caratteri per riga

    for (int i = 0; i < q.length(); i += charsPerLine) {
      String line = q.substring(i, min((int)(i + charsPerLine), (int)q.length()));
      gfx->setCursor(xPos, yPos);
      gfx->print(line);
      yPos += lineHeight;
      if (yPos > 70) break;  // Limite area domanda
    }
  }

  // Linea separatrice risposta
  gfx->drawLine(0, 75, 480, 75, CYAN);

  // Mostra la risposta con scroll
  if (geminiHasNewAnswer && geminiAnswer.length() > 0) {
    gfx->setFont(u8g2_font_helvB08_tr);
    gfx->setTextColor(GREEN);

    // Calcola quante righe occupa la risposta con font U8g2
    String a = geminiAnswer;
    int maxWidth = 470;
    int charWidth = 5;  // Larghezza media carattere helvB08
    int lineHeight = 11; // Altezza font helvB08 + spaziatura
    int charsPerLine = maxWidth / charWidth;  // ~94 caratteri per riga

    // Conta le righe necessarie
    int totalLines = 0;
    int startIdx = 0;
    while (startIdx < a.length()) {
      int newlineIdx = a.indexOf('\n', startIdx);
      if (newlineIdx == -1) newlineIdx = a.length();

      String segment = a.substring(startIdx, newlineIdx);
      int linesInSegment = (segment.length() + charsPerLine - 1) / charsPerLine;
      if (linesInSegment == 0) linesInSegment = 1;
      totalLines += linesInSegment;

      startIdx = newlineIdx + 1;
    }

    int maxVisibleLines = (480 - 80) / lineHeight;  // Righe visibili (~36 righe)
    geminiMaxScrollY = max(0, totalLines - maxVisibleLines);

    // Rendering del testo con scroll
    int yPos = 88 - (geminiScrollY * lineHeight);
    startIdx = 0;

    while (startIdx < a.length() && yPos < 480) {
      int newlineIdx = a.indexOf('\n', startIdx);
      if (newlineIdx == -1) newlineIdx = a.length();

      String segment = a.substring(startIdx, newlineIdx);

      // Word wrap per questo segmento
      for (int i = 0; i < segment.length(); i += charsPerLine) {
        if (yPos >= 78 && yPos < 475) {  // Solo se visibile
          String line = segment.substring(i, min((int)(i + charsPerLine), (int)segment.length()));
          gfx->setCursor(5, yPos);
          gfx->print(line);
        }
        yPos += lineHeight;
        if (yPos > 480) break;
      }

      if (newlineIdx < a.length()) {
        yPos += lineHeight;  // Spazio per newline
      }

      startIdx = newlineIdx + 1;
    }

    // Indicatore di scroll (se necessario)
    if (geminiMaxScrollY > 0) {
      int scrollBarHeight = 200;
      int scrollBarY = 80 + (scrollBarHeight * geminiScrollY) / (geminiMaxScrollY + 1);
      gfx->fillRect(475, scrollBarY, 3, 20, CYAN);
    }

    // Auto-scroll disabilitato per evitare flickering
    // Se vuoi riattivarlo, decommentare e impostare geminiNeedsRedraw = true nel blocco
    /*
    if (geminiMaxScrollY > 0 && millis() - geminiLastScrollTime > GEMINI_SCROLL_SPEED) {
      geminiScrollY++;
      if (geminiScrollY > geminiMaxScrollY) {
        geminiScrollY = 0;  // Ricomincia dall'inizio
      }
      geminiLastScrollTime = millis();
      geminiNeedsRedraw = true;  // Forza ridisegno per aggiornare scroll
    }
    */
  } else if (!geminiWaitingResponse && geminiAnswer.length() == 0) {
    // Nessuna risposta ancora
    gfx->setFont(u8g2_font_helvB08_tr);
    gfx->setTextColor(0x7BEF);  // Grigio chiaro (RGB565)
    gfx->setCursor(5, 88);
    gfx->print("In attesa di domanda...");
  }

  // Istruzioni in basso (solo se non c'è risposta lunga)
  if (geminiAnswer.length() < 200) {
    gfx->setFont(u8g2_font_helvB08_tr);
    gfx->setTextColor(0x2104);  // Grigio scuro (RGB565)
    gfx->setCursor(5, 470);
    gfx->print("Usa il link sopra per inviare domande");
  }
}

#endif // EFFECT_GEMINI_AI
