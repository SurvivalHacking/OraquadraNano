// ================== ASSISTENTE VOCALE - MICROFONO I2S + TTS ==================
//
// Questo file implementa un assistente vocale completo per Gemini AI:
// - Microfono INMP441 I2S per catturare audio
// - Google Cloud Speech-to-Text per trascrizione
// - Integrazione con Gemini AI
// - Google Cloud TTS per sintesi vocale
// - Invio audio al gateway per riproduzione
//
// Hardware richiesto:
// - INMP441 I2S Microphone
//   SCK  -> GPIO 26
//   WS   -> GPIO 25
//   SD   -> GPIO 33
//   L/R  -> GND (mono, canale sinistro)
//   VDD  -> 3.3V
//   GND  -> GND
//
// By Paolo Sambinello (www.survivalhacking.it)
// ==========================================================

#ifdef EFFECT_GEMINI_AI

#include <driver/i2s.h>
#include <HTTPClient.h>
#include <ESPmDNS.h>
#include <base64.h>

// ====== CONFIGURAZIONE MICROFONO I2S ======
#define I2S_MIC_SCK_PIN       26  // Clock
#define I2S_MIC_WS_PIN        25  // Word Select
#define I2S_MIC_SD_PIN        33  // Data
#define I2S_MIC_PORT          I2S_NUM_1
#define I2S_MIC_SAMPLE_RATE   16000
#define I2S_MIC_BUFFER_SIZE   1024

// ====== CONFIGURAZIONE GOOGLE CLOUD ======
// IMPORTANTE: Inserisci la tua API Key di Google Cloud
const char* GOOGLE_CLOUD_API_KEY = "AIzaSyCNgdSPlSdzlzuNm2h2wfk7A9oH-4aF5vs"; // Stessa key di Gemini

// ====== CONFIGURAZIONE mDNS ======
#define MDNS_HOSTNAME     "oraquadra"          // Questo device: oraquadra.local
#define ESP32C3_MDNS      "voiceassistant"     // ESP32-C3: voiceassistant.local

// ====== VARIABILI GLOBALI VOICE ======
bool voiceMicInitialized = false;
bool voiceListening = false;
uint8_t* voiceAudioBuffer = nullptr;
size_t voiceAudioBufferSize = 0;
unsigned long voiceRecordingStartTime = 0;
const unsigned long VOICE_MAX_RECORDING_TIME = 5000;  // 5 secondi max

// ESP32-C3 IP per invio comandi TTS (sostituisce il Gateway)
String voiceGatewayIP = "";  // Es: "192.168.1.102"

// ====== INIZIALIZZAZIONE MICROFONO I2S ======

bool setupVoiceMicrophone() {
  if (voiceMicInitialized) return true;

  Serial.println("\n🎤 Inizializzazione microfono INMP441...");

  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = I2S_MIC_SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 4,
    .dma_buf_len = I2S_MIC_BUFFER_SIZE,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
  };

  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_MIC_SCK_PIN,
    .ws_io_num = I2S_MIC_WS_PIN,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_MIC_SD_PIN
  };

  esp_err_t err = i2s_driver_install(I2S_MIC_PORT, &i2s_config, 0, NULL);
  if (err != ESP_OK) {
    Serial.printf("❌ Errore installazione driver I2S: %d\n", err);
    return false;
  }

  err = i2s_set_pin(I2S_MIC_PORT, &pin_config);
  if (err != ESP_OK) {
    Serial.printf("❌ Errore configurazione pin I2S: %d\n", err);
    i2s_driver_uninstall(I2S_MIC_PORT);
    return false;
  }

  // Alloca buffer audio (5 secondi a 16kHz, 16bit mono = 160KB)
  voiceAudioBufferSize = I2S_MIC_SAMPLE_RATE * 2 * 5;  // 5 secondi
  voiceAudioBuffer = (uint8_t*)malloc(voiceAudioBufferSize);
  if (!voiceAudioBuffer) {
    Serial.println("❌ Errore allocazione buffer audio");
    i2s_driver_uninstall(I2S_MIC_PORT);
    return false;
  }

  voiceMicInitialized = true;
  Serial.println("✅ Microfono INMP441 inizializzato");
  return true;
}

// ====== REGISTRAZIONE AUDIO ======

bool voiceStartRecording() {
  if (!voiceMicInitialized) {
    if (!setupVoiceMicrophone()) return false;
  }

  Serial.println("🎤 Inizio registrazione...");
  voiceListening = true;
  voiceRecordingStartTime = millis();
  memset(voiceAudioBuffer, 0, voiceAudioBufferSize);

  return true;
}

size_t voiceStopRecording() {
  voiceListening = false;
  unsigned long recordingTime = millis() - voiceRecordingStartTime;

  Serial.printf("🎤 Registrazione terminata (%lu ms)\n", recordingTime);

  // Calcola dimensione effettiva registrata
  size_t bytesRecorded = min((size_t)(recordingTime * I2S_MIC_SAMPLE_RATE * 2 / 1000), voiceAudioBufferSize);

  return bytesRecorded;
}

// Cattura audio dal microfono (chiamare in loop quando voiceListening = true)
void voiceCaptureAudio() {
  if (!voiceListening) return;

  static size_t bufferPos = 0;
  int32_t samples[64];
  size_t bytesRead = 0;

  // Leggi campioni dal microfono
  i2s_read(I2S_MIC_PORT, samples, sizeof(samples), &bytesRead, portMAX_DELAY);

  // Converti da 32bit a 16bit e scrivi nel buffer
  for (int i = 0; i < bytesRead / 4 && bufferPos < voiceAudioBufferSize; i++) {
    int16_t sample16 = samples[i] >> 14;  // Shift da 32bit a 16bit
    voiceAudioBuffer[bufferPos++] = (uint8_t)(sample16 & 0xFF);
    voiceAudioBuffer[bufferPos++] = (uint8_t)((sample16 >> 8) & 0xFF);
  }

  // Stop se raggiungo il limite di tempo
  if (millis() - voiceRecordingStartTime > VOICE_MAX_RECORDING_TIME) {
    voiceStopRecording();
  }
}

// ====== SPEECH-TO-TEXT (Google Cloud) ======

String voiceSpeechToText(uint8_t* audioData, size_t audioSize) {
  if (!audioData || audioSize == 0) {
    Serial.println("❌ Nessun audio da trascrivere");
    return "";
  }

  Serial.println("🔤 Invio audio a Google Speech-to-Text...");

  // Encode audio in base64
  String audioBase64 = base64::encode(audioData, audioSize);

  // Prepara JSON request
  DynamicJsonDocument doc(audioSize + 2048);
  doc["config"]["encoding"] = "LINEAR16";
  doc["config"]["sampleRateHertz"] = I2S_MIC_SAMPLE_RATE;
  doc["config"]["languageCode"] = "it-IT";  // Italiano
  doc["config"]["model"] = "command_and_search";  // Ottimizzato per comandi brevi

  JsonObject audio = doc.createNestedObject("audio");
  audio["content"] = audioBase64;

  String requestBody;
  serializeJson(doc, requestBody);

  // Invia richiesta a Google Cloud STT
  HTTPClient http;
  String url = "https://speech.googleapis.com/v1/speech:recognize?key=";
  url += GOOGLE_CLOUD_API_KEY;

  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(30000);  // 30 secondi

  int httpCode = http.POST(requestBody);

  if (httpCode != 200) {
    Serial.printf("❌ Errore STT: HTTP %d\n", httpCode);
    http.end();
    return "";
  }

  String response = http.getString();
  http.end();

  // Parse risposta
  DynamicJsonDocument responseDoc(4096);
  DeserializationError error = deserializeJson(responseDoc, response);

  if (error) {
    Serial.println("❌ Errore parsing risposta STT");
    return "";
  }

  // Estrai trascrizione
  if (responseDoc.containsKey("results") && responseDoc["results"].size() > 0) {
    String transcript = responseDoc["results"][0]["alternatives"][0]["transcript"].as<String>();
    Serial.printf("✅ Trascrizione: %s\n", transcript.c_str());
    return transcript;
  }

  Serial.println("⚠️  Nessuna trascrizione disponibile");
  return "";
}

// ====== TEXT-TO-SPEECH (Google Cloud TTS) ======

String voiceTextToSpeechURL(const String& text) {
  if (text.length() == 0) return "";

  Serial.printf("🔊 Generazione TTS per: %s\n", text.c_str());

  HTTPClient http;
  String url = "https://texttospeech.googleapis.com/v1/text:synthesize?key=";
  url += GOOGLE_CLOUD_API_KEY;

  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(15000);

  // Prepara JSON request
  DynamicJsonDocument doc(2048);
  doc["input"]["text"] = text;

  JsonObject voice = doc.createNestedObject("voice");
  voice["languageCode"] = "it-IT";
  voice["name"] = "it-IT-Wavenet-A";  // Voce femminile italiana di alta qualità
  voice["ssmlGender"] = "FEMALE";

  JsonObject audioConfig = doc.createNestedObject("audioConfig");
  audioConfig["audioEncoding"] = "MP3";
  audioConfig["speakingRate"] = 1.0;  // Velocità normale
  audioConfig["pitch"] = 0.0;         // Pitch normale
  audioConfig["volumeGainDb"] = 0.0;  // Volume normale

  String requestBody;
  serializeJson(doc, requestBody);

  int httpCode = http.POST(requestBody);

  if (httpCode != 200) {
    Serial.printf("❌ Errore TTS: HTTP %d\n", httpCode);
    http.end();
    return "";
  }

  String response = http.getString();
  http.end();

  // Parse risposta
  DynamicJsonDocument responseDoc(response.length() + 1024);
  DeserializationError error = deserializeJson(responseDoc, response);

  if (error) {
    Serial.println("❌ Errore parsing risposta TTS");
    return "";
  }

  // Estrai audio base64
  if (responseDoc.containsKey("audioContent")) {
    String audioBase64 = responseDoc["audioContent"].as<String>();
    Serial.printf("✅ TTS generato (%d bytes base64)\n", audioBase64.length());
    return audioBase64;
  }

  Serial.println("❌ Nessun audio nella risposta TTS");
  return "";
}

// Invia TTS all'ESP32-C3 per riproduzione
bool voiceSendTTSToGateway(const String& text) {
  Serial.println("\n╔════════════════════════════════════════════╗");
  Serial.println("║       INVIO TTS A ESP32-C3                ║");
  Serial.println("╚════════════════════════════════════════════╝");

  if (voiceGatewayIP.length() == 0) {
    Serial.println("❌ ESP32-C3 IP non configurato!");
    Serial.println("   Esegui mDNS discovery o configura manualmente.");
    return false;
  }

  Serial.printf("🎯 Destinazione: %s\n", voiceGatewayIP.c_str());
  Serial.printf("📝 Testo: \"%s\"\n", text.substring(0, 100).c_str());

  // Genera audio TTS con Google Cloud
  Serial.println("🔊 Generazione TTS con Google Cloud...");
  String audioBase64 = voiceTextToSpeechURL(text);

  if (audioBase64.length() == 0) {
    Serial.println("❌ Generazione TTS fallita");
    return false;
  }

  Serial.printf("✅ TTS generato: %d bytes (base64)\n", audioBase64.length());
  Serial.printf("   Dimensione MP3 stimata: ~%d bytes\n", (audioBase64.length() * 3) / 4);

  HTTPClient http;
  String url = "http://" + voiceGatewayIP + "/api/tts";

  Serial.printf("📡 POST %s\n", url.c_str());

  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(15000);  // 15 secondi per invio dati grandi

  // Invia audio base64 (MP3 da Google TTS) all'ESP32-C3
  DynamicJsonDocument doc(audioBase64.length() + 1024);
  doc["text"] = text;
  doc["audioBase64"] = audioBase64;

  String requestBody;
  serializeJson(doc, requestBody);

  Serial.printf("📦 Payload JSON: %d bytes\n", requestBody.length());
  Serial.println("⏳ Invio in corso...");

  unsigned long startTime = millis();
  int httpCode = http.POST(requestBody);
  unsigned long elapsedTime = millis() - startTime;

  if (httpCode == 200) {
    Serial.printf("✅ TTS inviato con successo! (%lu ms)\n", elapsedTime);
    String response = http.getString();
    Serial.printf("   Risposta ESP32-C3: %s\n", response.c_str());
    http.end();
    return true;
  } else if (httpCode > 0) {
    Serial.printf("❌ Errore HTTP %d (%lu ms)\n", httpCode, elapsedTime);
    String response = http.getString();
    if (response.length() > 0) {
      Serial.printf("   Errore: %s\n", response.c_str());
    }
    http.end();
    return false;
  } else {
    Serial.printf("❌ Errore connessione: %s\n", http.errorToString(httpCode).c_str());
    Serial.println("   Possibili cause:");
    Serial.println("   1. ESP32-C3 offline o non raggiungibile");
    Serial.println("   2. IP ESP32-C3 cambiato");
    Serial.println("   3. Firewall o problema di rete");
    http.end();
    return false;
  }
}

// ====== FUNZIONE COMPLETA: VOICE -> GEMINI -> TTS ======

void voiceProcessQuestion() {
  if (!voiceListening) return;

  // 1. Stop registrazione
  size_t audioSize = voiceStopRecording();

  if (audioSize < 1000) {  // Minimo 0.5 secondi
    Serial.println("⚠️  Audio troppo corto, ignorato");
    return;
  }

  // 2. Speech-to-Text
  String question = voiceSpeechToText(voiceAudioBuffer, audioSize);

  if (question.length() == 0) {
    Serial.println("❌ Trascrizione fallita");
    voiceSendTTSToGateway("Scusa, non ho capito. Riprova.");
    return;
  }

  // 3. Invia a Gemini AI
  Serial.printf("🤖 Invio a Gemini: %s\n", question.c_str());
  askGemini(question);

  // 4. Attendi risposta (max 30 secondi)
  unsigned long startWait = millis();
  while (geminiWaitingResponse && (millis() - startWait < 30000)) {
    delay(100);
  }

  // 5. Invia risposta a TTS
  if (geminiAnswer.length() > 0) {
    // Limita lunghezza risposta per TTS (max 500 caratteri)
    String ttsText = geminiAnswer.substring(0, 500);
    voiceSendTTSToGateway(ttsText);
  } else {
    voiceSendTTSToGateway("Errore nella risposta di Gemini.");
  }
}

// ====== CONFIGURAZIONE ESP32-C3 IP ======

void voiceSetGatewayIP(const String& ip) {
  voiceGatewayIP = ip;
  Serial.printf("🌐 ESP32-C3 IP configurato: %s\n", ip.c_str());
}

// ====== mDNS SETUP E DISCOVERY ======

bool setupVoiceMDNS() {
  Serial.println("\n╔════════════════════════════════════════════╗");
  Serial.println("║      VOICE ASSISTANT - MDNS SETUP         ║");
  Serial.println("╚════════════════════════════════════════════╝");

  if (!MDNS.begin(MDNS_HOSTNAME)) {
    Serial.println("❌ Errore avvio mDNS");
    return false;
  }

  Serial.printf("✅ mDNS avviato: %s.local\n", MDNS_HOSTNAME);
  Serial.printf("   IP locale: %s\n", WiFi.localIP().toString().c_str());

  // Aggiungi servizi
  MDNS.addService("http", "tcp", 8080);
  MDNS.addService("gemini", "tcp", 8080);
  MDNS.addServiceTxt("gemini", "tcp", "version", "1.0");
  MDNS.addServiceTxt("gemini", "tcp", "capabilities", "stt,gemini,tts");

  Serial.println("✅ Servizi mDNS pubblicati:");
  Serial.println("   - http.tcp (porta 8080)");
  Serial.println("   - gemini.tcp (porta 8080)");

  return true;
}

bool discoverVoiceAssistant() {
  Serial.println("\n╔════════════════════════════════════════════╗");
  Serial.println("║     ESP32-C3 VOICE ASSISTANT DISCOVERY    ║");
  Serial.println("╚════════════════════════════════════════════╝");
  Serial.printf("🔍 Ricerca: %s.local\n", ESP32C3_MDNS);

  // Metodo 1: Query diretta hostname
  Serial.println("⏳ Metodo 1: Query diretta hostname...");
  IPAddress ip = MDNS.queryHost(ESP32C3_MDNS);

  if (ip != IPAddress(0, 0, 0, 0)) {
    voiceGatewayIP = ip.toString();
    Serial.println("\n✅ ESP32-C3 TROVATO!");
    Serial.printf("   IP: %s\n", voiceGatewayIP.c_str());
    Serial.printf("   Endpoint TTS: http://%s/api/tts\n", voiceGatewayIP.c_str());
    return true;
  }

  Serial.println("   ⚠️  Metodo 1 fallito, provo Metodo 2...");

  // Metodo 2: Browse servizi
  Serial.println("⏳ Metodo 2: Browse servizi mDNS...");

  int nrOfServices = MDNS.queryService("voiceassistant", "tcp");

  if (nrOfServices > 0) {
    Serial.printf("📡 Trovati %d voice assistant sulla rete:\n", nrOfServices);

    for (int i = 0; i < nrOfServices; i++) {
      String hostname = MDNS.hostname(i);
      Serial.printf("  [%d] %s @ %s:%d\n",
                    i,
                    hostname.c_str(),
                    MDNS.IP(i).toString().c_str(),
                    MDNS.port(i));

      if (hostname.equalsIgnoreCase(ESP32C3_MDNS)) {
        voiceGatewayIP = MDNS.IP(i).toString();
        Serial.println("\n✅ ESP32-C3 TROVATO!");
        Serial.printf("   Hostname: %s\n", hostname.c_str());
        Serial.printf("   IP: %s\n", voiceGatewayIP.c_str());
        Serial.printf("   Porta: %d\n", MDNS.port(i));
        Serial.printf("   Endpoint TTS: http://%s/api/tts\n", voiceGatewayIP.c_str());
        return true;
      }
    }
  } else {
    Serial.println("   ⚠️  Nessun servizio 'voiceassistant' trovato sulla rete");
  }

  Serial.println("\n❌ ESP32-C3 NON TROVATO");
  Serial.println("   Possibili cause:");
  Serial.println("   1. ESP32-C3 non è acceso o non è connesso al WiFi");
  Serial.println("   2. ESP32-C3 non ha inizializzato mDNS");
  Serial.println("   3. I dispositivi sono su reti WiFi diverse");
  Serial.println("   💡 Soluzione: Configura manualmente l'IP con /gemini/voice/gateway?ip=X.X.X.X");
  return false;
}

#endif // EFFECT_GEMINI_AI
