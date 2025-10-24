// Controlla se è cambiata l'ora per annunciarla vocalmente
void checkTimeAndAnnounce() {
  #ifdef AUDIO // Blocco di codice compilato solo se la macro AUDIO è definita

  // Verifica se l'ora corrente (ottenuta dal fuso orario) è un valore valido (non maggiore di 23).
  if (myTZ.hour() > 23) {
    Serial.println("Errore lettura ora"); // Stampa un messaggio di errore se l'ora non è valida.
    return;                             // Esce dalla funzione senza fare altro.
  }

  // Ottiene l'ora corrente formattata come stringa (ore:minuti:secondi) per scopi di debug.
  String timeString = myTZ.dateTime("H:i:s");

  // Previene controlli multipli nello stesso minuto. Se il minuto corrente è lo stesso dell'ultimo minuto controllato,
  if (myTZ.minute() == lastMinuteChecked) {
    return; // esce dalla funzione per evitare annunci ripetuti nello stesso minuto.
  }

  // Aggiorna la variabile statica lastMinuteChecked con il minuto corrente, segnando che è stato controllato.
  lastMinuteChecked = myTZ.minute();

  // Stampa l'ora attuale sulla seriale per debug.
  Serial.println("Ora attuale: " + timeString);

  // Verifica se l'ora è cambiata rispetto all'ultima ora annunciata E se i minuti sono esattamente a zero.
  if (myTZ.hour() != lastHour && myTZ.minute() == 0) {
    Serial.println("Cambio ora rilevato: " + String(myTZ.hour()) + ":00"); // Indica un cambio di ora.

    // Chiama la funzione migliorata per annunciare l'ora vocalmente.
    announceTimeFixed();

    // Aggiorna la variabile statica lastHour con l'ora corrente.
    lastHour = myTZ.hour();
  }
#endif // Chiusura del blocco #ifdef AUDIO
}

// Funzione per annunciare l'ora corrente tramite sintesi vocale (TTS)
bool announceTime() {
  #ifdef AUDIO // Blocco di codice compilato solo se la macro AUDIO è definita

  // Pulisce le risorse audio utilizzate in precedenza per eventuali riproduzioni in corso.
  cleanupAudio();

  // Assicura che l'amplificatore audio sia abilitato impostando il pin di abilitazione (se definito) al livello appropriato.
  digitalWrite(I2S_PIN_ENABLE, LOW);  // Imposta il pin DE (Data Enable) a basso (potrebbe dipendere dalla circuiteria).
  delay(5);                           // Breve ritardo.
//  digitalWrite(I2S_PIN_ENABLE, HIGH); // Imposta il pin DE a alto (potrebbe dipendere dalla circuiteria).
//  delay(10);                          // Breve ritardo.

  // Costruisce il messaggio vocale dell'ora in base all'ora e ai minuti correnti.
  String timeMessage;

  if (currentHour == 0 || currentHour == 24) {
    timeMessage = "È mezzanotte";
  } else if (currentHour == 12) {
    timeMessage = "È mezzogiorno";
  } else if (currentHour == 1 || currentHour == 13) {
    timeMessage = "È l'una";
  } else {
    timeMessage = "Sono le " + String(currentHour > 12 ? currentHour - 12 : currentHour);
  }

  if (currentMinute > 0) {
    timeMessage += currentMinute == 1 ? " e un minuto" : " e " + String(currentMinute) + " minuti";
  }

  // Stampa il messaggio che verrà annunciato sulla seriale per debug.
  Serial.println("Annuncio ora: " + timeMessage);

  // Verifica se l'oggetto per l'output audio I2S è stato inizializzato correttamente.
  if (output == nullptr) {
    Serial.println("Output audio non inizializzato, reinizializzo...");
    output = new AudioOutputI2S();                 // Crea una nuova istanza dell'output audio I2S.
    output->SetPinout(I2S_BCLK, I2S_LRC, I2S_DOUT); // Imposta i pin per la comunicazione I2S.
    output->SetGain(VOLUME_LEVEL);                 // Imposta il livello del volume.
    output->SetChannels(1);                      // Imposta il numero di canali audio (mono).
    delay(100);                                   // Attende un breve periodo per l'inizializzazione.
  }

  // Tenta di riprodurre il messaggio TTS con una gestione dei tentativi in caso di fallimento iniziale.
  bool result = false;

  for (int retry = 0; retry < 2; retry++) {
    result = playTTS(timeMessage, "it"); // Chiama la funzione per riprodurre il testo tramite TTS in italiano.
    if (result) {
      Serial.println("TTS avviato con successo, attendo completamento...");
      delay(300); // Attende un breve periodo per assicurarsi che la riproduzione sia iniziata.

      // Attende che la riproduzione finisca, con un timeout massimo di 10 secondi per evitare blocchi.
      unsigned long startTime = millis();
      while (mp3 && mp3->isRunning() && millis() - startTime < 10000) {
        if (!mp3->loop()) break; // Interrompe il loop se la riproduzione è finita o in errore.
        delay(10);
      }

      Serial.println("Riproduzione terminata");
      cleanupAudio(); // Pulisce le risorse audio dopo la riproduzione.
      return true;   // Indica che l'annuncio è avvenuto con successo.
    } else if (retry == 0) {
      Serial.println("Primo tentativo TTS fallito, riprovo...");
      cleanupAudio(); // Pulisce le risorse audio prima di riprovare.
      delay(500);    // Breve ritardo prima del secondo tentativo.
    }
  }

  Serial.println("Errore avvio TTS dopo tentativi"); // Se entrambi i tentativi falliscono.
  return false;                                   // Indica che l'annuncio non è avvenuto.
   #endif // Chiusura del blocco #ifdef AUDIO
}

// Genera un'onda sinusoidale a una frequenza specifica per il buffer audio.
void generateSineWave() {
  #ifdef AUDIO // Blocco di codice compilato solo se la macro AUDIO è definita
  const float frequency = 440.0;  // Frequenza del tono (La4 a 440 Hz).
  const float amplitude = 10000.0; // Ampiezza dell'onda (valore massimo: +/- 32767 per int16_t).

  // Popola il buffer sineBuffer con i campioni dell'onda sinusoidale.
  for (int i = 0; i < bufferLen; i++) {
    // Calcola l'angolo per ogni campione basato sull'indice, la frequenza e la frequenza di campionamento.
    float angle = i * 2.0 * PI * frequency / sampleRate;
    // Calcola il valore del campione usando la funzione seno e l'ampiezza, convertendolo a un intero a 16 bit.
    sineBuffer[i] = (int16_t)(sin(angle) * amplitude);
  }
  #endif // Chiusura del blocco #ifdef AUDIO
}

// Riproduce un tono a una data frequenza per una data durata.
void playTone(int frequency, int duration_ms) {
  #ifdef AUDIO // Blocco di codice compilato solo se la macro AUDIO è definita
  // Calcola il numero totale di campioni necessari per la durata specificata (stereo).
  const int samples = sampleRate * duration_ms / 1000;

  // Alloca memoria per il buffer audio (stereo) nella PSRAM invece che in SRAM.
  int16_t *buffer = (int16_t*)heap_caps_malloc(samples * sizeof(int16_t) * 2, MALLOC_CAP_SPIRAM);

  // Verifica se l'allocazione di memoria ha avuto successo.
  if (!buffer) {
    Serial.println("Errore allocazione buffer in PSRAM - provo in SRAM");
    // Fallback: prova ad allocare in SRAM interna se PSRAM non disponibile
    buffer = (int16_t*)malloc(samples * sizeof(int16_t) * 2);
    if (!buffer) {
      Serial.println("Errore allocazione buffer anche in SRAM");
      return; // Esce dalla funzione.
    }
  }

  // Genera l'onda sinusoidale per la frequenza specificata e la popola nel buffer (stereo).
  for (int i = 0; i < samples; i++) {
    float angle = i * 2.0 * PI * frequency / sampleRate;
    int16_t sample = (int16_t)(sin(angle) * 10000); // Calcola il campione.
    buffer[i * 2] = sample;     // Scrive il campione per il canale sinistro.
    buffer[i * 2 + 1] = sample; // Scrive lo stesso campione per il canale destro (tono mono riprodotto su entrambi i canali).
  }

  // Riproduce il buffer audio tramite l'output I2S.
  int16_t samplePair[2]; // Buffer temporaneo per contenere una coppia di campioni (sinistro e destro).

  for (int i = 0; i < samples; i++) {
    // Copia una coppia di campioni dal buffer principale al buffer temporaneo.
    samplePair[0] = buffer[i * 2];     // Canale sinistro.
    samplePair[1] = buffer[i * 2 + 1]; // Canale destro.

    // Invia la coppia di campioni al DAC I2S tramite l'oggetto output.
    if (output) {
      output->ConsumeSample(samplePair); // Invia un frame stereo (due campioni).
    }

    // Permette al watchdog timer di resettarsi periodicamente durante la riproduzione lunga.
    if (i % 1000 == 0) {
      yield();
    }
  }

  // Libera la memoria allocata per il buffer audio (PSRAM o SRAM).
  heap_caps_free(buffer);

  // Attende un breve periodo per assicurarsi che l'audio abbia terminato di essere riprodotto.
  delay(100);
  #endif // Chiusura del blocco #ifdef AUDIO
}

// Riproduce un testo tramite sintesi vocale (TTS) utilizzando il servizio di Google Translate.
bool playTTS(const String& text, const String& language) {
   #ifdef AUDIO // Blocco di codice compilato solo se la macro AUDIO è definita
  // Pulisce le risorse audio utilizzate in precedenza.
  cleanupAudio();

  // Definisce il nome del file temporaneo dove verrà salvato l'audio MP3.
  String tempFile = "/tts_temp.mp3";
  // Se il file temporaneo esiste già, lo elimina.
  if (SPIFFS.exists(tempFile)) {
    SPIFFS.remove(tempFile);
  }

  // Costruisce l'URL per richiedere il TTS da Google Translate.
  String url = "https://translate.google.com/translate_tts?ie=UTF-8&client=tw-ob&q=";
  // CORREZIONE: Utilizza la funzione myUrlEncode per codificare correttamente il testo nell'URL.
  url += myUrlEncode(text);
  url += "&tl=" + language;                     // Specifica la lingua del testo.
  url += "&textlen=" + String(text.length()); // Indica la lunghezza del testo.

  // Stampa l'URL di download sulla seriale per debug.
  Serial.println("Download TTS: " + text);

  // Inizializza un oggetto HTTPClient per effettuare la richiesta GET.
  HTTPClient http;
  http.setTimeout(15000);           // Imposta un timeout per la connessione HTTP.
  http.setUserAgent("Mozilla/5.0"); // Imposta l'user agent per simulare un browser.

  // Inizia la connessione HTTP con l'URL specificato.
  if (!http.begin(url)) {
    Serial.println("HTTP begin fallito"); // Stampa un errore se la connessione non può essere stabilita.
    return false;                         // Indica un fallimento.
  }

  // Effettua la richiesta GET e ottiene il codice di risposta HTTP.
  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("HTTP error: %d\n", httpCode); // Stampa l'errore HTTP.
    http.end();                                  // Termina la connessione HTTP.
    return false;                                // Indica un fallimento.
  }

  // Apre il file temporaneo in modalità scrittura per salvare l'audio scaricato.
  File fileOut = SPIFFS.open(tempFile, FILE_WRITE);
  if (!fileOut) {
    Serial.println("Errore creazione file"); // Stampa un errore se il file non può essere creato.
    http.end();                              // Termina la connessione HTTP.
    return false;                            // Indica un fallimento.
  }

  // Inizia il download del flusso audio dal server HTTP e lo scrive nel file.
  uint32_t startTime = millis();
  WiFiClient* stream = http.getStreamPtr(); // Ottiene un puntatore al flusso di dati.
  uint8_t buffer[2048];                     // Buffer per leggere i dati dal flusso.
  size_t written = 0;

  // Continua a leggere dal flusso finché la connessione HTTP è attiva.
  while (http.connected()) {
    size_t size = stream->available(); // Ottiene il numero di byte disponibili nel flusso.
    if (size) {
      // Legge i byte disponibili nel buffer.
      int c = stream->readBytes(buffer, min((size_t)sizeof(buffer), size));
      fileOut.write(buffer, c); // Scrive i byte letti nel file.
      written += c;             // Aggiorna il numero di byte scritti.
    } else if (written > 0 && millis() - startTime > 1500) {
      break; // Interrompe se non ci sono dati e il download iniziale è avvenuto.
    }
    yield(); // Permette al watchdog di resettarsi.
  }

  fileOut.close(); // Chiude il file dopo il download.
  http.end();      // Termina la connessione HTTP.

  // Stampa informazioni sul download (dimensione e tempo).
  Serial.printf("Download completato: %d bytes in %d ms\n", written, millis() - startTime);

  // Verifica se il file scaricato è troppo piccolo (potrebbe indicare un errore).
  if (written < 1000) {
    Serial.println("File troppo piccolo");
    SPIFFS.remove(tempFile); // Elimina il file incompleto.
    return false;            // Indica un fallimento.
  }

  // Attende un breve periodo dopo il download.
  delay(100);

  // Prepara gli oggetti per la riproduzione dell'audio MP3 dal file scaricato.
  file = new AudioFileSourceSPIFFS(tempFile.c_str()); // Sorgente audio dal file SPIFFS.
  buff = new AudioFileSourceBuffer(file, 256);       // Buffer per la sorgente audio.
  mp3 = new AudioGeneratorMP3();                    // Decodificatore MP3.

  // Inizia la riproduzione dell'audio MP3.
  if (mp3->begin(buff, output)) {
    isPlaying = true;                      // Imposta il flag di riproduzione a true.
    Serial.println("Riproduzione avviata con successo"); // Indica che la riproduzione è iniziata.
    return true;                           // Indica successo.
  } else {
    Serial.println("Errore avvio riproduzione"); // Indica un errore nell'avvio della riproduzione.
    cleanupAudio();                          // Pulisce le risorse audio.
    return false;                            // Indica fallimento.
  }
 #endif // Chiusura del blocco #ifdef AUDIO
}

// Genera uno sweep di frequenze audio, utile per scopi diagnostici.
void playFrequencySweep() {
  #ifdef AUDIO // Blocco di codice compilato solo se la macro AUDIO è definita
  const int startFreq = 100;  // Frequenza iniziale dello sweep (Hz).
  const int endFreq = 3000;   // Frequenza finale dello sweep (Hz).
  const int stepFreq = 100;   // Incremento di frequenza ad ogni passo (Hz).
  const int durationMs = 200; // Durata di ogni tono nello sweep (millisecondi).

  // Cicla attraverso le frequenze dallo start all'end con l'incremento specificato.
  for (int freq = startFreq; freq <= endFreq; freq += stepFreq) {
    playTone(freq, durationMs); // Riproduce un tono alla frequenza corrente per la durata specificata.
    delay(50);                   // Breve pausa tra un tono e l'altro.
  }
#endif // Chiusura del blocco #ifdef AUDIO
}

// Pulisce le risorse audio allocate (decoder MP3, buffer, file).
void cleanupAudio() {
  #ifdef AUDIO // Blocco di codice compilato solo se la macro AUDIO è definita
  // Se l'oggetto del decoder MP3 esiste, verifica se è in esecuzione e lo ferma, quindi lo elimina
if (mp3) {
    if (mp3->isRunning()) mp3->stop();
    delete mp3;
    mp3 = nullptr;
  }

  // Se l'oggetto del buffer audio esiste, lo elimina.
  if (buff) {
    delete buff;
    buff = nullptr;
  }

  // Se l'oggetto del file audio esiste, lo elimina.
  if (file) {
    delete file;
    file = nullptr;
  }

  // Disabilita amplificatore audio (PIN 9)
  digitalWrite(I2S_PIN_ENABLE, LOW);
  delay(5);

  // RIPRISTINA PIN 0 per display RGB (rosso)
  pinMode(0, OUTPUT);  // PIN 0 ritorna come output per RGB
  delay(5);

  // Resetta il flag che indica se l'audio è in riproduzione.
  isPlaying = false;
#endif // Chiusura del blocco #ifdef AUDIO
}

// Funzione per codificare una stringa in formato URL (usata per l'invio a servizi web come Google TTS).
String myUrlEncode(const String& msg) {
  #ifdef AUDIO // Blocco di codice compilato solo se la macro AUDIO è definita
  const char *hex = "0123456789ABCDEF"; // Array di caratteri esadecimali.
  String encodedMsg = "";                // Stringa per contenere il messaggio codificato.

  // Itera attraverso ogni carattere del messaggio originale.
  for (int i = 0; i < msg.length(); i++) {
    char c = msg.charAt(i); // Ottiene il carattere corrente.
    if (c == ' ') {
      encodedMsg += '+'; // Gli spazi vengono sostituiti con il simbolo '+'.
    } else if (isAlphaNumeric(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      encodedMsg += c; // I caratteri alfanumerici e alcuni simboli non vengono codificati.
    } else {
      encodedMsg += '%';                      // Gli altri caratteri vengono codificati con il simbolo '%'.
      encodedMsg += hex[c >> 4];               // Aggiunge la parte alta del byte in esadecimale.
      encodedMsg += hex[c & 15];              // Aggiunge la parte bassa del byte in esadecimale.
    }
  }
  return encodedMsg; // Restituisce la stringa codificata.
#endif // Chiusura del blocco #ifdef AUDIO
}

// ================== FUNZIONI AUDIO LOCALE ==================

// Funzione per riprodurre un singolo file MP3 locale da SPIFFS
bool playLocalMP3(const char* filename) {
  #ifdef AUDIO

  // Costruisce il percorso completo del file
  String filePath = "/";
  filePath += filename;

  // Verifica se il file esiste
  if (!SPIFFS.exists(filePath)) {
    Serial.println("File audio non trovato: " + filePath);
    return false;
  }

  Serial.println("Riproduzione: " + filePath);

  // Pulisce eventuali risorse audio esistenti (ma NON disabilita amplificatore)
  if (mp3) {
    if (mp3->isRunning()) mp3->stop();
    delete mp3;
    mp3 = nullptr;
  }
  if (buff) {
    delete buff;
    buff = nullptr;
  }
  if (file) {
    delete file;
    file = nullptr;
  }

  // Crea un nuovo oggetto per l'output audio I2S se necessario
  if (output == nullptr) {
    output = new AudioOutputI2S();
    output->SetPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    output->SetGain(VOLUME_LEVEL);
    output->SetChannels(1);
    delay(50);
  }

  // Prepara gli oggetti per la riproduzione
  file = new AudioFileSourceSPIFFS(filePath.c_str());
  buff = new AudioFileSourceBuffer(file, 256);  // Buffer ottimale
  mp3 = new AudioGeneratorMP3();

  // Inizia la riproduzione
  if (mp3->begin(buff, output)) {
    isPlaying = true;

    // Attende il completamento della riproduzione
    unsigned long playStartTime = millis();
    while (mp3->isRunning() && millis() - playStartTime < 10000) {
      if (!mp3->loop()) break;
      delay(1);  // Loop veloce
      yield();
    }

    mp3->stop();

    // Pulisce solo le risorse MP3 (NON l'amplificatore)
    if (mp3) {
      delete mp3;
      mp3 = nullptr;
    }
    if (buff) {
      delete buff;
      buff = nullptr;
    }
    if (file) {
      delete file;
      file = nullptr;
    }

    isPlaying = false;

    // Pausa tra i file per stabilizzare l'hardware audio
    delay(100);
    return true;
  } else {
    Serial.println("Errore avvio riproduzione: " + filePath);
    cleanupAudio();
    return false;
  }

  #endif
  return false;
}

// Funzione per riprodurre una sequenza di file MP3 locali
bool playMP3Sequence(const String files[], int count) {
  #ifdef AUDIO

  Serial.printf("Riproduzione sequenza di %d file\n", count);

  // ABILITA AMPLIFICATORE E DISABILITA PIN 0 (RGB) ALL'INIZIO DELLA SEQUENZA
  pinMode(0, INPUT);  // PIN 0 in alta impedenza per evitare conflitto
  delay(5);
  digitalWrite(I2S_PIN_ENABLE, HIGH);  // Abilita amplificatore (PIN 9)
  delay(10);

  // Riproduci tutti i file della sequenza
  bool success = true;
  for (int i = 0; i < count; i++) {
    Serial.printf("[%d/%d] Tentativo riproduzione: %s\n", i+1, count, files[i].c_str());

    if (!playLocalMP3(files[i].c_str())) {
      Serial.println("ERRORE riproduzione file: " + files[i] + " - CONTINUO con il prossimo");
      success = false;
      // NON fare break - continua con i file successivi
    } else {
      Serial.println("OK: " + files[i]);
    }
  }

  // DISABILITA AMPLIFICATORE E RIPRISTINA PIN 0 ALLA FINE DELLA SEQUENZA
  digitalWrite(I2S_PIN_ENABLE, LOW);  // Disabilita amplificatore
  delay(5);
  pinMode(0, OUTPUT);  // PIN 0 ritorna come output per RGB
  delay(5);

  return success;

  #endif
  return false;
}

// Funzione per concatenare più file MP3 in un unico file temporaneo
bool concatenateMP3Files(const String files[], int count, const char* outputFile) {
  #ifdef AUDIO

  // Concatenazione veloce senza log dettagliati

  // Rimuovi il file di output se esiste già
  String outputPath = "/";
  outputPath += outputFile;
  if (SPIFFS.exists(outputPath)) {
    SPIFFS.remove(outputPath);
  }

  // Apri il file di output in modalità scrittura
  File outFile = SPIFFS.open(outputPath, FILE_WRITE);
  if (!outFile) {
    Serial.println("Errore apertura file output: " + outputPath);
    return false;
  }

  // Buffer per la copia dei dati (4KB per velocizzare la concatenazione)
  uint8_t buffer[4096];
  size_t totalWritten = 0;

  // Concatena tutti i file
  for (int i = 0; i < count; i++) {
    String filePath = "/";
    filePath += files[i];

    // Verifica se il file esiste
    if (!SPIFFS.exists(filePath)) {
      continue;
    }

    // Apri il file sorgente
    File inFile = SPIFFS.open(filePath, FILE_READ);
    if (!inFile) {
      continue;
    }

    // Copia il contenuto del file nel file di output (SENZA log per velocizzare)
    size_t fileSize = inFile.size();
    size_t fileWritten = 0;

    while (inFile.available()) {
      size_t bytesRead = inFile.read(buffer, sizeof(buffer));
      outFile.write(buffer, bytesRead);
      fileWritten += bytesRead;
      totalWritten += bytesRead;

      // Yield solo ogni 8KB per ridurre overhead
      if (fileWritten % 8192 == 0) {
        yield();
      }
    }

    inFile.close();
  }

  outFile.close();

  Serial.printf("File pronto: %d bytes\n", totalWritten);

  return totalWritten > 0;

  #endif
  return false;
}

// Funzione per annunciare l'ora usando file MP3 locali
bool announceTimeLocal() {
  #ifdef AUDIO

  // Array per contenere i nomi dei file da riprodurre
  String audioFiles[15];
  int fileCount = 0;

  // Gestione casi speciali
  if (currentHour == 0 || currentHour == 24) {
    // Mezzanotte
    audioFiles[fileCount++] = "mezzanotte.mp3";
  } else {
    // Ora normale: "Sono le..."
    audioFiles[fileCount++] = "sonole.mp3";
    // Usa formato 24h (NO conversione a 12h)
    audioFiles[fileCount++] = String(currentHour) + ".mp3";
  }

  // Aggiungi minuti se > 0
  if (currentMinute > 0) {
    audioFiles[fileCount++] = "e.mp3";
    audioFiles[fileCount++] = String(currentMinute) + ".mp3";
    audioFiles[fileCount++] = "minuti.mp3";
  }

  // Nome del file temporaneo concatenato
  const char* tempConcatenated = "announce_temp.mp3";

  // Concatena tutti i file in uno unico
  if (!concatenateMP3Files(audioFiles, fileCount, tempConcatenated)) {
    return false;
  }

  // ABILITA AMPLIFICATORE E DISABILITA PIN 0 (RGB) prima della riproduzione
  pinMode(0, INPUT);  // PIN 0 in alta impedenza per evitare conflitto
  delay(5);
  digitalWrite(I2S_PIN_ENABLE, HIGH);  // Abilita amplificatore (PIN 9)
  delay(10);

  // Riproduci il file concatenato
  bool result = playLocalMP3(tempConcatenated);

  // DISABILITA AMPLIFICATORE E RIPRISTINA PIN 0 dopo la riproduzione
  digitalWrite(I2S_PIN_ENABLE, LOW);  // Disabilita amplificatore
  delay(5);
  pinMode(0, OUTPUT);  // PIN 0 ritorna come output per RGB
  delay(5);

  // Rimuovi il file temporaneo dopo la riproduzione
  String tempPath = "/";
  tempPath += tempConcatenated;
  if (SPIFFS.exists(tempPath)) {
    SPIFFS.remove(tempPath);
  }

  return result;

  #endif
  return false;
}

// Funzione migliorata per annunciare l'ora corrente tramite sintesi vocale (TTS).
bool announceTimeFixed() {
  #ifdef AUDIO // Blocco di codice compilato solo se la macro AUDIO è definita
  // Evita annunci troppo frequenti controllando l'intervallo di tempo dall'ultimo annuncio.
  unsigned long currentTime = millis();
  if (currentTime - lastAnnounceTime < 10000) { // Se sono passati meno di 10 secondi.
    Serial.println("Annuncio orario ignorato - troppo recente");
    return false; // Esce dalla funzione senza annunciare.
  }

  Serial.println("Funzione annuncio ora migliorata");
  lastAnnounceTime = currentTime; // Aggiorna il timestamp dell'ultimo annuncio.

  // 1. Emette un breve tono per indicare l'inizio dell'annuncio (feedback uditivo).
//  playTone(440, 200);
//  delay(100);

  // 2. Annuncio con file MP3 locali (SOLO LOCALE - NO WEB)
  Serial.println("Annuncio ora con file MP3 locali");
  bool result = announceTimeLocal();

  if (result) {
    Serial.println("Annuncio completato con successo");

    // CLEAR SCREEN dopo audio completato
    gfx->fillScreen(BLACK);
    Serial.println("Schermo pulito dopo audio");

    return true;
  }

  // 3. Fallback con toni audio di emergenza
  Serial.println("Audio locale fallito - uso toni di emergenza");

  int hourTones = (currentHour == 0 || currentHour == 12) ? 12 : currentHour % 12;
  for (int i = 0; i < hourTones; i++) {
    playTone(880, 150);
    delay(150);
  }

  // CLEAR SCREEN anche dopo fallback
  gfx->fillScreen(BLACK);

  return false;
#endif // Chiusura del blocco #ifdef AUDIO
}
