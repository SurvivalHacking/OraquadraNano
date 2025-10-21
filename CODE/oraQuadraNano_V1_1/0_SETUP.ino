// ================== IMPLEMENTAZIONE FUNZIONI DI SETUP ==================
void setup_eeprom() {
  EEPROM.begin(EEPROM_SIZE); // Inizializza la libreria EEPROM con la dimensione definita.

  // Verifica se la EEPROM è già stata configurata in precedenza.
  if (EEPROM.read(0) != EEPROM_CONFIGURED_MARKER) {
    // Prima configurazione: esegue solo se il marker non è presente.
    EEPROM.write(0, EEPROM_CONFIGURED_MARKER);     // Scrive un marker per indicare che la EEPROM è stata configurata.
    EEPROM.write(EEPROM_PRESET_ADDR, 0);          // Imposta il preset predefinito a 0.
    EEPROM.write(EEPROM_MODE_ADDR, MODE_FAST);     // Imposta la modalità di visualizzazione predefinita a FAST.
    EEPROM.write(EEPROM_WORD_E_STATE_ADDR, 1);    // Imposta lo stato predefinito della parola "E" a visibile e fissa.
    // Imposta i colori predefiniti a bianco (R=255, G=255, B=255).
    EEPROM.write(EEPROM_COLOR_R_ADDR, 255);
    EEPROM.write(EEPROM_COLOR_G_ADDR, 255);
    EEPROM.write(EEPROM_COLOR_B_ADDR, 255);
    EEPROM.commit(); // Scrive i dati dalla cache della EEPROM alla memoria fisica.
  }

  // Carica il preset salvato dalla EEPROM.
  currentPreset = EEPROM.read(EEPROM_PRESET_ADDR);

  // Carica la modalità di visualizzazione salvata dalla EEPROM e la converte al tipo DisplayMode.
  userMode = (DisplayMode)EEPROM.read(EEPROM_MODE_ADDR);


  // Carica lo stato della parola "E" dalla EEPROM.
  word_E_state = EEPROM.read(EEPROM_WORD_E_STATE_ADDR);
  if (word_E_state > 1) word_E_state = 1;  // Imposta un valore di sicurezza a 1 se il valore letto è fuori range (0 o 1).

  // Carica i valori dei colori salvati dalla EEPROM.
  userColor.r = EEPROM.read(EEPROM_COLOR_R_ADDR);
  userColor.g = EEPROM.read(EEPROM_COLOR_G_ADDR);
  userColor.b = EEPROM.read(EEPROM_COLOR_B_ADDR);
  loadSavedSettings(); // Chiama una funzione per caricare altre impostazioni salvate (se presenti).
}

void setup_display() {
  Serial.println("Setup Display"); // Stampa un messaggio sulla seriale.

  // Configurazione del pin di backlight con controllo PWM (Pulse Width Modulation).
  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION); // Imposta il canale PWM, la frequenza e la risoluzione.
  ledcAttachPin(GFX_BL, PWM_CHANNEL);              // Associa il pin della backlight al canale PWM.

  gfx->begin(6600000);
  //gfx->begin(1000000);

  gfx->fillScreen(BLACK); // Riempi lo schermo di nero all'inizio.

  // Mostra il LOGO all'avvio.
  uint16_t* buffer = (uint16_t*)malloc(480 * 480 * 2);  // Alloca memoria RAM temporanea per l'immagine (2 byte per pixel).
  memcpy_P(buffer, sh_logo_480x480, 480 * 480 * 2);     // Copia i dati del logo dalla memoria flash (PROGMEM) alla RAM.
  gfx->draw16bitRGBBitmap(0, 0, buffer, 480, 480);      // Disegna l'immagine del logo sullo schermo.
  free(buffer);                                         // Libera la memoria RAM allocata per il buffer.
  gfx->setFont(u8g2_font_helvB08_tr);                 // Imposta un font per il testo.
  gfx->setCursor(10, 470);                             // Imposta la posizione del cursore per il testo.
  gfx->print("Versione Firmware: ");                   // Stampa l'etichetta.
  gfx->println(ino);                                   // Stampa la versione del firmware.

  backLightPwmFadeIn(); // Effettua un fade-in della backlight.
  delay(1000);
  backLightPwmFadeOut(); // Effettua un fade-out della backlight.
  gfx->fillScreen(BLACK); // Riempi nuovamente lo schermo di nero.

  gfx->setTextColor(WHITE);             // Imposta il colore del testo a bianco.
  gfx->setFont(u8g2_font_maniac_te);    // Imposta un font specifico.
  gfx->setCursor(120, 200);            // Imposta la posizione del cursore.
  gfx->println(F("ORAQUADRA NANO"));   // Stampa il nome del prodotto.
  gfx->setCursor(220, 270);            // Imposta un'altra posizione del cursore.
  gfx->println(F("BY"));               // Stampa "BY".

  backLightPwmFadeIn(); // Altro fade-in della backlight.
  delay(1000);
  backLightPwmFadeOut(); // Altro fade-out della backlight.
  gfx->fillScreen(BLACK);

  int duty = 0;
  // Esegue un'animazione di introduzione finché updateIntro() restituisce vero.
  while (!updateIntro()) {
    if(duty <= 250){
      ledcWrite(PWM_CHANNEL, duty); // Imposta il duty cycle della PWM per la backlight (aumento graduale).
      duty=duty+10;
    }
    delay(20);
  }
  delay(1000);
  backLightPwmFadeOut();
  gfx->fillScreen(BLACK);
  delay(100);


  ledcWrite(PWM_CHANNEL, 250); // Imposta la backlight a un livello fisso (250).
  gfx->setFont(u8g2_font_crox5hb_tr); // Imposta un font diverso.
  delay(100);
}

void backLightPwmFadeIn() {
  // Aumenta gradualmente la luminosità della backlight usando la PWM.
  for (int duty = 0; duty <= 250; duty++) {
    ledcWrite(PWM_CHANNEL, duty); // Imposta il duty cycle della PWM.
    delay(10);
  }
}

void backLightPwmFadeOut() {
  // Diminuisce gradualmente la luminosità della backlight usando la PWM.
  for (int duty = 250 ; duty >= 0; duty--) {
    ledcWrite(PWM_CHANNEL, duty); // Imposta il duty cycle della PWM.
    delay(10);
  }
}

void setup_touch() {
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN); // Inizializza la comunicazione I2C.
  ts.begin();                         // Inizializza il controller del touch screen.
  ts.setRotation(TOUCH_ROTATION);     // Imposta la rotazione del touch screen in base alla configurazione.
}


void setup_wifi() {

  // Imposta la modalità WiFi su STATION (necessario per Espalexa)
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false); // Disabilita risparmio energetico WiFi per Espalexa
  WiFi.disconnect();
  delay(100);

  displayWifiInit(); // Inizializza la visualizzazione per mostrare informazioni sul WiFi.

  gfx->setTextColor(WHITE);
  displayWordWifi(16, "CONFIGURAZIONE"); // Mostra "CONFIGURAZIONE".
  displayWordWifi(32, "WIFI:");         // Mostra "WIFI:".
  delay(250);

  // Verifica se ci sono credenziali WiFi salvate nella EEPROM.
  bool hasCredentials = false;
  char ssid[33] = {0};  // Buffer per memorizzare l'SSID (max 32 caratteri + terminatore null).
  char pass[65] = {0};  // Buffer per memorizzare la password (max 64 caratteri + terminatore null).

  // Legge un marker dalla EEPROM per verificare se le credenziali WiFi sono valide.
  if (EEPROM.read(EEPROM_WIFI_VALID_ADDR) == EEPROM_WIFI_VALID_VALUE) {
    // Se il marker è valido, leggi l'SSID e la password dalla EEPROM.
    for (int i = 0; i < 32; i++) {
      ssid[i] = EEPROM.read(EEPROM_WIFI_SSID_ADDR + i);
    }
    ssid[32] = 0; // Assicura che la stringa SSID sia terminata correttamente.

    for (int i = 0; i < 64; i++) {
      pass[i] = EEPROM.read(EEPROM_WIFI_PASS_ADDR + i);
    }
    pass[64] = 0; // Assicura che la stringa della password sia terminata correttamente.

    // Se l'SSID letto dalla EEPROM non è vuoto, prova a connettersi alla rete WiFi.
    if (strlen(ssid) > 0) {
      hasCredentials = true;
      gfx->setTextColor(GREEN);
      displayWordWifi(64, "CONNESSIONE A:"); // Mostra "CONNESSIONE A:".
      displayWordWifi(80, ssid);           // Mostra l'SSID.
      delay(500);

      // Tenta di connettersi alla rete WiFi con le credenziali salvate.
      WiFi.begin(ssid, pass);

      int retries = 0;

      // Attende la connessione o un timeout (60 tentativi * 250ms = 15 secondi).
      while (WiFi.status() != WL_CONNECTED && retries < 60) {
        delay(250);
        displayWifiDot(retries); // Mostra un punto animato durante il tentativo di connessione.
        retries++;

        // Se i tentativi superano un certo limite (30), riavvia il dispositivo.
        if (retries >= 30) {
          // Prima di riavviare, mostra un messaggio di errore.
          displayWifiInit();
          gfx->setTextColor(RED);
          displayWordWifi(96, "NON CONNESSO!");   // Mostra "NON CONNESSO!".
          displayWordWifi(128, "RIAVVIO IN CORSO"); // Mostra "RIAVVIO IN CORSO".
          delay(2000);
          ESP.restart();
         }

      }

      // Se la connessione WiFi ha successo, mostra un messaggio e esce dalla funzione.
      if (WiFi.status() == WL_CONNECTED) {
        // Assicurati che la modalità sia STATION
        WiFi.mode(WIFI_STA);

        gfx->setTextColor(BLUE);
        displayWordWifi(128, "CONNESSO!"); // Mostra "CONNESSO!".
        displayWordWifi(144, "IP:");       // Mostra "IP:".
        displayWordWifi(160, WiFi.localIP().toString()); // Mostra l'indirizzo IP.
        delay(2000);
        gfx->fillScreen(BLACK); // Pulisce lo schermo.
        return;
      }
    }
  }

  // Se si arriva qui, non ci sono credenziali salvate o la connessione è fallita.
  // Crea un'istanza della libreria WiFiManager per gestire la configurazione.
  WiFiManager wifiManager;

  // Imposta il timeout del portale di configurazione (Access Point) a 3 minuti (180 secondi).
  wifiManager.setConfigPortalTimeout(180);

  // Imposta una callback da eseguire quando le credenziali WiFi vengono salvate tramite il portale.
  wifiManager.setSaveConfigCallback([]() {
    // Questa lambda function viene chiamata dopo il salvataggio delle credenziali.
    String current_ssid = WiFi.SSID(); // Ottiene l'SSID a cui si è connesso.
    String current_pass = WiFi.psk();  // Ottiene la password della rete WiFi.

    // Salva l'SSID nella EEPROM.
    for (size_t i = 0; i < 32; i++) {
      EEPROM.write(EEPROM_WIFI_SSID_ADDR + i, (i < current_ssid.length()) ? current_ssid[i] : 0);
    }

    // Salva la password nella EEPROM.
    for (size_t i = 0; i < 64; i++) {
      EEPROM.write(EEPROM_WIFI_PASS_ADDR + i, (i < current_pass.length()) ? current_pass[i] : 0);
    }

    // Imposta il flag di validità delle credenziali WiFi nella EEPROM.
    EEPROM.write(EEPROM_WIFI_VALID_ADDR, EEPROM_WIFI_VALID_VALUE);

    // Scrive i dati dalla cache della EEPROM alla memoria fisica.
    EEPROM.commit();
  });

  // Genera un nome per l'Access Point (AP) basato sull'indirizzo MAC dell'ESP32.
  String apName = "OraQuadra_" + String((uint32_t)(ESP.getEfuseMac() & 0xFFFFFF), HEX);

  // Disabilita la configurazione IP statica per forzare l'uso di DHCP.
  wifiManager.setSTAStaticIPConfig(IPAddress(0, 0, 0, 0), IPAddress(0, 0, 0, 0), IPAddress(0, 0, 0, 0));

  // Mostra QR code sul display per configurazione WiFi
  displayWiFiQRCode(apName);
  delay(1000);  // Mostra per 1 secondo prima di avviare WiFiManager

  // Avvia il portale di configurazione automatico. Tenta di connettersi a reti salvate,
  // altrimenti avvia un AP per la configurazione tramite browser.
  bool connected = wifiManager.autoConnect(apName.c_str());

  // Se la connessione fallisce dopo il timeout del portale.
  if (!connected) {
    gfx->setTextColor(RED);
    gfx->setCursor(120, 300);
    gfx->println(F("Errore connessione")); // Mostra "Errore connessione".
    gfx->setCursor(120, 330);
    gfx->println(F("Riavvio fra 5 secondi...")); // Mostra "Riavvio fra 5 secondi...".
    delay(5000);
    ESP.restart(); // Riavvia il dispositivo.
    return;
  }

  // Se si arriva qui, la connessione WiFi è stata stabilita.
  // Assicurati che la modalità sia STATION (WiFiManager può lasciarla in AP_STA)
  WiFi.mode(WIFI_STA);

  // Salva le credenziali correnti nella EEPROM (backup nel caso la callback non fosse chiamata).
  String current_ssid = WiFi.SSID();
  String current_pass = WiFi.psk();

  // Salva l'SSID nella EEPROM.
  for (size_t i = 0; i < 32; i++) {
    EEPROM.write(EEPROM_WIFI_SSID_ADDR + i, (i < current_ssid.length()) ? current_ssid[i] : 0);
  }

  // Salva la password nella EEPROM.
  for (size_t i = 0; i < 64; i++) {
    EEPROM.write(EEPROM_WIFI_PASS_ADDR + i, (i < current_pass.length()) ? current_pass[i] : 0);
  }

  // Imposta il flag di validità delle credenziali WiFi.
  EEPROM.write(EEPROM_WIFI_VALID_ADDR, EEPROM_WIFI_VALID_VALUE);

  // Scrive i dati nella EEPROM.
  EEPROM.commit();

  displayWifiInit(); // Reinizializza la visualizzazione WiFi.
  gfx->setTextColor(WHITE);
  displayWordWifi(16, "CONFIGURAZIONE"); // Mostra "CONFIGURAZIONE".
  displayWordWifi(32, "WIFI:");         // Mostra "WIFI:".
  gfx->setTextColor(GREEN);
  displayWordWifi(64, "CONNESSO A:"); // Mostra "CONNESSO A:".
  displayWordWifi(80, WiFi.SSID());   // Mostra l'SSID della rete connessa.
  gfx->setTextColor(BLUE);
  displayWordWifi(144, "IP:");       // Mostra "IP:".
  displayWordWifi(160, WiFi.localIP().toString()); // Mostra l'indirizzo IP assegnato.
  delay(2000);
  gfx->fillScreen(BLACK); // Pulisce lo schermo.
}

void setup_OTA() {
  // Configura gli aggiornamenti Over-The-Air (OTA) se la connessione WiFi è attiva.
  if (WiFi.status() == WL_CONNECTED) {
    ArduinoOTA.setHostname("ORAQUADRA"); // Imposta l'hostname per l'OTA.

    // Funzione da eseguire all'inizio dell'aggiornamento OTA.
    ArduinoOTA.onStart([]() {
      gfx->fillScreen(BLACK);
      gfx->setTextColor(WHITE);
      gfx->setFont(u8g2_font_inb21_mr);
      gfx->setCursor(120, 180);
      gfx->print(F("OTA UPDATE")); // Mostra "OTA UPDATE".
      gfx->drawRect(120, 240, 240, 30, WHITE); // Disegna un rettangolo per la barra di progresso.
    });

    // Funzione da eseguire durante l'aggiornamento OTA per mostrare la progressione.
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
      uint8_t percentComplete = (progress / (total / 100)); // Calcola la percentuale di completamento.
      gfx->fillRect(122, 242, percentComplete * 236 / 100, 26, BLUE); // Riempi la barra di progresso.
      gfx->fillRect(350, 200, 320, 30, BLACK); // Cancella il testo della percentuale precedente.
      gfx->setCursor(350, 230);
      gfx->print(percentComplete); // Stampa la percentuale.
      gfx->print("%");
    });

    // Funzione da eseguire al termine dell'aggiornamento OTA.
    ArduinoOTA.onEnd([]() {
      gfx->fillScreen(GREEN);
      gfx->setTextColor(BLACK);
      gfx->setCursor(120, 200);
      gfx->print(F("UPDATE COMPLETATO")); // Mostra "UPDATE COMPLETATO".
      delay(2000);
    });

    // Funzione da eseguire in caso di errore durante l'aggiornamento OTA.
    ArduinoOTA.onError([](ota_error_t error) {
      gfx->fillScreen(RED);
      gfx->setTextColor(WHITE);
      gfx->setCursor(120, 180);
      gfx->print(F("ERRORE OTA")); // Mostra "ERRORE OTA".
      delay(3000);
      ESP.restart(); // Riavvia il dispositivo in caso di errore.
    });

    ArduinoOTA.begin(); // Inizializza il servizio OTA.
  }
}

void setup_alexa() {
  // Configura l'integrazione con Amazon Alexa se la connessione WiFi è attiva.
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Inizializzazione Espalexa...");

    // Inizializza mDNS (necessario per Espalexa)
    if (!MDNS.begin("oraquadra")) {
      Serial.println("Errore inizializzazione mDNS!");
      return; // Non proseguire se mDNS fallisce
    } else {
      Serial.println("mDNS inizializzato su oraquadra.local");
    }

    // Piccolo delay per stabilizzare mDNS
    delay(100);

    // Aggiunge un dispositivo Alexa con il nome specificato e la callback per il cambio colore.
    // Specifica il tipo come "extendedcolor" per supportare RGB
    EspalexaDevice* alexaDevice = new EspalexaDevice("ORAQUADRANANO", colorLightChanged, EspalexaDeviceType::extendedcolor);
    espalexa.addDevice(alexaDevice);

    // Inizializza il servizio ESPalexa (server web sulla porta 80)
    espalexa.begin();

    Serial.println("Espalexa inizializzato - Dispositivo: ORAQUADRANANO");
    Serial.print("IP dispositivo: ");
    Serial.println(WiFi.localIP());
    Serial.println("Pronto per discovery Alexa!");
  } else {
    Serial.println("WiFi non connesso - Espalexa disabilitato");
  }
}

void resetWiFiSettings() {
  // Funzione per resettare le impostazioni WiFi salvate.
  // Mostra un messaggio sullo schermo.
  gfx->fillScreen(RED);
  gfx->setTextColor(WHITE);
  gfx->setFont(u8g2_font_inb21_mr);
  gfx->setCursor(100, 180);
  gfx->println(F("RESET WIFI")); // Mostra "RESET WIFI".
  gfx->setCursor(100, 220);
  gfx->println(F("IN CORSO...")); // Mostra "IN CORSO...".

  // Crea un'istanza della libreria WiFiManager.
  WiFiManager wm;
  wm.resetSettings(); // Resetta le impostazioni WiFi memorizzate da WiFiManager.

  // Resetta anche il flag di validità delle credenziali WiFi nella EEPROM.
  EEPROM.write(EEPROM_WIFI_VALID_ADDR, 0);

  // Opzionalmente, cancella anche i dati di SSID e password dalla EEPROM.
  for (size_t i = 0; i < 32; i++) {
    EEPROM.write(EEPROM_WIFI_SSID_ADDR + i, 0);
  }

  for (size_t i = 0; i < 64; i++) {
    EEPROM.write(EEPROM_WIFI_PASS_ADDR + i, 0);
  }

  // Salva le modifiche nella EEPROM.
  EEPROM.commit();

  delay(2000);
  gfx->fillScreen(GREEN);
  gfx->setTextColor(BLACK);
  gfx->setCursor(100, 200);
  gfx->println(F("RESET COMPLETATO")); // Mostra "RESET COMPLETATO".
  delay(1000);

  ESP.restart(); // Riavvia il dispositivo dopo il reset del WiFi.
}

void loadSavedSettings() {
  // Funzione per caricare le impostazioni salvate dalla EEPROM.
  // Verifica se la EEPROM è stata configurata in precedenza.
  uint8_t configMarker = EEPROM.read(0);

  if (configMarker != EEPROM_CONFIGURED_MARKER) {
    // EEPROM non configurata, imposta i valori predefiniti.
    Serial.println("EEPROM non configurata, inizializzazione in corso...");

    // Valori predefiniti.
    currentMode = MODE_FAST;
    userMode = currentMode;
    currentColor = Color(255, 255, 255);  // Bianco come colore predefinito.

    // Salva i valori predefiniti nella EEPROM.
    EEPROM.write(0, EEPROM_CONFIGURED_MARKER);
    EEPROM.write(EEPROM_PRESET_ADDR, 0);  // Preset 0 (random).
    EEPROM.write(EEPROM_MODE_ADDR, currentMode);
    EEPROM.write(EEPROM_COLOR_R_ADDR, 255);
    EEPROM.write(EEPROM_COLOR_G_ADDR, 255);
    EEPROM.write(EEPROM_COLOR_B_ADDR, 255);
    EEPROM.commit();

    return;
  }

  // Carica la modalità di visualizzazione salvata dalla EEPROM.
  uint8_t savedMode = EEPROM.read(EEPROM_MODE_ADDR);
  if (savedMode < NUM_MODES) {
    currentMode = (DisplayMode)savedMode;
    userMode = currentMode;
  } else {
    // Imposta la modalità predefinita se il valore letto non è valido.
    currentMode = MODE_FAST;
    userMode = currentMode;
  }

  // Carica il preset salvato dalla EEPROM (solo per informazione).
  uint8_t savedPreset = EEPROM.read(EEPROM_PRESET_ADDR);

  // Carica i valori dei colori salvati dalla EEPROM.
  uint8_t r = EEPROM.read(EEPROM_COLOR_R_ADDR);
  uint8_t g = EEPROM.read(EEPROM_COLOR_G_ADDR);
  uint8_t b = EEPROM.read(EEPROM_COLOR_B_ADDR);
  currentColor = Color(r, g, b);
  userColor = currentColor;  // Aggiorna anche la variabile userColor.

  // Informazioni di debug (opzionale).
  Serial.println("Impostazioni caricate da EEPROM:");
  Serial.print("Modalità: "); Serial.println(currentMode);
  Serial.print("Preset: "); Serial.println(savedPreset);
  Serial.print("Colore: R="); Serial.print(r);
  Serial.print(" G="); Serial.print(g);
  Serial.print(" B="); Serial.println(b);
}


void resetWiFi() {
  // Alias per la funzione resetWiFiSettings().
  resetWiFiSettings();
}

void colorLightChanged(EspalexaDevice* device) {
  // Callback per Amazon Alexa - NON BLOCCANTE
  if (device == nullptr) return;

  uint8_t brightness = device->getValue();

  Serial.printf("Alexa callback - Brightness: %d\n", brightness);

  if (brightness == 0) {
    alexaOff = 1; // Imposta un flag per indicare che Alexa ha spento la luce.
    alexaUpdatePending = true; // Segnala che serve un aggiornamento
    Serial.println("Alexa: dispositivo SPENTO");
    return;
  } else {
    alexaOff = 0; // Resetta il flag se la luce è accesa.
  }

  // Estrae i valori RGB dal dispositivo Espalexa
  userColor.r = device->getR();
  userColor.g = device->getG();
  userColor.b = device->getB();

  currentColor = userColor; // Aggiorna anche il colore corrente

  // NON chiamare forceDisplayUpdate() qui! Usa un flag invece
  alexaUpdatePending = true; // Segnala che serve un aggiornamento

  Serial.printf("Alexa color: R=%d G=%d B=%d\n", userColor.r, userColor.g, userColor.b);
}

void displayWiFiQRCode(String ssid) {
  Serial.println("Generazione QR Code WiFi...");

  // Prepara stringa QR WiFi (formato standard)
  String qrData = "WIFI:T:nopass;S:" + ssid + ";P:;;";

  // Crea QR code (versione 5 = 37x37 moduli) - allocazione dinamica per evitare stack overflow
  QRCode qrcode;
  uint8_t *qrcodeData = (uint8_t *)malloc(qrcode_getBufferSize(5));
  if (qrcodeData == NULL) {
    Serial.println("Errore allocazione memoria QR code");
    return;
  }
  qrcode_initText(&qrcode, qrcodeData, 5, 0, qrData.c_str());  // 0 = ECC_LOW

  // Layout per display 480x480
  int scale = 7;  // Ogni modulo QR = 7x7 pixel (37*7 = 259px)
  int qrSize = qrcode.size * scale;
  int offsetX = (480 - qrSize) / 2;  // Centro orizzontale
  int offsetY = 120;  // Margine superiore maggiore per dare spazio al testo

  // Sfondo nero
  gfx->fillScreen(BLACK);

  // Testo SOPRA il QR code - MINIMALE
  gfx->setTextColor(WHITE);
  gfx->setTextSize(1);

  // "CONFIG WiFi" 
  gfx->setCursor(10, 30);
  gfx->print("Configurazione WiFi");

  // "Scansiona QR code:" 
  gfx->setCursor(10, 50);
  gfx->print("Scansiona QR code:");

  // Box bianco per QR code
  gfx->fillRect(offsetX - 15, offsetY - 15, qrSize + 30, qrSize + 30, WHITE);

  // Disegna QR code
  for (uint8_t y = 0; y < qrcode.size; y++) {
    for (uint8_t x = 0; x < qrcode.size; x++) {
      if (qrcode_getModule(&qrcode, x, y)) {
        gfx->fillRect(
          offsetX + x * scale,
          offsetY + y * scale,
          scale,
          scale,
          BLACK
        );
      }
    }
  }

  // Nome rete sotto QR code 
  gfx->setTextColor(CYAN);
  gfx->setTextSize(1);
  gfx->setCursor(10, offsetY + qrSize + 60);
  gfx->print(ssid);

  // IP alternativo
  gfx->setTextColor(YELLOW);
  gfx->setTextSize(1);
  gfx->setCursor(10, offsetY + qrSize + 90);
  gfx->print("IP: 192.168.4.1");

  // Libera memoria allocata
  free(qrcodeData);

  Serial.println("QR Code generato!");
}

void clearDisplay() {
  // Cancella lo schermo riempiendolo con il colore di sfondo.
  gfx->fillScreen(BLACK);

  // Inizializza il buffer dei LED con il colore di sfondo.
  for (uint16_t idx = 0; idx < NUM_LEDS; idx++) {
    displayBuffer[idx] = convertColor(TextBackColor);
    pixelChanged[idx] = true;

    // Resetta anche gli array di stato dei pixel attivi.
    activePixels[idx] = false;
  }

  // Imposta tutti i caratteri sullo schermo con il colore di sfondo, di fatto "cancellandoli".
  gfx->setFont(u8g2_font_inb21_mr);
  for (uint16_t idx = 0; idx < NUM_LEDS; idx++) {
    gfx->setTextColor(displayBuffer[idx]);
    gfx->setCursor(pgm_read_word(&TFT_X[idx]), pgm_read_word(&TFT_Y[idx]));
    gfx->write(pgm_read_byte(&TFT_L[idx]));
  }
}
