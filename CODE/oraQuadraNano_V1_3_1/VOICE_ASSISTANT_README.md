# 🎤 ASSISTENTE VOCALE GEMINI AI - GUIDA COMPLETA

## 📋 Panoramica

L'assistente vocale integrato in OraQuadraNano permette di:
- **Parlare** al dispositivo tramite microfono INMP441
- **Ricevere risposte vocali** da Gemini AI riprodotte tramite il gateway
- **Visualizzare** domande e risposte sul display

## 🔧 Hardware Necessario

### 1. Microfono INMP441 I2S
```
Collegamento ESP32:
- SCK  -> GPIO 26
- WS   -> GPIO 25
- SD   -> GPIO 33
- L/R  -> GND (mono, canale sinistro)
- VDD  -> 3.3V
- GND  -> GND
```

### 2. Gateway ORAQUADRA_GATEWAY_HTTP
- ESP32 separato con altoparlante/amplificatore audio
- Connesso alla stessa rete WiFi
- Con firmware aggiornato (endpoint TTS aggiunti)

## 🚀 Setup Iniziale

### Step 1: Configurare Google Cloud API

1. Vai su [Google Cloud Console](https://console.cloud.google.com/)
2. Crea un nuovo progetto o seleziona uno esistente
3. Abilita le seguenti API:
   - **Cloud Speech-to-Text API** (per riconoscimento vocale)
   - **Cloud Text-to-Speech API** (per sintesi vocale)
   - **Generative Language API** (già abilitata per Gemini)

4. Crea una API Key:
   - Vai su "Credentials" → "Create Credentials" → "API Key"
   - Copia l'API Key

5. **IMPORTANTE**: Usa la stessa API Key in:
   - `15_GEMINI_AI.ino` (già configurata)
   - `17_VOICE_ASSISTANT.ino` (già configurata con la stessa key)

### Step 2: Caricare il Firmware

1. **Compila e carica** OraQuadraNano con la nuova integrazione vocale
2. **Compila e carica** il Gateway con i nuovi endpoint TTS

### Step 3: Configurare il Gateway IP

Tramite browser, vai su:
```
http://[IP_ORAQUADRA]:8080/gemini/voice/gateway?ip=[IP_GATEWAY]
```

Esempio:
```
http://192.168.1.100:8080/gemini/voice/gateway?ip=192.168.1.101
```

## 💡 Utilizzo

### Metodo 1: Via Web Interface (Consigliato)

1. Apri browser e vai su: `http://[IP_ORAQUADRA]:8080/gemini`

2. **Per fare una domanda vocale**:
   - Clicca "🎤 Inizia Registrazione"
   - Parla chiaramente (max 5 secondi)
   - Clicca "⏹️ Stop e Invia"
   - Attendi la trascrizione e la risposta
   - L'audio verrà riprodotto automaticamente dal gateway

3. **Per fare una domanda testuale**:
   - Scrivi nel campo di testo
   - Clicca "Invia Domanda"
   - La risposta apparirà sia sul display che sul browser

### Metodo 2: Via API REST

#### Avviare registrazione:
```bash
curl -X POST http://[IP_ORAQUADRA]:8080/gemini/voice/start
```

#### Fermare registrazione e processare:
```bash
curl -X POST http://[IP_ORAQUADRA]:8080/gemini/voice/stop
```

## 🔄 Flusso di Funzionamento

```
1. [MICROFONO] Cattura audio (5 secondi max)
           ↓
2. [GOOGLE STT] Converte audio → testo italiano
           ↓
3. [GEMINI AI] Processa la domanda
           ↓
4. [GOOGLE TTS] Genera audio MP3 dalla risposta
           ↓
5. [GATEWAY] Riproduce l'audio
           ↓
6. [DISPLAY] Mostra domanda e risposta
```

## ⚙️ Configurazione Avanzata

### Modificare la Voce TTS

In `17_VOICE_ASSISTANT.ino`, linea 244:
```cpp
voice["name"] = "it-IT-Wavenet-A";  // Voce femminile
```

Voci disponibili:
- `it-IT-Wavenet-A`: Femminile, alta qualità
- `it-IT-Wavenet-B`: Femminile, variante
- `it-IT-Wavenet-C`: Maschile, alta qualità
- `it-IT-Wavenet-D`: Maschile, variante

### Modificare Tempo Massimo Registrazione

In `17_VOICE_ASSISTANT.ino`, linea 45:
```cpp
const unsigned long VOICE_MAX_RECORDING_TIME = 5000;  // 5 secondi
```

### Modificare Lingua

In `17_VOICE_ASSISTANT.ino`:

**Speech-to-Text** (linea 196):
```cpp
doc["config"]["languageCode"] = "it-IT";  // Italiano
```

**Text-to-Speech** (linea 243):
```cpp
voice["languageCode"] = "it-IT";  // Italiano
```

Lingue supportate: `en-US`, `en-GB`, `es-ES`, `fr-FR`, `de-DE`, ecc.

## 🐛 Troubleshooting

### "Gateway IP non configurato"
**Soluzione**: Configura l'IP del gateway tramite l'endpoint:
```
/gemini/voice/gateway?ip=[IP_GATEWAY]
```

### "Errore STT: HTTP 403"
**Soluzione**:
- Verifica che l'API Key sia corretta
- Verifica che Cloud Speech-to-Text API sia abilitata
- Controlla le quote del progetto Google Cloud

### "Trascrizione fallita"
**Soluzioni**:
- Parla più forte e chiaramente
- Verifica che il microfono sia collegato correttamente
- Controlla i log del Serial Monitor per errori I2S

### "Errore TTS"
**Soluzione**:
- Verifica Cloud Text-to-Speech API sia abilitata
- Controlla le quote del progetto
- Verifica connessione WiFi

### Audio non riprodotto dal Gateway
**Soluzioni**:
- Verifica che il gateway sia online
- Controlla l'IP del gateway sia configurato correttamente
- Verifica che il gateway abbia l'hardware audio collegato

## 📊 Costi Google Cloud

**Free Tier** (mensile):
- **Speech-to-Text**: Primi 60 minuti gratis
- **Text-to-Speech**:
  - Standard voices: 1M caratteri gratis
  - WaveNet voices: 1M caratteri gratis

**Dopo il free tier**:
- STT: ~$0.006 per 15 secondi
- TTS: ~$0.000016 per carattere (WaveNet)

💡 **Tip**: Con un uso normale (10-20 domande/giorno), rimarrai nel free tier!

## 🔒 Sicurezza

⚠️ **IMPORTANTE**:
- **NON condividere** la tua API Key
- **NON committare** l'API Key su GitHub
- Usa **variabili d'ambiente** in produzione
- Abilita **restrizioni API** su Google Cloud Console

## 📝 Note Tecniche

- **Sample Rate**: 16kHz (ottimale per riconoscimento vocale)
- **Encoding**: PCM 16-bit mono
- **Formato Audio**: LINEAR16 per STT, MP3 per TTS
- **Buffer Size**: 160KB (5 secondi a 16kHz)
- **Latenza totale**: ~3-5 secondi (STT + Gemini + TTS)

## 🎯 Esempi di Comandi Vocali

- "Che ore sono?"
- "Qual è la temperatura oggi?"
- "Raccontami una barzelletta"
- "Spiegami cos'è l'intelligenza artificiale"
- "Traduci 'hello' in italiano"
- "Dimmi un fatto interessante"

## 📚 Riferimenti

- [Google Cloud Speech-to-Text](https://cloud.google.com/speech-to-text)
- [Google Cloud Text-to-Speech](https://cloud.google.com/text-to-speech)
- [INMP441 Datasheet](https://www.invensense.com/products/digital/inmp441/)
- [ESP32 I2S Driver](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/i2s.html)

---

**Developed by**: Paolo Sambinello, Alessandro Spagnoletti, Davide Gatti
**Website**: [www.survivalhacking.it](https://www.survivalhacking.it)
**License**: MIT
