# 🎙️ VOICE ASSISTANT - GUIDA COMPLETA

Sistema di assistente vocale **bidirezionale** tra ESP32-S3 (OraQuadraNano) e ESP32-C3 (VOICE).

---

## 📊 ARCHITETTURA DEL SISTEMA

```
┌─────────────────────────────────────────────────────────────┐
│                    VOICE ASSISTANT FLOW                      │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  ESP32-C3 (VOICE)                ESP32-S3 (OraQuadraNano)   │
│  ═════════════════                ════════════════════════   │
│  voiceassistant.local             oraquadra.local:8080      │
│                                                              │
│  1. 🎤 Utente preme pulsante                                │
│     INMP441 registra 5s                                     │
│         │                                                    │
│         ├──POST /api/voice──────────►                       │
│         │  {audioContent: base64}    │                      │
│         │                             ▼                      │
│         │                      🔤 Google STT                │
│         │                        (Speech-to-Text)           │
│         │                             │                      │
│         │                             ▼                      │
│         │                      🤖 Gemini AI                 │
│         │                        (Elaborazione)             │
│         │                             │                      │
│         │                             ▼                      │
│         │                      🔊 Google TTS                │
│         │                        (Text-to-Speech)           │
│         │                             │                      │
│         ◄──POST /api/tts─────────────┤                      │
│         │  {text, audioBase64}       │                      │
│         ▼                                                    │
│  💾 Decodifica base64                                       │
│  💾 Salva su SPIFFS                                         │
│  🔊 MAX98357A                                               │
│     Riproduce MP3                                           │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

---

## 🔧 CONFIGURAZIONE INIZIALE

### **1. ESP32-S3 (OraQuadraNano)**

#### Hardware richiesto:
- ESP32-S3 con display
- Microfono INMP441 I2S (opzionale, per futuro sviluppo locale)

#### Configurazione WiFi:
```cpp
// 0_SETUP.ino - WiFi configurato via WiFiManager
```

#### API Key Google Cloud:
```cpp
// 17_VOICE_ASSISTANT.ino - Riga 39
const char* GOOGLE_CLOUD_API_KEY = "AIzaSyCNgdSPlSdzlzuNm2h2wfk7A9oH-4aF5vs";
```

#### Hostname mDNS:
```cpp
#define MDNS_HOSTNAME "oraquadra"  // oraquadra.local
#define ESP32C3_MDNS "voiceassistant"  // voiceassistant.local
```

### **2. ESP32-C3 (VOICE)**

#### Hardware richiesto:
- ESP32-C3 SuperMini
- Microfono INMP441 I2S
- Amplificatore MAX98357A I2S
- Pulsante su GPIO 0 (BOOT)
- LED su GPIO 2

#### Connessioni Hardware:

**Microfono INMP441** (modalità RX):
```
SCK  → GPIO 8
WS   → GPIO 9
SD   → GPIO 10
L/R  → GND
VDD  → 3.3V
GND  → GND
```

**Speaker MAX98357A** (modalità TX):
```
BCLK → GPIO 4
LRC  → GPIO 5
DIN  → GPIO 6
SD   → GPIO 7 (Enable)
VIN  → 5V
GND  → GND
```

#### Configurazione WiFi:
```cpp
// VOICE.ino - Righe 93-94
const char* WIFI_SSID = "SambinelloLan";
const char* WIFI_PASSWORD = "Smbpla62h02l872U=";
```

#### Hostname mDNS:
```cpp
#define MDNS_HOSTNAME "voiceassistant"  // voiceassistant.local
#define ORAQUADRA_MDNS "oraquadra"  // oraquadra.local
```

---

## 🚀 AVVIO DEL SISTEMA

### **Passo 1: Compila e carica OraQuadraNano (ESP32-S3)**

1. Apri `oraQuadraNano_V1_3_1.ino` in Arduino IDE
2. Verifica che `EFFECT_GEMINI_AI` sia definito (riga 85)
3. Seleziona scheda **ESP32S3 Dev Module**
4. Compila e carica

### **Passo 2: Compila e carica VOICE (ESP32-C3)**

1. Apri `VOICE/VOICE.ino` in Arduino IDE
2. Seleziona scheda **ESP32C3 Dev Module**
3. Configura partizione: **Default 4MB with spiffs (1.2MB APP/1.5MB SPIFFS)**
4. Compila e carica

### **Passo 3: Verifica connessione**

**ESP32-S3 Serial Monitor** dovrebbe mostrare:
```
╔════════════════════════════════════════════╗
║      VOICE ASSISTANT - MDNS SETUP         ║
╚════════════════════════════════════════════╝
✅ mDNS avviato: oraquadra.local
   IP locale: 192.168.1.50
✅ Servizi mDNS pubblicati:
   - http.tcp (porta 8080)
   - gemini.tcp (porta 8080)

╔════════════════════════════════════════════╗
║     ESP32-C3 VOICE ASSISTANT DISCOVERY    ║
╚════════════════════════════════════════════╝
🔍 Ricerca: voiceassistant.local
⏳ Metodo 1: Query diretta hostname...

✅ ESP32-C3 TROVATO!
   IP: 192.168.1.100
   Endpoint TTS: http://192.168.1.100/api/tts
```

**ESP32-C3 Serial Monitor** dovrebbe mostrare:
```
╔════════════════════════════════════════════╗
║   ESP32-C3 VOICE ASSISTANT COMPLETO       ║
║   MICROFONO INMP441 + SPEAKER MAX98357A   ║
╚════════════════════════════════════════════╝

✅ WiFi connesso!
📍 IP Address: 192.168.1.100

🎤 Inizializzazione microfono INMP441...
✅ Microfono INMP441 OK (I2S condiviso)

🔊 Inizializzazione speaker MAX98357A...
✅ Speaker MAX98357A OK (I2S condiviso)

🌐 Inizializzazione mDNS...
✅ mDNS OK: voiceassistant.local

🔍 Ricerca OraQuadraNano via mDNS...
✅ OraQuadraNano trovato: 192.168.1.50

╔════════════════════════════════════════════╗
║           CONFIGURAZIONE                   ║
╠════════════════════════════════════════════╣
║ OraQuadra IP:  192.168.1.50                ║
║ Web Server:    http://192.168.1.100        ║
╠════════════════════════════════════════════╣
║ I2S Port:      I2S_NUM_0 (CONDIVISO)       ║
║ Modalità:      Time-sharing (Simplex)      ║
╚════════════════════════════════════════════╝
```

---

## 🧪 TEST DEL SISTEMA

### **Test 1: Registrazione e invio audio**

1. Premi il **pulsante BOOT (GPIO 0)** su ESP32-C3
2. Parla per 5 secondi (es: "Ciao Gemini, chi sei?")
3. Rilascia il pulsante

**LOG ATTESI su ESP32-C3:**
```
🎤 ► Registrazione avviata
🎤 ⏹ Registrazione terminata (5000 ms, 160000 bytes)
📤 Invio audio a OraQuadraNano...
📦 Invio 213460 bytes JSON...
✅ Audio inviato con successo!
```

**LOG ATTESI su ESP32-S3:**
```
╔════════════════════════════════════════════╗
║   RICEVUTO AUDIO DA ESP32-C3              ║
╚════════════════════════════════════════════╝
📥 Inizio ricezione: 213460 bytes totali
   Ricevuti 1460/213460 bytes (0.7%)
   Ricevuti 5840/213460 bytes (2.7%)
   ...
✅ Ricezione completata in 3456 ms
🔊 Audio decodificato: 160000 bytes

🔤 Invio audio a Google Speech-to-Text...
✅ Trascrizione: Ciao Gemini, chi sei?

╔════════════════════════════════════════════╗
║         ELABORAZIONE GEMINI AI            ║
╚════════════════════════════════════════════╝
🤖 Domanda: "Ciao Gemini, chi sei?"
⏳ Invio a Gemini...
✅ Risposta Gemini ricevuta in 2345 ms
📝 Risposta: "Sono Gemini, un modello di intelligenza artificiale..."

🔊 Preparazione TTS (245 caratteri)...

╔════════════════════════════════════════════╗
║       INVIO TTS A ESP32-C3                ║
╚════════════════════════════════════════════╝
🎯 Destinazione: 192.168.1.100
📝 Testo: "Sono Gemini, un modello di intelligenza..."
🔊 Generazione TTS con Google Cloud...
✅ TTS generato: 18540 bytes (base64)
   Dimensione MP3 stimata: ~13905 bytes
📡 POST http://192.168.1.100/api/tts
📦 Payload JSON: 18892 bytes
⏳ Invio in corso...
✅ TTS inviato con successo! (1234 ms)
   Risposta ESP32-C3: {"success":true}

✅ ═══ PROCESSING COMPLETATO ═══
```

**LOG ATTESI su ESP32-C3 (riproduzione):**
```
📥 Inizio ricezione dati TTS...
   Dimensione totale: 18892 bytes
   Ricevuti 1460/18892 bytes (7.7%)
   ...
✅ Ricezione completata, parsing JSON...
✅ JSON parsato correttamente
   Text: Sono Gemini, un modello di intelligenza...
   AudioBase64 length: 18540 bytes
🔊 Modalità: Riproduzione da base64
   Schedulazione riproduzione asincrona...
✅ Audio accodato per riproduzione

🎵 Avvio riproduzione audio dalla coda...
🔊 Decodifica base64 e preparazione riproduzione...
✅ Base64 decodificato: 13905 bytes
💾 Salvato file temporaneo: 13905 bytes
🔊 Avvio riproduzione MP3 da SPIFFS...
🎵 Inizializzazione decoder MP3...
✅ Riproduzione TTS avviata da SPIFFS!

[AUDIO RIPRODOTTO VIA SPEAKER]

🔊 Riproduzione completata
🧹 Cleanup audio player...
✅ Cleanup completato
```

---

## 🎛️ COMANDI SERIAL MONITOR

### **ESP32-C3 (VOICE)**

| Comando | Descrizione |
|---------|-------------|
| `r` | Avvia/Stop registrazione audio |
| `t` | Test riproduzione ultimo file da SPIFFS |
| `i` | Mostra informazioni sistema |

### **ESP32-S3 (OraQuadraNano)**

- **Web Interface**: `http://oraquadra.local:8080/gemini`
- **Configura ESP32-C3 IP manualmente**: `http://oraquadra.local:8080/gemini/voice/gateway?ip=192.168.1.100`

---

## 🔍 TROUBLESHOOTING

### **Problema: ESP32-C3 non trovato via mDNS**

**Sintomi:**
```
❌ ESP32-C3 NON TROVATO
   Possibili cause:
   1. ESP32-C3 non è acceso o non è connesso al WiFi
   2. ESP32-C3 non ha inizializzato mDNS
   3. I dispositivi sono su reti WiFi diverse
```

**Soluzioni:**
1. Verifica che entrambi i dispositivi siano sulla stessa rete WiFi
2. Riavvia ESP32-C3
3. Configura manualmente IP: `http://oraquadra.local:8080/gemini/voice/gateway?ip=<IP_ESP32C3>`

### **Problema: Non riproduce audio**

**Sintomi:**
```
Pending Audio: SÌ (in coda)
Playing: NO
MP3 Running: NO
```

**Soluzioni:**
1. Premi `i` su ESP32-C3 per vedere stato
2. Verifica che `SPIFFS Free` > 20KB
3. Verifica che `Free Heap` > 100KB
4. Premi `t` per testare riproduzione file esistente
5. Verifica connessioni MAX98357A (GPIO 4,5,6,7)
6. Verifica alimentazione amplificatore (5V)

### **Problema: Audio distorto**

**Soluzioni:**
1. Verifica GPIO corretti per I2S
2. Controlla alimentazione stabile (5V @ 1A min)
3. Aggiungi condensatore 100µF su alimentazione speaker
4. Riduci volume in `VOICE.ino` (currentVolume = 0.5)

### **Problema: Errore HTTP invio TTS**

**Sintomi:**
```
❌ Errore HTTP 404
   Errore: Not Found
```

**Soluzioni:**
1. Verifica ESP32-C3 raggiungibile: `ping <IP_ESP32C3>`
2. Testa endpoint manualmente: `curl http://<IP_ESP32C3>/status`
3. Riavvia ESP32-C3

---

## 📚 FILE IMPORTANTI

### **ESP32-S3 (OraQuadraNano)**
- `17_VOICE_ASSISTANT.ino` - Logica voice assistant (STT, Gemini, TTS)
- `16_WEBSERVER_GEMINI.ino` - Endpoint `/api/voice` per ricevere audio
- `0_SETUP.ino` - Setup WiFi e mDNS

### **ESP32-C3 (VOICE)**
- `VOICE.ino` - Gestione microfono, speaker, comunicazione

---

## 🎯 ENDPOINT API

### **ESP32-S3 → ESP32-C3**

#### `POST /api/tts`
Invia audio TTS da riprodurre.

**Request:**
```json
{
  "text": "Ciao, come stai?",
  "audioBase64": "SUQzBAAAAAAAI1RTU0UAAAAPAAADTGF2ZjU5LjI3LjEwMAAAAAAAAAAAAAAA..."
}
```

**Response:**
```json
{
  "success": true
}
```

### **ESP32-C3 → ESP32-S3**

#### `POST /api/voice`
Invia audio registrato per elaborazione.

**Request:**
```json
{
  "encoding": "LINEAR16",
  "sampleRate": 16000,
  "channels": 1,
  "languageCode": "it-IT",
  "audioContent": "//7+/v7+/v7+/v7+/v7+..."
}
```

**Response:**
```json
{
  "success": true,
  "message": "Audio in elaborazione"
}
```

---

## ⚙️ CONFIGURAZIONI AVANZATE

### **Cambiare lingua STT/TTS**

```cpp
// 17_VOICE_ASSISTANT.ino
doc["config"]["languageCode"] = "en-US";  // Inglese
voice["languageCode"] = "en-US";
voice["name"] = "en-US-Wavenet-D";  // Voce inglese
```

### **Cambiare voce TTS**

Voci disponibili: https://cloud.google.com/text-to-speech/docs/voices

```cpp
// 17_VOICE_ASSISTANT.ino - Riga 249
voice["name"] = "it-IT-Wavenet-A";  // Femmina
voice["name"] = "it-IT-Wavenet-D";  // Maschio
```

### **Aumentare tempo registrazione**

```cpp
// VOICE.ino - Riga 124
#define MAX_RECORD_TIME_MS   10000  // 10 secondi
```

### **Regolare volume speaker**

```cpp
// VOICE.ino - Riga 113
float currentVolume = 1.0;  // 100% (max)
float currentVolume = 0.5;  // 50%
float currentVolume = 0.3;  // 30%
```

---

## 📊 PERFORMANCE

- **Latenza registrazione → risposta**: ~8-15 secondi
  - Registrazione: 5s
  - Upload + STT: 2-3s
  - Gemini AI: 1-3s
  - TTS generation: 1-2s
  - Download + playback: 2-5s

- **Consumo RAM ESP32-C3**: ~120KB (con buffer audio)
- **Consumo SPIFFS**: ~15-30KB per file TTS
- **Bitrate audio**: 16kHz, 16bit, mono = 256 kbit/s

---

## ✅ CHECKLIST PRE-TEST

- [ ] ESP32-S3 connesso al WiFi (stessa rete di ESP32-C3)
- [ ] ESP32-C3 connesso al WiFi (stessa rete di ESP32-S3)
- [ ] Google Cloud API Key configurata
- [ ] mDNS attivo su entrambi i dispositivi
- [ ] Microfono INMP441 collegato correttamente
- [ ] Speaker MAX98357A collegato correttamente
- [ ] Alimentazione 5V stabile per amplificatore
- [ ] SPIFFS inizializzato su ESP32-C3
- [ ] Serial Monitor aperto per vedere i log

---

**By Paolo Sambinello - 2025**
**www.survivalhacking.it**
