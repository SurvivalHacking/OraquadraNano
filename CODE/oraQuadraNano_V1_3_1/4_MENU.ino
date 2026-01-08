bool checkSetupScroll() {
  // Dichiarazione di variabili statiche per mantenere lo stato tra le chiamate alla funzione.
  static int16_t lastY = -1;      // Memorizza l'ultima coordinata Y del tocco per calcolare lo spostamento. Inizializzato a -1 per indicare che non c'è stato un tocco precedente.
  static uint32_t lastScrollTime = 0; // Memorizza il timestamp dell'ultimo evento di scroll per evitare scroll troppo rapidi.

  try {
    // Verifica se lo schermo non è attualmente toccato.
    if (!ts.isTouched) {
      setupScrollActive = false; // Resetta il flag di scroll attivo.
      lastY = -1;              // Resetta l'ultima coordinata Y.
      return false;            // Indica che non c'è stato uno scroll.
    }

    // Ottiene la coordinata Y del primo punto di contatto. Mappa il valore grezzo del touch screen all'intervallo delle coordinate del display (0-479).
    int y = map(ts.points[0].y, TOUCH_MAP_Y1, TOUCH_MAP_Y2, 0, 480 - 1);
    uint32_t currentTime = millis(); // Ottiene il tempo attuale in millisecondi.

    // Gestisce la prima inizializzazione della coordinata Y al primo tocco.
    if (lastY == -1) {
      lastY = y;               // Memorizza la coordinata Y corrente come l'ultima.
      lastScrollTime = currentTime; // Memorizza il tempo corrente come l'ultimo tempo di scroll.
      return false;            // Non c'è ancora uno spostamento sufficiente per uno scroll.
    }

    // Previene lo scrolling troppo rapido ignorando i movimenti avvenuti troppo di recente.
    if (currentTime - lastScrollTime < 200) {
      return false; // Ignora questo movimento.
    }

    // Calcola la differenza tra la coordinata Y corrente e l'ultima coordinata Y.
    int deltaY = y - lastY;

    // Verifica se lo spostamento verticale è sufficiente per essere considerato uno scroll.
    if (abs(deltaY) > SETUP_SCROLL_THRESHOLD) {
      // Se il dito si muove verso il basso (deltaY positivo) e non siamo all'inizio della lista (setupCurrentScroll > 0).
      if (deltaY > 0 && setupCurrentScroll > 0) {
        setupCurrentScroll--; // Decrementa l'indice di scroll per mostrare gli elementi precedenti.
        updateSetupPage();    // Aggiorna la visualizzazione della pagina di setup.
        lastScrollTime = currentTime; // Aggiorna il tempo dell'ultimo scroll.
        lastY = y;              // Aggiorna l'ultima coordinata Y.
        return true;            // Indica che c'è stato uno scroll.
      }
      // Se il dito si muove verso l'alto (deltaY negativo) e non siamo alla fine della lista.
      else if (deltaY < 0 && setupCurrentScroll < SETUP_ITEMS_COUNT - 5) { // Considera che al massimo 5 elementi sono visibili contemporaneamente.
        setupCurrentScroll++; // Incrementa l'indice di scroll per mostrare gli elementi successivi.
        updateSetupPage();    // Aggiorna la visualizzazione della pagina di setup.
        lastScrollTime = currentTime; // Aggiorna il tempo dell'ultimo scroll.
        lastY = y;              // Aggiorna l'ultima coordinata Y.
        return true;            // Indica che c'è stato uno scroll.
      }
    }

    // Aggiorna l'ultima coordinata Y e il tempo anche se non c'è stato uno scroll, ma solo dopo un certo intervallo per evitare aggiornamenti troppo frequenti.
    if (currentTime - lastScrollTime > 500) {
      lastY = y;
      lastScrollTime = currentTime;
    }

    return false; // Indica che non c'è stato uno scroll in questo ciclo.
  }
  catch (...) {
    // Blocco catch per intercettare eventuali eccezioni e prevenire crash. In caso di errore, restituisce false.
    return false;
  }
}

void handleSetupTouch(int x, int y) {
  // Aggiunge una protezione per evitare che la funzione venga eseguita contemporaneamente più volte, il che potrebbe causare problemi.
  static bool alreadyHandling = false;
  if (alreadyHandling) {
    return; // Se la funzione è già in esecuzione, esce.
  }
  alreadyHandling = true; // Imposta il flag per indicare che la funzione è in esecuzione.

  // Aggiorna il timestamp dell'ultima interazione con la pagina di setup.
  setupLastActivity = millis();

  // Se il tocco è nella parte superiore dello schermo (area del titolo), esce dalla pagina di setup.
  if (y < 100) {
    hideSetupPage();      // Chiama la funzione per nascondere la pagina di setup.
    alreadyHandling = false; // Resetta il flag.
    return;               // Esce dalla funzione.
  }

  // SICUREZZA: Verifica che l'indice di scroll sia un valore non negativo.
  if (setupCurrentScroll < 0) {
    setupCurrentScroll = 0; // Resetta a 0 se per qualche motivo è negativo.
  }

  // Calcola l'indice dell'opzione di menu toccata in base alla posizione Y del tocco e all'offset di scroll.
  int optionIdx = setupCurrentScroll + ((y - 160) / 60); // Ogni opzione ha un'altezza approssimativa di 60 pixel, iniziando da Y=160.

  // Verifica che l'indice dell'opzione rientri nei limiti validi.
  if (optionIdx < 0 || optionIdx >= SETUP_ITEMS_COUNT) {
    alreadyHandling = false; // Resetta il flag.
    return;               // Esce se l'indice non è valido.
  }

  // SICUREZZA: Verifica che l'elemento del menu all'indice calcolato sia valido (etichetta non NULL).
  if (optionIdx >= 0 && optionIdx < SETUP_ITEMS_COUNT) {
    if (setupMenuItems[optionIdx].label == NULL) {
      alreadyHandling = false; // Resetta il flag.
      return;               // Esce se l'elemento del menu non è valido.
    }
  } else {
    alreadyHandling = false; // Resetta il flag.
    return;               // Esce se l'indice non è valido.
  }

  // Se il tocco è nella parte destra dell'opzione (area dell'interruttore o del selettore).
  if (x > 300) {
    // Se l'elemento del menu è un selettore di modalità.
    if (setupMenuItems[optionIdx].isModeSelector) {
      // Verifica che il puntatore al valore della modalità non sia NULL.
      if (setupMenuItems[optionIdx].modeValuePtr != NULL) {
        // SICUREZZA: Legge il valore corrente della modalità e verifica che sia entro i limiti validi.
        uint8_t currentMode = *setupMenuItems[optionIdx].modeValuePtr;
        if (currentMode >= NUM_MODES) {
          currentMode = MODE_FAST; // Resetta a una modalità sicura in caso di valore non valido.
        }

        // Cicla alla modalità successiva. L'operatore modulo (%) assicura che il valore rimanga entro l'intervallo valido.
        uint8_t newMode = (currentMode + 1) % NUM_MODES;
        *setupMenuItems[optionIdx].modeValuePtr = newMode; // Aggiorna il valore della modalità.
      }
    } else {
      // Se l'elemento del menu è un'opzione booleana (ON/OFF).
      // Verifica che il puntatore al valore booleano non sia NULL.
      if (setupMenuItems[optionIdx].valuePtr != NULL) {
        bool newValue = !(*setupMenuItems[optionIdx].valuePtr); // Inverte il valore booleano corrente.
        *setupMenuItems[optionIdx].valuePtr = newValue;      // Aggiorna il valore booleano.
      }
    }

    // Emette un breve suono di feedback al tocco (se abilitato).
    playTouchSound();

    // Aggiunge una breve pausa per dare tempo al sistema di stabilizzarsi prima di aggiornare la visualizzazione.
    delay(50);

    // Aggiorna la visualizzazione della pagina di setup. Inserito in un blocco try-catch per gestire eventuali errori durante l'aggiornamento.
    try {
      updateSetupPage();
    }
    catch (...) {
      // Gestisce silenziosamente eventuali eccezioni durante l'aggiornamento della pagina.
    }
  }

  // Resetta il flag per indicare che la gestione del tocco è completata.
  alreadyHandling = false;
}

void showSetupPage() {
  // Protezione contro chiamate ricorsive (se la funzione viene chiamata mentre è già in esecuzione).
  static bool alreadyInSetupPage = false;
  if (alreadyInSetupPage) {
    return; // Esce se la funzione è già in esecuzione.
  }
  alreadyInSetupPage = true; // Imposta il flag per indicare che la funzione è in esecuzione.

  // Imposta il flag per indicare che la pagina di setup è attiva e registra l'ultimo momento di attività.
  setupPageActive = true;
  setupLastActivity = millis();

  // Pulisce lo schermo riempiendolo di nero.
  gfx->fillScreen(BLACK);

  // Disattiva temporaneamente la visualizzazione di eventuali elementi dell'orologio.
  memset(activePixels, 0, sizeof(activePixels));
  memset(targetPixels, 0, sizeof(targetPixels));

  // Assicura che la struttura del menu di setup sia stata inizializzata.
  // PROTEZIONE: Verifica che l'array setupMenuItems contenga almeno un elemento con un'etichetta non NULL.
  bool menuInitialized = false;
  for (int i = 0; i < SETUP_ITEMS_COUNT; i++) {
    if (setupMenuItems[i].label != NULL) {
      menuInitialized = true;
      break;
    }
  }

  // Se il menu non è stato inizializzato, lo inizializza ora.
  if (!menuInitialized) {
    initSetupMenu();
  }

  // PROTEZIONE: Verifica che la modalità di visualizzazione predefinita sia un valore valido.
  if (setupOptions.defaultDisplayMode >= NUM_MODES) {
    setupOptions.defaultDisplayMode = MODE_FAST; // Imposta un valore sicuro di default.
  }

  // PROTEZIONE: Assicura che l'indice di scroll sia inizializzato a un valore sicuro (l'inizio della lista).
  setupCurrentScroll = 0;

  // Disegna l'interfaccia della pagina di setup. Inserito in un blocco try-catch per gestire eventuali errori di disegno.
  try {
    // Pulisce nuovamente lo schermo (ridondante ma sicuro).
    gfx->fillScreen(BLACK);

    // Disegna il titolo "SETUP" nella parte superiore dello schermo.
    gfx->setFont(u8g2_font_inb21_mr);
    gfx->setTextColor(YELLOW);
    gfx->setCursor(150, 40);
    gfx->println(F("SETUP"));

    // Disegna un breve testo esplicativo per l'interazione.
    gfx->setFont(u8g2_font_crox5hb_tr);
    gfx->setTextColor(WHITE);
    gfx->setCursor(20, 80);
    gfx->println(F("Tocca per cambiare"));

    // Disegna una linea orizzontale per separare il titolo dalle opzioni.
    gfx->drawFastHLine(20, 110, 440, WHITE);

    // PROTEZIONE: Chiama la funzione per aggiornare la visualizzazione delle opzioni del menu, all'interno di un blocco try-catch.
    updateSetupPage();
  }
  catch (...) {
    // Blocco catch per gestire eventuali errori durante il disegno. Visualizza un messaggio di errore sullo schermo.
    gfx->fillScreen(RED);
    gfx->setFont(u8g2_font_inb21_mr);
    gfx->setTextColor(WHITE);
    gfx->setCursor(120, 240);
    gfx->println(F("ERRORE SETUP"));
    delay(2000);

    // In caso di errore, esce dalla pagina di setup e ripristina lo schermo principale.
    setupPageActive = false;
    gfx->fillScreen(BLACK);

    alreadyInSetupPage = false; // Resetta il flag.
    return;                     // Esce dalla funzione.
  }

  // Indica che la pagina di setup è ora visualizzata e la funzione può uscire.
  alreadyInSetupPage = false;
}

void hideSetupPage() {
  // Protezione contro chiamate ricorsive (se la funzione viene chiamata mentre è già in esecuzione).
  static bool isHiding = false;
  if (isHiding) {
    return; // Esce se la funzione è già in esecuzione.
  }
  isHiding = true; // Imposta il flag per indicare che la funzione è in esecuzione.

  try {
    // Salva le opzioni di configurazione correnti nella EEPROM.
    saveSetupOptions();
    //updateCurrentTime(); // Potrebbe essere una funzione per aggiornare l'ora visualizzata dopo aver modificato le impostazioni.

    // Imposta il flag per indicare che la pagina di setup non è più attiva.
    setupPageActive = false;

    // Pulisce completamente lo schermo riempiendolo di nero.
    gfx->fillScreen(BLACK);

    // Resetta le variabili per forzare un aggiornamento completo della visualizzazione dell'orologio.
    lastHour = 255;
    lastMinute = 255;

    // Resetta gli array che tengono traccia dello stato dei pixel.
    memset(activePixels, 0, sizeof(activePixels));
    memset(targetPixels, 0, sizeof(targetPixels));
    memset(pixelChanged, 1, sizeof(pixelChanged)); // Forza un aggiornamento completo al prossimo ciclo.

    // Resetta lo stato di inizializzazione degli effetti speciali in modo che vengano reinizializzati quando la modalità viene riattivata.
    if (currentMode == MODE_MATRIX || currentMode == MODE_MATRIX2) {
      matrixInitialized = false;
    } else if (currentMode == MODE_SNAKE) {
      snakeInitNeeded = true;
    } else if (currentMode == MODE_WATER) {
      waterDropInitNeeded = true;
    }

    // Aggiunge una breve pausa per dare tempo al sistema di stabilizzarsi dopo il cambio di pagina.
    delay(100);
  }
  catch (...) {
    // Blocco catch per gestire eventuali eccezioni durante il processo di chiusura della pagina di setup.
  }

  isHiding = false; // Resetta il flag.
}

void updateSetupPage() {
  // Protezione contro chiamate ricorsive (se la funzione viene chiamata mentre è già in esecuzione).
  static bool isUpdating = false;
  if (isUpdating) {
    return; // Esce se la funzione è già in esecuzione.
  }
  isUpdating = true; // Imposta il flag per indicare che la funzione è in esecuzione.

  // Cancella l'area dello schermo dove vengono visualizzate le opzioni del menu.
  gfx->fillRect(0, 130, 480, 340, BLACK);

  // SICUREZZA: Assicura che l'indice di inizio per la visualizzazione delle opzioni sia non negativo.
  int startIdx = setupCurrentScroll;
  if (startIdx < 0) {
    startIdx = 0;
    setupCurrentScroll = 0;
  }

  // Calcola l'indice finale delle opzioni da visualizzare, assicurandosi di non superare il numero totale di opzioni.
  int endIdx = min(startIdx + 5, SETUP_ITEMS_COUNT); // Mostra al massimo 5 opzioni alla volta.

  // Cicla attraverso le opzioni del menu visibili.
  for (int i = startIdx; i < endIdx; i++) {
    // SICUREZZA: Verifica che l'indice corrente non superi il numero totale di elementi del menu.
    if (i >= SETUP_ITEMS_COUNT) {
      continue; // Salta all'iterazione successiva se l'indice non è valido.
    }

    // SICUREZZA: Verifica che l'etichetta dell'elemento del menu corrente non sia NULL.
    if (setupMenuItems[i].label == NULL) {
      continue; // Salta se l'etichetta è NULL.
    }

    // Disegna l'etichetta dell'opzione sulla sinistra dello schermo.
    gfx->setFont(u8g2_font_crox5hb_tr);
    gfx->setTextColor(WHITE);
    gfx->setCursor(30, 190 + (i - startIdx) * 60); // Posiziona il testo verticalmente in base all'indice visibile.
    gfx->print(setupMenuItems[i].label);
    delay(10); // Breve ritardo per dare tempo al display di aggiornarsi.

    // Visualizza il valore dell'opzione sulla destra.
    if (setupMenuItems[i].isModeSelector) {
      // SICUREZZA: Verifica il range del valore della modalità.
      uint8_t mode = *setupMenuItems[i].modeValuePtr;
      if (mode >= NUM_MODES) {
        mode = MODE_FAST; // Valore di default sicuro.
        *setupMenuItems[i].modeValuePtr = mode; // Aggiorna il valore se fuori range.
      }

      // Mostra la modalità corrente come testo.
      switch (mode) {
        case MODE_FADE: gfx->print("FADE"); break;
        case MODE_SLOW: gfx->print("SLOW"); break;
        case MODE_FAST: gfx->print("FAST"); break;
        case MODE_MATRIX: gfx->print("MATRIX"); break;
        case MODE_MATRIX2: gfx->print("MATRIX2"); break;
        case MODE_SNAKE: gfx->print("SNAKE"); break;
        case MODE_WATER: gfx->print("WATER"); break;
        default: gfx->print("FAST"); break; // Valore di fallback.
      }
    } else {
      // SICUREZZA: Verifica che il puntatore al valore booleano non sia NULL.
      if (setupMenuItems[i].valuePtr == NULL) {
        continue; // Salta se il puntatore è NULL.
      }

      // Per le opzioni booleane (ON/OFF).
      bool currentValue = false;
      try {
        currentValue = *setupMenuItems[i].valuePtr; // Tenta di leggere il valore.
      } catch (...) {
        continue; // Salta in caso di errore di lettura.
      }

      // Visualizza "ON" in verde o "OFF" in rosso a seconda del valore.
      if (currentValue) {
        gfx->setTextColor(GREEN);
        gfx->setCursor(350, 190 + (i - startIdx) * 60);
        gfx->print("ON");
        delay(10);

        // Disegna un interruttore in stato ON (cursore a destra).
        gfx->fillRoundRect(400, 190 + (i - startIdx) * 60 - 20, 50, 30, 15, GREEN);
        gfx->fillCircle(430, 190 + (i - startIdx) * 60 - 5, 15, WHITE);
      } else {
        gfx->setTextColor(RED);
        gfx->setCursor(350, 190 + (i - startIdx) * 60);
        gfx->print("OFF");
        delay(10);

        // Disegna un interruttore in stato OFF (cursore a sinistra).
        gfx->fillRoundRect(400, 190 + (i - startIdx) * 60 - 20, 50, 30, 15, DARKGREY);
        gfx->fillCircle(420, 190 + (i - startIdx) * 60 - 5, 15, WHITE);
      }
    }

    yield(); // Permette al watchdog timer di non resettare il dispositivo durante operazioni lunghe.
  }

  isUpdating = false; // Rilascia il flag per permettere futuri aggiornamenti.
}

void initSetupMenu() {
  try {
    // Inizializza gli elementi del menu con le loro etichette, puntatori alle variabili di configurazione e flag per indicare se sono selettori di modalità.
    setupMenuItems[0] = {"Auto Night Mode", &setupOptions.autoNightModeEnabled, NULL, false};
    setupMenuItems[1] = {"Touch Sounds", &setupOptions.touchSoundsEnabled, NULL, false};
    setupMenuItems[2] = {"Rainbow Seconds", &setupOptions.rainbowSecondsEnabled, NULL, false};
    setupMenuItems[3] = {"Power Save", &setupOptions.powerSaveEnabled, NULL, false};
    setupMenuItems[4] = {"Display Mode", NULL, &setupOptions.defaultDisplayMode, true};

    // Controllo di sicurezza per la modalità di visualizzazione predefinita. Se è fuori range, la reimposta a un valore sicuro.
    if (setupOptions.defaultDisplayMode >= NUM_MODES) {
      setupOptions.defaultDisplayMode = MODE_FAST;
    }
  }
  catch (...) {
    // Blocco catch per gestire eventuali errori durante l'inizializzazione del menu. Tenta di ripristinare con valori predefiniti.
    setupMenuItems[0] = {"Auto Night Mode", &setupOptions.autoNightModeEnabled, NULL, false};
    setupMenuItems[1] = {"Touch Sounds", &setupOptions.touchSoundsEnabled, NULL, false};
    setupMenuItems[2] = {"Rainbow Seconds", &setupOptions.rainbowSecondsEnabled, NULL, false};
    setupMenuItems[3] = {"Power Save", &setupOptions.powerSaveEnabled, NULL, false};
    setupMenuItems[4] = {"Display Mode", NULL, &setupOptions.defaultDisplayMode, true};
  }
}

void initSetupOptions() {
  // Inizializza le opzioni di setup con valori predefiniti.
  setupOptions.autoNightModeEnabled = true;
  setupOptions.touchSoundsEnabled = true;
  setupOptions.powerSaveEnabled = false;
  setupOptions.rainbowSecondsEnabled = true;
  setupOptions.defaultDisplayMode = MODE_FAST;
  setupOptions.wifiEnabled = true;   // Impostato sempre a true in questa implementazione.
  setupOptions.ntpEnabled = true;    // Impostato sempre a true in questa implementazione.
  setupOptions.alexaEnabled = true;  // Impostato sempre a true in questa implementazione.

  // Verifica se la EEPROM è stata precedentemente configurata (tramite un marker).
  if (EEPROM.read(EEPROM_SETUP_OPTIONS_ADDR) == EEPROM_CONFIGURED_MARKER) {
    // Se configurata, carica le impostazioni salvate dalla EEPROM.
    setupOptions.wifiEnabled = true;   // Rileggi (anche se forzato a true).
    setupOptions.ntpEnabled = true;    // Rileggi (anche se forzato a true).
    setupOptions.alexaEnabled = true;  // Rileggi (anche se forzato a true).

    // Carica le opzioni configurabili dall'EEPROM.
    setupOptions.autoNightModeEnabled = EEPROM.read(EEPROM_SETUP_OPTIONS_ADDR + 4);
    setupOptions.touchSoundsEnabled = EEPROM.read(EEPROM_SETUP_OPTIONS_ADDR + 5);
    setupOptions.powerSaveEnabled = EEPROM.read(EEPROM_SETUP_OPTIONS_ADDR + 6);
    setupOptions.defaultDisplayMode = EEPROM.read(EEPROM_SETUP_OPTIONS_ADDR + 7);

    // Carica l'opzione per l'arcobaleno dei secondi (aggiunta in una versione successiva).
    if (EEPROM_SIZE >= 120) { // Verifica che ci sia spazio sufficiente nella EEPROM.
      setupOptions.rainbowSecondsEnabled = EEPROM.read(EEPROM_SETUP_OPTIONS_ADDR + 8);
    }
  } else {
    // Se la EEPROM non è stata configurata, salva i valori predefiniti.
    saveSetupOptions();
  }
}

void saveSetupOptions() {
  try {
    // Scrive un marker nella EEPROM per indicare che la configurazione è stata salvata.
    EEPROM.write(EEPROM_SETUP_OPTIONS_ADDR, EEPROM_CONFIGURED_MARKER);

    // Salva (o meglio, ri-salva, dato che sono forzati a true) le impostazioni relative a WiFi, NTP e Alexa.
    EEPROM.write(EEPROM_SETUP_OPTIONS_ADDR + 1, true); // wifiEnabled
    EEPROM.write(EEPROM_SETUP_OPTIONS_ADDR + 2, true); // ntpEnabled
    EEPROM.write(EEPROM_SETUP_OPTIONS_ADDR + 3, true); // alexaEnabled

    // Salva le opzioni di configurazione effettive nella EEPROM.
    EEPROM.write(EEPROM_SETUP_OPTIONS_ADDR + 4, setupOptions.autoNightModeEnabled);
    EEPROM.write(EEPROM_SETUP_OPTIONS_ADDR + 5, setupOptions.touchSoundsEnabled);
    EEPROM.write(EEPROM_SETUP_OPTIONS_ADDR + 6, setupOptions.powerSaveEnabled);

    // Verifica che la modalità di visualizzazione predefinita sia un valore valido prima di salvarla.
    uint8_t modeToSave = setupOptions.defaultDisplayMode;
    if (modeToSave >= NUM_MODES) {
      modeToSave = MODE_FAST; // Usa un valore sicuro se quello attuale non è valido.
    }
    EEPROM.write(EEPROM_SETUP_OPTIONS_ADDR + 7, modeToSave);

    // Salva l'opzione per l'arcobaleno dei secondi.
    EEPROM.write(EEPROM_SETUP_OPTIONS_ADDR + 8, setupOptions.rainbowSecondsEnabled);

    // Scrive i dati dalla cache della EEPROM alla memoria fisica.
    EEPROM.commit();

    // Applica immediatamente alcune opzioni che hanno un effetto visivo diretto.
    currentMode = (DisplayMode)setupOptions.defaultDisplayMode;
  }
  catch (...) {
    // Blocco catch per gestire eventuali errori durante la scrittura nella EEPROM.
  }
}