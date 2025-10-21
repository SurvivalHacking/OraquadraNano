// ================== GESTIONE TOUCH E PULSANTI ==================

void checkButtons() {
  // Variabili statiche per mantenere lo stato tra le diverse chiamate a questa funzione.
  static bool waitingForRelease = false; // Flag che indica se stiamo aspettando che il tocco venga rilasciato.
  static uint32_t touchStartTime = 0;    // Memorizza il timestamp dell'inizio di un tocco prolungato (es. per il reset WiFi).
  static int16_t scrollStartY = -1;     // Memorizza la coordinata Y iniziale di un potenziale scroll nella pagina di setup.
  static bool checkingSetupScroll = false; // Flag che indica se stiamo attualmente rilevando un gesto di scroll per aprire il setup.
  static uint32_t lastCornerCheck = 0;   // Memorizza il timestamp dell'ultimo controllo degli angoli per il reset WiFi.
  static uint8_t cornerTouchLostCounter = 0; // Contatore per tenere traccia di quanti controlli consecutivi degli angoli non hanno rilevato tutti e quattro.
  static uint8_t touchSampleCounter = 0;   // Contatore per gestire la frequenza di campionamento del touch durante l'antirimbalzo.
  uint32_t currentMillis = millis();      // Ottiene il tempo attuale in millisecondi.


  // ====================== GESTIONE RILASCIO TOUCH ======================

  if(waitingForRelease){
    ts.read(); // Legge lo stato attuale del touch screen.
    if (!ts.isTouched) { // Se il touch screen NON è più toccato (rilascio).
      waitingForRelease = false; // Resetta il flag di attesa del rilascio.

      if (colorCycle.isActive) {      // FINE CICLAGGIO COLORI???
        colorCycle.isActive = false;  // Disattiva il ciclo dei colori.
        userColor = currentColor;     // Memorizza l'ultimo colore visualizzato come colore scelto dall'utente.

        // Salva la modalità corrente, il preset (13 indica colore utente) e il colore RGB nella EEPROM.
        EEPROM.write(EEPROM_MODE_ADDR, currentMode);
        EEPROM.write(EEPROM_PRESET_ADDR, 13);
        EEPROM.write(EEPROM_COLOR_R_ADDR, currentColor.r);
        EEPROM.write(EEPROM_COLOR_G_ADDR, currentColor.g);
        EEPROM.write(EEPROM_COLOR_B_ADDR, currentColor.b);

        EEPROM.commit(); // Scrive i dati dalla cache della EEPROM alla memoria fisica.


        // Emette un breve suono di conferma al rilascio del tocco.
        playTouchSound();
        // Forza un aggiornamento completo del display per mostrare il nuovo colore.
        forceDisplayUpdate();
      }
      return; // Esce dalla funzione dopo aver gestito il rilascio.
    }
  }


  // ====================== ANTIRIMBALZO GLOBALE ======================
  static uint32_t lastTouchCheckTime = 0; // Memorizza l'ultimo timestamp in cui è stato controllato il touch.
  touchSampleCounter++; // Incrementa il contatore di campionamento del touch.

  // Implementa un antirimbalzo riducendo la frequenza di polling del touch screen.
  if (currentMillis - lastTouchCheckTime < 50) {
    // Riduce la frequenza di polling ma non la salta completamente.
    if (touchSampleCounter % 3 != 0) {  // Esegue una lettura periodica (ogni 3 campioni) anche durante l'antirimbalzo.
      if (resetCountdownStarted && currentMillis - lastCornerCheck > 200) {
        lastCornerCheck = currentMillis;
        // Se è iniziato il countdown per il reset WiFi, continua a controllare gli angoli.
        ts.read();
      } else {
        return; // Se non è in corso il reset WiFi, esce dalla funzione per evitare letture troppo frequenti.
      }
    }
  }
  lastTouchCheckTime = currentMillis; // Aggiorna il timestamp dell'ultimo controllo del touch.

  // ====================== LETTURA TOUCH ======================
  uint8_t readAttempts = 0;   // Contatore per il numero di tentativi di lettura del touch.
  bool touchDetected = false; // Flag per indicare se un tocco è stato rilevato.

  // Tenta di leggere lo stato del touch screen fino a 3 volte se non viene rilevato un tocco.
  while (readAttempts < 3 && !touchDetected) {
    ts.read(); // Legge lo stato del touch screen.
    if (ts.isTouched) {
      touchDetected = true; // Imposta il flag se viene rilevato un tocco.
    } else {
      readAttempts++;     // Incrementa il contatore dei tentativi.
      delay(10);          // Piccolo ritardo tra un tentativo e l'altro.
    }
  }

  // ====================== GESTIONE RESET WIFI ======================
  if (resetCountdownStarted) { // Se è stata avviata la sequenza di reset del WiFi (pressione dei 4 angoli).
    // Calcola il tempo trascorso dall'inizio della pressione.
    uint32_t elapsedTime = currentMillis - touchStartTime;

    // Variabili per tenere traccia se ogni angolo è ancora premuto.
    bool tl = false, tr = false, bl = false, br = false;
    bool anyCornerDetected = false; // Flag per indicare se è stato rilevato il tocco in almeno un angolo.

    // Itera attraverso i punti di contatto rilevati.
    for (int i = 0; i < ts.touches && i < 10; i++) {
      // Mappa le coordinate del touch screen alle coordinate del display.
      int x = map(ts.points[i].x, TOUCH_MAP_X1, TOUCH_MAP_X2, 0, 480 - 1);
      int y = map(ts.points[i].y, TOUCH_MAP_Y1, TOUCH_MAP_Y2, 0, 480 - 1);

      // Verifica se il punto di contatto rientra in una delle zone degli angoli (con una tolleranza).
      if (x < 100 && y < 100) {
        tl = true;
        anyCornerDetected = true;
      }
      else if (x > 380 && y < 100) {
        tr = true;
        anyCornerDetected = true;
      }
      else if (x < 100 && y > 380) {
        bl = true;
        anyCornerDetected = true;
      }
      else if (x > 380 && y > 380) {
        br = true;
        anyCornerDetected = true;
      }
    }

    bool allCornersPressed = (tl && tr && bl && br); // Vero se tutti e quattro gli angoli sono premuti.

    // Se tutti gli angoli sono ancora premuti, resetta il contatore di perdita del tocco.
    if (allCornersPressed) {
      cornerTouchLostCounter = 0;

      // Aggiorna la visualizzazione del countdown ogni secondo.
      static int lastSecondDisplayed = -1;
      int currentSecond = 5 - (elapsedTime / 1000); // Calcola i secondi rimanenti.

      if (currentSecond != lastSecondDisplayed) {
        gfx->fillScreen(YELLOW);
        gfx->setTextColor(BLACK);
        gfx->setCursor(80, 180);
        gfx->println(F("RESET WIFI TRA"));
        gfx->setCursor(210, 240);
        gfx->print(currentSecond);
        gfx->println(F(" SEC"));
        gfx->setCursor(40, 300);
        gfx->println(F("RILASCIA PER ANNULLARE"));

        lastSecondDisplayed = currentSecond; // Aggiorna l'ultimo secondo visualizzato.
      }
    }
    // Se non tutti gli angoli sono premuti, ma ne è rilevato almeno uno.
    else if (anyCornerDetected) {
      // Tolleranza: permette fino a 3 controlli consecutivi con angoli mancanti.
      if (cornerTouchLostCounter < 3) {
        cornerTouchLostCounter++;
      } else {
        // Troppi controlli con angoli mancanti, annulla la sequenza di reset.
        resetCountdownStarted = false;
        cornerTouchLostCounter = 0;

        gfx->fillScreen(BLACK);
        gfx->setTextColor(RED);
        gfx->setCursor(80, 240);
        gfx->println(F("RESET WIFI ANNULLATO"));
        delay(1000);

        gfx->fillScreen(BLACK);

      }
    }
    // Se non viene rilevato alcun tocco in nessun angolo.
    else {
      // Annulla immediatamente la sequenza di reset.
      resetCountdownStarted = false;
      cornerTouchLostCounter = 0;

      gfx->fillScreen(BLACK);
      gfx->setTextColor(RED);
      gfx->setCursor(80, 240);
      gfx->println(F("RESET WIFI ANNULLATO"));
      delay(1000);

      gfx->fillScreen(BLACK);

    }

    // Se sono trascorsi 5 secondi dall'inizio della pressione, esegui il reset indipendentemente dallo stato degli angoli.
    if (elapsedTime >= 5000) {
      resetCountdownStarted = false;
      cornerTouchLostCounter = 0;

      // Esegui la funzione per resettare le impostazioni WiFi.
      resetWiFiSettings();
    }

    return; // Esce dalla funzione per concentrarsi sulla gestione del reset.
  }

  // ====================== GESTIONE PAGINA SETUP ======================
  if (setupPageActive) { // Se la pagina di setup è attiva.
    if (ts.isTouched) { // Se lo schermo è toccato.
      // Calcola le coordinate del tocco.
      int x = map(ts.points[0].x, TOUCH_MAP_X1, TOUCH_MAP_X2, 0, 480 - 1);
      int y = map(ts.points[0].y, TOUCH_MAP_Y1, TOUCH_MAP_Y2, 0, 480 - 1);

      // Se non stiamo già gestendo uno scroll e non stiamo aspettando il rilascio di un tocco precedente.
      if (!setupScrollActive && !waitingForRelease) {
        // Verifica se il tocco corrente è l'inizio di uno scroll nella pagina di setup.
        if (checkSetupScroll()) {
          setupScrollActive = true;       // Imposta il flag di scroll attivo.
          waitingForRelease = true;     // Imposta il flag per aspettare il rilascio dopo lo scroll.
        } else {
          // Se non è uno scroll, gestisci il tocco come una normale interazione con un elemento del menu.
          handleSetupTouch(x, y);
          waitingForRelease = true;     // Imposta il flag per aspettare il rilascio dopo l'interazione.
        }
      }
    } else {
      // Se il tocco è rilasciato.
      waitingForRelease = false;     // Resetta il flag di attesa del rilascio.
      setupScrollActive = false;     // Resetta il flag di scroll attivo.
    }

    // Aggiorna il timestamp dell'ultima attività nella pagina di setup.
    setupLastActivity = currentMillis;
    return;  // Esce dalla funzione per concentrarsi solo sulla gestione della pagina di setup.
  }

  // ====================== RILEVAMENTO RESET WIFI ======================
  /*
   * Rileva una pressione simultanea sui 4 angoli dello schermo per avviare la sequenza di reset del WiFi.
   */
  if (ts.touches >= 4 && !resetCountdownStarted) { // Se ci sono almeno 4 tocchi e la sequenza di reset non è già iniziata.
    bool tl = false, tr = false, bl = false, br = false; // Flag per ogni angolo.

    // Verifica se ci sono tocchi nelle aree dei quattro angoli.
    for (int i = 0; i < ts.touches && i < 10; i++) {
      int x = map(ts.points[i].x, TOUCH_MAP_X1, TOUCH_MAP_X2, 0, 480 - 1);
      int y = map(ts.points[i].y, TOUCH_MAP_Y1, TOUCH_MAP_Y2, 0, 480 - 1);

      if (x < 100 && y < 100) tl = true;
      else if (x > 380 && y < 100) tr = true;
      else if (x < 100 && y > 380) bl = true;
      else if (x > 380 && y > 380) br = true;
    }

    // Se tutti e quattro gli angoli sono toccati contemporaneamente.
    if (tl && tr && bl && br) {
      resetCountdownStarted = true;   // Avvia il countdown per il reset.
      touchStartTime = currentMillis;  // Memorizza il tempo di inizio.
      cornerTouchLostCounter = 0;      // Resetta il contatore di perdita del tocco.

      // Fornisce un feedback visivo iniziale all'utente.
      gfx->fillScreen(YELLOW);
      gfx->setTextColor(BLACK);
      gfx->setCursor(80, 180);
      gfx->println(F("RESET WIFI TRA"));
      gfx->setCursor(210, 240);
      gfx->println(F("5 SEC"));
      gfx->setCursor(40, 300);
      gfx->println(F("RILASCIA PER ANNULLARE"));

      waitingForRelease = true; // Imposta il flag per aspettare il rilascio (anche se non necessario per l'avvio).
      return;                   // Esce dalla funzione per concentrarsi sulla gestione del reset.
    }
  }


#ifdef MENU_SCROLL
  // ====================== CONTROLLO SCROLL SETUP ======================
  if (ts.isTouched && !waitingForRelease && !checkingSetupScroll) {
    // ANTIRIMBALZO - Controlla se è passato abbastanza tempo dall'ultimo tocco.
    if (currentMillis - lastTouchTime < TOUCH_DEBOUNCE_MS) {
      return;
    }

    // Aggiorna il timestamp dell'ultimo tocco.
    lastTouchTime = currentMillis;

    // Ottieni la coordinata Y del primo punto di contatto.
    int16_t y = map(ts.points[0].y, TOUCH_MAP_Y1, TOUCH_MAP_Y2, 0, 480 - 1);

    // Se il tocco è nella parte superiore dello schermo (area tipicamente non usata per interazioni dirette).
    if (y < 50) {
      scrollStartY = y;          // Memorizza la coordinata Y iniziale del potenziale scroll.
      checkingSetupScroll = true; // Imposta il flag per indicare che stiamo controllando uno scroll.
      return;
    }

    // Se il tocco è in una posizione normale, memorizza le coordinate per le interazioni standard (quadranti).
    touch_last_x = map(ts.points[0].x, TOUCH_MAP_X1, TOUCH_MAP_X2, 0, 480 - 1);
    touch_last_y = map(ts.points[0].y, TOUCH_MAP_Y1, TOUCH_MAP_Y2, 0, 480 - 1);
#else
  // Se MENU_SCROLL non è definito, comunque otteniamo le coordinate del tocco se presente.
  if (ts.isTouched && !waitingForRelease) {
    touch_last_x = map(ts.points[0].x, TOUCH_MAP_X1, TOUCH_MAP_X2, 0, 480 - 1);
    touch_last_y = map(ts.points[0].y, TOUCH_MAP_Y1, TOUCH_MAP_Y2, 0, 480 - 1);
  }
#endif

  // ====================== GESTIONE DEI QUATTRO QUADRANTI ======================
  if (ts.isTouched && !waitingForRelease) {
// --------------------- TOCCO CENTRALE: Annuncia ora attuale --------------------------------//
#ifdef AUDIO
if (touch_last_x >= 170 && touch_last_x <= 310 && touch_last_y >= 170 && touch_last_y <= 310) {
  // Area centrale più ampia (140x140 pixel) per facilitare il tocco.
  playTouchSound();  // Emette un suono di feedback al tocco.

  // Feedback visivo temporaneo.
  gfx->fillScreen(BLACK);
//  gfx->setTextColor(CYAN);
//  gfx->setCursor(120, 200);
//  gfx->println(F("ANNUNCIO ORA"));

  // Visualizza l'ora attuale in formato digitale al centro dello schermo.
  gfx->setCursor(180, 250);
  gfx->setTextColor(WHITE);
  gfx->printf("%02d:%02d", currentHour, currentMinute);

  delay(300);

  // Tenta di annunciare l'ora vocalmente utilizzando la funzione migliorata.
  bool result = announceTimeFixed();

  // Se l'annuncio vocale fallisce.
  if (!result) {
//    // Fallback: usa i toni per indicare l'ora (commentato nel codice).
//    gfx->setTextColor(RED);
//    gfx->setCursor(100, 320);
//    gfx->println(F("AUDIO FALLITO - USO TONI"));
//    delay(500);

    // Riproduce una serie di toni per indicare l'ora (un tono per ogni ora, max 12).
    int hourTones = (currentHour == 0 || currentHour == 12) ? 12 : currentHour % 12;
    for (int i = 0; i < hourTones; i++) {
      playTone(880, 150);
      delay(150);
    }

    // Breve pausa tra l'indicazione delle ore e dei minuti.
    delay(600);

    // Riproduce una serie di toni per indicare i minuti (un tono ogni 5 minuti).
    int minGroups = currentMinute / 5;
    for (int i = 0; i < minGroups; i++) {
      playTone(1320, 80);
      delay(120);
    }
  }

  // Attende un breve periodo e forza un aggiornamento completo del display per tornare alla visualizzazione normale.
  delay(1000);
  forceDisplayUpdate();
  return; // Esce per evitare di processare altri tocchi.
}

#endif

    // --------------------- QUADRANTE SINISTRA --------------------------------//
    if (touch_last_x < 240) {

      // --------------------- QUADRANTE 1 - ALTO SINISTRA: Cambia modalità --------------------------------//
      if (touch_last_y < 240) {
        handleModeChange(); // Chiama la funzione per cambiare la modalità di visualizzazione dell'orologio.
      }

      // --------------------- QUADRANTE 3 - BASSO SINISTRA: Avvia ciclaggio colori --------------------------------//
      else if (touch_last_y > 240) {
        // Quadrante 3 (in basso a sinistra): Gestione del ciclo dei colori.
        if (!colorCycle.isActive) { // Se il ciclo dei colori non è attivo.
          waitingForRelease = true; // Imposta il flag per aspettare il rilascio del tocco.

          // Ottiene la tonalità, la saturazione e il valore (HSV) del colore corrente.
          uint16_t startHue;
          uint8_t startS, startV;
          rgbToHsv(currentColor.r, currentColor.g, currentColor.b, startHue, startS, startV);

          // Se il colore corrente è bianco (saturazione zero), riparte dal rosso.
          if (startS == 0) {
            startHue = 0;
            startS = 255;
          }

          // Attiva il ciclo dei colori e inizializza le sue proprietà.
          colorCycle.isActive = true;
          colorCycle.lastColorChange = millis();
          colorCycle.hue = (startHue * 360) / 255; // Riscala la tonalità da 0-255 a 0-360.
          colorCycle.saturation = startS;
          colorCycle.fadingToWhite = false;
          colorCycle.showingWhite = false;

          // playTouchSound();  // Feedback sonoro (commentato nel codice).
        }

      }

    }
    // --------------------- QUADRANTE DESTRA --------------------------------//
    else if (touch_last_x > 240) {

      // --------------------- QUADRANTE 2 - ALTO DESTRA: Cambia preset --------------------------------//
      if (touch_last_y < 240) {
        uint8_t oldPreset = currentPreset; // Memorizza il preset corrente.
        currentPreset = (currentPreset + 1) % 14; // Passa al preset successivo (cicla da 0 a 13).
        EEPROM.write(EEPROM_PRESET_ADDR, currentPreset); // Salva il nuovo preset nella EEPROM.
        EEPROM.commit(); // Scrive i dati.
        playTouchSound();  // Emette un suono di feedback.

        // Applica il nuovo preset chiamando la funzione apposita.
        applyPreset(currentPreset);
      }


      // --------------------- QUADRANTE 4 - BASSO DESTRA: Cambia stato lettera E --------------------------------//
      else if (touch_last_y > 240) {
        playTouchSound();  // Feedback sonoro.
        gfx->fillScreen(BLACK); // Pulisce lo schermo.
        delay(10);
        // Avanza allo stato successivo della visualizzazione della lettera 'E' (cicla tra 0 e 1).
        word_E_state = (word_E_state + 1) % 2;
          // Scrive il nuovo stato nella EEPROM.
        EEPROM.write(EEPROM_WORD_E_STATE_ADDR, word_E_state);
        EEPROM.commit();

        const char* Msg;
        switch (word_E_state) {
          case 0:
            Msg = "BLINK OFF";
            gfx->setTextColor(RED);
            break;
          case 1:
            Msg = "BLINK ON";
            gfx->setTextColor(GREEN);
            break;

        }

        // Calcola la posizione orizzontale per centrare il testo.
        int textWidth = strlen(Msg) * 18;  // Stima approssimativa della larghezza del testo.
        int xPos = (480 - textWidth) / 2;
        if (xPos < 0) xPos = 0;
        gfx->setFont(u8g2_font_maniac_te); // Imposta un font.
        gfx->setCursor(xPos, 240);
        gfx->println(Msg); // Stampa il messaggio ON/OFF.
        gfx->setFont(u8g2_font_inb21_mr); // Ripristina il font principale.

        delay(1000);
        forceDisplayUpdate(); // Forza l'aggiornamento del display.

      }
    }



    // Segnala che stiamo aspettando il rilascio del tocco corrente.
    waitingForRelease = true;
  }

  // ====================== CONTROLLO SCROLL SETUP ======================
  #ifdef MENU_SCROLL
  if (checkingSetupScroll) {
    if (!ts.isTouched) {
      // Rilascio del tocco, annulla il controllo dello scroll.
      checkingSetupScroll = false;
      scrollStartY = -1;
    } else {
      // Verifica se il gesto di scroll verso il basso è sufficiente.
      int16_t y = map(ts.points[0].y, TOUCH_MAP_Y1, TOUCH_MAP_Y2, 0, 480 - 1);

      if (y - scrollStartY > 100) {  // Se lo spostamento verticale è di almeno 100 pixel verso il basso.
        checkingSetupScroll = false; // Annulla il controllo dello scroll.
        scrollStartY = -1;           // Resetta la posizione iniziale dello scroll.

        // Attiva la visualizzazione della pagina di setup.
        showSetupPage();
        playTouchSound();  // Emette un suono di feedback.
      }
    }
  }
  #endif
}

void playTouchSound() {
 #ifdef AUDIO
  playTone(440, 80); // Riproduce un breve tono a 440 Hz per 80 millisecondi.
  #endif
}