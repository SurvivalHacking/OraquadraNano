//===================================================================//
//                        EFFETTO INTRODUTTIVO                        //
//===================================================================//
bool updateIntro() {
  static bool introInitialized = false;        // Flag per indicare se l'animazione introduttiva è stata inizializzata.
  static bool introAnimationCompleted = false; // Flag per indicare se l'animazione introduttiva è terminata.
  static uint8_t dropsLaunchedThisRound = 0;  // Contatore delle "gocce" lanciate nell'iterazione corrente.

  if (introAnimationCompleted) return true;  // Se l'animazione è finita, restituisce vero.

  if (!introInitialized) {
    Serial.println("[INTRO] Init Animazione."); // Stampa un messaggio sulla seriale all'inizio dell'animazione.
    gfx->fillScreen(BLACK);                   // Pulisce lo schermo riempiendolo di nero.
    memset(targetPixels, 0, sizeof(targetPixels));     // Resetta l'array dei pixel target (per il primo nome).
    memset(targetPixels_1, 0, sizeof(targetPixels_1));   // Resetta l'array dei pixel target (per il secondo nome).
    memset(targetPixels_2, 0, sizeof(targetPixels_2));   // Resetta l'array dei pixel target (per il terzo nome).
    memset(activePixels, 0, sizeof(activePixels));     // Resetta l'array dei pixel attivi (inizialmente nessuno è acceso).

    displayWordToTarget(WORD_DAVIDE); // Imposta i pixel target per visualizzare il primo nome.
    displayWordToTarget_1(WORD_PAOLO); // Imposta i pixel target per visualizzare il secondo nome.
    displayWordToTarget_2(WORD_ALE);   // Imposta i pixel target per visualizzare il terzo nome.

    // Inizializza le proprietà di ogni "goccia" dell'effetto matrix.
    for (int i = 0; i < NUM_DROPS; i++) {
      drops[i].isMatrix2 = false; // Indica che la goccia non è della variante Matrix2 (non usata qui).
      drops[i].active = false;    // Indica che la goccia non è ancora attiva (non è in movimento).
    }

    dropsLaunchedThisRound = 0;  // Resetta il contatore delle gocce lanciate.
    introInitialized = true;     // Imposta il flag per indicare che l'animazione è stata inizializzata.
    introAnimationCompleted = false; // Assicura che il flag di completamento sia falso all'inizio.

    gfx->setFont(u8g2_font_inb21_mr); // Imposta un font per il testo.
    // Prepara lo sfondo iniziale con il colore di sfondo per le lettere.
    for (uint16_t i = 0; i < NUM_LEDS; i++) {
      displayBuffer[i] = convertColor(TextBackColor); // Imposta il colore di sfondo nel buffer.
      gfx->setTextColor(convertColor(TextBackColor)); // Imposta il colore del testo a sfondo.
      gfx->setCursor(pgm_read_word(&TFT_X[i]), pgm_read_word(&TFT_Y[i])); // Imposta la posizione del cursore.
      gfx->write(pgm_read_byte(&TFT_L_INTRO[i])); // Scrive il carattere (inizialmente invisibile).
    }
  }

  gfx->setFont(u8g2_font_inb21_mr); // Assicura che il font sia impostato per il disegno.

  // Aggiorna il colore dei pixel che fanno parte dei nomi, se sono diventati attivi.
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    if (activePixels[i]) { // Se il pixel è attivo.
      if (targetPixels[i]) {
        gfx->setTextColor(GREEN);   // Imposta il colore a verde per il primo nome.
      } else if (targetPixels_1[i]) {
        gfx->setTextColor(WHITE);   // Imposta il colore a bianco per il secondo nome.
      } else if (targetPixels_2[i]) {
        gfx->setTextColor(RED);     // Imposta il colore a rosso per il terzo nome.
      } else {
        continue; // Se il pixel è attivo ma non fa parte di nessun target, continua.
      }
      gfx->setCursor(pgm_read_word(&TFT_X[i]), pgm_read_word(&TFT_Y[i])); // Imposta la posizione del cursore.
      gfx->write(pgm_read_byte(&TFT_L_INTRO[i])); // Scrive il carattere con il colore target.
    }
  }

  // Gestisce l'effetto delle "gocce" che rivelano i nomi.
  for (int i = 0; i < MATRIX_WIDTH; i++) {
    Drop& drop = drops[i]; // Ottiene un riferimento alla goccia corrente.
    if (!drop.active) continue; // Se la goccia non è attiva, passa alla successiva.

    uint16_t pos = ((int)drop.y * MATRIX_WIDTH) + drop.x; // Calcola la posizione lineare del pixel nella "matrice".

    // Se la posizione della goccia è valida all'interno della "matrice" di LED.
    if (drop.y >= 0 && drop.y < MATRIX_HEIGHT && pos < NUM_LEDS) {
      // Se la goccia raggiunge un pixel target e quel pixel non è ancora attivo.
      if (targetPixels[pos] && !activePixels[pos]) {
        activePixels[pos] = true;        // Imposta il pixel come attivo.
        gfx->setTextColor(GREEN);       // Imposta il colore a verde.
      } else if (targetPixels_1[pos] && !activePixels[pos]) {
        activePixels[pos] = true;        // Imposta il pixel come attivo.
        gfx->setTextColor(WHITE);       // Imposta il colore a bianco.
      } else if (targetPixels_2[pos] && !activePixels[pos]) {
        activePixels[pos] = true;        // Imposta il pixel come attivo.
        gfx->setTextColor(RED);         // Imposta il colore a rosso.
      } else if (!activePixels[pos]) {
        // Se il pixel non è ancora attivo e non è un target, mostra una "scia" blu debole.
        uint8_t intensity = 255 - ((int)drop.y * 16); // L'intensità diminuisce con la profondità.
        RGB blueColor(BackColor, BackColor, intensity); // Crea un colore blu.
        gfx->setTextColor(RGBtoRGB565(blueColor));   // Imposta il colore blu.
      }
      gfx->setCursor(pgm_read_word(&TFT_X[pos]), pgm_read_word(&TFT_Y[pos])); // Imposta la posizione.
      gfx->write(pgm_read_byte(&TFT_L_INTRO[pos])); // Scrive il carattere.
    }

    // Effetto scia dietro la "goccia".
    int headIntensity = 255 - ((int)drop.y * 8); // Intensità della "testa" della scia.
    headIntensity = max((int)BackColor, headIntensity); // Assicura che l'intensità non sia inferiore al colore di sfondo.
    float decayStep = (headIntensity - BackColor) / (MATRIX_TRAIL_LENGTH - 1); // Calcola la diminuzione di intensità per ogni segmento della scia.

    for (int trail = 1; trail <= MATRIX_TRAIL_LENGTH; trail++) {
      int trailY = (int)(drop.y - trail); // Calcola la coordinata Y del segmento della scia.
      if (trailY >= 0 && trailY < MATRIX_HEIGHT) {
        int trailPos = trailY * MATRIX_WIDTH + drop.x; // Calcola la posizione del segmento.
        // Se il segmento della scia non è un target e non è ancora attivo.
        if (!targetPixels[trailPos] && !targetPixels_1[trailPos] && !targetPixels_2[trailPos] && !activePixels[trailPos]) {
          uint8_t trailBlueColor = headIntensity - roundf((trail - 1) * decayStep); // Calcola l'intensità del blu per il segmento.
          RGB trailColor(BackColor, BackColor, trailBlueColor); // Crea il colore della scia.
          gfx->setTextColor(RGBtoRGB565(trailColor)); // Imposta il colore.
          gfx->setCursor(pgm_read_word(&TFT_X[trailPos]), pgm_read_word(&TFT_Y[trailPos])); // Imposta la posizione.
          gfx->write(pgm_read_byte(&TFT_L_INTRO[trailPos])); // Scrive il carattere della scia.
        }
      }
    }

    drop.y += drop.speed; // Aggiorna la posizione Y della "goccia".
    // Se la "goccia" è scesa completamente oltre il bordo inferiore.
    if (drop.y >= MATRIX_HEIGHT + MATRIX_TRAIL_LENGTH + 2) {
      drop.active = false; // Disattiva la "goccia".
    }
  }

  // Lancia nuove "gocce" fino a raggiungere la larghezza della "matrice".
  if (dropsLaunchedThisRound < MATRIX_WIDTH) {
    if (!drops[dropsLaunchedThisRound].active) {
      drops[dropsLaunchedThisRound].x = dropsLaunchedThisRound; // Inizia la goccia dalla colonna corrispondente.
      drops[dropsLaunchedThisRound].y = random(MATRIX_START_Y_MIN, MATRIX_START_Y_MAX); // Posizione Y iniziale casuale.
      drops[dropsLaunchedThisRound].speed = MATRIX_BASE_SPEED + (random(100) / 100.0f * MATRIX_SPEED_VAR); // Velocità casuale.
      drops[dropsLaunchedThisRound].active = true; // Attiva la goccia.
      drops[dropsLaunchedThisRound].isMatrix2 = false; // Non è Matrix2.
      dropsLaunchedThisRound++; // Incrementa il contatore delle gocce lanciate.
    }
  }

  // Verifica se tutte le "gocce" sono diventate inattive, indicando la fine dell'animazione.
  if (dropsLaunchedThisRound >= MATRIX_WIDTH) {
    bool allInactive = true;
    for (int i = 0; i < MATRIX_WIDTH; i++) {
      if (drops[i].active) {
        allInactive = false;
        break;
      }
    }
    if (allInactive) {
      introAnimationCompleted = true; // Imposta il flag di completamento.
      Serial.println("[INTRO] Animazione completata."); // Stampa un messaggio.
      return true;  // Restituisce vero per indicare che l'animazione è finita.
    }
  }

  return false;  // L'animazione è ancora in corso.
}


//===================================================================//
//                        EFFETTO WIFI                                //
//===================================================================//
void displayWifiInit() {
  gfx->fillScreen(BLACK); // Pulisce lo schermo.

  // Reset completo degli array di stato.
  memset(activePixels, 0, sizeof(activePixels));
  memset(targetPixels, 0, sizeof(targetPixels));
  memset(pixelChanged, 1, sizeof(pixelChanged));

  // Prepara lo sfondo iniziale con il colore di sfondo per le lettere.
  gfx->setFont(u8g2_font_inb21_mr);
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    displayBuffer[i] = convertColor(TextBackColor); // Imposta il colore di sfondo nel buffer.
    gfx->setTextColor(convertColor(TextBackColor)); // Imposta il colore del testo a sfondo.
    gfx->setCursor(pgm_read_word(&TFT_X[i]), pgm_read_word(&TFT_Y[i])); // Imposta la posizione del cursore.
    gfx->write(pgm_read_byte(&TFT_L[i])); // Scrive il carattere (inizialmente invisibile).
  }
}

void displayWordWifi(uint8_t pos, const String& text) {
  // Visualizza una parola sullo schermo per l'effetto WiFi.
  for (uint8_t i = 0; i < text.length(); i++) {
    // Pulisce un'area rettangolare per il nuovo carattere.
    gfx->fillRect(pgm_read_word(&TFT_X[pos]) - 5, pgm_read_word(&TFT_Y[pos]) - 25, 30, 30, BLACK);
    gfx->setCursor(pgm_read_word(&TFT_X[pos]), pgm_read_word(&TFT_Y[pos]));
    gfx->print(text[i]); // Stampa il carattere.
    pos++; // Passa alla posizione del carattere successivo.
  }
}

void displayWifiDot(uint8_t n) {
  // Visualizza un punto animato durante il tentativo di connessione WiFi.
  uint8_t pos = 96; // Posizione iniziale per i punti.
  pos = n + pos;    // Sposta la posizione in base al numero del tentativo.
  gfx->fillRect(pgm_read_word(&TFT_X[pos]) - 5, pgm_read_word(&TFT_Y[pos]) - 25, 30, 30, BLACK); // Pulisce l'area.
  gfx->setCursor(pgm_read_word(&TFT_X[pos]), pgm_read_word(&TFT_Y[pos]));
  gfx->print("."); // Stampa il punto.
}

//===================================================================//
//                        EFFETTO MODALITÀ VELOCE                     //
//===================================================================//
void updateFastMode() {
  lastHour = currentHour;   // Memorizza l'ora precedente per rilevare i cambiamenti.
  lastMinute = currentMinute; // Memorizza il minuto precedente.
  // Pulizia completa dello schermo.
  gfx->fillScreen(BLACK);

  // Reset completo degli array di stato.
  memset(activePixels, 0, sizeof(activePixels));
  memset(targetPixels, 0, sizeof(targetPixels));
  memset(pixelChanged, 1, sizeof(pixelChanged));

  // Prepara lo sfondo con il colore di sfondo per le lettere.
  gfx->setFont(u8g2_font_inb21_mr);
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    displayBuffer[i] = convertColor(TextBackColor); // Imposta il colore di sfondo nel buffer.
    gfx->setTextColor(convertColor(TextBackColor)); // Imposta il colore del testo a sfondo.
    gfx->setCursor(pgm_read_word(&TFT_X[i]), pgm_read_word(&TFT_Y[i])); // Imposta la posizione del cursore.
    gfx->write(pgm_read_byte(&TFT_L[i])); // Scrive il carattere (inizialmente invisibile).
  }

  // Imposta i pixel target per visualizzare l'ora corrente.
  if (currentHour == 0) {
    strncpy(&TFT_L[6], "MEZZANOTTE", 10); // Aggiorna la parola "SONO LE" con "MEZZANOTTE".
    displayWord(WORD_MEZZANOTTE, currentColor); // Visualizza "MEZZANOTTE".
  } else {
    strncpy(&TFT_L[6], "EYOREXZERO", 10); // Ripristina la parola "SONO LE".
    displayWord(WORD_SONO_LE, currentColor); // Visualizza "SONO LE".
    const uint8_t* hourWord = (const uint8_t*)pgm_read_ptr(&HOUR_WORDS[currentHour]); // Ottiene la parola per l'ora corrente.
    displayWord(hourWord, currentColor); // Visualizza la parola dell'ora.
  }
  // Visualizza i minuti se sono maggiori di zero.
  if (currentMinute > 0) {
    displayWord(WORD_E, currentColor); // Visualizza "E".
    showMinutes(currentMinute, currentColor); // Visualizza la parola dei minuti.
    displayWord(WORD_MINUTI, currentColor); // Visualizza "MINUTI".
  }

}

void displayWord(const uint8_t* word, const Color& color) {
  // Funzione per visualizzare una parola sullo schermo con un dato colore.
  if (!word) return; // Se la parola è nulla, esce.

  uint16_t colorRGB565 = convertColor(color); // Converte il colore in formato RGB565.

  uint8_t idx = 0;
  uint8_t pixel;

  // Itera attraverso gli indici dei pixel che compongono la parola.
  while ((pixel = pgm_read_byte(&word[idx])) != 4) { // Il valore 4 indica la fine della parola.
    activePixels[pixel] = true;   // Imposta il pixel come attivo.
    pixelChanged[pixel] = true;   // Imposta il pixel come cambiato.



    // Aggiorna il buffer in memoria
    displayBuffer[pixel] = colorRGB565; // Aggiorna il colore nel buffer.

    // Aggiorna direttamente il pixel sul display.
    gfx->setTextColor(colorRGB565);
    gfx->setCursor(pgm_read_word(&TFT_X[pixel]), pgm_read_word(&TFT_Y[pixel]));
    gfx->write(pgm_read_byte(&TFT_L[pixel]));

    idx++; // Passa all'indice del pixel successivo nella parola.
  }
}

void showMinutes(uint8_t minutes, const Color& color) {
  // Funzione per visualizzare la parola corrispondente ai minuti.
  if (minutes <= 0) return; // Se i minuti sono zero o negativi, esce.

  // Gestione dei minuti da 0 a 19.
  if (minutes <= 19) {
    const uint8_t* minuteWord = (const uint8_t*)pgm_read_ptr(&MINUTE_WORDS[minutes]);
    if (minuteWord) {
      displayWord(minuteWord, color); // Visualizza la parola corrispondente ai minuti.
    }
  } else {
    // Gestione delle decine e delle unità per i minuti maggiori di 19.
    uint8_t tens = minutes / 10; // Ottiene la cifra delle decine.
    uint8_t ones = minutes % 10; // Ottiene la cifra delle unità.

    // Visualizza la parte delle decine.
    switch (tens) {
      case 2: // 20-29
        displayWord(ones == 1 || ones == 8 ? WORD_MVENT : WORD_MVENTI, color); // "VENTI" o "MVENT" (per 21 e 28).
        break;
      case 3: // 30-39
        displayWord(ones == 1 || ones == 8 ? WORD_MTRENT : WORD_MTRENTA, color); // "TRENTA" o "MTRENT".
        break;
      case 4: // 40-49
        displayWord(ones == 1 || ones == 8 ? WORD_MQUARANT : WORD_MQUARANTA, color); // "QUARANTA" o "MQUARANT".
        break;
      case 5: // 50-59
        displayWord(ones == 1 || ones == 8 ? WORD_MCINQUANT : WORD_MCINQUANTA, color); // "CINQUANTA" o "MCINQUANT".
        break;
    }

    // Visualizza la parte delle unità, se presente.
    if (ones > 0) {
      if (ones == 1) {
        displayWord(WORD_MUN, color); // Visualizza "UN".
      } else {
        const uint8_t* onesWord = (const uint8_t*)pgm_read_ptr(&MINUTE_WORDS[ones]);
        if (onesWord) {
          displayWord(onesWord, color); // Visualizza la parola per l'unità.
        }
      }
    }
  }
}


//===================================================================//
//                        EFFETTO MODALITÀ LENTA                      //
//===================================================================//
void updateSlowMode() {

  // Inizializza l'effetto solo se l'ora o il minuto cambiano, o se è la prima volta che viene eseguito.
  if (currentHour != lastHour || currentMinute != lastMinute || !slowInitialized) {
    Serial.println("[SLOW] Refresh matrice per cambio ora o inizializzazione");

    gfx->fillScreen(BLACK); // Pulisce lo schermo.
    memset(activePixels, 0, sizeof(activePixels));   // Resetta l'array dei pixel attivi.
    memset(pixelChanged, true, sizeof(pixelChanged)); // Forza l'aggiornamento di tutti i pixel.
    memset(targetPixels, 0, sizeof(targetPixels));   // Resetta anche l'array dei pixel target.

    // Inizializza lo sfondo in modo più accurato.
    gfx->setFont(u8g2_font_inb21_mr);
    for (uint16_t i = 0; i < NUM_LEDS; i++) {
      // Reset completo di ogni pixel.
      activePixels[i] = false;
      pixelChanged[i] = true;
      targetPixels[i] = false;

      // Imposta tutti i caratteri con il colore di sfondo.
      displayBuffer[i] = convertColor(TextBackColor);
      gfx->setTextColor(displayBuffer[i]);
      gfx->setCursor(pgm_read_word(&TFT_X[i]), pgm_read_word(&TFT_Y[i]));
      gfx->write(pgm_read_byte(&TFT_L[i]));
    }

    // Imposta i pixel target per visualizzare l'ora corrente.
    if (currentHour == 0) {
      strncpy(&TFT_L[6], "MEZZANOTTE", 10);  // Sostituisce da posizione 7 (dove inizia "E").
      displayWordToTarget(WORD_MEZZANOTTE);
    } else {
      strncpy(&TFT_L[6], "EYOREXZERO", 10);  // Stessa posizione, lunghezza 10.
      displayWordToTarget(WORD_SONO_LE);
      const uint8_t* hourWord = (const uint8_t*)pgm_read_ptr(&HOUR_WORDS[currentHour]);
      displayWordToTarget(hourWord);
    }

    // Imposta i pixel target per visualizzare i minuti, se maggiori di zero.
    if (currentMinute > 0) {
      if (word_E_state == 0) displayWordToTarget(WORD_E); // Visualizza "E" se lo stato lo permette.
      displayMinutesToTarget(currentMinute);
      displayWordToTarget(WORD_MINUTI);
    }

    // Salva l'ora corrente per il confronto nel prossimo ciclo.
    lastHour = currentHour;
    lastMinute = currentMinute;
    fadeStartTime = millis(); // Registra l'ora di inizio del fade.
    fadeDone = false;        // Resetta il flag di completamento del fade.
    slowInitialized = true;  // Indica che l'effetto è stato inizializzato.

  }

  // Se il fade è già completato, esce dalla funzione.
  if (fadeDone) return;

  // Calcola la progressione del fade basata sul tempo trascorso.
  float progress = (float)(millis() - fadeStartTime) / fadeDuration;
  // Se la progressione raggiunge o supera 1.0, il fade è completo.
  if (progress >= 1.0f) {
    // Imposta il colore finale per i pixel target.
    uint16_t finalColor = convertColor(currentColor);
    for (uint16_t i = 0; i < NUM_LEDS; i++) {
      if (targetPixels[i]) {
        activePixels[i] = true;
        pixelChanged[i] = true;
        displayBuffer[i] = finalColor;
        gfx->setTextColor(finalColor);
        gfx->setCursor(pgm_read_word(&TFT_X[i]), pgm_read_word(&TFT_Y[i]));
        gfx->write(pgm_read_byte(&TFT_L[i]));
      }
    }
    fadeDone = true; // Imposta il flag di completamento del fade.
    return;
  }

  // Fade attivo: calcola il colore interpolato.
  float eased = pow(progress, 1 / 2.2f); // Applica una curva di easing (gamma correction).
  uint8_t r = TextBackColor.r + (uint8_t)(eased * (currentColor.r - TextBackColor.r));
  uint8_t g = TextBackColor.g + (uint8_t)(eased * (currentColor.g - TextBackColor.g));
  uint8_t b = TextBackColor.b + (uint8_t)(eased * (currentColor.b - TextBackColor.b));

  Color stepColor(r, g, b);       // Crea il colore intermedio.
  uint16_t pixelColor = convertColor(stepColor); // Converte al formato del display.

  // Applica il colore interpolato ai pixel target.
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    if (targetPixels[i]) {
      activePixels[i] = true;
      pixelChanged[i] = true;
      displayBuffer[i] = pixelColor;
      gfx->setTextColor(pixelColor);
      gfx->setCursor(pgm_read_word(&TFT_X[i]), pgm_read_word(&TFT_Y[i]));
      gfx->write(pgm_read_byte(&TFT_L[i]));
    }
  }
}



//===================================================================//
//                        EFFETTO MODALITÀ FADE                      //
//===================================================================//
void updateFadeMode() {
  const uint16_t stepInterval = wordFadeDuration / fadeSteps; // Calcola l'intervallo di tempo tra ogni step del fade.

  // Inizializza l'effetto se l'ora o il minuto cambiano, o se è la prima volta.
  if (lastHour != currentHour || lastMinute != currentMinute || !fadeInitialized ) {
    fadePhase = FADE_SONO_LE; // Inizia con la fase "SONO LE".
    fadeInitialized = true;
    fadeStep = 0;             // Resetta il contatore degli step del fade.
    lastFadeUpdate = millis(); // Registra l'ultimo aggiornamento del fade.

    gfx->fillScreen(BLACK); // Pulisce lo schermo.
    memset(activePixels, 0, sizeof(activePixels));   // Resetta i pixel attivi.
    memset(pixelChanged, true, sizeof(pixelChanged)); // Forza l'aggiornamento.
    memset(targetPixels, 0, sizeof(targetPixels));   // Resetta i target.

    // Imposta i caratteri per MEZZANOTTE o SONO LE in base all'ora
    if (currentHour == 0) {
      strncpy(&TFT_L[6], "MEZZANOTTE", 10); // Aggiorna la parola "SONO LE" con "MEZZANOTTE".
    } else {
      strncpy(&TFT_L[6], "EYOREXZERO", 10); // Ripristina la parola "SONO LE".
    }

    gfx->setFont(u8g2_font_inb21_mr);
    for (uint16_t i = 0; i < NUM_LEDS; i++) {
      activePixels[i] = false;
      pixelChanged[i] = true;
      displayBuffer[i] = convertColor(TextBackColor); // Imposta il colore di sfondo.
      gfx->setTextColor(displayBuffer[i]);
      gfx->setCursor(pgm_read_word(&TFT_X[i]), pgm_read_word(&TFT_Y[i]));
      gfx->write(pgm_read_byte(&TFT_L[i]));
    }

    lastHour = currentHour;   // Aggiorna l'ora precedente.
    lastMinute = currentMinute; // Aggiorna il minuto precedente.

  }

  uint32_t now = millis();
  if (now - lastFadeUpdate < stepInterval) return; // Se non è ancora tempo per il prossimo step, esce.
  lastFadeUpdate = now; // Aggiorna l'ultimo tempo di aggiornamento.

  // Gestisce le diverse fasi del fade per visualizzare l'ora.
  switch (fadePhase) {
    case FADE_SONO_LE:
      if (currentHour > 0) { // Se l'ora non è mezzanotte.
        if (fadeStep <= fadeSteps) {
          fadeWordPixels(WORD_SONO_LE, fadeStep); // Esegue il fade dei pixel della parola "SONO LE".
          fadeStep++;
        } else {
          fadePhase = FADE_ORA; // Passa alla fase dell'ora.
          fadeStep = 0;
        }
      } else {
        fadePhase = FADE_ORA; // Se è mezzanotte, salta direttamente alla fase dell'ora.
        fadeStep = 0;
      }
      break;

    case FADE_ORA: {
      const uint8_t* word = (currentHour == 0) // Determina la parola da visualizzare per l'ora.
        ? WORD_MEZZANOTTE
        : (const uint8_t*)pgm_read_ptr(&HOUR_WORDS[currentHour]);

      if (fadeStep <= fadeSteps) {
        fadeWordPixels(word, fadeStep); // Esegue il fade della parola dell'ora.
        fadeStep++;
      } else {
        fadePhase = (currentMinute > 0) ? FADE_E : FADE_DONE; // Passa a "E" se ci sono minuti, altrimenti a "DONE".
        fadeStep = 0;
      }
      break;
    }

    case FADE_E:
      if (fadeStep <= fadeSteps && word_E_state == 0) { // Se lo stato di "E" permette la visualizzazione.
        fadeWordPixels(WORD_E, fadeStep); // Esegue il fade della parola "E".
        fadeStep++;
      } else {
        fadePhase = FADE_MINUTI_NUMERO; // Passa alla fase dei numeri dei minuti.
        fadeStep = 0;
      }
      break;

    case FADE_MINUTI_NUMERO: {
      if (fadeStep <= fadeSteps) {
        MinuteWords mw = getMinuteWord(currentMinute); // Ottiene le parole per le decine e le unità dei minuti.
        if (mw.tens) fadeWordPixels(mw.tens, fadeStep);   // Esegue il fade delle decine.
        if (mw.ones) fadeWordPixels(mw.ones, fadeStep);   // Esegue il fade delle unità.
        fadeStep++;
      } else {
        fadePhase = FADE_MINUTI_PAROLA; // Passa alla fase della parola "MINUTI".
        fadeStep = 0;
      }
      break;
    }

    case FADE_MINUTI_PAROLA:
      if (fadeStep <= fadeSteps) {
        fadeWordPixels(WORD_MINUTI, fadeStep); // Esegue il fade della parola "MINUTI".
        fadeStep++;
      } else {
        fadePhase = FADE_DONE; // Passa alla fase di completamento.
        fadeStep = 0;
      }
      break;

    case FADE_DONE: {
      // Imposta il colore finale per tutti i pixel attivi.
      uint16_t finalColor = convertColor(currentColor);
      for (uint16_t i = 0; i < NUM_LEDS; i++) {
        if (activePixels[i]) {
          displayBuffer[i] = finalColor;
          gfx->setTextColor(finalColor);
          gfx->setCursor(pgm_read_word(&TFT_X[i]), pgm_read_word(&TFT_Y[i]));
          gfx->write(pgm_read_byte(&TFT_L[i]));
        }
      }
      break;
    }
  }
}



MinuteWords getMinuteWord(uint8_t minutes) {
  // Funzione per ottenere le parole per le decine e le unità dei minuti.
  MinuteWords result = {nullptr, nullptr};
  if (minutes == 0) return result; // Se i minuti sono 0, restituisce una struttura vuota.

  // Gestione dei minuti da 0 a 19.
  if (minutes <= 19) {
    result.tens = (const uint8_t*)pgm_read_ptr(&MINUTE_WORDS[minutes]); // La parola è direttamente nell'array.
    return result;
  }

  // Gestione delle decine e delle unità per i minuti maggiori di 19.
  uint8_t tens = (minutes / 10) - 2; // Calcola l'indice per l'array delle decine (20 diventa indice 0).
  uint8_t ones = minutes % 10;      // Ottiene la cifra delle unità.
  const MinuteTens* tensWords = &TENS_WORDS[tens]; // Ottiene la struttura con le parole per la decina.

  // Gestisce le forme troncate per "ventun" e "ventott", "trentun" ecc.
  if (ones == 8 || (ones == 1 && minutes >= 21)) {
    result.tens = (const uint8_t*)pgm_read_ptr(&tensWords->truncated);
  } else {
    result.tens = (const uint8_t*)pgm_read_ptr(&tensWords->normal);
  }

  // Ottiene la parola per le unità, se presenti.
  if (ones > 0) {
    result.ones = (ones == 1) ? WORD_MUN : (const uint8_t*)pgm_read_ptr(&MINUTE_WORDS[ones]);
  }

  return result;
}

void fadeWordPixels(const uint8_t* word, uint8_t step) {
  // Funzione per eseguire il fade dei pixel di una parola.
  float progress = (float)step / fadeSteps; // Calcola la progressione del fade (da 0 a 1).
  float eased = progress * progress * (3.0 - 2.0 * progress); // Applica una curva di easing (easeInOutCubic).

  // Calcola i valori RGB interpolati tra il colore di sfondo e il colore target.
  uint8_t r = TextBackColor.r + (uint8_t)(eased * (currentColor.r - TextBackColor.r));
  uint8_t g = TextBackColor.g + (uint8_t)(eased * (currentColor.g - TextBackColor.g));
  uint8_t b = TextBackColor.b + (uint8_t)(eased * (currentColor.b - TextBackColor.b));
  uint16_t pixelColor = convertColor(Color(r, g, b)); // Converte il colore interpolato al formato del display.

  uint8_t idx = 0, pixel;
  // Itera attraverso gli indici dei pixel che compongono la parola.
  while ((pixel = pgm_read_byte(&word[idx])) != 4) { // Il valore 4 indica la fine della parola.
    activePixels[pixel] = true;   // Imposta il pixel come attivo.
    pixelChanged[pixel] = true;   // Imposta il pixel come cambiato.
    displayBuffer[pixel] = pixelColor; // Aggiorna il colore nel buffer.
    gfx->setTextColor(pixelColor); // Imposta il colore del testo.
    gfx->setCursor(pgm_read_word(&TFT_X[pixel]), pgm_read_word(&TFT_Y[pixel])); // Imposta la posizione.
    gfx->write(pgm_read_byte(&TFT_L[pixel])); // Scrive il carattere con il colore corrente del fade.
    idx++; // Passa al pixel successivo.
  }
}
  

//===================================================================//
//                      EFFETTO MODALITÀ MATRIX                      //
//===================================================================//
void updateMatrix() {
  static bool matrixAnimationActive = true;   // Flag per indicare se l'animazione matrix è attiva.
  static uint8_t dropsLaunchedThisRound = 0;  // Contatore delle gocce lanciate nel ciclo corrente.

  // Se l'ora o il minuto cambiano, o se la modalità non è stata inizializzata.
  if (currentHour != lastHour || currentMinute != lastMinute || !matrixInitialized) {
    Serial.println("[MATRIX] Refresh matrice per cambio ora o inizializzazione");
    gfx->fillScreen(BLACK);                       // Pulisce lo schermo.
    memset(targetPixels, 0, sizeof(targetPixels)); // Resetta l'array dei pixel target.
    memset(activePixels, 0, sizeof(activePixels)); // Resetta l'array dei pixel attivi.

    // Imposta i pixel target per visualizzare l'ora corrente.
    if (currentHour == 0) {
      strncpy(&TFT_L[6], "MEZZANOTTE", 10);
      displayWordToTarget(WORD_MEZZANOTTE);
    } else {
      strncpy(&TFT_L[6], "EYOREXZERO", 10);
      displayWordToTarget(WORD_SONO_LE);
      const uint8_t* hourWord = (const uint8_t*)pgm_read_ptr(&HOUR_WORDS[currentHour]);
      displayWordToTarget(hourWord);
    }

    // Imposta i pixel target per visualizzare i minuti, se maggiori di zero.
    if (currentMinute > 0) {
      if (word_E_state == 0) displayWordToTarget(WORD_E); // Visualizza "E" se lo stato lo permette.
      displayMinutesToTarget(currentMinute);
      displayWordToTarget(WORD_MINUTI);
    }

    // Inizializza tutte le gocce per l'effetto matrix.
    for (int i = 0; i < NUM_DROPS; i++) {
      drops[i].isMatrix2 = false; // Indica che non è una goccia della modalità Matrix2.
      drops[i].active = false;    // Indica che la goccia non è ancora attiva (in movimento).
    }

    lastHour = currentHour;     // Aggiorna l'ora precedente.
    lastMinute = currentMinute;   // Aggiorna il minuto precedente.
    matrixInitialized = true;     // Indica che la modalità è stata inizializzata.
    matrixAnimationActive = true; // Riattiva l'animazione se l'ora cambia.
    dropsLaunchedThisRound = 0;   // Resetta il contatore delle gocce lanciate.

    // Prepara lo sfondo con il colore di sfondo per le lettere.
    gfx->setFont(u8g2_font_inb21_mr);
    for (uint16_t i = 0; i < NUM_LEDS; i++) {
      displayBuffer[i] = convertColor(TextBackColor);
      gfx->setTextColor(convertColor(TextBackColor));
      gfx->setCursor(pgm_read_word(&TFT_X[i]), pgm_read_word(&TFT_Y[i]));
      gfx->write(pgm_read_byte(&TFT_L[i]));
    }
  } // Inizializzazione completata.

  gfx->setFont(u8g2_font_inb21_mr);

  // Accende le lettere dell'orario se sono state "colpite" dalle gocce.
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    if (targetPixels[i] && activePixels[i]) {
      gfx->setTextColor(RGBtoRGB565(currentColor)); // Imposta il colore della lettera.
      gfx->setCursor(pgm_read_word(&TFT_X[i]), pgm_read_word(&TFT_Y[i]));
      gfx->write(pgm_read_byte(&TFT_L[i]));
    }
  }

  // Gestione delle gocce attive.
  for (int i = 0; i < NUM_DROPS; i++) {
    Drop& drop = drops[i];
    if (!drop.active) continue; // Se la goccia non è attiva, passa alla successiva.

    uint16_t pos = ((int)drop.y * MATRIX_WIDTH) + drop.x; // Calcola la posizione lineare della goccia.

    // Disegna la goccia solo se è visibile e non sulla lettera "E" (se fissa).
    if (drop.y >= 0 && drop.y < MATRIX_HEIGHT && pos < NUM_LEDS && !(word_E_state == 1 && pos == 116)) {
      if (targetPixels[pos]) {
        // Se la goccia colpisce un pixel target e questo non è ancora attivo.
        if (!activePixels[pos]) {
          activePixels[pos] = true; // Imposta il pixel come attivo.
          gfx->setTextColor(RGBtoRGB565(currentColor)); // Imposta il colore della lettera.
          gfx->setCursor(pgm_read_word(&TFT_X[pos]), pgm_read_word(&TFT_Y[pos]));
          gfx->write(pgm_read_byte(&TFT_L[pos]));
        }
        // Se il pixel è già attivo, non ridisegnarlo.
      } else {
        // Disegna la "testa" della goccia con un colore verde brillante.
        int headIntensity = 255 - ((int)drop.y * 8);
        headIntensity = max((int)BackColor, headIntensity);
        RGB greenColor(BackColor, headIntensity, BackColor);
        gfx->setTextColor(RGBtoRGB565(greenColor));
        gfx->setCursor(pgm_read_word(&TFT_X[pos]), pgm_read_word(&TFT_Y[pos]));
        gfx->write(pgm_read_byte(&TFT_L[pos]));
      }
    }

    // Effetto scia adattiva dietro la goccia.
    int headIntensity = 255 - ((int)drop.y * 8);
    headIntensity = max((int)BackColor, headIntensity);
    float decayStep = (headIntensity - BackColor) / (MATRIX_TRAIL_LENGTH - 1);

    for (int trail = 1; trail <= MATRIX_TRAIL_LENGTH; trail++) {
      int trailY = (int)(drop.y - trail);
      if (trailY >= 0 && trailY < MATRIX_HEIGHT) {
        int trailPos = trailY * MATRIX_WIDTH + drop.x;
        // Disegna la scia se il pixel non è un target, non è attivo e non è la lettera "E" fissa.
        if (!targetPixels[trailPos] && !activePixels[trailPos] && !(word_E_state == 1 && trailPos == 116)) {
          uint8_t trailGreenColor = headIntensity - roundf((trail - 1) * decayStep);
          RGB trailColor(BackColor, trailGreenColor, BackColor);
          gfx->setTextColor(RGBtoRGB565(trailColor));
          gfx->setCursor(pgm_read_word(&TFT_X[trailPos]), pgm_read_word(&TFT_Y[trailPos]));
          gfx->write(pgm_read_byte(&TFT_L[trailPos]));
        }
      }
    }

    // Avanza la posizione Y della goccia.
    drop.y += drop.speed;

    // Disattiva la goccia quando esce completamente dallo schermo.
    if (drop.y >= MATRIX_HEIGHT + MATRIX_TRAIL_LENGTH + 2) {
      drop.active = false;
    }
  }

  // Lancia nuove gocce se l'animazione è attiva.
  if (matrixAnimationActive) {
    if (dropsLaunchedThisRound < MATRIX_WIDTH) {
      // Lancia una nuova goccia se la posizione corrente non è attiva.
      if (!drops[dropsLaunchedThisRound].active) {
        drops[dropsLaunchedThisRound].x = dropsLaunchedThisRound; // Posizione X iniziale.
        drops[dropsLaunchedThisRound].y = random(MATRIX_START_Y_MIN, MATRIX_START_Y_MAX); // Posizione Y iniziale casuale.
        drops[dropsLaunchedThisRound].speed = MATRIX_BASE_SPEED + (random(100) / 100.0f * MATRIX_SPEED_VAR); // Velocità casuale.
        drops[dropsLaunchedThisRound].active = true; // Attiva la goccia.
        drops[dropsLaunchedThisRound].isMatrix2 = false;
        dropsLaunchedThisRound++; // Incrementa il contatore delle gocce lanciate.
      }
    } else {
      // L'animazione di lancio è completata per questo ciclo.
      matrixAnimationActive = false;
      Serial.println("[MATRIX] Animazione completata dopo un round");
    }
  }

  // GESTIONE DEL BLINK DELLA LETTERA "E" (se abilitato).
  if (currentMinute > 0 && word_E_state == 1 && areAllDropsInactive()) {
    static bool E_on = false;
    static bool E_off = false;
    uint8_t pos = 116; // Posizione della lettera "E".
    if (currentSecond % 2 == 0) { // Se il secondo corrente è pari.
      E_off = false;
      if (!E_on) {
        E_on = true;
        gfx->setTextColor(RGBtoRGB565(currentColor)); // Accende la "E" con il colore corrente.
        gfx->setCursor(pgm_read_word(&TFT_X[pos]), pgm_read_word(&TFT_Y[pos]));
        gfx->write(pgm_read_byte(&TFT_L[pos]));
      }
    } else { // Se il secondo corrente è dispari.
      E_on = false;
      if (!E_on) {
        E_off = true;
        gfx->setTextColor(convertColor(TextBackColor)); // Spegne la "E" con il colore di sfondo.
        gfx->setCursor(pgm_read_word(&TFT_X[pos]), pgm_read_word(&TFT_Y[pos]));
        gfx->write(pgm_read_byte(&TFT_L[pos]));
      }
    }
  }

}

// Funzione per verificare se tutte le gocce sono inattive.
bool areAllDropsInactive() {
  for (int i = 0; i < NUM_DROPS; i++) {
    if (drops[i].active) {
      return false; // Se almeno una goccia è attiva, restituisce false.
    }
  }
  return true; // Se nessuna goccia è attiva, restituisce true.
}


//===================================================================//
//                      EFFETTO MODALITÀ MATRIX2                     //
//===================================================================//
void updateMatrix2() {
  // Se l'ora o il minuto cambiano, o se la modalità non è stata inizializzata.
  if (currentHour != lastHour || currentMinute != lastMinute || !matrixInitialized) {
    Serial.println("[MATRIX] Refresh matrice per cambio ora o inizializzazione");
    gfx->fillScreen(BLACK);                       // Pulisce lo schermo.
    memset(targetPixels, 0, sizeof(targetPixels)); // Resetta l'array dei pixel target.
    memset(activePixels, 0, sizeof(activePixels)); // Resetta l'array dei pixel attivi.

    // Imposta i pixel target per visualizzare l'ora corrente.
    if (currentHour == 0) {
      strncpy(&TFT_L[6], "MEZZANOTTE", 10);
      displayWordToTarget(WORD_MEZZANOTTE);
    } else {
      strncpy(&TFT_L[6], "EYOREXZERO", 10);
      displayWordToTarget(WORD_SONO_LE);
      const uint8_t* hourWord = (const uint8_t*)pgm_read_ptr(&HOUR_WORDS[currentHour]);
      displayWordToTarget(hourWord);
    }

    // Imposta i pixel target per visualizzare i minuti, se maggiori di zero.
    if (currentMinute > 0) {
      if (word_E_state == 0) displayWordToTarget(WORD_E); // Visualizza "E" se lo stato lo permette.
      displayMinutesToTarget(currentMinute);
      displayWordToTarget(WORD_MINUTI);
    }

    // Inizializza tutte le gocce per l'effetto matrix.
    for (int i = 0; i < NUM_DROPS; i++) {
      drops[i].isMatrix2 = false; // Indica che non è una goccia della modalità Matrix2 (anche se qui è impostato a false).
      drops[i].active = false;    // Indica che la goccia non è ancora attiva (in movimento).
    }

    lastHour = currentHour;     // Aggiorna l'ora precedente.
    lastMinute = currentMinute;   // Aggiorna il minuto precedente.
    matrixInitialized = true;     // Indica che la modalità è stata inizializzata.

    // Prepara lo sfondo con il colore di sfondo per le lettere.
    gfx->setFont(u8g2_font_inb21_mr);
    for (uint16_t i = 0; i < NUM_LEDS; i++) {
      displayBuffer[i] = convertColor(TextBackColor);
      gfx->setTextColor(convertColor(TextBackColor));
      gfx->setCursor(pgm_read_word(&TFT_X[i]), pgm_read_word(&TFT_Y[i]));
      gfx->write(pgm_read_byte(&TFT_L[i]));
    }
  } // Inizializzazione completata.

  gfx->setFont(u8g2_font_inb21_mr);

  // Accende le lettere dell'orario se sono state "colpite" dalle gocce.
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    if (targetPixels[i] && activePixels[i]) {
      gfx->setTextColor(RGBtoRGB565(currentColor)); // Imposta il colore della lettera.
      gfx->setCursor(pgm_read_word(&TFT_X[i]), pgm_read_word(&TFT_Y[i]));
      gfx->write(pgm_read_byte(&TFT_L[i]));
    }
  }

  // Gestione delle gocce attive.
  for (int i = 0; i < NUM_DROPS; i++) {
    Drop& drop = drops[i];
    if (!drop.active) continue; // Se la goccia non è attiva, passa alla successiva.

    uint16_t pos = ((int)drop.y * MATRIX_WIDTH) + drop.x; // Calcola la posizione lineare della goccia.

    // Disegna la goccia solo se è visibile e non sulla lettera "E" (se fissa).
    if (drop.y >= 0 && drop.y < MATRIX_HEIGHT && pos < NUM_LEDS && !(word_E_state == 1 && pos == 116)) {
      if (targetPixels[pos]) {
        // Se la goccia colpisce un pixel target e questo non è ancora attivo.
        if (!activePixels[pos]) {
          activePixels[pos] = true; // Imposta il pixel come attivo.
          gfx->setTextColor(RGBtoRGB565(currentColor)); // Imposta il colore della lettera.
          gfx->setCursor(pgm_read_word(&TFT_X[pos]), pgm_read_word(&TFT_Y[pos]));
          gfx->write(pgm_read_byte(&TFT_L[pos]));
        }
        // Se il pixel è già attivo, non ridisegnarlo.
      } else {
        // Disegna la "testa" della goccia con un colore blu brillante.
        int headIntensity = 255 - ((int)drop.y * 8);
        headIntensity = max((int)BackColor, headIntensity);
        RGB blueColor(BackColor, BackColor, headIntensity);
        gfx->setTextColor(RGBtoRGB565(blueColor));
        gfx->setCursor(pgm_read_word(&TFT_X[pos]), pgm_read_word(&TFT_Y[pos]));
        gfx->write(pgm_read_byte(&TFT_L[pos]));
      }
    }

    // Effetto scia adattiva dietro la goccia.
    int headIntensity = 255 - ((int)drop.y * 8);
    headIntensity = max((int)BackColor, headIntensity);
    float decayStep = (headIntensity - BackColor) / (MATRIX_TRAIL_LENGTH - 1);

    for (int trail = 1; trail <= MATRIX_TRAIL_LENGTH; trail++) {
      int trailY = (int)(drop.y - trail);
      if (trailY >= 0 && trailY < MATRIX_HEIGHT) {
        int trailPos = trailY * MATRIX_WIDTH + drop.x;
        // Disegna la scia se il pixel non è un target, non è attivo e non è la lettera "E" fissa.
        if (!targetPixels[trailPos] && !activePixels[trailPos] && !(word_E_state == 1 && trailPos == 116)) {
          uint8_t trailblueColor = headIntensity - roundf((trail - 1) * decayStep);
          RGB trailColor(BackColor, BackColor, trailblueColor);
          gfx->setTextColor(RGBtoRGB565(trailColor));
          gfx->setCursor(pgm_read_word(&TFT_X[trailPos]), pgm_read_word(&TFT_Y[trailPos]));
          gfx->write(pgm_read_byte(&TFT_L[trailPos]));
        }
      }
    }

    // Avanza la posizione Y della goccia.
    drop.y += drop.speed;

    // Disattiva la goccia quando esce completamente dallo schermo.
    if (drop.y >= MATRIX_HEIGHT + MATRIX_TRAIL_LENGTH + 2) {
      drop.active = false;
    }
  }

  // Lancia nuove gocce in modo continuo.
  for (uint8_t x = 0; x < MATRIX_WIDTH; x++) {
    if (canDropInColumn(x)) { // Verifica se è possibile far cadere una goccia in questa colonna.
      for (int i = 0; i < NUM_DROPS; i++) {
        if (!drops[i].active) { // Trova una goccia inattiva.
          drops[i].x = x; // Imposta la posizione X iniziale della goccia.
          drops[i].y = random(MATRIX_START_Y_MIN, MATRIX_START_Y_MAX); // Imposta la posizione Y iniziale casuale.
          drops[i].speed = MATRIX_BASE_SPEED + (random(100) / 100.0f * MATRIX_SPEED_VAR); // Imposta la velocità casuale.
          drops[i].active = true; // Attiva la goccia.
          drops[i].isMatrix2 = false;
          break; // Esce dal loop delle gocce inattive dopo averne trovata una.
        }
      }
    }
  }

  // GESTIONE DEL BLINK DELLA LETTERA "E" (se abilitato e tutte le lettere sono disegnate).
  if (currentMinute > 0 && word_E_state == 1 && areAllLettersDrawn()) {
    static bool E_on = false;
    static bool E_off = false;
    uint8_t pos = 116; // Posizione della lettera "E".
    if (currentSecond % 2 == 0) { // Se il secondo corrente è pari.
      E_off = false;
      if (!E_on) {
        E_on = true;
        gfx->setTextColor(RGBtoRGB565(currentColor)); // Accende la "E" con il colore corrente.
        gfx->setCursor(pgm_read_word(&TFT_X[pos]), pgm_read_word(&TFT_Y[pos]));
        gfx->write(pgm_read_byte(&TFT_L[pos]));
      }
    } else { // Se il secondo corrente è dispari.
      E_on = false;
      if (!E_on) {
        E_off = true;
        gfx->setTextColor(convertColor(TextBackColor)); // Spegne la "E" con il colore di sfondo.
        gfx->setCursor(pgm_read_word(&TFT_X[pos]), pgm_read_word(&TFT_Y[pos]));
        gfx->write(pgm_read_byte(&TFT_L[pos]));
      }
    }
  }

}

// Funzione per verificare se è possibile far cadere una goccia in una data colonna.
bool canDropInColumn(uint8_t col) {
  for (int i = 0; i < NUM_DROPS; i++) {
    // Se c'è una goccia attiva nella colonna specificata.
    if (drops[i].active && drops[i].x == col) {
      // E se la sua posizione Y è ancora vicina alla cima (per evitare sovrapposizioni troppo fitte).
      if ((int)drops[i].y < MATRIX_TRAIL_LENGTH + 1) return false; // Non permettere una nuova goccia.
    }
  }
  return true; // Nessuna goccia troppo vicina alla cima in questa colonna, si può far cadere.
}

// Funzione per verificare se tutte le lettere target sono state "colpite" e sono attive.
bool areAllLettersDrawn() {
  for (int i = 0; i < NUM_LEDS; i++) {
    if (targetPixels[i] && !activePixels[i]) {
      return false;  // Se c'è ancora almeno una lettera target che non è attiva, restituisce false.
    }
  }
  return true; // Se tutti i pixel target sono attivi, restituisce true.
}

// Funzione per impostare i pixel target di una parola.
// I pixel target sono quelli che devono essere accesi per formare la parola.
// Questa funzione non accende direttamente i pixel, ma marca quali devono essere accesi.
void displayWordToTarget(const uint8_t* word) {
  if (!word) return; // Se il puntatore alla parola è nullo, esce dalla funzione.

  uint8_t idx = 0;   // Indice per scorrere i byte della parola.
  uint8_t pixel;   // Variabile per memorizzare l'indice del pixel.
  // Scorre i byte della parola fino a quando non incontra il valore 4 (che indica la fine della parola).
  while ((pixel = pgm_read_byte(&word[idx])) != 4) {
    targetPixels[pixel] = true; // Imposta il pixel corrente come target (da accendere).
    idx++;                     // Passa al byte successivo della parola.
  }
}

// Funzione simile a displayWordToTarget, ma imposta i pixel target in un array separato (targetPixels_1).
// Questo può essere utile per visualizzare più parole contemporaneamente con effetti diversi.
void displayWordToTarget_1(const uint8_t* word) {
  if (!word) return; // Se il puntatore alla parola è nullo, esce dalla funzione.

  uint8_t idx = 0;   // Indice per scorrere i byte della parola.
  uint8_t pixel;   // Variabile per memorizzare l'indice del pixel.
  // Scorre i byte della parola fino a quando non incontra il valore 4 (che indica la fine della parola).
  while ((pixel = pgm_read_byte(&word[idx])) != 4) {
    targetPixels_1[pixel] = true; // Imposta il pixel corrente come target nell'array targetPixels_1.
    idx++;                     // Passa al byte successivo della parola.
  }
}

// Funzione simile a displayWordToTarget, ma imposta i pixel target in un array separato (targetPixels_2).
// Utile per un terzo set di parole o effetti simultanei.
void displayWordToTarget_2(const uint8_t* word) {
  if (!word) return; // Se il puntatore alla parola è nullo, esce dalla funzione.

  uint8_t idx = 0;   // Indice per scorrere i byte della parola.
  uint8_t pixel;   // Variabile per memorizzare l'indice del pixel.
  // Scorre i byte della parola fino a quando non incontra il valore 4 (che indica la fine della parola).
  while ((pixel = pgm_read_byte(&word[idx])) != 4) {
    targetPixels_2[pixel] = true; // Imposta il pixel corrente come target nell'array targetPixels_2.
    idx++;                     // Passa al byte successivo della parola.
  }
}

// Funzione per impostare i pixel target per visualizzare i minuti.
void displayMinutesToTarget(uint8_t minutes) {
  if (minutes <= 0) return; // Se i minuti sono zero o negativi, esce dalla funzione.

  // Gestisce i minuti da 0 a 19 (le parole sono memorizzate direttamente).
  if (minutes <= 19) {
    const uint8_t* minuteWord = (const uint8_t*)pgm_read_ptr(&MINUTE_WORDS[minutes]);
    displayWordToTarget(minuteWord); // Imposta come target la parola corrispondente ai minuti.
  } else {
    // Gestisce le decine dei minuti (20-59).
    uint8_t tens = (minutes / 10) - 2;  // Calcola l'indice per l'array delle decine (20 -> 0, 30 -> 1, ecc.).
    uint8_t ones = minutes % 10;     // Ottiene la cifra delle unità dei minuti.
    const MinuteTens* tensWords = &TENS_WORDS[tens]; // Ottiene la struttura contenente le forme normale e troncata della decina.

    // Sceglie la forma corretta della decina in base alla cifra delle unità (per es. "venti" vs "ventun").
    const uint8_t* decinaWord;
    if (ones == 8 || (ones == 1 && minutes >= 21)) {
      decinaWord = (const uint8_t*)pgm_read_ptr(&tensWords->truncated); // Usa la forma troncata (es. "vent").
    } else {
      decinaWord = (const uint8_t*)pgm_read_ptr(&tensWords->normal);    // Usa la forma normale (es. "trenta").
    }

    displayWordToTarget(decinaWord); // Imposta come target la parola della decina.

    // Se ci sono unità (minuti da 1 a 9), imposta come target anche la parola dell'unità.
    if (ones > 0) {
      if (ones == 1) {
        displayWordToTarget(WORD_MUN); // Imposta come target la parola "un".
      } else {
        const uint8_t* onesWord = (const uint8_t*)pgm_read_ptr(&MINUTE_WORDS[ones]);
        displayWordToTarget(onesWord); // Imposta come target la parola dell'unità (es. "due", "tre").
      }
    }
  }
}



//===================================================================//
//                        EFFETTO GOCCIA D'ACQUA                      //
//===================================================================//
void initWaterDrop() {
  // PULIZIA COMPLETA: rimuove ogni traccia dell'orario precedente dallo schermo.
  gfx->fillScreen(BLACK);

  // Reset completo dello stato degli array di gestione dei pixel.
  memset(targetPixels, 0, sizeof(targetPixels)); // Nessun pixel è inizialmente target.
  memset(activePixels, 0, sizeof(activePixels)); // Nessun pixel è inizialmente attivo (acceso).
  memset(pixelChanged, 1, sizeof(pixelChanged)); // Forza l'aggiornamento di tutti i pixel al prossimo ciclo.

  // Imposta i pixel target per visualizzare l'ora corrente.
  if (currentHour == 0) {
    strncpy(&TFT_L[6], "MEZZANOTTE", 10);
    displayWordToTarget(WORD_MEZZANOTTE);
  } else {
    strncpy(&TFT_L[6], "EYOREXZERO", 10);
    displayWordToTarget(WORD_SONO_LE);
    const uint8_t* hourWord = (const uint8_t*)pgm_read_ptr(&HOUR_WORDS[currentHour]);
    displayWordToTarget(hourWord);
  }

  if (currentMinute > 0) {
    if (word_E_state == 0) displayWordToTarget(WORD_E);
    displayMinutesToTarget(currentMinute);
    displayWordToTarget(WORD_MINUTI);
  }

  // Inizializza le proprietà della "goccia d'acqua".
  waterDrop.centerX = MATRIX_WIDTH / 2;   // Centro orizzontale dello schermo.
  waterDrop.centerY = MATRIX_HEIGHT / 2;  // Centro verticale dello schermo.
  waterDrop.currentRadius = 0.0;        // Raggio iniziale dell'onda è zero.
  waterDrop.startTime = millis();       // Memorizza l'ora di inizio dell'effetto.
  waterDrop.active = true;            // L'effetto è attivo.
  waterDrop.completed = false;         // L'animazione non è ancora completata.
  waterDrop.cleanupDone = false;       // La pulizia finale (visualizzazione stabile dell'ora) non è fatta.
  waterDropInitNeeded = false;       // Indica che l'inizializzazione non è più necessaria.
}

void updateWaterDrop() {
  static uint8_t lastRadius = 0;        // Memorizza il raggio intero precedente per ottimizzazione.
  static uint32_t completionTimeout = 0; // Timeout di sicurezza per evitare blocchi.
  uint32_t currentMillis = millis();     // Ottiene il tempo corrente.

  // Verifica se l'ora è cambiata o se è richiesto un reset.
  if (currentHour != lastHour || currentMinute != lastMinute || !waterDrop.active || waterDropInitNeeded) {
    // Inizializza l'effetto water drop.
    initWaterDrop();
    lastHour = currentHour;       // Aggiorna l'ora precedente.
    lastMinute = currentMinute;     // Aggiorna il minuto precedente.
    completionTimeout = currentMillis; // Resetta il timeout di completamento.
    return;
  }

  // Se l'animazione è completata, mostra solo le parole dell'orario.
  if (waterDrop.completed) {
    if (!waterDrop.cleanupDone) {
      // Aggiornamento finale per assicurare colori coerenti dell'orario.
      gfx->setFont(u8g2_font_inb21_mr);

      // Prima riempiamo completamente lo sfondo con il colore di sfondo.
      for (uint16_t i = 0; i < NUM_LEDS; i++) {
        if (!targetPixels[i]) {
          gfx->setTextColor(convertColor(TextBackColor));
          gfx->setCursor(pgm_read_word(&TFT_X[i]), pgm_read_word(&TFT_Y[i]));
          gfx->write(pgm_read_byte(&TFT_L[i]));
        }
      }

      // Poi, in un passaggio separato, coloriamo tutti i pixel target con il colore corrente.
      for (uint16_t i = 0; i < NUM_LEDS; i++) {
        if (targetPixels[i]) {
          activePixels[i] = true; // Imposta il pixel come attivo (la lettera è visualizzata).
          gfx->setTextColor(RGBtoRGB565(currentColor));
          gfx->setCursor(pgm_read_word(&TFT_X[i]), pgm_read_word(&TFT_Y[i]));
          gfx->write(pgm_read_byte(&TFT_L[i]));
        }
      }

      waterDrop.cleanupDone = true; // Indica che la pulizia finale è stata completata.
    }

    // GESTIONE DEL BLINK DELLA LETTERA "E" (se abilitato).
    if (currentMinute > 0 && word_E_state == 1) {
      static bool E_on = false;
      static bool E_off = false;
      uint8_t pos = 116;
      if (currentSecond % 2 == 0) {
        E_off = false;
        if (!E_on) {
          E_on = true;
          gfx->setTextColor(RGBtoRGB565(currentColor));
          gfx->setCursor(pgm_read_word(&TFT_X[pos]), pgm_read_word(&TFT_Y[pos]));
          gfx->write(pgm_read_byte(&TFT_L[pos]));
        }
      } else {
        E_on = false;
        if (!E_on) {
          E_off = true;
          gfx->setTextColor(convertColor(TextBackColor));
          gfx->setCursor(pgm_read_word(&TFT_X[pos]), pgm_read_word(&TFT_Y[pos]));
          gfx->write(pgm_read_byte(&TFT_L[pos]));
        }
      }
    }

    return; // Esce dalla funzione se l'animazione è completata.
  }

  // Calcola il tempo trascorso dall'inizio dell'effetto.
  uint32_t elapsedTime = currentMillis - waterDrop.startTime;

  // Controlla se la durata massima dell'animazione è stata raggiunta.
  if (elapsedTime >= DROP_DURATION) {
    waterDrop.completed = true;
    return;
  }

  // Calcola il raggio corrente dell'onda in base al tempo trascorso e alla velocità.
  waterDrop.currentRadius = (elapsedTime / 1000.0) * RIPPLE_SPEED;

  // Verifica se il raggio ha superato il raggio massimo previsto.
  if (waterDrop.currentRadius >= MAX_RIPPLE_RADIUS) {
    waterDrop.completed = true;
    return;
  }

  // Ottimizzazione: aggiorna il display solo se il raggio intero è cambiato.
  uint8_t currentIntRadius = (uint8_t)waterDrop.currentRadius;
  if (currentIntRadius == lastRadius && currentIntRadius > 0) {
    return;
  }
  lastRadius = currentIntRadius; // Aggiorna l'ultimo raggio intero.

  // Timeout di sicurezza per forzare la fine dell'animazione in caso di problemi.
  if (currentMillis - completionTimeout > 15000) {
    waterDrop.completed = true;
    return;
  }

  // CALCOLO DELL'EFFETTO GOCCIA D'ACQUA SUI PIXEL.
  for (uint16_t i = 0; i < NUM_LEDS; i++) {

    // Calcola le coordinate X e Y del pixel corrente sulla matrice.
    uint8_t x, y;
    ledIndexToXY(i, x, y);

    // Calcola la distanza del pixel dal centro della "goccia".
    float dist = distance(x, y, waterDrop.centerX, waterDrop.centerY);

    // Definisce i bordi interni ed esterni dell'onda corrente.
    float rippleInner = waterDrop.currentRadius - 1.0;
    float rippleOuter = waterDrop.currentRadius;

    // Se il pixel si trova all'interno dell'onda corrente.
    if (dist >= rippleInner && dist < rippleOuter) {
      // Calcola l'intensità dell'effetto onda in base alla distanza dal bordo interno.
      float intensity = 1.0 - ((dist - rippleInner) / (rippleOuter - rippleInner));

      // Definisce un colore blu-ciano per l'onda.
      uint8_t blueValue = 150 + (uint8_t)(100 * intensity);
      uint8_t cyanValue = (uint8_t)(150 * intensity);
      RGB rippleColor(0, cyanValue, blueValue);

      // Se il pixel è un pixel target (fa parte dell'orario), lo attiva permanentemente.
      if (targetPixels[i]) {
        activePixels[i] = true;
        gfx->setTextColor(RGBtoRGB565(currentColor));
      } else {
        // Altrimenti, colora il pixel con l'effetto temporaneo dell'onda.
        gfx->setTextColor(RGBtoRGB565(rippleColor));
      }

      gfx->setCursor(pgm_read_word(&TFT_X[i]), pgm_read_word(&TFT_Y[i]));
      gfx->write(pgm_read_byte(&TFT_L[i]));
    }
    // Se il pixel è fuori dall'onda corrente.
    else if (dist >= rippleOuter || dist < rippleInner - 1.0) {
      // Ripristina il pixel al colore di sfondo se non è un pixel target già attivato.
      if (!targetPixels[i] || !activePixels[i]) {
        gfx->setTextColor(convertColor(TextBackColor));
        gfx->setCursor(pgm_read_word(&TFT_X[i]), pgm_read_word(&TFT_Y[i]));
        gfx->write(pgm_read_byte(&TFT_L[i]));
      }
    }
  }

  // Se il raggio dell'onda è abbastanza grande da coprire l'intero schermo, considera l'effetto completato.
  if (waterDrop.currentRadius > sqrt(pow(MATRIX_WIDTH, 2) + pow(MATRIX_HEIGHT, 2))) {
    waterDrop.completed = true;
  }
}

// ================== FUNZIONI DI SUPPORTO ==================
// Converte un indice lineare di LED (da 0 a NUM_LEDS - 1) nelle coordinate X e Y della matrice.
// Questo è utile per localizzare fisicamente un LED sulla griglia.
void ledIndexToXY(uint16_t index, uint8_t &x, uint8_t &y) {
  x = index % MATRIX_WIDTH;      // La coordinata X è il resto della divisione dell'indice per la larghezza.
  y = index / MATRIX_WIDTH;      // La coordinata Y è il risultato intero della divisione dell'indice per la larghezza.
}

// Calcola la distanza euclidea (in linea retta) tra due punti nel piano cartesiano.
// Utilizzata per determinare la distanza di un pixel dal centro dell'effetto goccia d'acqua.
float distance(float x1, float y1, float x2, float y2) {
  return sqrt(pow(x2 - x1, 2) + pow(y2 - y1, 2)); // Radice quadrata della somma dei quadrati delle differenze tra le coordinate.
}


//===================================================================//
//                        EFFETTO SERPENTE                           //
//===================================================================//
void updateSnake() {
  static uint8_t lastHour = 255;
  static uint8_t lastMinute = 255;
  static bool snakeInitialized = false;
  static bool snakeCompleted = false;
  static uint16_t pathIndex = 0;
  static const uint16_t* currentPath = nullptr;  // Puntatore al percorso scelto
  static uint8_t pathChoice = 0;
  uint32_t currentMillis = millis();

  // Inizializzazione o cambio orario
  if (currentHour != lastHour || currentMinute != lastMinute || snakeInitNeeded || !snakeInitialized) {
    Serial.println("[SNAKE] Inizializzazione effetto Snake");
    gfx->fillScreen(BLACK);

    memset(targetPixels, 0, sizeof(targetPixels));
    memset(activePixels, 0, sizeof(activePixels));
    memset(snakeTrailColors, 0, sizeof(snakeTrailColors));

    // Disegna TUTTE le lettere con il colore di sfondo grigio
    gfx->setFont(u8g2_font_inb21_mr);
    for (uint16_t i = 0; i < NUM_LEDS; i++) {
      gfx->setTextColor(convertColor(TextBackColor));
      gfx->setCursor(pgm_read_word(&TFT_X[i]), pgm_read_word(&TFT_Y[i]));
      gfx->write(pgm_read_byte(&TFT_L[i]));
    }

    pathIndex = 0;
    snakeInitialized = true;
    snakeCompleted = false;
    snakeInitNeeded = false;
    lastHour = currentHour;
    lastMinute = currentMinute;

    // PERCORSI ZIG ZAG - 4 VARIANTI
    // 1. ZIG ZAG TOP LEFT (parte da 0, va verso il basso)
    static const uint16_t ZIGZAG_TOP_LEFT[256] = {
      0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
      31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16,
      32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
      63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48,
      64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
      95, 94, 93, 92, 91, 90, 89, 88, 87, 86, 85, 84, 83, 82, 81, 80,
      96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
      127, 126, 125, 124, 123, 122, 121, 120, 119, 118, 117, 116, 115, 114, 113, 112,
      128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
      159, 158, 157, 156, 155, 154, 153, 152, 151, 150, 149, 148, 147, 146, 145, 144,
      160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175,
      191, 190, 189, 188, 187, 186, 185, 184, 183, 182, 181, 180, 179, 178, 177, 176,
      192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207,
      223, 222, 221, 220, 219, 218, 217, 216, 215, 214, 213, 212, 211, 210, 209, 208,
      224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
      255, 254, 253, 252, 251, 250, 249, 248, 247, 246, 245, 244, 243, 242, 241, 240
    };

    // 2. ZIG ZAG TOP RIGHT (parte da 15, va verso il basso)
    static const uint16_t ZIGZAG_TOP_RIGHT[256] = {
      15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0,
      16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
      47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32,
      48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
      79, 78, 77, 76, 75, 74, 73, 72, 71, 70, 69, 68, 67, 66, 65, 64,
      80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
      111, 110, 109, 108, 107, 106, 105, 104, 103, 102, 101, 100, 99, 98, 97, 96,
      112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127,
      143, 142, 141, 140, 139, 138, 137, 136, 135, 134, 133, 132, 131, 130, 129, 128,
      144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159,
      175, 174, 173, 172, 171, 170, 169, 168, 167, 166, 165, 164, 163, 162, 161, 160,
      176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191,
      207, 206, 205, 204, 203, 202, 201, 200, 199, 198, 197, 196, 195, 194, 193, 192,
      208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223,
      239, 238, 237, 236, 235, 234, 233, 232, 231, 230, 229, 228, 227, 226, 225, 224,
      240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255
    };

    // 3. ZIG ZAG BOTTOM LEFT (parte da 240, va verso l'alto)
    static const uint16_t ZIGZAG_BOTTOM_LEFT[256] = {
      240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255,
      239, 238, 237, 236, 235, 234, 233, 232, 231, 230, 229, 228, 227, 226, 225, 224,
      208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223,
      207, 206, 205, 204, 203, 202, 201, 200, 199, 198, 197, 196, 195, 194, 193, 192,
      176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191,
      175, 174, 173, 172, 171, 170, 169, 168, 167, 166, 165, 164, 163, 162, 161, 160,
      144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159,
      143, 142, 141, 140, 139, 138, 137, 136, 135, 134, 133, 132, 131, 130, 129, 128,
      112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127,
      111, 110, 109, 108, 107, 106, 105, 104, 103, 102, 101, 100, 99, 98, 97, 96,
      80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
      79, 78, 77, 76, 75, 74, 73, 72, 71, 70, 69, 68, 67, 66, 65, 64,
      48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
      47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32,
      16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
      15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0
    };

    // 4. ZIG ZAG BOTTOM RIGHT (parte da 255, va verso l'alto)
    static const uint16_t ZIGZAG_BOTTOM_RIGHT[256] = {
      255, 254, 253, 252, 251, 250, 249, 248, 247, 246, 245, 244, 243, 242, 241, 240,
      224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
      223, 222, 221, 220, 219, 218, 217, 216, 215, 214, 213, 212, 211, 210, 209, 208,
      192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207,
      191, 190, 189, 188, 187, 186, 185, 184, 183, 182, 181, 180, 179, 178, 177, 176,
      160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175,
      159, 158, 157, 156, 155, 154, 153, 152, 151, 150, 149, 148, 147, 146, 145, 144,
      128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
      127, 126, 125, 124, 123, 122, 121, 120, 119, 118, 117, 116, 115, 114, 113, 112,
      96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
      95, 94, 93, 92, 91, 90, 89, 88, 87, 86, 85, 84, 83, 82, 81, 80,
      64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
      63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48,
      32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
      31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16,
      0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
    };

    // Seleziona casualmente uno dei 4 percorsi zig zag
    pathChoice = random(0, 4);
    switch(pathChoice) {
      case 0: currentPath = ZIGZAG_TOP_LEFT; Serial.println("[SNAKE] ZIG ZAG da alto-sinistra"); break;
      case 1: currentPath = ZIGZAG_TOP_RIGHT; Serial.println("[SNAKE] ZIG ZAG da alto-destra"); break;
      case 2: currentPath = ZIGZAG_BOTTOM_LEFT; Serial.println("[SNAKE] ZIG ZAG da basso-sinistra"); break;
      case 3: currentPath = ZIGZAG_BOTTOM_RIGHT; Serial.println("[SNAKE] ZIG ZAG da basso-destra"); break;
    }

    snake.length = 8;
    snake.speed = SNAKE_SPEED;
    snake.lastMove = currentMillis;

    // Inizializza tutti i segmenti del serpente alla posizione iniziale (fuori schermo)
    for (uint8_t i = 0; i < snake.length; i++) {
      snake.segments[i].ledIndex = 999;  // Valore invalido, verrà sovrascritto
    }

    // Disegna TUTTE le lettere in grigio come sfondo
    gfx->setFont(u8g2_font_inb21_mr);
    for (uint16_t i = 0; i < NUM_LEDS; i++) {
      gfx->setTextColor(convertColor(TextBackColor));
      gfx->setCursor(pgm_read_word(&TFT_X[i]), pgm_read_word(&TFT_Y[i]));
      gfx->write(pgm_read_byte(&TFT_L[i]));
    }

    // Imposta i pixel target per visualizzare l'ora corrente
    if (currentHour == 0) {
      strncpy(&TFT_L[6], "MEZZANOTTE", 10); // Aggiorna la parola "SONO LE" con "MEZZANOTTE".
      displayWordToTarget(WORD_MEZZANOTTE);
    } else {
      strncpy(&TFT_L[6], "EYOREXZERO", 10); // Ripristina la parola "SONO LE".
      displayWordToTarget(WORD_SONO_LE);
      const uint8_t* hourWord = (const uint8_t*)pgm_read_ptr(&HOUR_WORDS[currentHour]);
      displayWordToTarget(hourWord);
    }

    if (currentMinute > 0) {
      if (word_E_state == 0) displayWordToTarget(WORD_E);
      displayMinutesToTarget(currentMinute);
      displayWordToTarget(WORD_MINUTI);
    }

    return; // Esce dalla funzione dopo l'inizializzazione.
  }


  // Se il serpente ha completato tutto il percorso (256 posizioni)
  if (snakeCompleted) {
    // EFFETTO ARCOBALENO CICLICO sulle lettere dell'orario - OTTIMIZZATO
    static uint32_t lastRainbowUpdate = 0;
    static uint8_t rainbowOffset = 0;

    // Aggiorna l'effetto arcobaleno ogni 50ms
    if (currentMillis - lastRainbowUpdate >= 50) {
      lastRainbowUpdate = currentMillis;
      rainbowOffset = (rainbowOffset + 2) % 255;

      gfx->setFont(u8g2_font_inb21_mr);

      // Ridisegna SOLO le lettere dell'orario (non tutte le 256)
      for (uint16_t pos = 0; pos < NUM_LEDS; pos++) {
        if (targetPixels[pos] && activePixels[pos]) {
          // Lettera dell'orario: effetto arcobaleno ciclico
          uint8_t hue = (rainbowOffset + (pos * 10)) % 255;
          Color rainbowColor = hsvToRgb(hue, 255, 255);
          gfx->setTextColor(convertColor(rainbowColor));
          gfx->setCursor(pgm_read_word(&TFT_X[pos]), pgm_read_word(&TFT_Y[pos]));
          gfx->write(pgm_read_byte(&TFT_L[pos]));
        }
      }
    }
    return;
  }

  // Movimento del serpente - OTTIMIZZATO: ridisegna solo pixel cambiati
  if (currentMillis - snake.lastMove >= snake.speed) {
    snake.lastMove = currentMillis;

    // Avanza lungo il percorso LED
    if (pathIndex >= 256) {
      snakeCompleted = true;
      Serial.println("[SNAKE] Percorso completato");

      // PULIZIA FINALE: ridisegna tutte le lettere NON orario in grigio
      gfx->setFont(u8g2_font_inb21_mr);
      for (uint16_t pos = 0; pos < NUM_LEDS; pos++) {
        if (!targetPixels[pos]) {
          // Lettera NON orario: forza grigio
          gfx->setTextColor(convertColor(TextBackColor));
          gfx->setCursor(pgm_read_word(&TFT_X[pos]), pgm_read_word(&TFT_Y[pos]));
          gfx->write(pgm_read_byte(&TFT_L[pos]));
        }
      }

      return;
    }

    // SALVA la posizione della coda prima di muoverla
    uint16_t oldTailPos = snake.segments[snake.length - 1].ledIndex;

    // Ottieni la nuova posizione della testa dal percorso
    uint16_t newHeadPos = currentPath[pathIndex];

    // Muovi tutti i segmenti indietro di uno (la coda scompare)
    for (int8_t i = snake.length - 1; i > 0; i--) {
      snake.segments[i].ledIndex = snake.segments[i - 1].ledIndex;
    }

    // La testa si muove nella nuova posizione
    snake.segments[0].ledIndex = newHeadPos;
    pathIndex++;

    // Calcola i colori per gli effetti
    uint8_t baseHue = (millis() / 30) % 255;

    gfx->setFont(u8g2_font_inb21_mr);

    // STEP 1: Ridisegna SOLO la vecchia coda (se valida)
    if (oldTailPos < NUM_LEDS) {
      if (targetPixels[oldTailPos] && activePixels[oldTailPos]) {
        // Era una lettera dell'orario: mostra colore salvato (trail)
        gfx->setTextColor(RGBtoRGB565(snakeTrailColors[oldTailPos]));
      } else {
        // Non era orario: torna grigia
        gfx->setTextColor(convertColor(TextBackColor));
      }
      gfx->setCursor(pgm_read_word(&TFT_X[oldTailPos]), pgm_read_word(&TFT_Y[oldTailPos]));
      gfx->write(pgm_read_byte(&TFT_L[oldTailPos]));
    }

    // STEP 2: Disegna SOLO il corpo del serpente (8 lettere)
    for (uint8_t i = 0; i < snake.length; i++) {
      uint16_t pos = snake.segments[i].ledIndex;
      if (pos >= NUM_LEDS) continue;

      uint8_t hue = (baseHue + (i * 255 / snake.length)) % 255;
      Color segmentColor = hsvToRgb(hue, 255, 255);

      // Se questa posizione è una lettera dell'orario, marcala come attiva e salva il colore
      if (targetPixels[pos]) {
        activePixels[pos] = true;
        snakeTrailColors[pos].r = segmentColor.r;
        snakeTrailColors[pos].g = segmentColor.g;
        snakeTrailColors[pos].b = segmentColor.b;
      }

      gfx->setTextColor(convertColor(segmentColor));
      gfx->setCursor(pgm_read_word(&TFT_X[pos]), pgm_read_word(&TFT_Y[pos]));
      gfx->write(pgm_read_byte(&TFT_L[pos]));
    }
  }
}

// Funzione per memorizzare le posizioni dei LED che compongono una parola dell'orario.
// Queste posizioni vengono utilizzate per definire il percorso che il serpente dovrà seguire.
void memorizeTimeWord(const uint8_t* word) {
  if (!word) return; // Se il puntatore alla parola è nullo, esce.

  uint8_t idx = 0;   // Indice per scorrere i byte della parola.
  uint8_t pixel;   // Variabile per memorizzare l'indice del pixel corrente.

  // Scorre i byte della parola fino a quando non incontra il valore 4 (indicatore di fine parola).
  while ((pixel = pgm_read_byte(&word[idx])) != 4) {
    // Memorizza l'indice del LED nella struttura timeDisplay, se c'è ancora spazio.
    if (timeDisplay.count < MAX_TIME_LEDS) {
      timeDisplay.positions[timeDisplay.count++] = pixel; // Aggiunge la posizione e incrementa il contatore.
    }
    idx++; // Passa al byte successivo della parola.
  }
}

//===================================================================//
//                     MODALITA' MARIO BROS                          //
//===================================================================//

// Sprite Mario Bros ad alta definizione
// Risoluzione: 12 colonne x 16 righe, ogni blocco è 3x3 pixel = 36x48 pixel totali
// Dettagli: occhi neri, baffi, guanti bianchi, bottoni gialli, logo M sul cappello
// Codici colore: 0=trasparente, 1=rosso, 2=pelle, 3=blu, 4=marrone, 5=nero, 6=bianco, 7=giallo
const uint8_t MARIO_SPRITE_STAND[16][12] = {
  {0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0},  // Riga 0: Cappello - bordo
  {0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0},  // Riga 1: Cappello - rosso
  {0, 1, 1, 2, 1, 1, 1, 1, 2, 1, 1, 0},  // Riga 2: Cappello con logo M
  {0, 1, 2, 2, 2, 2, 2, 2, 2, 2, 1, 0},  // Riga 3: Bordo cappello/viso
  {0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0},  // Riga 4: Fronte
  {0, 0, 2, 5, 2, 2, 2, 2, 5, 2, 0, 0},  // Riga 5: Occhi
  {0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0},  // Riga 6: Viso centrale
  {0, 0, 2, 5, 5, 5, 5, 5, 5, 2, 0, 0},  // Riga 7: Baffi
  {0, 0, 1, 1, 1, 7, 7, 1, 1, 1, 0, 0},  // Riga 8: Camicia con bottoni
  {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},  // Riga 9: Camicia larga
  {6, 6, 1, 1, 1, 1, 1, 1, 1, 1, 6, 6},  // Riga 10: Camicia con guanti
  {0, 6, 6, 3, 3, 3, 3, 3, 3, 6, 6, 0},  // Riga 11: Guanti e pantaloni
  {0, 0, 3, 3, 3, 3, 3, 3, 3, 3, 0, 0},  // Riga 12: Pantaloni
  {0, 0, 3, 3, 3, 0, 0, 3, 3, 3, 0, 0},  // Riga 13: Gambe separate
  {0, 0, 4, 4, 4, 0, 0, 4, 4, 4, 0, 0},  // Riga 14: Scarpe
  {0, 0, 4, 4, 4, 0, 0, 4, 4, 4, 0, 0}   // Riga 15: Scarpe base
};

const uint8_t MARIO_SPRITE_WALK1[16][12] = {
  {0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0},  // Cappello
  {0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0},
  {0, 1, 1, 2, 1, 1, 1, 1, 2, 1, 1, 0},
  {0, 1, 2, 2, 2, 2, 2, 2, 2, 2, 1, 0},
  {0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0},
  {0, 0, 2, 5, 2, 2, 2, 2, 5, 2, 0, 0},  // Occhi
  {0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0},
  {0, 0, 2, 5, 5, 5, 5, 5, 5, 2, 0, 0},  // Baffi
  {0, 0, 1, 1, 1, 7, 7, 1, 1, 1, 0, 0},
  {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
  {6, 6, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0},  // Braccio sinistro avanti
  {0, 6, 6, 3, 3, 3, 3, 3, 3, 0, 0, 0},
  {0, 0, 3, 3, 3, 3, 3, 3, 0, 0, 0, 0},  // Gamba sinistra avanti
  {0, 0, 4, 4, 4, 0, 0, 0, 0, 3, 3, 0},  // Piede sinistro, gamba destra
  {0, 0, 4, 4, 4, 0, 0, 0, 0, 4, 4, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 4, 0}
};

const uint8_t MARIO_SPRITE_WALK2[16][12] = {
  {0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0},  // Cappello
  {0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0},
  {0, 1, 1, 2, 1, 1, 1, 1, 2, 1, 1, 0},
  {0, 1, 2, 2, 2, 2, 2, 2, 2, 2, 1, 0},
  {0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0},
  {0, 0, 2, 5, 2, 2, 2, 2, 5, 2, 0, 0},  // Occhi
  {0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0},
  {0, 0, 2, 5, 5, 5, 5, 5, 5, 2, 0, 0},  // Baffi
  {0, 0, 1, 1, 1, 7, 7, 1, 1, 1, 0, 0},
  {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
  {0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 6, 6},  // Braccio destro avanti
  {0, 0, 0, 3, 3, 3, 3, 3, 3, 6, 6, 0},
  {0, 0, 0, 0, 3, 3, 3, 3, 3, 3, 0, 0},  // Gamba destra avanti
  {0, 3, 3, 0, 0, 0, 0, 4, 4, 4, 0, 0},  // Gamba sinistra, piede destro
  {0, 4, 4, 0, 0, 0, 0, 4, 4, 4, 0, 0},
  {0, 4, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};

const uint8_t MARIO_SPRITE_JUMP[16][12] = {
  {0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0},  // Cappello
  {0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0},
  {0, 1, 1, 2, 1, 1, 1, 1, 2, 1, 1, 0},
  {0, 1, 2, 2, 2, 2, 2, 2, 2, 2, 1, 0},
  {0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0},
  {0, 0, 2, 5, 2, 2, 2, 2, 5, 2, 0, 0},  // Occhi
  {0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0},
  {0, 0, 2, 5, 5, 5, 5, 5, 5, 2, 0, 0},  // Baffi
  {6, 6, 1, 1, 1, 7, 7, 1, 1, 1, 6, 6},  // Braccia spalancate
  {6, 6, 1, 1, 1, 1, 1, 1, 1, 1, 6, 6},
  {0, 6, 1, 1, 1, 1, 1, 1, 1, 1, 6, 0},
  {0, 0, 6, 3, 3, 3, 3, 3, 3, 6, 0, 0},
  {0, 0, 0, 3, 3, 3, 3, 3, 3, 0, 0, 0},
  {0, 0, 0, 3, 3, 0, 0, 3, 3, 0, 0, 0},  // Gambe piegate
  {0, 0, 0, 4, 4, 0, 0, 4, 4, 0, 0, 0},
  {0, 0, 0, 4, 4, 0, 0, 4, 4, 0, 0, 0}
};

// Colori di Mario - molto brillanti per essere ben visibili
const Color MARIO_RED = Color(255, 0, 0);        // 1: Cappello e camicia (rosso brillante)
const Color MARIO_SKIN = Color(255, 220, 180);   // 2: Pelle (chiara e rosata)
const Color MARIO_BLUE = Color(0, 100, 255);     // 3: Pantaloni (blu elettrico)
const Color MARIO_BROWN = Color(140, 70, 0);     // 4: Scarpe (marrone scuro)
const Color MARIO_BLACK = Color(0, 0, 0);        // 5: Dettagli (occhi, baffi)
const Color MARIO_WHITE = Color(255, 255, 255);  // 6: Guanti (bianco puro)
const Color MARIO_YELLOW = Color(255, 220, 0);   // 7: Bottoni (giallo oro)
const Color MARIO_TRANSPARENT = Color(0, 0, 0);  // 0: Trasparente (nero = sfondo)

void updateMarioMode() {
  uint32_t currentMillis = millis();

  // Percorso zig-zag da alto-sinistra (come snake)
  static const uint16_t MARIO_ZIGZAG_PATH[256] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
    31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16,
    32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
    63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48,
    64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
    95, 94, 93, 92, 91, 90, 89, 88, 87, 86, 85, 84, 83, 82, 81, 80,
    96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
    127, 126, 125, 124, 123, 122, 121, 120, 119, 118, 117, 116, 115, 114, 113, 112,
    128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
    159, 158, 157, 156, 155, 154, 153, 152, 151, 150, 149, 148, 147, 146, 145, 144,
    160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175,
    191, 190, 189, 188, 187, 186, 185, 184, 183, 182, 181, 180, 179, 178, 177, 176,
    192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207,
    223, 222, 221, 220, 219, 218, 217, 216, 215, 214, 213, 212, 211, 210, 209, 208,
    224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
    255, 254, 253, 252, 251, 250, 249, 248, 247, 246, 245, 244, 243, 242, 241, 240
  };

  // Inizializzazione
  if (marioInitNeeded || currentHour != lastHour || currentMinute != lastMinute) {
    Serial.println("[MARIO] Inizializzazione modalità Mario Bros");

    marioInitNeeded = false;
    lastHour = currentHour;
    lastMinute = currentMinute;

    // Pulisce lo schermo
    gfx->fillScreen(BLACK);
    memset(activePixels, 0, sizeof(activePixels));
    memset(targetPixels, 0, sizeof(targetPixels));
    memset(pixelChanged, true, sizeof(pixelChanged));

    // Prepara lo sfondo con tutte le lettere in grigio scuro
    gfx->setFont(u8g2_font_inb21_mr);
    for (uint16_t i = 0; i < NUM_LEDS; i++) {
      gfx->setTextColor(convertColor(TextBackColor));
      gfx->setCursor(pgm_read_word(&TFT_X[i]), pgm_read_word(&TFT_Y[i]));
      gfx->write(pgm_read_byte(&TFT_L[i]));
    }

    // Imposta i pixel target per visualizzare l'ora corrente
    if (currentHour == 0) {
      strncpy(&TFT_L[6], "MEZZANOTTE", 10); // Aggiorna la parola "SONO LE" con "MEZZANOTTE".
      displayWordToTarget(WORD_MEZZANOTTE);
    } else {
      strncpy(&TFT_L[6], "EYOREXZERO", 10); // Ripristina la parola "SONO LE".
      displayWordToTarget(WORD_SONO_LE);
      const uint8_t* hourWord = (const uint8_t*)pgm_read_ptr(&HOUR_WORDS[currentHour]);
      displayWordToTarget(hourWord);
    }

    if (currentMinute > 0) {
      if (word_E_state == 0) displayWordToTarget(WORD_E);
      displayMinutesToTarget(currentMinute);
      displayWordToTarget(WORD_MINUTI);
    }

    // Inizializza Mario per seguire il percorso zig-zag
    mario.pathIndex = 0;  // Inizia dalla posizione 0 (alto sinistra)

    // Ottieni la prima posizione dal percorso
    uint16_t startPos = MARIO_ZIGZAG_PATH[0];
    int startMatrixX = startPos % MATRIX_WIDTH;
    int startMatrixY = startPos / MATRIX_WIDTH;

    // Converti in coordinate pixel
    mario.x = pgm_read_word(&TFT_X[startPos]);
    mario.y = pgm_read_word(&TFT_Y[startPos]) - 20;  // Leggermente sopra la lettera

    mario.velocityX = 0.0;
    mario.velocityY = 0.0;
    mario.animFrame = 0;
    mario.lastAnimUpdate = currentMillis;
    mario.lastMove = currentMillis;
    mario.facingRight = true;
    mario.isJumping = false;
    mario.isOnGround = true;
    mario.missionComplete = false;

    Serial.printf("[MARIO] Posizione iniziale: matrice(%d,%d) pixel(%.0f,%.0f)\n",
                  startMatrixX, startMatrixY, mario.x, mario.y);

    return;
  }

  // Se Mario ha completato la missione, mostra effetto finale
  if (mario.missionComplete) {
    // Effetto arcobaleno sulle lettere dell'orario
    static uint32_t lastRainbowUpdate = 0;
    static uint8_t rainbowOffset = 0;

    if (currentMillis - lastRainbowUpdate >= 80) {
      lastRainbowUpdate = currentMillis;
      rainbowOffset = (rainbowOffset + 3) % 255;

      gfx->setFont(u8g2_font_inb21_mr);
      for (uint16_t pos = 0; pos < NUM_LEDS; pos++) {
        if (targetPixels[pos] && activePixels[pos]) {
          uint8_t hue = (rainbowOffset + (pos * 8)) % 255;
          Color rainbowColor = hsvToRgb(hue, 255, 255);
          gfx->setTextColor(convertColor(rainbowColor));
          gfx->setCursor(pgm_read_word(&TFT_X[pos]), pgm_read_word(&TFT_Y[pos]));
          gfx->write(pgm_read_byte(&TFT_L[pos]));
        }
      }
    }
    return;
  }

  // Aggiornamento animazione più veloce per camminata veloce (ogni 100ms)
  if (currentMillis - mario.lastAnimUpdate >= 100) {
    mario.lastAnimUpdate = currentMillis;
    mario.animFrame = (mario.animFrame + 1) % 4;
  }

  // Aggiornamento movimento (ogni 30ms per completare in ~10 secondi)
  if (currentMillis - mario.lastMove >= 30) {
    mario.lastMove = currentMillis;

    // Salva posizione precedente (coordinate pixel)
    int oldX = (int)mario.x;
    int oldY = (int)mario.y;

    // Controlla se Mario ha completato il percorso
    if (mario.pathIndex >= 256) {
      mario.missionComplete = true;
      Serial.println("[MARIO] Percorso completato!");
      return;
    }

    // Ottieni la posizione target corrente dal percorso
    uint16_t currentMatrixPos = MARIO_ZIGZAG_PATH[mario.pathIndex];
    int targetPixelX = pgm_read_word(&TFT_X[currentMatrixPos]);
    int targetPixelY = pgm_read_word(&TFT_Y[currentMatrixPos]) - 20;

    // Calcola direzione verso il target
    float dx = targetPixelX - mario.x;
    float dy = targetPixelY - mario.y;
    float distance = sqrt(dx * dx + dy * dy);

    // Velocità di movimento
    float speed = 22.0;  // pixel per frame (completa in ~10 secondi)

    // Debug periodico (ogni 20 waypoints)
    if (mario.pathIndex % 20 == 0) {
      Serial.printf("[MARIO] pathIdx=%d pos=(%.0f,%.0f) target=(%d,%d) dist=%.1f\n",
                    mario.pathIndex, mario.x, mario.y, targetPixelX, targetPixelY, distance);
    }

    // Se Mario raggiungerebbe o supererebbe il target in questo frame, snap al target
    if (distance <= speed) {
      // Snap direttamente alla posizione target
      mario.x = targetPixelX;
      mario.y = targetPixelY;

      // Avanza al waypoint successivo
      mario.pathIndex++;

      // Determina direzione per animazione
      if (mario.pathIndex < 256) {
        uint16_t nextMatrixPos = MARIO_ZIGZAG_PATH[mario.pathIndex];
        int nextX = pgm_read_word(&TFT_X[nextMatrixPos]);
        mario.facingRight = (nextX > targetPixelX);
      }
    } else {
      // Muovi Mario verso il target con velocità ALTA per completare in ~10 secondi
      // Calcolo: 256 posizioni * ~30 pixel/pos = ~7680 pixel totali
      // 7680 pixel / 10 secondi = 768 pixel/sec
      // Con 30ms/frame (33.3 fps): 768 / 33.3 = ~23 pixel/frame
      mario.x += (dx / distance) * speed;
      mario.y += (dy / distance) * speed;
    }

    // Controlla collisione con lettere dell'orario
    // Verifica tutte le lettere target e attiva quelle che Mario tocca REALMENTE
    // Bounding box Mario HD: 36 pixel largo x 48 pixel alto (12x16 blocchi da 3x3)
    // Piccolo margine di 8 pixel per compensare movimento veloce
    const int COLLISION_MARGIN = 8;

    for (uint16_t i = 0; i < NUM_LEDS; i++) {
      if (targetPixels[i] && !activePixels[i]) {
        // Ottieni coordinate pixel della lettera
        int16_t letterX = pgm_read_word(&TFT_X[i]);
        int16_t letterY = pgm_read_word(&TFT_Y[i]);

        // Bounding box Mario con piccolo margine:
        // [mario.x - MARGIN, mario.y - MARGIN] a [mario.x + 36 + MARGIN, mario.y + 48 + MARGIN]
        // Bounding box lettera: [letterX, letterY-24] a [letterX+24, letterY+4]

        if (mario.x - COLLISION_MARGIN < letterX + 24 &&
            mario.x + 36 + COLLISION_MARGIN > letterX &&
            mario.y - COLLISION_MARGIN < letterY + 4 &&
            mario.y + 48 + COLLISION_MARGIN > letterY - 24) {
          // Collisione! Attiva la lettera
          activePixels[i] = true;
          gfx->setFont(u8g2_font_inb21_mr);
          gfx->setTextColor(convertColor(currentColor));
          gfx->setCursor(letterX, letterY);
          gfx->write(pgm_read_byte(&TFT_L[i]));
        }
      }
    }

    // Cancella la vecchia posizione di Mario
    eraseMarioSprite(oldX, oldY);

    // Disegna Mario nella nuova posizione
    if ((int)mario.x >= -40 && (int)mario.x < 520) {
      drawMarioSprite((int)mario.x, (int)mario.y, mario.animFrame);
    }
  }
}

// Cancella lo sprite di Mario dalla vecchia posizione ridisegnando le lettere
void eraseMarioSprite(int x, int y) {
  // Sprite Mario HD: 36 pixel largo x 48 pixel alto (12x16 blocchi da 3x3)
  // Bounding box Mario: [x, y] a [x+36, y+48]

  // STEP 1: Cancella completamente l'area con nero (leggermente più grande per sicurezza)
  // Espandiamo di 2 pixel in tutte le direzioni per evitare artefatti
  if (x >= -40 && x < 520 && y >= 0 && y < 480) {
    gfx->fillRect(x - 2, y - 2, 40, 52, BLACK);
  }

  // STEP 2: Ridisegna tutte le lettere che erano coperte da Mario (con area espansa)
  gfx->setFont(u8g2_font_inb21_mr);

  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    // Ottieni coordinate pixel della lettera
    int16_t letterX = pgm_read_word(&TFT_X[i]);
    int16_t letterY = pgm_read_word(&TFT_Y[i]);

    // Bounding box lettera: [letterX, letterY-24] a [letterX+24, letterY+4]
    // Controlla se la lettera interseca con l'area ESPANSA di Mario
    if (x - 2 < letterX + 24 && x + 38 > letterX &&
        y - 2 < letterY + 4 && y + 50 > letterY - 24) {

      // La lettera era coperta da Mario, ridisegnala
      if (activePixels[i] && targetPixels[i]) {
        // Lettera dell'orario già attivata - colore orario
        gfx->setTextColor(convertColor(currentColor));
      } else {
        // Lettera normale - grigio
        gfx->setTextColor(convertColor(TextBackColor));
      }

      gfx->setCursor(letterX, letterY);
      gfx->write(pgm_read_byte(&TFT_L[i]));
    }
  }
}

// Ottiene il colore in base al codice dello sprite
Color getMarioSpriteColor(uint8_t colorCode) {
  switch(colorCode) {
    case 1: return MARIO_RED;
    case 2: return MARIO_SKIN;
    case 3: return MARIO_BLUE;
    case 4: return MARIO_BROWN;
    case 5: return MARIO_BLACK;
    case 6: return MARIO_WHITE;
    case 7: return MARIO_YELLOW;
    default: return Color(0, 0, 0);
  }
}

// Disegna lo sprite di Mario usando fillRect con coordinate pixel assolute
void drawMarioSprite(int x, int y, uint8_t animFrame) {
  // Seleziona lo sprite corretto (12 colonne x 16 righe)
  const uint8_t (*sprite)[12];

  if (mario.isJumping) {
    sprite = MARIO_SPRITE_JUMP;
  } else {
    if (animFrame % 3 == 0) {
      sprite = MARIO_SPRITE_STAND;
    } else if (animFrame % 3 == 1) {
      sprite = MARIO_SPRITE_WALK1;
    } else {
      sprite = MARIO_SPRITE_WALK2;
    }
  }

  // Disegna lo sprite Mario 12x16 blocchi (ogni blocco 3x3 pixel = 36x48 pixel totali)
  const int BLOCK_SIZE = 3;

  for (int row = 0; row < 16; row++) {
    for (int col = 0; col < 12; col++) {
      uint8_t colorCode = sprite[row][col];

      if (colorCode != 0) {  // 0 = trasparente
        Color pixelColor = getMarioSpriteColor(colorCode);
        int pixelX = x + (col * BLOCK_SIZE);
        int pixelY = y + (row * BLOCK_SIZE);

        // Disegna blocco 3x3 pixel
        gfx->fillRect(pixelX, pixelY, BLOCK_SIZE, BLOCK_SIZE, convertColor(pixelColor));
      }
    }
  }
}


//===================================================================//
//                     MODALITA' TRON                                //
//===================================================================//

void updateTron() {
  static bool firstRender = true;  // Flag per il primo rendering

  // Inizializzazione o reinizializzazione quando cambia l'ora
  if (!tronInitialized || currentHour != lastHour || currentMinute != lastMinute) {
    Serial.println("[TRON] Inizializzazione effetto Tron");
    gfx->fillScreen(BLACK);

    // Azzera la griglia e activePixels
    memset(tronGrid, 0, sizeof(tronGrid));
    memset(activePixels, 0, sizeof(activePixels));
    memset(targetPixels, 0, sizeof(targetPixels));

    firstRender = true;  // Reset flag per ridisegnare tutto

    // Imposta i pixel target per visualizzare l'ora corrente
    if (currentHour == 0) {
      strncpy(&TFT_L[6], "MEZZANOTTE", 10); // Aggiorna la parola "SONO LE" con "MEZZANOTTE".
      displayWordToTarget(WORD_MEZZANOTTE);
    } else {
      strncpy(&TFT_L[6], "EYOREXZERO", 10); // Ripristina la parola "SONO LE".
      displayWordToTarget(WORD_SONO_LE);
      const uint8_t* hourWord = (const uint8_t*)pgm_read_ptr(&HOUR_WORDS[currentHour]);
      displayWordToTarget(hourWord);
    }

    // Imposta i pixel target per visualizzare i minuti, se maggiori di zero
    if (currentMinute > 0) {
      if (word_E_state == 0) displayWordToTarget(WORD_E);
      displayMinutesToTarget(currentMinute);
      displayWordToTarget(WORD_MINUTI);
    }

    // Salva l'ora corrente
    lastHour = currentHour;
    lastMinute = currentMinute;

    // Inizializza le moto in posizioni diverse con colori diversi
    Color colors[NUM_TRON_BIKES] = {
      Color(0, 150, 255),    // Blu ciano (classico Tron)
      Color(255, 100, 0),    // Arancione (avversario)
      Color(0, 255, 0)       // Verde
    };

    for (int i = 0; i < NUM_TRON_BIKES; i++) {
      tronBikes[i].x = random(0, MATRIX_WIDTH);
      tronBikes[i].y = random(0, MATRIX_HEIGHT);
      tronBikes[i].direction = random(0, 4);
      tronBikes[i].color = colors[i];
      tronBikes[i].active = true;
      tronBikes[i].crashed = false;
      tronBikes[i].lastMove = millis();
      tronBikes[i].crashTime = 0;
      tronBikes[i].trailLength = 0;  // Inizializza scia vuota
      memset(tronBikes[i].trail, 0, sizeof(tronBikes[i].trail));
    }

    // Disegna TUTTE le lettere in grigio come sfondo iniziale
    gfx->setFont(u8g2_font_inb21_mr);
    for (uint16_t i = 0; i < NUM_LEDS; i++) {
      gfx->setTextColor(convertColor(TextBackColor));
      gfx->setCursor(pgm_read_word(&TFT_X[i]), pgm_read_word(&TFT_Y[i]));
      gfx->write(pgm_read_byte(&TFT_L[i]));
    }

    tronInitialized = true;
    Serial.println("[TRON] Inizializzazione completata - sfondo grigio disegnato");
    return;  // Esce dopo l'inizializzazione, il rendering inizierà dal prossimo frame
  }

  uint32_t currentTime = millis();

  // Conta quanti pixel dell'orario sono stati illuminati
  uint16_t targetCount = 0;
  uint16_t activeCount = 0;
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    if (targetPixels[i]) {
      targetCount++;
      if (activePixels[i]) activeCount++;
    }
  }

  // Se l'orario è completo, ferma le moto
  bool timeComplete = (targetCount > 0 && activeCount >= targetCount);

  // Aggiorna ogni moto SOLO se l'orario non è completo
  if (!timeComplete) {
    // Prima di muovere le moto, ricostruisci la griglia con tutte le scie
    memset(tronGrid, 0, sizeof(tronGrid));
    for (int i = 0; i < NUM_TRON_BIKES; i++) {
      if (!tronBikes[i].active) continue;

      // Aggiungi tutte le posizioni della scia alla griglia
      for (int t = 0; t < tronBikes[i].trailLength; t++) {
        uint16_t trailPos = tronBikes[i].trail[t];
        uint8_t tx = trailPos % MATRIX_WIDTH;
        uint8_t ty = trailPos / MATRIX_WIDTH;
        tronGrid[ty][tx] = i + 1;  // Marca la cella come occupata dalla moto i
      }

      // Aggiungi anche la posizione corrente della moto
      tronGrid[tronBikes[i].y][tronBikes[i].x] = i + 1;
    }

    for (int i = 0; i < NUM_TRON_BIKES; i++) {
      TronBike& bike = tronBikes[i];

      if (!bike.active) continue;

      // Controlla se è ora di muoversi
      if (currentTime - bike.lastMove < TRON_SPEED) continue;
      bike.lastMove = currentTime;

      // Trova il pixel target più vicino non ancora illuminato
      int16_t targetX = -1, targetY = -1;
      float minDist = 9999;

      for (uint16_t pos = 0; pos < NUM_LEDS; pos++) {
        if (targetPixels[pos] && !activePixels[pos]) {
          uint8_t px = pos % MATRIX_WIDTH;
          uint8_t py = pos / MATRIX_WIDTH;
          float dist = sqrt(pow(px - bike.x, 2) + pow(py - bike.y, 2));

          if (dist < minDist) {
            minDist = dist;
            targetX = px;
            targetY = py;
          }
        }
      }

      uint8_t nextX = bike.x;
      uint8_t nextY = bike.y;

      // Se c'è un target, muoviti verso di esso
      if (targetX >= 0 && targetY >= 0) {
        // Determina la direzione migliore
        int dx = targetX - bike.x;
        int dy = targetY - bike.y;

        // Scegli se muoverti in X o Y in base alla distanza maggiore
        if (abs(dx) > abs(dy)) {
          // Muovi in orizzontale
          if (dx > 0 && bike.x < MATRIX_WIDTH - 1) {
            nextX = bike.x + 1;
            bike.direction = 1; // Right
          } else if (dx < 0 && bike.x > 0) {
            nextX = bike.x - 1;
            bike.direction = 3; // Left
          }
        } else {
          // Muovi in verticale
          if (dy > 0 && bike.y < MATRIX_HEIGHT - 1) {
            nextY = bike.y + 1;
            bike.direction = 2; // Down
          } else if (dy < 0 && bike.y > 0) {
            nextY = bike.y - 1;
            bike.direction = 0; // Up
          }
        }
      } else {
        // Nessun target trovato - movimento casuale
        bike.direction = random(0, 4);
        switch (bike.direction) {
          case 0: if (bike.y > 0) nextY = bike.y - 1; break;
          case 1: if (bike.x < MATRIX_WIDTH - 1) nextX = bike.x + 1; break;
          case 2: if (bike.y < MATRIX_HEIGHT - 1) nextY = bike.y + 1; break;
          case 3: if (bike.x > 0) nextX = bike.x - 1; break;
        }
      }

      // CONTROLLO COLLISIONE: se la prossima posizione è occupata, trova un percorso alternativo
      if (tronGrid[nextY][nextX] != 0) {
        // La posizione è occupata! Prova direzioni alternative
        bool foundAlternative = false;

        // Array delle 4 direzioni possibili
        int8_t directions[4][2] = {{0, -1}, {1, 0}, {0, 1}, {-1, 0}}; // Up, Right, Down, Left

        // Prova tutte le direzioni partendo da quella corrente
        for (int d = 0; d < 4; d++) {
          uint8_t testX = bike.x + directions[d][0];
          uint8_t testY = bike.y + directions[d][1];

          // Verifica bounds
          if (testX >= 0 && testX < MATRIX_WIDTH && testY >= 0 && testY < MATRIX_HEIGHT) {
            // Verifica se la cella è libera
            if (tronGrid[testY][testX] == 0) {
              nextX = testX;
              nextY = testY;
              bike.direction = d;
              foundAlternative = true;
              break;
            }
          }
        }

        // Se non c'è via d'uscita, resta fermo
        if (!foundAlternative) {
          nextX = bike.x;
          nextY = bike.y;
        }
      }

      // Aggiungi posizione corrente alla scia PRIMA di muovere
      uint16_t currentPos = (bike.y * MATRIX_WIDTH) + bike.x;
      if (bike.trailLength < TRON_TRAIL_LENGTH) {
        bike.trail[bike.trailLength] = currentPos;
        bike.trailLength++;
      } else {
        // Sposta tutte le posizioni indietro di 1 (rimuovi la più vecchia)
        for (int t = 0; t < TRON_TRAIL_LENGTH - 1; t++) {
          bike.trail[t] = bike.trail[t + 1];
        }
        bike.trail[TRON_TRAIL_LENGTH - 1] = currentPos;
      }

      // Muovi la moto
      bike.x = nextX;
      bike.y = nextY;

      // Calcola posizione nel buffer
      uint16_t pos = (bike.y * MATRIX_WIDTH) + bike.x;

      // Se la moto passa su un pixel dell'orario, illuminalo
      if (targetPixels[pos]) {
        activePixels[pos] = true;
        tronGrid[bike.y][bike.x] = i + 1;
      }
    }
  }  // Fine aggiornamento moto

  // RENDERING OTTIMIZZATO - Ridisegna solo pixel cambiati
  gfx->setFont(u8g2_font_inb21_mr);

  // Array per tracciare quali posizioni sono occupate ADESSO (frame corrente)
  static uint8_t currentOccupied[NUM_LEDS];
  static uint8_t previousOccupied[NUM_LEDS];

  // Se è il primo render, disegna tutto lo sfondo
  if (firstRender) {
    for (uint16_t pos = 0; pos < NUM_LEDS; pos++) {
      gfx->setTextColor(convertColor(TextBackColor));
      gfx->setCursor(pgm_read_word(&TFT_X[pos]), pgm_read_word(&TFT_Y[pos]));
      gfx->write(pgm_read_byte(&TFT_L[pos]));
    }
    firstRender = false;
    memset(previousOccupied, 0, sizeof(previousOccupied));
  }

  // Reset array corrente
  memset(currentOccupied, 0, sizeof(currentOccupied));

  // Marca pixel dell'orario già illuminati come permanenti
  for (uint16_t pos = 0; pos < NUM_LEDS; pos++) {
    if (targetPixels[pos] && activePixels[pos]) {
      uint8_t x = pos % MATRIX_WIDTH;
      uint8_t y = pos / MATRIX_WIDTH;
      int bikeIndex = tronGrid[y][x] - 1;

      // Se orario completo, forza ridisegno per effetto rainbow
      if (timeComplete) {
        // Usa valore che cambia ogni frame per forzare ridisegno
        currentOccupied[pos] = 200 + ((millis() / 50) % 2);
      } else {
        if (bikeIndex >= 0 && bikeIndex < NUM_TRON_BIKES) {
          currentOccupied[pos] = bikeIndex + 1;
        } else {
          currentOccupied[pos] = 255;  // Orario senza moto specifica
        }
      }
    }
  }

  // Marca posizioni delle scie e moto (se orario non completo)
  if (!timeComplete) {
    for (int i = 0; i < NUM_TRON_BIKES; i++) {
      if (!tronBikes[i].active) continue;

      // Scie
      for (int t = 0; t < tronBikes[i].trailLength; t++) {
        uint16_t trailPos = tronBikes[i].trail[t];
        if (trailPos < NUM_LEDS && currentOccupied[trailPos] == 0) {
          currentOccupied[trailPos] = i + 1;
        }
      }

      // Moto
      uint16_t bikePos = (tronBikes[i].y * MATRIX_WIDTH) + tronBikes[i].x;
      if (bikePos < NUM_LEDS) {
        currentOccupied[bikePos] = i + 1;
      }
    }
  }

  // STEP 1: Cancella pixel che non sono più occupati
  for (uint16_t pos = 0; pos < NUM_LEDS; pos++) {
    if (previousOccupied[pos] != 0 && currentOccupied[pos] == 0) {
      // Questo pixel era occupato, ora non lo è più
      gfx->setTextColor(convertColor(TextBackColor));
      gfx->setCursor(pgm_read_word(&TFT_X[pos]), pgm_read_word(&TFT_Y[pos]));
      gfx->write(pgm_read_byte(&TFT_L[pos]));
    }
  }

  // STEP 2: Disegna pixel dell'orario illuminati
  for (uint16_t pos = 0; pos < NUM_LEDS; pos++) {
    if (targetPixels[pos] && activePixels[pos]) {
      uint8_t x = pos % MATRIX_WIDTH;
      uint8_t y = pos / MATRIX_WIDTH;
      int bikeIndex = tronGrid[y][x] - 1;

      // Se orario completo, usa effetto rainbow ciclico
      if (timeComplete) {
        static uint8_t rainbowOffset = 0;

        // Aggiorna offset rainbow ogni frame per animazione fluida
        rainbowOffset = (millis() / 20) % 255;

        // Calcola hue basato su offset e posizione per effetto scorrente
        uint8_t hue = (rainbowOffset + (pos * 8)) % 255;
        Color rainbowColor = hsvToRgb(hue, 255, 255);
        gfx->setTextColor(convertColor(rainbowColor));
      } else {
        // Durante il completamento, usa colore della moto
        if (bikeIndex >= 0 && bikeIndex < NUM_TRON_BIKES) {
          gfx->setTextColor(convertColor(tronBikes[bikeIndex].color));
        } else {
          gfx->setTextColor(convertColor(Color(0, 150, 255)));
        }
      }

      gfx->setCursor(pgm_read_word(&TFT_X[pos]), pgm_read_word(&TFT_Y[pos]));
      gfx->write(pgm_read_byte(&TFT_L[pos]));
    }
  }

  // STEP 3: Disegna scie delle moto con effetto dissolvenza (se orario non completo)
  if (!timeComplete) {
    for (int i = 0; i < NUM_TRON_BIKES; i++) {
      if (!tronBikes[i].active) continue;

      for (int t = 0; t < tronBikes[i].trailLength; t++) {
        uint16_t trailPos = tronBikes[i].trail[t];
        if (trailPos >= NUM_LEDS) continue;

        // Salta se questa posizione è dell'orario (già disegnata)
        if (targetPixels[trailPos] && activePixels[trailPos]) continue;

        float fadeFactor = (float)(t + 1) / (float)TRON_TRAIL_LENGTH;
        Color trailColor = tronBikes[i].color;
        trailColor.r = (uint8_t)(trailColor.r * fadeFactor);
        trailColor.g = (uint8_t)(trailColor.g * fadeFactor);
        trailColor.b = (uint8_t)(trailColor.b * fadeFactor);

        gfx->setTextColor(convertColor(trailColor));
        gfx->setCursor(pgm_read_word(&TFT_X[trailPos]), pgm_read_word(&TFT_Y[trailPos]));
        gfx->write(pgm_read_byte(&TFT_L[trailPos]));
      }
    }

    // STEP 4: Disegna le moto con colore brillante
    for (int i = 0; i < NUM_TRON_BIKES; i++) {
      if (!tronBikes[i].active) continue;

      uint16_t pos = (tronBikes[i].y * MATRIX_WIDTH) + tronBikes[i].x;
      if (pos >= NUM_LEDS) continue;

      Color brightColor = tronBikes[i].color;
      brightColor.r = min(255, brightColor.r + 100);
      brightColor.g = min(255, brightColor.g + 100);
      brightColor.b = min(255, brightColor.b + 100);

      gfx->setTextColor(convertColor(brightColor));
      gfx->setCursor(pgm_read_word(&TFT_X[pos]), pgm_read_word(&TFT_Y[pos]));
      gfx->write(pgm_read_byte(&TFT_L[pos]));
    }
  }

  // Salva stato corrente per il prossimo frame
  memcpy(previousOccupied, currentOccupied, sizeof(currentOccupied));
}

// ==================== MODALITÀ GALAGA ====================
// Sprite dell'astronave Galaga (10 colonne x 12 righe, ogni blocco è 3x3 pixel = 30x36 pixel totali)
// Codici colore: 0=trasparente, 1=rosso, 2=bianco, 3=giallo, 4=blu, 5=ciano
const uint8_t GALAGA_SHIP_FRAME1[12][10] = {
  {0, 0, 0, 0, 2, 2, 0, 0, 0, 0},  // Riga 0: Antenne superiori
  {0, 0, 0, 4, 4, 4, 4, 0, 0, 0},  // Riga 1: Testa blu
  {0, 0, 4, 4, 3, 3, 4, 4, 0, 0},  // Riga 2: Testa con occhi gialli
  {0, 4, 4, 3, 3, 3, 3, 4, 4, 0},  // Riga 3: Testa larga
  {0, 4, 2, 2, 4, 4, 2, 2, 4, 0},  // Riga 4: Dettagli bianchi
  {4, 4, 4, 4, 4, 4, 4, 4, 4, 4},  // Riga 5: Corpo centrale
  {4, 1, 4, 4, 5, 5, 4, 4, 1, 4},  // Riga 6: Corpo con dettagli rossi e ciano
  {4, 1, 1, 4, 5, 5, 4, 1, 1, 4},  // Riga 7: Più dettagli rossi
  {0, 4, 1, 4, 4, 4, 4, 1, 4, 0},  // Riga 8: Ali con rosso
  {0, 0, 4, 4, 0, 0, 4, 4, 0, 0},  // Riga 9: Ali esterne
  {0, 0, 2, 0, 0, 0, 0, 2, 0, 0},  // Riga 10: Propulsori
  {0, 0, 2, 0, 0, 0, 0, 2, 0, 0}   // Riga 11: Propulsori fiamma
};

const uint8_t GALAGA_SHIP_FRAME2[12][10] = {
  {0, 0, 0, 0, 2, 2, 0, 0, 0, 0},  // Riga 0: Antenne superiori
  {0, 0, 0, 4, 4, 4, 4, 0, 0, 0},  // Riga 1: Testa blu
  {0, 0, 4, 4, 3, 3, 4, 4, 0, 0},  // Riga 2: Testa con occhi gialli
  {0, 4, 4, 3, 3, 3, 3, 4, 4, 0},  // Riga 3: Testa larga
  {0, 4, 2, 2, 4, 4, 2, 2, 4, 0},  // Riga 4: Dettagli bianchi
  {4, 4, 4, 4, 4, 4, 4, 4, 4, 4},  // Riga 5: Corpo centrale
  {4, 1, 4, 4, 5, 5, 4, 4, 1, 4},  // Riga 6: Corpo con dettagli rossi e ciano
  {4, 1, 1, 4, 5, 5, 4, 1, 1, 4},  // Riga 7: Più dettagli rossi
  {0, 4, 1, 4, 4, 4, 4, 1, 4, 0},  // Riga 8: Ali con rosso
  {0, 0, 4, 4, 0, 0, 4, 4, 0, 0},  // Riga 9: Ali esterne
  {0, 3, 3, 0, 0, 0, 0, 3, 3, 0},  // Riga 10: Propulsori gialli (animazione)
  {0, 3, 3, 0, 0, 0, 0, 3, 3, 0}   // Riga 11: Propulsori fiamma gialla
};

// Colori dell'astronave Galaga
const Color GALAGA_RED = Color(255, 0, 0);       // 1: Rosso
const Color GALAGA_WHITE = Color(255, 255, 255); // 2: Bianco
const Color GALAGA_YELLOW = Color(255, 220, 0);  // 3: Giallo
const Color GALAGA_BLUE = Color(0, 100, 255);    // 4: Blu
const Color GALAGA_CYAN = Color(0, 255, 255);    // 5: Ciano

// Funzione helper per ottenere il colore dallo sprite code
Color getGalagaSpriteColor(uint8_t colorCode) {
  switch(colorCode) {
    case 1: return GALAGA_RED;
    case 2: return GALAGA_WHITE;
    case 3: return GALAGA_YELLOW;
    case 4: return GALAGA_BLUE;
    case 5: return GALAGA_CYAN;
    default: return Color(0, 0, 0);
  }
}

// Disegna l'astronave Galaga
void drawGalagaShip(int x, int y, uint8_t animFrame) {
  const uint8_t (*sprite)[10] = (animFrame % 2 == 0) ? GALAGA_SHIP_FRAME1 : GALAGA_SHIP_FRAME2;

  for (int row = 0; row < 12; row++) {
    for (int col = 0; col < 10; col++) {
      uint8_t colorCode = sprite[row][col];
      if (colorCode == 0) continue; // Trasparente

      Color pixelColor = getGalagaSpriteColor(colorCode);

      // Disegna blocco 3x3 pixel
      for (int dy = 0; dy < 3; dy++) {
        for (int dx = 0; dx < 3; dx++) {
          int px = x + (col * 3) + dx;
          int py = y + (row * 3) + dy;
          if (px >= 0 && px < 480 && py >= 0 && py < 480) {
            gfx->drawPixel(px, py, convertColor(pixelColor));
          }
        }
      }
    }
  }
}

// Cancella l'astronave ridisegnando lo sfondo
void eraseGalagaShip(int x, int y) {
  // Sprite Galaga: 30 pixel largo x 36 pixel alto (10x12 blocchi da 3x3)
  // Bounding box Galaga: [x, y] a [x+30, y+36]

  // STEP 1: Cancella completamente l'area con nero (leggermente più grande per sicurezza)
  // Espandiamo di 2 pixel in tutte le direzioni per evitare artefatti
  if (x >= -32 && x < 512 && y >= 0 && y < 480) {
    gfx->fillRect(x - 2, y - 2, 34, 40, BLACK);
  }

  // STEP 2: Ridisegna tutte le lettere che erano coperte dall'astronave (con area espansa)
  gfx->setFont(u8g2_font_inb21_mr);

  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    // Ottieni coordinate pixel della lettera
    int16_t letterX = pgm_read_word(&TFT_X[i]);
    int16_t letterY = pgm_read_word(&TFT_Y[i]);

    // Bounding box lettera: [letterX, letterY-24] a [letterX+24, letterY+4]
    // Controlla se la lettera interseca con l'area ESPANSA dell'astronave
    if (x - 2 < letterX + 24 && x + 32 > letterX &&
        y - 2 < letterY + 4 && y + 38 > letterY - 24) {

      // La lettera era coperta dall'astronave, ridisegnala
      if (activePixels[i] && targetPixels[i]) {
        // Lettera dell'orario già attivata - colore orario
        gfx->setTextColor(convertColor(currentColor));
      } else {
        // Lettera normale - grigio
        gfx->setTextColor(convertColor(TextBackColor));
      }

      gfx->setCursor(letterX, letterY);
      gfx->write(pgm_read_byte(&TFT_L[i]));
    }
  }
}

// Disegna un proiettile
void drawBullet(int x, int y) {
  // Proiettile 2x6 pixel (rosso brillante)
  Color bulletColor = Color(255, 255, 0); // Giallo brillante
  for (int dy = 0; dy < 6; dy++) {
    for (int dx = 0; dx < 2; dx++) {
      int px = x + dx;
      int py = y + dy;
      if (px >= 0 && px < 480 && py >= 0 && py < 480) {
        gfx->drawPixel(px, py, convertColor(bulletColor));
      }
    }
  }
}

// Cancella un proiettile
void eraseBullet(int x, int y) {
  // Proiettile: 2 pixel largo x 6 pixel alto
  // Bounding box: [x, y] a [x+2, y+6]

  // STEP 1: Cancella completamente l'area con nero
  if (x >= 0 && x < 480 && y >= 0 && y < 480) {
    gfx->fillRect(x, y, 2, 6, BLACK);
  }

  // STEP 2: Ridisegna eventuali lettere coperte
  gfx->setFont(u8g2_font_inb21_mr);
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    int16_t letterX = pgm_read_word(&TFT_X[i]);
    int16_t letterY = pgm_read_word(&TFT_Y[i]);

    // Bounding box lettera: [letterX, letterY-24] a [letterX+24, letterY+4]
    // Controlla se la lettera interseca con l'area del proiettile
    if (x < letterX + 24 && x + 2 > letterX &&
        y < letterY + 4 && y + 6 > letterY - 24) {

      // La lettera era coperta dal proiettile, ridisegnala
      if (activePixels[i] && targetPixels[i]) {
        // Lettera dell'orario già attivata - colore orario
        gfx->setTextColor(convertColor(currentColor));
      } else {
        // Lettera normale - grigio
        gfx->setTextColor(convertColor(TextBackColor));
      }

      gfx->setCursor(letterX, letterY);
      gfx->write(pgm_read_byte(&TFT_L[i]));
    }
  }
}

// Aggiorna e disegna le stelle dello sfondo di Galaga
void updateGalagaStars() {
  // Aggiorna la posizione e disegna tutte le stelle direttamente sul display
  for (int i = 0; i < MAX_GALAGA_STARS; i++) {
    // Salva posizione precedente
    int oldX = (int)galagaStars[i].x;
    int oldY = (int)galagaStars[i].y;

    // Muove la stella verso sinistra (effetto scorrimento spaziale)
    galagaStars[i].x -= galagaStars[i].speed;

    // Se la stella esce dallo schermo a sinistra, riposizionala a destra
    if (galagaStars[i].x < -galagaStars[i].size) {
      galagaStars[i].x = 480;
      galagaStars[i].y = random(480);
      galagaStars[i].speed = 0.3 + (random(20) / 10.0);
      galagaStars[i].brightness = 100 + random(156);
    }

    // Nuova posizione
    int newX = (int)galagaStars[i].x;
    int newY = (int)galagaStars[i].y;

    // Cancella la vecchia posizione solo se si è effettivamente spostata
    // Ridisegna con BLACK (le lettere vengono ridisegnate periodicamente)
    if (oldX >= 0 && oldX < 480 && oldY >= 0 && oldY < 480 && (oldX != newX || oldY != newY)) {
      if (galagaStars[i].size == 1) {
        gfx->drawPixel(oldX, oldY, BLACK);
      } else {
        gfx->fillRect(oldX, oldY, 2, 2, BLACK);
      }
    }

    // Disegna la stella nella nuova posizione
    if (newX >= 0 && newX < 480 && newY >= 0 && newY < 480) {
      uint16_t starColor = gfx->color565(galagaStars[i].brightness,
                                          galagaStars[i].brightness,
                                          galagaStars[i].brightness);

      if (galagaStars[i].size == 1) {
        gfx->drawPixel(newX, newY, starColor);
      } else {
        gfx->fillRect(newX, newY, 2, 2, starColor);
      }
    }
  }
}
// Funzione principale modalità Galaga
void updateGalagaMode() {
  uint32_t currentMillis = millis();
  static bool cannonErased = false; // Flag per cancellare il cannone una sola volta
  static int lastLineEndX = 0;
  static int lastLineEndY = 0;
  static bool firstDraw = true;

  // Inizializzazione
  if (galagaInitNeeded || currentHour != lastHour || currentMinute != lastMinute) {
    Serial.println("[GALAGA] Inizializzazione modalità Galaga");

    galagaInitNeeded = false;
    cannonErased = false; // Reset flag per permettere cancellazione del nuovo cannone
    firstDraw = true;     // Reset flag per il disegno della linea
    lastHour = currentHour;
    lastMinute = currentMinute;

    // Pulisce lo schermo
    gfx->fillScreen(BLACK);
    memset(activePixels, 0, sizeof(activePixels));
    memset(targetPixels, 0, sizeof(targetPixels));
    memset(pixelChanged, true, sizeof(pixelChanged));

    // IMPORTANTE: Aggiorna TFT_L PRIMA di scrivere i caratteri sullo schermo
    if (currentHour == 0) {
      strncpy(&TFT_L[6], "MEZZANOTTE", 10);
    } else {
      strncpy(&TFT_L[6], "EYOREXZERO", 10);
    }

    // Prepara lo sfondo con tutte le lettere in grigio scuro
    gfx->setFont(u8g2_font_inb21_mr);
    for (uint16_t i = 0; i < NUM_LEDS; i++) {
      gfx->setTextColor(convertColor(TextBackColor));
      gfx->setCursor(pgm_read_word(&TFT_X[i]), pgm_read_word(&TFT_Y[i]));
      gfx->write(pgm_read_byte(&TFT_L[i]));
    }

    // Imposta i pixel target per visualizzare l'ora corrente
    if (currentHour == 0) {
      displayWordToTarget(WORD_MEZZANOTTE);
    } else {
      displayWordToTarget(WORD_SONO_LE);
      const uint8_t* hourWord = (const uint8_t*)pgm_read_ptr(&HOUR_WORDS[currentHour]);
      displayWordToTarget(hourWord);
    }

    if (currentMinute > 0) {
      if (word_E_state == 0) displayWordToTarget(WORD_E);
      displayMinutesToTarget(currentMinute);
      displayWordToTarget(WORD_MINUTI);
    }

    // Costruisce la lista delle lettere target da colpire
    galagaShip.targetCount = 0;
    for (uint16_t i = 0; i < NUM_LEDS; i++) {
      if (targetPixels[i]) {
        galagaShip.targetList[galagaShip.targetCount] = i;
        galagaShip.targetCount++;
      }
    }

    // Inizializza il cannone nell'angolo estremo in basso a sinistra
    galagaShip.x = 5;         // Angolo estremo sinistra
    galagaShip.y = 475;       // Angolo estremo basso
    galagaShip.angle = 0.0;   // Inizia rivolto verso destra
    galagaShip.targetAngle = 0.0;
    galagaShip.animFrame = 0;
    galagaShip.lastAnimUpdate = currentMillis;
    galagaShip.lastShot = 0;
    galagaShip.currentTargetIndex = 0; // Inizia dal primo target
    galagaShip.passNumber = 1;         // Primo passaggio
    galagaShip.readyToShoot = false;
    galagaShip.missionComplete = false;

    // Inizializza tutti i proiettili come inattivi
    for (int i = 0; i < MAX_GALAGA_BULLETS; i++) {
      galagaBullets[i].active = false;
    }

    // Inizializza le stelle dello sfondo con posizioni casuali
    for (int i = 0; i < MAX_GALAGA_STARS; i++) {
      galagaStars[i].x = random(480);           // Posizione X casuale
      galagaStars[i].y = random(480);           // Posizione Y casuale
      galagaStars[i].speed = 0.3 + (random(20) / 10.0); // Velocità 0.3-2.3 pixel/frame
      galagaStars[i].brightness = 100 + random(156);    // Luminosità 100-255
      galagaStars[i].size = (random(100) < 80) ? 1 : 2; // 80% piccole, 20% medie
    }

    // NON disegniamo lo sprite dell'astronave, solo la linea di mira
    // Il cannone sarà rappresentato solo dalla linea rotante

    // Calcola l'angolo verso il primo target
    if (galagaShip.targetCount > 0) {
      uint16_t firstTargetPos = galagaShip.targetList[0];
      int targetX = pgm_read_word(&TFT_X[firstTargetPos]) + 12; // Centro della lettera
      int targetY = pgm_read_word(&TFT_Y[firstTargetPos]) - 10;  // Centro della lettera

      // Calcola angolo usando atan2 (ritorna radianti, convertiamo in gradi)
      float dx = targetX - galagaShip.x; // Punto origine nell'angolo
      float dy = targetY - galagaShip.y; // Punto origine nell'angolo
      galagaShip.targetAngle = atan2(dy, dx) * 180.0 / PI;
    }

    Serial.printf("[GALAGA] Cannone posizionato a (%.0f, %.0f), targets: %d\n",
                  galagaShip.x, galagaShip.y, galagaShip.targetCount);

    return;
  }

  // Ridisegna periodicamente solo le lettere in grigio scuro (non ancora accese)
  // per compensare le cancellazioni delle stelle
  static uint32_t lastLetterRedraw = 0;
  if (currentMillis - lastLetterRedraw > 300) { // Ogni 300ms
    lastLetterRedraw = currentMillis;
    gfx->setFont(u8g2_font_inb21_mr);
    gfx->setTextColor(convertColor(TextBackColor));
    for (uint16_t i = 0; i < NUM_LEDS; i++) {
      // Ridisegna solo le lettere NON ancora accese (in grigio scuro)
      if (!(activePixels[i] && targetPixels[i])) {
        gfx->setCursor(pgm_read_word(&TFT_X[i]), pgm_read_word(&TFT_Y[i]));
        gfx->write(pgm_read_byte(&TFT_L[i]));
      }
    }
  }

  // Aggiorna e disegna le stelle dello sfondo (dopo aver ridisegnato le lettere)
  updateGalagaStars();

  // Se la missione è completa, mostra effetto finale
  if (galagaShip.missionComplete) {
    static uint32_t lastRainbowUpdate = 0;
    static uint8_t rainbowOffset = 0;

    // Cancella il cannone e la linea di mira una sola volta
    if (!cannonErased) {
      // Cancella la linea di mira e ridisegna lettere coperte
      float rad = galagaShip.angle * PI / 180.0;
      int lineStartX = galagaShip.x;
      int lineStartY = galagaShip.y;
      int lineEndX = lineStartX + (int)(cos(rad) * 50);
      int lineEndY = lineStartY + (int)(sin(rad) * 50);

      // Cancella la linea
      gfx->drawLine(lineStartX, lineStartY, lineEndX, lineEndY, BLACK);

      // Ridisegna lettere coperte dalla linea
      gfx->setFont(u8g2_font_inb21_mr);
      int minX = min(lineStartX, lineEndX) - 2;
      int maxX = max(lineStartX, lineEndX) + 2;
      int minY = min(lineStartY, lineEndY) - 2;
      int maxY = max(lineStartY, lineEndY) + 2;

      for (uint16_t i = 0; i < NUM_LEDS; i++) {
        int16_t letterX = pgm_read_word(&TFT_X[i]);
        int16_t letterY = pgm_read_word(&TFT_Y[i]);

        if (letterX + 24 >= minX && letterX <= maxX &&
            letterY + 4 >= minY && letterY - 24 <= maxY) {
          if (activePixels[i] && targetPixels[i]) {
            gfx->setTextColor(convertColor(currentColor));
          } else {
            gfx->setTextColor(convertColor(TextBackColor));
          }
          gfx->setCursor(letterX, letterY);
          gfx->write(pgm_read_byte(&TFT_L[i]));
        }
      }

      // Non c'è sprite dell'astronave da cancellare, solo la linea è stata cancellata sopra

      cannonErased = true;

      Serial.println("[GALAGA] Linea di mira cancellata!");
    }

    if (currentMillis - lastRainbowUpdate >= 80) {
      lastRainbowUpdate = currentMillis;
      rainbowOffset = (rainbowOffset + 3) % 255;

      gfx->setFont(u8g2_font_inb21_mr);
      for (uint16_t pos = 0; pos < NUM_LEDS; pos++) {
        if (targetPixels[pos] && activePixels[pos]) {
          uint8_t hue = (rainbowOffset + (pos * 10)) % 255;
          Color rainbowColor = hsvToRgb(hue, 255, 255);
          gfx->setTextColor(convertColor(rainbowColor));
          gfx->setCursor(pgm_read_word(&TFT_X[pos]), pgm_read_word(&TFT_Y[pos]));
          gfx->write(pgm_read_byte(&TFT_L[pos]));
        }
      }
    }
    return;
  }

  // Controlla se tutti i target della lista corrente sono stati sparati
  if (galagaShip.currentTargetIndex >= galagaShip.targetCount) {
    // Tutti i target della lista sparati, aspetta che tutti i proiettili finiscano
    bool allBulletsGone = true;
    for (int i = 0; i < MAX_GALAGA_BULLETS; i++) {
      if (galagaBullets[i].active) {
        allBulletsGone = false;
        break;
      }
    }

    // Quando tutti i proiettili sono spariti, verifica se tutte le lettere target sono accese
    if (allBulletsGone && !galagaShip.missionComplete) {
      // Conta quante lettere target sono ancora spente
      uint16_t missedTargets = 0;
      uint16_t tempTargetList[NUM_LEDS];

      for (uint16_t pos = 0; pos < NUM_LEDS; pos++) {
        if (targetPixels[pos] && !activePixels[pos]) {
          // Questa lettera target non è ancora accesa
          tempTargetList[missedTargets] = pos;
          missedTargets++;
        }
      }

      if (missedTargets > 0 && galagaShip.passNumber < 4) {
        // Ci sono ancora lettere mancanti e non abbiamo superato il limite di passaggi
        Serial.printf("[GALAGA] Passaggio %d completato. Lettere mancanti: %d. Inizio passaggio %d...\n",
                      galagaShip.passNumber, missedTargets, galagaShip.passNumber + 1);

        // Ricostruisci la targetList con solo le lettere mancanti
        galagaShip.targetCount = missedTargets;
        for (uint16_t i = 0; i < missedTargets; i++) {
          galagaShip.targetList[i] = tempTargetList[i];
        }

        // Reset per ricominciare
        galagaShip.currentTargetIndex = 0;
        galagaShip.passNumber++;
        galagaShip.readyToShoot = false;
        galagaShip.lastShot = currentMillis; // Piccola pausa prima di ricominciare

      } else {
        // Tutte le lettere sono accese O abbiamo raggiunto il limite di passaggi
        galagaShip.missionComplete = true;

        if (missedTargets > 0) {
          Serial.printf("[GALAGA] Missione completa dopo %d passaggi. Lettere mancanti: %d (limite raggiunto)\n",
                        galagaShip.passNumber, missedTargets);
        } else {
          Serial.printf("[GALAGA] Missione completa! Tutte le lettere accese dopo %d passaggi.\n",
                        galagaShip.passNumber);
        }
      }
    }
  }

  // Aggiorna animazione del cannone (ogni 150ms)
  if (currentMillis - galagaShip.lastAnimUpdate >= 150) {
    galagaShip.lastAnimUpdate = currentMillis;
    galagaShip.animFrame = (galagaShip.animFrame + 1) % 2;
  }

  // Se ci sono ancora target da colpire
  if (galagaShip.currentTargetIndex < galagaShip.targetCount) {
    // Ottieni il target corrente
    uint16_t currentTargetPos = galagaShip.targetList[galagaShip.currentTargetIndex];
    int targetX = pgm_read_word(&TFT_X[currentTargetPos]) + 12; // Centro della lettera
    int targetY = pgm_read_word(&TFT_Y[currentTargetPos]) - 10; // Centro della lettera

    // Calcola angolo verso il target
    float dx = targetX - galagaShip.x;
    float dy = targetY - galagaShip.y;
    galagaShip.targetAngle = atan2(dy, dx) * 180.0 / PI;

    // Ruota gradualmente verso il target
    float angleDiff = galagaShip.targetAngle - galagaShip.angle;

    // Normalizza la differenza di angolo tra -180 e 180
    while (angleDiff > 180) angleDiff -= 360;
    while (angleDiff < -180) angleDiff += 360;

    // Se l'angolo è abbastanza vicino, considera il cannone allineato
    if (fabs(angleDiff) < 2.0) {
      galagaShip.angle = galagaShip.targetAngle;
      galagaShip.readyToShoot = true;
    } else {
      // Ruota gradualmente (5 gradi per frame)
      float rotationStep = 5.0;
      if (angleDiff > 0) {
        galagaShip.angle += min(rotationStep, angleDiff);
      } else {
        galagaShip.angle -= min(rotationStep, -angleDiff);
      }
      galagaShip.readyToShoot = false;
    }

    // CANCELLA SEMPRE la vecchia linea prima di disegnare quella nuova
    if (!firstDraw) {
      int lineStartX = galagaShip.x;
      int lineStartY = galagaShip.y;

      // Calcola l'area che copre TUTTA la vecchia linea
      int minX = min(lineStartX, lastLineEndX) - 10;
      int maxX = max(lineStartX, lastLineEndX) + 10;
      int minY = min(lineStartY, lastLineEndY) - 10;
      int maxY = max(lineStartY, lastLineEndY) + 10;

      // Assicurati che l'area sia valida
      if (minX < 0) minX = 0;
      if (minY < 0) minY = 0;
      if (maxX > 480) maxX = 480;
      if (maxY > 480) maxY = 480;

      // Cancella completamente l'area della vecchia linea con nero
      gfx->fillRect(minX, minY, maxX - minX, maxY - minY, BLACK);

        // Ridisegna tutte le lettere nell'area della vecchia linea
        gfx->setFont(u8g2_font_inb21_mr);
        for (uint16_t i = 0; i < NUM_LEDS; i++) {
          int16_t letterX = pgm_read_word(&TFT_X[i]);
          int16_t letterY = pgm_read_word(&TFT_Y[i]);

          // Bounding box lettera: [letterX, letterY-24] a [letterX+24, letterY+4]
          if (letterX + 24 >= minX && letterX <= maxX &&
              letterY + 4 >= minY && letterY - 24 <= maxY) {
            // Ridisegna la lettera
            if (activePixels[i] && targetPixels[i]) {
              // Lettera dell'orario già attivata - colore orario
              gfx->setTextColor(convertColor(currentColor));
            } else {
              // Lettera normale - grigio
              gfx->setTextColor(convertColor(TextBackColor));
            }
            gfx->setCursor(letterX, letterY);
            gfx->write(pgm_read_byte(&TFT_L[i]));
          }
        }

      // Ridisegna anche le stelle nell'area della vecchia linea
      for (int s = 0; s < MAX_GALAGA_STARS; s++) {
        int starX = (int)galagaStars[s].x;
        int starY = (int)galagaStars[s].y;

        // Se la stella è nell'area della vecchia linea, ridisegnala
        if (starX >= minX && starX <= maxX && starY >= minY && starY <= maxY) {
          uint16_t starColor = gfx->color565(galagaStars[s].brightness,
                                              galagaStars[s].brightness,
                                              galagaStars[s].brightness);
          if (galagaStars[s].size == 1) {
            gfx->drawPixel(starX, starY, starColor);
          } else {
            gfx->fillRect(starX, starY, 2, 2, starColor);
          }
        }
      }
    }

    // Disegna SEMPRE la nuova linea di mira
    float rad = galagaShip.angle * PI / 180.0;
    int lineEndX = galagaShip.x + (int)(cos(rad) * 60);  // Linea più lunga (60 pixel)
    int lineEndY = galagaShip.y + (int)(sin(rad) * 60);

    // Rosso se pronto a sparare, giallo se sta mirando
    uint16_t lineColor = galagaShip.readyToShoot ? RED : YELLOW;
    gfx->drawLine(galagaShip.x, galagaShip.y, lineEndX, lineEndY, lineColor);

    // Salva i valori per la prossima cancellazione
    lastLineEndX = lineEndX;
    lastLineEndY = lineEndY;
    firstDraw = false;

    // Spara quando allineato (attendi 300ms dopo l'ultimo colpo)
    if (galagaShip.readyToShoot && currentMillis - galagaShip.lastShot >= 300) {
      // Trova uno slot libero per il proiettile
      for (int i = 0; i < MAX_GALAGA_BULLETS; i++) {
        if (!galagaBullets[i].active) {
          galagaBullets[i].active = true;

          // Posizione di partenza: posizione del cannone (angolo estremo)
          galagaBullets[i].x = galagaShip.x;
          galagaBullets[i].y = galagaShip.y;

          // Calcola velocità verso il target
          float angleRad = galagaShip.angle * PI / 180.0;
          float bulletSpeed = 10.0; // Velocità del proiettile
          galagaBullets[i].velocityX = cos(angleRad) * bulletSpeed;
          galagaBullets[i].velocityY = sin(angleRad) * bulletSpeed;

          galagaBullets[i].creationTime = currentMillis;
          galagaShip.lastShot = currentMillis;

          // Passa al target successivo
          galagaShip.currentTargetIndex++;
          galagaShip.readyToShoot = false;

          Serial.printf("[GALAGA] Passaggio %d: Sparato al target %d/%d\n",
                        galagaShip.passNumber, galagaShip.currentTargetIndex, galagaShip.targetCount);
          break;
        }
      }
    }
  }

  // Aggiorna proiettili
  for (int i = 0; i < MAX_GALAGA_BULLETS; i++) {
    if (!galagaBullets[i].active) continue;

    // Salva posizione precedente
    int oldX = (int)galagaBullets[i].x;
    int oldY = (int)galagaBullets[i].y;

    // Aggiorna posizione con velocità direzionale
    galagaBullets[i].x += galagaBullets[i].velocityX;
    galagaBullets[i].y += galagaBullets[i].velocityY;

    // Controlla se il proiettile esce dallo schermo
    bool outOfBounds = (galagaBullets[i].x < -10 || galagaBullets[i].x > 490 ||
                        galagaBullets[i].y < -10 || galagaBullets[i].y > 490);

    // Controlla collisione con le lettere target
    bool hitTarget = false;
    for (uint16_t pos = 0; pos < NUM_LEDS; pos++) {
      if (!targetPixels[pos]) continue;
      if (activePixels[pos]) continue; // Già illuminata

      int letterX = pgm_read_word(&TFT_X[pos]);
      int letterY = pgm_read_word(&TFT_Y[pos]);

      // Bounding box della lettera: [letterX, letterY-24] a [letterX+24, letterY+4]
      // Bounding box del proiettile: [bulletX, bulletY] a [bulletX+2, bulletY+6]
      if (galagaBullets[i].x + 2 >= letterX && galagaBullets[i].x <= letterX + 24 &&
          galagaBullets[i].y + 6 >= letterY - 24 && galagaBullets[i].y <= letterY + 4) {

        // Colpita! Illumina la lettera
        activePixels[pos] = true;
        pixelChanged[pos] = true;

        gfx->setFont(u8g2_font_inb21_mr);
        gfx->setTextColor(convertColor(currentColor));
        gfx->setCursor(letterX, letterY);
        gfx->write(pgm_read_byte(&TFT_L[pos]));

        hitTarget = true;
        break;
      }
    }

    if (hitTarget || outOfBounds) {
      // Rimuovi proiettile
      eraseBullet(oldX, oldY);
      galagaBullets[i].active = false;
    } else {
      // Ridisegna proiettile in nuova posizione
      eraseBullet(oldX, oldY);
      drawBullet((int)galagaBullets[i].x, (int)galagaBullets[i].y);
    }
  }

  // Controlla se tutte le lettere target sono illuminate
  bool allTargetsHit = true;
  for (uint16_t pos = 0; pos < NUM_LEDS; pos++) {
    if (targetPixels[pos] && !activePixels[pos]) {
      allTargetsHit = false;
      break;
    }
  }

  if (allTargetsHit && !galagaShip.missionComplete) {
    galagaShip.missionComplete = true;
    Serial.println("[GALAGA] Missione completata!");
  }
}

// ==================== OROLOGIO ANALOGICO CON IMMAGINE DI SFONDO ====================

// Callback per disegnare pixel JPEG direttamente sul display E salvare nel buffer
int jpegDrawCallback(JPEGDRAW *pDraw) {
  // *** DOUBLE BUFFERING: Non disegnare più direttamente sul display ***
  // L'immagine verrà mostrata solo quando sarà pronta nel frameBuffer

  // Salva SOLO nel buffer di sfondo (se allocato)
  if (clockBackgroundBuffer != nullptr) {
    for (int y = 0; y < pDraw->iHeight; y++) {
      int destY = pDraw->y + y;
      if (destY >= 0 && destY < 480) {
        for (int x = 0; x < pDraw->iWidth; x++) {
          int destX = pDraw->x + x;
          if (destX >= 0 && destX < 480) {
            int bufferIndex = destY * 480 + destX;
            int pixelIndex = y * pDraw->iWidth + x;
            clockBackgroundBuffer[bufferIndex] = pDraw->pPixels[pixelIndex];
          }
        }
      }
    }
  }

  return 1; // Continua il rendering
}

// Carica la configurazione specifica per una skin dalla SD card
bool loadClockConfig(const char* skinName) {
  // Costruisci il nome del file cfg (es: "orologio.jpg" -> "orologio.cfg")
  String cfgFilename = String(skinName);
  int dotIndex = cfgFilename.lastIndexOf('.');
  if (dotIndex > 0) {
    cfgFilename = cfgFilename.substring(0, dotIndex);
  }
  cfgFilename += ".cfg";
  String cfgPath = "/" + cfgFilename;

  Serial.printf("[CLOCK CFG] Tentativo caricamento configurazione: '%s'\n", cfgPath.c_str());

  // Verifica se esiste il file di configurazione
  if (!SD.exists(cfgPath.c_str())) {
    Serial.printf("[CLOCK CFG] File configurazione non trovato, uso valori di default\n");
    // Valori di default lancette
    clockHourHandColor = BLACK;
    clockMinuteHandColor = BLACK;
    clockSecondHandColor = RED;
    clockHourHandLength = 80;
    clockMinuteHandLength = 110;
    clockSecondHandLength = 120;

    // Valori di default campi data
    clockWeekdayField = {true, 190, 280, WHITE, 2};
    clockDayField = {true, 210, 320, WHITE, 3};
    clockMonthField = {true, 200, 360, WHITE, 2};
    clockYearField = {true, 195, 400, WHITE, 2};
    return false;
  }

  // Apri il file di configurazione
  File cfgFile = SD.open(cfgPath.c_str(), FILE_READ);
  if (!cfgFile) {
    Serial.printf("[CLOCK CFG] Errore apertura file configurazione\n");
    return false;
  }

  size_t fileSize = cfgFile.size();

  // Formato nuovo V1.3: 41 bytes
  // 6 bytes colori lancette + 3 bytes lunghezze lancette + 4 campi data x 8 bytes (1 enabled + 2 x + 2 y + 2 color + 1 fontSize)
  if (fileSize >= 41) {
    uint8_t buffer[41];
    cfgFile.read(buffer, 41);

    // Leggi colori lancette
    clockHourHandColor = (buffer[0] << 8) | buffer[1];
    clockMinuteHandColor = (buffer[2] << 8) | buffer[3];
    clockSecondHandColor = (buffer[4] << 8) | buffer[5];

    // Leggi lunghezze lancette
    int idx = 6;
    clockHourHandLength = buffer[idx++];
    clockMinuteHandLength = buffer[idx++];
    clockSecondHandLength = buffer[idx++];

    // Leggi campo giorno settimana (weekday)
    clockWeekdayField.enabled = buffer[idx++];
    clockWeekdayField.x = (buffer[idx] << 8) | buffer[idx+1]; idx += 2;
    clockWeekdayField.y = (buffer[idx] << 8) | buffer[idx+1]; idx += 2;
    clockWeekdayField.color = (buffer[idx] << 8) | buffer[idx+1]; idx += 2;
    clockWeekdayField.fontSize = buffer[idx++];

    // Leggi campo giorno numero (day)
    clockDayField.enabled = buffer[idx++];
    clockDayField.x = (buffer[idx] << 8) | buffer[idx+1]; idx += 2;
    clockDayField.y = (buffer[idx] << 8) | buffer[idx+1]; idx += 2;
    clockDayField.color = (buffer[idx] << 8) | buffer[idx+1]; idx += 2;
    clockDayField.fontSize = buffer[idx++];

    // Leggi campo mese (month)
    clockMonthField.enabled = buffer[idx++];
    clockMonthField.x = (buffer[idx] << 8) | buffer[idx+1]; idx += 2;
    clockMonthField.y = (buffer[idx] << 8) | buffer[idx+1]; idx += 2;
    clockMonthField.color = (buffer[idx] << 8) | buffer[idx+1]; idx += 2;
    clockMonthField.fontSize = buffer[idx++];

    // Leggi campo anno (year)
    clockYearField.enabled = buffer[idx++];
    clockYearField.x = (buffer[idx] << 8) | buffer[idx+1]; idx += 2;
    clockYearField.y = (buffer[idx] << 8) | buffer[idx+1]; idx += 2;
    clockYearField.color = (buffer[idx] << 8) | buffer[idx+1]; idx += 2;
    clockYearField.fontSize = buffer[idx++];

    Serial.printf("[CLOCK CFG] Configurazione caricata (formato V1.3 - 41 bytes):\n");
    Serial.printf("[CLOCK CFG]   Lancette colori - Hour: 0x%04X, Minute: 0x%04X, Second: 0x%04X\n",
                  clockHourHandColor, clockMinuteHandColor, clockSecondHandColor);
    Serial.printf("[CLOCK CFG]   Lancette lunghezze - Hour: %dpx, Minute: %dpx, Second: %dpx\n",
                  clockHourHandLength, clockMinuteHandLength, clockSecondHandLength);
    Serial.printf("[CLOCK CFG]   Weekday: %s (%d,%d) 0x%04X fontSize=%d\n",
                  clockWeekdayField.enabled ? "ON" : "OFF", clockWeekdayField.x, clockWeekdayField.y,
                  clockWeekdayField.color, clockWeekdayField.fontSize);
    Serial.printf("[CLOCK CFG]   Day: %s (%d,%d) 0x%04X fontSize=%d\n",
                  clockDayField.enabled ? "ON" : "OFF", clockDayField.x, clockDayField.y,
                  clockDayField.color, clockDayField.fontSize);
    Serial.printf("[CLOCK CFG]   Month: %s (%d,%d) 0x%04X fontSize=%d\n",
                  clockMonthField.enabled ? "ON" : "OFF", clockMonthField.x, clockMonthField.y,
                  clockMonthField.color, clockMonthField.fontSize);
    Serial.printf("[CLOCK CFG]   Year: %s (%d,%d) 0x%04X fontSize=%d\n",
                  clockYearField.enabled ? "ON" : "OFF", clockYearField.x, clockYearField.y,
                  clockYearField.color, clockYearField.fontSize);

    cfgFile.close();
    return true;
  }
  // Formato vecchio V1.2: 34 bytes (retrocompatibilità)
  // 6 bytes lancette (3 colori x 2 bytes) + 4 campi data x 7 bytes (1 enabled + 2 x + 2 y + 2 color)
  else if (fileSize >= 34) {
    uint8_t buffer[34];
    cfgFile.read(buffer, 34);

    // Leggi colori lancette
    clockHourHandColor = (buffer[0] << 8) | buffer[1];
    clockMinuteHandColor = (buffer[2] << 8) | buffer[3];
    clockSecondHandColor = (buffer[4] << 8) | buffer[5];

    // Usa valori di default per lunghezze
    clockHourHandLength = 80;
    clockMinuteHandLength = 110;
    clockSecondHandLength = 120;

    // Leggi campo giorno settimana (weekday)
    int idx = 6;
    clockWeekdayField.enabled = buffer[idx++];
    clockWeekdayField.x = (buffer[idx] << 8) | buffer[idx+1]; idx += 2;
    clockWeekdayField.y = (buffer[idx] << 8) | buffer[idx+1]; idx += 2;
    clockWeekdayField.color = (buffer[idx] << 8) | buffer[idx+1]; idx += 2;
    clockWeekdayField.fontSize = 2; // Default

    // Leggi campo giorno numero (day)
    clockDayField.enabled = buffer[idx++];
    clockDayField.x = (buffer[idx] << 8) | buffer[idx+1]; idx += 2;
    clockDayField.y = (buffer[idx] << 8) | buffer[idx+1]; idx += 2;
    clockDayField.color = (buffer[idx] << 8) | buffer[idx+1]; idx += 2;
    clockDayField.fontSize = 3; // Default

    // Leggi campo mese (month)
    clockMonthField.enabled = buffer[idx++];
    clockMonthField.x = (buffer[idx] << 8) | buffer[idx+1]; idx += 2;
    clockMonthField.y = (buffer[idx] << 8) | buffer[idx+1]; idx += 2;
    clockMonthField.color = (buffer[idx] << 8) | buffer[idx+1]; idx += 2;
    clockMonthField.fontSize = 2; // Default

    // Leggi campo anno (year)
    clockYearField.enabled = buffer[idx++];
    clockYearField.x = (buffer[idx] << 8) | buffer[idx+1]; idx += 2;
    clockYearField.y = (buffer[idx] << 8) | buffer[idx+1]; idx += 2;
    clockYearField.color = (buffer[idx] << 8) | buffer[idx+1]; idx += 2;
    clockYearField.fontSize = 2; // Default

    Serial.printf("[CLOCK CFG] Configurazione caricata (formato vecchio V1.2 - 34 bytes):\n");
    Serial.printf("[CLOCK CFG]   Hour: 0x%04X, Minute: 0x%04X, Second: 0x%04X\n",
                  clockHourHandColor, clockMinuteHandColor, clockSecondHandColor);
    Serial.printf("[CLOCK CFG]   Weekday: %s (%d,%d) 0x%04X\n",
                  clockWeekdayField.enabled ? "ON" : "OFF", clockWeekdayField.x, clockWeekdayField.y, clockWeekdayField.color);
    Serial.printf("[CLOCK CFG]   Day: %s (%d,%d) 0x%04X\n",
                  clockDayField.enabled ? "ON" : "OFF", clockDayField.x, clockDayField.y, clockDayField.color);
    Serial.printf("[CLOCK CFG]   Month: %s (%d,%d) 0x%04X\n",
                  clockMonthField.enabled ? "ON" : "OFF", clockMonthField.x, clockMonthField.y, clockMonthField.color);
    Serial.printf("[CLOCK CFG]   Year: %s (%d,%d) 0x%04X\n",
                  clockYearField.enabled ? "ON" : "OFF", clockYearField.x, clockYearField.y, clockYearField.color);

    cfgFile.close();
    return true;
  }

  cfgFile.close();
  Serial.printf("[CLOCK CFG] File configurazione formato sconosciuto (%d bytes)\n", fileSize);
  return false;
}

// Salva la configurazione specifica per una skin sulla SD card
bool saveClockConfig(const char* skinName) {
  // Costruisci il nome del file cfg (es: "orologio.jpg" -> "orologio.cfg")
  String cfgFilename = String(skinName);
  int dotIndex = cfgFilename.lastIndexOf('.');
  if (dotIndex > 0) {
    cfgFilename = cfgFilename.substring(0, dotIndex);
  }
  cfgFilename += ".cfg";
  String cfgPath = "/" + cfgFilename;

  Serial.printf("[CLOCK CFG] Salvataggio configurazione: '%s'\n", cfgPath.c_str());

  // Apri il file in scrittura (sovrascrive se esiste)
  File cfgFile = SD.open(cfgPath.c_str(), FILE_WRITE);
  if (!cfgFile) {
    Serial.printf("[CLOCK CFG] Errore creazione file configurazione\n");
    return false;
  }

  // Scrivi 41 bytes: 6 colori + 3 lunghezze + 4 campi x 8 bytes
  uint8_t buffer[41];
  int idx = 0;

  // Scrivi colori lancette
  buffer[idx++] = (clockHourHandColor >> 8) & 0xFF;
  buffer[idx++] = clockHourHandColor & 0xFF;
  buffer[idx++] = (clockMinuteHandColor >> 8) & 0xFF;
  buffer[idx++] = clockMinuteHandColor & 0xFF;
  buffer[idx++] = (clockSecondHandColor >> 8) & 0xFF;
  buffer[idx++] = clockSecondHandColor & 0xFF;

  // Scrivi lunghezze lancette
  buffer[idx++] = clockHourHandLength;
  buffer[idx++] = clockMinuteHandLength;
  buffer[idx++] = clockSecondHandLength;

  // Scrivi campo weekday
  buffer[idx++] = clockWeekdayField.enabled ? 1 : 0;
  buffer[idx++] = (clockWeekdayField.x >> 8) & 0xFF;
  buffer[idx++] = clockWeekdayField.x & 0xFF;
  buffer[idx++] = (clockWeekdayField.y >> 8) & 0xFF;
  buffer[idx++] = clockWeekdayField.y & 0xFF;
  buffer[idx++] = (clockWeekdayField.color >> 8) & 0xFF;
  buffer[idx++] = clockWeekdayField.color & 0xFF;
  buffer[idx++] = clockWeekdayField.fontSize;

  // Scrivi campo day
  buffer[idx++] = clockDayField.enabled ? 1 : 0;
  buffer[idx++] = (clockDayField.x >> 8) & 0xFF;
  buffer[idx++] = clockDayField.x & 0xFF;
  buffer[idx++] = (clockDayField.y >> 8) & 0xFF;
  buffer[idx++] = clockDayField.y & 0xFF;
  buffer[idx++] = (clockDayField.color >> 8) & 0xFF;
  buffer[idx++] = clockDayField.color & 0xFF;
  buffer[idx++] = clockDayField.fontSize;

  // Scrivi campo month
  buffer[idx++] = clockMonthField.enabled ? 1 : 0;
  buffer[idx++] = (clockMonthField.x >> 8) & 0xFF;
  buffer[idx++] = clockMonthField.x & 0xFF;
  buffer[idx++] = (clockMonthField.y >> 8) & 0xFF;
  buffer[idx++] = clockMonthField.y & 0xFF;
  buffer[idx++] = (clockMonthField.color >> 8) & 0xFF;
  buffer[idx++] = clockMonthField.color & 0xFF;
  buffer[idx++] = clockMonthField.fontSize;

  // Scrivi campo year
  buffer[idx++] = clockYearField.enabled ? 1 : 0;
  buffer[idx++] = (clockYearField.x >> 8) & 0xFF;
  buffer[idx++] = clockYearField.x & 0xFF;
  buffer[idx++] = (clockYearField.y >> 8) & 0xFF;
  buffer[idx++] = clockYearField.y & 0xFF;
  buffer[idx++] = (clockYearField.color >> 8) & 0xFF;
  buffer[idx++] = clockYearField.color & 0xFF;
  buffer[idx++] = clockYearField.fontSize;

  cfgFile.write(buffer, 41);
  cfgFile.close();

  Serial.printf("[CLOCK CFG] Configurazione salvata con successo (41 bytes - formato V1.3)\n");
  Serial.printf("[CLOCK CFG]   Lancette colori - Hour: 0x%04X, Minute: 0x%04X, Second: 0x%04X\n",
                clockHourHandColor, clockMinuteHandColor, clockSecondHandColor);
  Serial.printf("[CLOCK CFG]   Lancette lunghezze - Hour: %dpx, Minute: %dpx, Second: %dpx\n",
                clockHourHandLength, clockMinuteHandLength, clockSecondHandLength);
  Serial.printf("[CLOCK CFG]   Weekday: %s (%d,%d) 0x%04X fontSize=%d\n",
                clockWeekdayField.enabled ? "ON" : "OFF", clockWeekdayField.x, clockWeekdayField.y,
                clockWeekdayField.color, clockWeekdayField.fontSize);
  Serial.printf("[CLOCK CFG]   Day: %s (%d,%d) 0x%04X fontSize=%d\n",
                clockDayField.enabled ? "ON" : "OFF", clockDayField.x, clockDayField.y,
                clockDayField.color, clockDayField.fontSize);
  Serial.printf("[CLOCK CFG]   Month: %s (%d,%d) 0x%04X fontSize=%d\n",
                clockMonthField.enabled ? "ON" : "OFF", clockMonthField.x, clockMonthField.y,
                clockMonthField.color, clockMonthField.fontSize);
  Serial.printf("[CLOCK CFG]   Year: %s (%d,%d) 0x%04X fontSize=%d\n",
                clockYearField.enabled ? "ON" : "OFF", clockYearField.x, clockYearField.y,
                clockYearField.color, clockYearField.fontSize);

  return true;
}

// Carica e visualizza l'immagine JPEG dalla SD card
bool loadClockImage() {
  // Costruisci il path completo
  String filepath = "/";
  filepath += clockActiveSkin;

  Serial.printf("[CLOCK] Tentativo caricamento skin: '%s'\n", filepath.c_str());
  Serial.printf("[CLOCK] Variabile clockActiveSkin: '%s'\n", clockActiveSkin);

  // Carica la configurazione specifica per questa skin
  loadClockConfig(clockActiveSkin);

  // Verifica esistenza file prima di aprirlo
  if (!SD.exists(filepath.c_str())) {
    Serial.printf("[CLOCK] ERRORE: Il file '%s' NON ESISTE sulla SD card!\n", filepath.c_str());

    // Lista tutti i file JPG presenti sulla SD per debug
    Serial.println("[CLOCK] Lista file JPG disponibili sulla SD card:");
    File root = SD.open("/");
    if (root) {
      File file = root.openNextFile();
      int count = 0;
      while (file) {
        if (!file.isDirectory()) {
          String fname = String(file.name());
          if (fname.endsWith(".jpg") || fname.endsWith(".JPG") ||
              fname.endsWith(".jpeg") || fname.endsWith(".JPEG")) {
            Serial.printf("[CLOCK]   - %s (%d bytes)\n", fname.c_str(), file.size());
            count++;
          }
        }
        file.close();
        file = root.openNextFile();
      }
      root.close();
      Serial.printf("[CLOCK] Totale file JPG trovati: %d\n", count);
    }

    return false;
  }

  File jpegFile = SD.open(filepath.c_str(), FILE_READ);

  if (!jpegFile) {
    Serial.printf("[CLOCK] ERRORE: impossibile aprire %s dalla SD card (anche se exists() ha returnato true)\n", filepath.c_str());
    return false;
  }

  Serial.printf("[CLOCK] File %s aperto con successo!\n", filepath.c_str());

  // Ottieni dimensione file
  size_t fileSize = jpegFile.size();
  Serial.printf("[CLOCK] Dimensione file %s: %d bytes\n", filepath.c_str(), fileSize);

  // Alloca buffer per il file JPEG
  uint8_t *jpegBuffer = (uint8_t *)malloc(fileSize);
  if (!jpegBuffer) {
    Serial.println("Errore: memoria insufficiente per buffer JPEG");
    jpegFile.close();
    return false;
  }

  // Leggi il file nel buffer
  size_t bytesRead = jpegFile.read(jpegBuffer, fileSize);
  jpegFile.close();

  if (bytesRead != fileSize) {
    Serial.println("Errore: lettura incompleta del file JPEG");
    free(jpegBuffer);
    return false;
  }

  // Decodifica e visualizza l'immagine JPEG
  int result = jpeg.openRAM(jpegBuffer, fileSize, jpegDrawCallback);

  if (result != 1) {
    Serial.printf("Errore: impossibile aprire JPEG, codice: %d\n", result);
    free(jpegBuffer);
    return false;
  }

  // Configura formato pixel RGB565 little-endian
  jpeg.setPixelType(RGB565_LITTLE_ENDIAN);

  // Decodifica l'immagine
  jpeg.decode(0, 0, 0); // x, y, flags (0 = no scaling)
  jpeg.close();

  free(jpegBuffer);

  Serial.println("Immagine orologio.jpg caricata con successo!");
  return true;
}

// Disegna una lancetta dell'orologio
void drawClockHand(int centerX, int centerY, float angle, int length, uint16_t color, int thickness) {
  // Converti angolo da gradi a radianti (0° = 12 o'clock, senso orario)
  float angleRad = (angle - 90.0) * PI / 180.0;

  int endX = centerX + (int)(cos(angleRad) * length);
  int endY = centerY + (int)(sin(angleRad) * length);

  // Determina il colore del contorno (bianco se lancetta scura, nero se lancetta chiara)
  // Calcola luminosità del colore usando RGB565
  uint8_t r = ((color >> 11) & 0x1F) << 3;
  uint8_t g = ((color >> 5) & 0x3F) << 2;
  uint8_t b = (color & 0x1F) << 3;
  uint16_t brightness = (r + g + b) / 3;

  // Se il colore è scuro (brightness < 128), usa contorno bianco, altrimenti nero
  uint16_t outlineColor = (brightness < 128) ? WHITE : BLACK;

  // DISEGNA IL CONTORNO (lancetta più spessa)
  int outlineThickness = thickness + 2; // Contorno 2px più spesso

  if (outlineThickness == 1) {
    gfx->drawLine(centerX, centerY, endX, endY, outlineColor);
  } else {
    for (int i = -outlineThickness/2; i <= outlineThickness/2; i++) {
      for (int j = -outlineThickness/2; j <= outlineThickness/2; j++) {
        gfx->drawLine(centerX + i, centerY + j, endX + i, endY + j, outlineColor);
      }
    }
  }

  // DISEGNA LA LANCETTA PRINCIPALE (sopra il contorno)
  if (thickness == 1) {
    gfx->drawLine(centerX, centerY, endX, endY, color);
  } else {
    for (int i = -thickness/2; i <= thickness/2; i++) {
      for (int j = -thickness/2; j <= thickness/2; j++) {
        gfx->drawLine(centerX + i, centerY + j, endX + i, endY + j, color);
      }
    }
  }

  // Disegna contorno del perno centrale
  gfx->fillCircle(centerX, centerY, thickness + 3, outlineColor);
  // Disegna perno centrale
  gfx->fillCircle(centerX, centerY, thickness + 2, color);
}

// Seleziona il font in base al valore fontSize (1-5)
const uint8_t* getDateFont(uint8_t fontSize) {
  switch(fontSize) {
    case 1: return u8g2_font_helvB08_tr; // 8pt - molto piccolo
    case 2: return u8g2_font_helvB12_tr; // 12pt - medio-piccolo
    case 3: return u8g2_font_helvB14_tr; // 14pt - medio (default)
    case 4: return u8g2_font_helvB18_tr; // 18pt - medio-grande
    case 5: return u8g2_font_helvB24_tr; // 24pt - grande
    default: return u8g2_font_helvB14_tr; // Default
  }
}

// Disegna un singolo campo della data
void drawDateField(const char* text, DateField& field) {
  if (!field.enabled) return; // Campo disabilitato

  // Seleziona il font in base alla dimensione configurata
  gfx->setFont(getDateFont(field.fontSize));
  gfx->setTextColor(field.color);

  // Calcola larghezza testo per centratura (opzionale)
  int textWidth = strlen(text) * 9; // Approssimazione

  // Disegna il testo alla posizione configurata
  gfx->setCursor(field.x, field.y);
  gfx->print(text);
}

// Disegna la data sull'orologio con campi separati e configurabili
void drawClockDate() {
  if (!showClockDate) return; // Se la data è disabilitata globalmente, non disegnare nulla

  // Ottieni giorno, mese e anno corrente da ezTime
  int day = myTZ.day();
  int month = myTZ.month();
  int year = myTZ.year();

  // Array con nomi dei giorni della settimana
  // ezTime weekday() restituisce 1-7 (1=Dom, 2=Lun, 3=Mar, 4=Mer, 5=Gio, 6=Ven, 7=Sab)
  const char* dayNames[] = {"", "Dom", "Lun", "Mar", "Mer", "Gio", "Ven", "Sab"};

  // Array con nomi dei mesi
  const char* monthNames[] = {"", "Gen", "Feb", "Mar", "Apr", "Mag", "Giu",
                              "Lug", "Ago", "Set", "Ott", "Nov", "Dic"};

  // Ottieni il giorno della settimana (1=lunedì, 7=domenica)
  int weekday = myTZ.weekday();

  // Prepara stringhe per ogni campo
  char weekdayStr[10];
  char dayStr[5];
  char monthStr[10];
  char yearStr[10];

  sprintf(weekdayStr, "%s", dayNames[weekday]);
  sprintf(dayStr, "%02d", day);
  sprintf(monthStr, "%s", monthNames[month]);
  sprintf(yearStr, "%04d", year);

  // Disegna ogni campo separatamente se abilitato
  drawDateField(weekdayStr, clockWeekdayField);  // Giorno settimana (es: "Lun")
  drawDateField(dayStr, clockDayField);          // Giorno numero (es: "25")
  drawDateField(monthStr, clockMonthField);      // Mese (es: "Dic")
  drawDateField(yearStr, clockYearField);        // Anno (es: "2025")
}

// Cancella una lancetta dell'orologio usando il buffer di sfondo
void eraseClockHandFromBuffer(int centerX, int centerY, float angle, int length, int thickness) {
  if (clockBackgroundBuffer == nullptr) return;

  // Calcola la posizione finale della lancetta
  float angleRad = (angle - 90.0) * PI / 180.0;
  int endX = centerX + (int)(cos(angleRad) * length);
  int endY = centerY + (int)(sin(angleRad) * length);

  // Calcola bounding box con margine maggiore per includere il contorno (+2px) e il perno centrale
  // thickness + 2 (contorno) + 5 (margine sicurezza) = thickness + 7
  int minX = min(centerX, endX) - thickness - 7;
  int maxX = max(centerX, endX) + thickness + 7;
  int minY = min(centerY, endY) - thickness - 7;
  int maxY = max(centerY, endY) + thickness + 7;

  // Limita ai bordi dello schermo
  minX = max(0, minX);
  maxX = min(479, maxX);
  minY = max(0, minY);
  maxY = min(479, maxY);

  // Calcola larghezza e altezza dell'area da ripristinare
  int width = maxX - minX + 1;
  int height = maxY - minY + 1;

  // Ripristina l'area dal buffer usando draw16bitRGBBitmap per massima velocità
  // Crea un buffer temporaneo per l'area da copiare
  if (width > 0 && height > 0 && width <= 480 && height <= 480) {
    // Copia linea per linea dal buffer al display (più veloce di pixel singoli)
    for (int y = minY; y <= maxY; y++) {
      // Puntatore all'inizio della linea nel buffer
      uint16_t* lineBuffer = &clockBackgroundBuffer[y * 480 + minX];
      // Disegna l'intera linea in una volta
      gfx->draw16bitRGBBitmap(minX, y, lineBuffer, width, 1);
    }
  }
}

// ========== FUNZIONI OFFSCREEN PER DOUBLE BUFFERING ==========

// Disegna una lancetta dell'orologio su un target GFX specifico (offscreen o display)
void drawClockHandOffscreen(Arduino_GFX* targetGfx, int centerX, int centerY, float angle, int length, uint16_t color, int thickness) {
  // Converti angolo da gradi a radianti (0° = 12 o'clock, senso orario)
  float angleRad = (angle - 90.0) * PI / 180.0;

  int endX = centerX + (int)(cos(angleRad) * length);
  int endY = centerY + (int)(sin(angleRad) * length);

  // Determina il colore del contorno (bianco se lancetta scura, nero se lancetta chiara)
  uint8_t r = ((color >> 11) & 0x1F) << 3;
  uint8_t g = ((color >> 5) & 0x3F) << 2;
  uint8_t b = (color & 0x1F) << 3;
  uint16_t brightness = (r + g + b) / 3;

  uint16_t outlineColor = (brightness < 128) ? WHITE : BLACK;

  // DISEGNA IL CONTORNO (lancetta più spessa)
  int outlineThickness = thickness + 2;

  if (outlineThickness == 1) {
    targetGfx->drawLine(centerX, centerY, endX, endY, outlineColor);
  } else {
    for (int i = -outlineThickness/2; i <= outlineThickness/2; i++) {
      for (int j = -outlineThickness/2; j <= outlineThickness/2; j++) {
        targetGfx->drawLine(centerX + i, centerY + j, endX + i, endY + j, outlineColor);
      }
    }
  }

  // DISEGNA LA LANCETTA PRINCIPALE (sopra il contorno)
  if (thickness == 1) {
    targetGfx->drawLine(centerX, centerY, endX, endY, color);
  } else {
    for (int i = -thickness/2; i <= thickness/2; i++) {
      for (int j = -thickness/2; j <= thickness/2; j++) {
        targetGfx->drawLine(centerX + i, centerY + j, endX + i, endY + j, color);
      }
    }
  }

  // Disegna contorno del perno centrale
  targetGfx->fillCircle(centerX, centerY, thickness + 3, outlineColor);
  // Disegna perno centrale
  targetGfx->fillCircle(centerX, centerY, thickness + 2, color);
}

// Disegna un singolo campo della data su target GFX offscreen
void drawDateFieldOffscreen(Arduino_GFX* targetGfx, const char* text, DateField& field) {
  if (!field.enabled) return;

  targetGfx->setFont(getDateFont(field.fontSize));
  targetGfx->setTextColor(field.color);
  targetGfx->setCursor(field.x, field.y);
  targetGfx->print(text);
}

// Disegna la data sull'orologio su target GFX offscreen
void drawClockDateOffscreen(Arduino_GFX* targetGfx) {
  if (!showClockDate) return;

  int day = myTZ.day();
  int month = myTZ.month();
  int year = myTZ.year();

  const char* dayNames[] = {"", "Dom", "Lun", "Mar", "Mer", "Gio", "Ven", "Sab"};
  const char* monthNames[] = {"", "Gen", "Feb", "Mar", "Apr", "Mag", "Giu",
                              "Lug", "Ago", "Set", "Ott", "Nov", "Dic"};

  int weekday = myTZ.weekday();

  char weekdayStr[10];
  char dayStr[5];
  char monthStr[10];
  char yearStr[10];

  sprintf(weekdayStr, "%s", dayNames[weekday]);
  sprintf(dayStr, "%02d", day);
  sprintf(monthStr, "%s", monthNames[month]);
  sprintf(yearStr, "%04d", year);

  drawDateFieldOffscreen(targetGfx, weekdayStr, clockWeekdayField);
  drawDateFieldOffscreen(targetGfx, dayStr, clockDayField);
  drawDateFieldOffscreen(targetGfx, monthStr, clockMonthField);
  drawDateFieldOffscreen(targetGfx, yearStr, clockYearField);
}

// Funzione principale per aggiornare l'orologio analogico
// Controlla periodicamente la presenza della SD Card
void checkSDCardStatus() {
  static uint32_t checkCounter = 0;
  uint32_t currentTime = millis();

  // Se abbiamo appena reinizializzato, aspetta un po' prima di controllare di nuovo
  if (sdReinitTime > 0 && (currentTime - sdReinitTime < SD_REINIT_DELAY)) {
    return; // Salta i controlli per qualche secondo dopo la reinizializzazione
  }

  // Controlla solo ogni SD_CHECK_INTERVAL millisecondi
  if (currentTime - lastSDCheckTime < SD_CHECK_INTERVAL) {
    return;
  }

  lastSDCheckTime = currentTime;
  checkCounter++;

  // Log ogni 10 controlli (ogni ~5 secondi) per non intasare la seriale
  if (checkCounter % 10 == 0) {
    Serial.printf("[SD CHECK] Controllo #%lu - Stato attuale: %s\n",
                  checkCounter, sdCardPresent ? "PRESENTE" : "ASSENTE");
  }

  // Verifica se la SD Card è ancora presente usando metodi leggeri
  bool currentSDPresent = false;

  // Metodo 1: Controlla il cardType (più leggero e affidabile)
  uint8_t cardType = SD.cardType();

  // Debug dettagliato ogni 10 controlli
  if (checkCounter % 10 == 0) {
    Serial.printf("[SD CHECK] cardType: %d (0=NONE, 1=MMC, 2=SD, 3=SDHC, 4=INVALIDO)\n", cardType);
  }

  // Valori validi di cardType: 1=MMC, 2=SD, 3=SDHC
  // cardType 0=CARD_NONE (no card) o 4+ = stati invalidi/transienti
  bool validCardType = (cardType >= 1 && cardType <= 3);

  if (validCardType) {
    // cardType è valido (1-3), MA dobbiamo verificare che la SD sia effettivamente accessibile
    // Metodo 2: Verifica ulteriore - controlla se esiste un file noto
    String testFile = "/";
    testFile += clockActiveSkin;

    if (SD.exists(testFile.c_str())) {
      currentSDPresent = true;

      // Log ogni 10 controlli
      if (checkCounter % 10 == 0) {
        Serial.printf("[SD CHECK] ✓ File '%s' esiste - SD OK\n", testFile.c_str());
      }
    } else {
      // File non esiste - SD probabilmente rimossa o corrotta
      Serial.printf("[SD CHECK] ⚠️ cardType=%d MA file '%s' NON esiste!\n", cardType, testFile.c_str());

      // Prova a verificare se esiste almeno un file qualsiasi
      File root = SD.open("/");
      if (root) {
        File file = root.openNextFile();
        if (file) {
          // C'è almeno un file, ma non quello che cerchiamo
          Serial.println("[SD CHECK] ⚠️ Root accessibile ma file skin mancante - possibile cambio SD");
          currentSDPresent = true;
          file.close();
        } else {
          Serial.println("[SD CHECK] ⚠️ Root vuota - SD rimossa o formattata!");
          currentSDPresent = false;
        }
        root.close();
      } else {
        // Non può aprire root E file non esiste = SD RIMOSSA
        Serial.println("[SD CHECK] ✗ Impossibile aprire root E file non esiste = SD RIMOSSA!");
        currentSDPresent = false;
      }
    }
  } else {
    // cardType è 0 (CARD_NONE) o 4+ (invalido) - SD Card NON presente
    if (cardType == CARD_NONE) {
      Serial.println("[SD CHECK] ⚠️⚠️⚠️ cardType = CARD_NONE (0) - SD FISICAMENTE ASSENTE!");
    } else {
      Serial.printf("[SD CHECK] ⚠️⚠️⚠️ cardType INVALIDO (%d) - SD RIMOSSA O ERRORE!\n", cardType);
    }
    currentSDPresent = false;
  }

  // Se lo stato è cambiato
  if (currentSDPresent != sdCardPresent) {
    Serial.printf("[SD CHECK] ⚠️⚠️⚠️ STATO CAMBIATO! Da %d a %d\n", sdCardPresent, currentSDPresent);
    sdCardPresent = currentSDPresent;

    if (!sdCardPresent) {
      // SD CARD RIMOSSA!
      Serial.println("[SD CHECK] ⚠️ SD CARD RIMOSSA!");
      Serial.println("[SD CHECK] ═══════════════════════════════════");
      Serial.println("[SD CHECK] ██████ SD RIMOSSA ██████");
      Serial.println("[SD CHECK] ═══════════════════════════════════");

      // Mostra messaggio di errore sul display
      gfx->fillScreen(RED);
      delay(200);
      gfx->fillScreen(BLACK);
      gfx->setTextColor(RED);
      gfx->setFont(u8g2_font_helvB24_tr);
      gfx->setCursor(20, 200);
      gfx->println("SD CARD RIMOSSA!");

      gfx->setFont(u8g2_font_helvB14_tr);
      gfx->setTextColor(YELLOW);
      gfx->setCursor(40, 260);
      gfx->println("Reinserire la scheda");
      gfx->setCursor(80, 290);
      gfx->println("per continuare");

      // Invalida l'immagine caricata
      clockImageLoaded = false;
      analogClockInitNeeded = true;

    } else {
      // SD CARD REINSERITA!
      Serial.println("[SD CHECK] ✅ SD CARD REINSERITA!");
      Serial.println("[SD CHECK] ═══════════════════════════════════");
      Serial.println("[SD CHECK] ██████ SD REINSERITA ██████");
      Serial.println("[SD CHECK] ═══════════════════════════════════");

      // Mostra messaggio di successo
      gfx->fillScreen(GREEN);
      delay(200);
      gfx->fillScreen(BLACK);
      gfx->setTextColor(GREEN);
      gfx->setFont(u8g2_font_helvB24_tr);
      gfx->setCursor(30, 200);
      gfx->println("SD CARD OK!");

      gfx->setFont(u8g2_font_helvB14_tr);
      gfx->setTextColor(WHITE);
      gfx->setCursor(60, 240);
      gfx->println("Reinizializzo SD...");

      // IMPORTANTE: Reinizializza la SD Card
      Serial.println("[SD CHECK] Reinizializzazione SD Card...");
      SPI.begin(SD_CLK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);

      delay(500); // Aspetta che la SD si stabilizzi

      if (SD.begin(SD_CS_PIN, SPI)) {
        Serial.println("[SD CHECK] ✓ SD reinizializzata con successo!");

        // Verifica che il file esista
        String testFile = "/";
        testFile += clockActiveSkin;
        if (SD.exists(testFile.c_str())) {
          Serial.printf("[SD CHECK] ✓ File '%s' trovato!\n", testFile.c_str());

          gfx->setCursor(60, 280);
          gfx->println("Ricarico orologio...");
          delay(1000);

          // Forza reinizializzazione orologio
          analogClockInitNeeded = true;
          clockImageLoaded = false;

          // Imposta il tempo di reinizializzazione per evitare controlli immediati
          sdReinitTime = millis();

          Serial.println("[SD CHECK] ✓ Ricaricamento orologio avviato");
        } else {
          Serial.printf("[SD CHECK] ⚠️ File '%s' non trovato sulla SD!\n", testFile.c_str());
          gfx->fillScreen(BLACK);
          gfx->setTextColor(YELLOW);
          gfx->setCursor(40, 240);
          gfx->println("File immagine non trovato!");
          delay(2000);
        }
      } else {
        Serial.println("[SD CHECK] ✗ Errore reinizializzazione SD!");
        gfx->fillScreen(BLACK);
        gfx->setTextColor(RED);
        gfx->setCursor(40, 240);
        gfx->println("Errore reinizializzazione!");
        delay(2000);
      }
    }
  }
}

void updateAnalogClock() {
  static uint8_t lastDisplayedSecond = 255;

  // Controlla periodicamente lo stato della SD Card
  checkSDCardStatus();

  // Se la SD Card è stata rimossa, non continuare
  if (!sdCardPresent && clockImageLoaded) {
    // Mostra solo il messaggio di errore
    return;
  }

  // Inizializzazione
  if (analogClockInitNeeded) {
    Serial.println("[ANALOG CLOCK] Inizializzazione orologio analogico con DOUBLE BUFFERING");

    analogClockInitNeeded = false;
    clockImageLoaded = false;
    lastDisplayedSecond = 255;
    lastSecondAngle = 0;
    lastMinuteAngle = 0;
    lastHourAngle = 0;

    // Alloca buffer in PSRAM per salvare l'immagine di sfondo
    if (clockBackgroundBuffer == nullptr) {
      clockBackgroundBuffer = (uint16_t*)ps_malloc(CLOCK_BUFFER_SIZE * sizeof(uint16_t));
      if (clockBackgroundBuffer == nullptr) {
        Serial.println("[ANALOG CLOCK] ERRORE: Impossibile allocare buffer in PSRAM!");
      } else {
        Serial.printf("[ANALOG CLOCK] Buffer allocato in PSRAM: %d KB\n",
                      (CLOCK_BUFFER_SIZE * sizeof(uint16_t)) / 1024);
      }
    }

    // Alloca frameBuffer per double buffering
    if (analogClockFrameBuffer == nullptr) {
      analogClockFrameBuffer = (uint16_t*)heap_caps_malloc(
        480 * 480 * sizeof(uint16_t),
        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT
      );

      if (analogClockFrameBuffer == nullptr) {
        analogClockFrameBuffer = (uint16_t*)ps_malloc(480 * 480 * sizeof(uint16_t));
        if (analogClockFrameBuffer == nullptr) {
          Serial.println("[ANALOG CLOCK] ERRORE: Impossibile allocare frameBuffer!");
          return;
        }
      }
      Serial.println("[ANALOG CLOCK] FrameBuffer allocato");
    }

    // Crea GFX offscreen
    if (analogClockOffscreenGfx == nullptr) {
      analogClockOffscreenGfx = new OffscreenGFX(analogClockFrameBuffer, 480, 480);
      Serial.println("[ANALOG CLOCK] OffscreenGFX creato");
    }

    // Carica l'immagine di sfondo (carica SOLO nel clockBackgroundBuffer, NON sul display)
    clockImageLoaded = loadClockImage();

    // Copia lo sfondo caricato nel frameBuffer
    if (clockImageLoaded && clockBackgroundBuffer != nullptr && analogClockFrameBuffer != nullptr) {
      // Copia l'immagine caricata dal clockBackgroundBuffer al frameBuffer
      memcpy(analogClockFrameBuffer, clockBackgroundBuffer, 480 * 480 * sizeof(uint16_t));
      Serial.println("[ANALOG CLOCK] Immagine copiata dal buffer di sfondo al frameBuffer");
    } else if (analogClockFrameBuffer != nullptr && analogClockOffscreenGfx != nullptr) {
      // Se non c'è l'immagine, disegna un cerchio semplice SUL FRAMEBUFFER
      for (int i = 0; i < 480 * 480; i++) {
        analogClockFrameBuffer[i] = BLACK;
      }

      analogClockOffscreenGfx->drawCircle(CLOCK_CENTER_X, CLOCK_CENTER_Y, 200, WHITE);
      analogClockOffscreenGfx->drawCircle(CLOCK_CENTER_X, CLOCK_CENTER_Y, 199, WHITE);

      // Disegna i numeri delle ore
      analogClockOffscreenGfx->setFont(u8g2_font_inb21_mr);
      analogClockOffscreenGfx->setTextColor(WHITE);

      // 12
      analogClockOffscreenGfx->setCursor(CLOCK_CENTER_X - 10, CLOCK_CENTER_Y - 170);
      analogClockOffscreenGfx->print("12");

      // 3
      analogClockOffscreenGfx->setCursor(CLOCK_CENTER_X + 160, CLOCK_CENTER_Y + 10);
      analogClockOffscreenGfx->print("3");

      // 6
      analogClockOffscreenGfx->setCursor(CLOCK_CENTER_X - 5, CLOCK_CENTER_Y + 190);
      analogClockOffscreenGfx->print("6");

      // 9
      analogClockOffscreenGfx->setCursor(CLOCK_CENTER_X - 180, CLOCK_CENTER_Y + 10);
      analogClockOffscreenGfx->print("9");

      // Copia nel clockBackgroundBuffer per uso futuro
      if (clockBackgroundBuffer != nullptr) {
        memcpy(clockBackgroundBuffer, analogClockFrameBuffer, 480 * 480 * sizeof(uint16_t));
      }
    }

    // Mostra l'immagine iniziale
    if (analogClockFrameBuffer != nullptr) {
      gfx->draw16bitRGBBitmap(0, 0, analogClockFrameBuffer, 480, 480);
    } else {
      gfx->fillScreen(BLACK);
    }
  }

  // Aggiorna le lancette ogni secondo (o continuamente se smooth seconds è attivo)
  bool needsUpdate = (currentSecond != lastDisplayedSecond) || clockSmoothSeconds;

  if (needsUpdate) {
    // Aggiorna lastDisplayedSecond solo se cambia
    if (currentSecond != lastDisplayedSecond) {
      lastDisplayedSecond = currentSecond;
    }

    // Verifica che il double buffering sia inizializzato
    if (analogClockFrameBuffer == nullptr || analogClockOffscreenGfx == nullptr) {
      Serial.println("[ANALOG CLOCK] ERRORE: Double buffering non inizializzato!");
      return;
    }

    // Log solo quando cambia secondo (non continuamente se smooth)
    if (currentSecond != lastDisplayedSecond || !clockSmoothSeconds) {
      Serial.println("[ANALOG CLOCK] *** RENDERING OFFSCREEN - INIZIO ***");
    }

    // Calcola angoli delle lancette
    // Ore (0-12): ogni ora = 30 gradi, più frazione per minuti
    float hourAngle = ((currentHour % 12) * 30.0) + (currentMinute * 0.5);

    // Minuti (0-60): ogni minuto = 6 gradi
    float minuteAngle = currentMinute * 6.0;

    // Secondi (0-60): ogni secondo = 6 gradi
    // Se smooth seconds è attivo, usa i millisecondi per movimento fluido
    float secondAngle;
    if (clockSmoothSeconds) {
      // Sincronizza millis() con ogni cambio di secondo
      static unsigned long lastSecondMillis = 0;
      static int lastSecondValue = -1;

      unsigned long currentMillis = millis();

      // Se il secondo è cambiato, memorizza il timestamp
      if (currentSecond != lastSecondValue) {
        lastSecondMillis = currentMillis;
        lastSecondValue = currentSecond;
      }

      // Calcola quanto tempo è passato dall'inizio di questo secondo
      unsigned long elapsed = currentMillis - lastSecondMillis;

      // Limita a 1000ms per evitare overflow (sicurezza)
      if (elapsed > 1000) elapsed = 1000;

      // Calcola frazione e angolo
      float secondFraction = elapsed / 1000.0;
      secondAngle = (currentSecond + secondFraction) * 6.0;
    } else {
      // Movimento a scatto (classico)
      secondAngle = currentSecond * 6.0;
    }

    // ===== FASE 1: COPIA LO SFONDO NEL FRAMEBUFFER =====
    if (clockBackgroundBuffer != nullptr) {
      memcpy(analogClockFrameBuffer, clockBackgroundBuffer, 480 * 480 * sizeof(uint16_t));
    } else {
      // Fallback: riempi di nero
      for (int i = 0; i < 480 * 480; i++) {
        analogClockFrameBuffer[i] = BLACK;
      }
    }

    // ===== FASE 2: DISEGNA TUTTO SUL FRAMEBUFFER =====

    // Disegna la data (se abilitata)
    drawClockDateOffscreen(analogClockOffscreenGfx);

    // Disegna le lancette (dalla più corta alla più lunga)
    // Lancetta delle ore
    drawClockHandOffscreen(analogClockOffscreenGfx, CLOCK_CENTER_X, CLOCK_CENTER_Y, hourAngle, clockHourHandLength, clockHourHandColor, 8);

    // Lancetta dei minuti
    drawClockHandOffscreen(analogClockOffscreenGfx, CLOCK_CENTER_X, CLOCK_CENTER_Y, minuteAngle, clockMinuteHandLength, clockMinuteHandColor, 6);

    // Lancetta dei secondi
    drawClockHandOffscreen(analogClockOffscreenGfx, CLOCK_CENTER_X, CLOCK_CENTER_Y, secondAngle, clockSecondHandLength, clockSecondHandColor, 2);

    // Mostra indirizzo IP in alto a sinistra (solo se WiFi connesso)
    if (WiFi.status() == WL_CONNECTED) {
      // Determina colore testo in base allo sfondo del JPEG
      uint16_t textColor = WHITE;  // Default bianco

      if (clockBackgroundBuffer != nullptr) {
        // Campiona alcuni pixel nell'area dove verrà mostrato il testo
        // Calcola luminosità media per decidere se usare bianco o nero
        uint32_t totalLuminance = 0;
        int sampleCount = 0;

        // Campiona 10 pixel in orizzontale nell'area del testo
        for (int x = 5; x < 100; x += 10) {
          int y = 10;  // Poco sopra il testo
          if (x >= 0 && x < 480 && y >= 0 && y < 480) {
            uint16_t pixel = clockBackgroundBuffer[y * 480 + x];

            // Converti RGB565 -> RGB888
            uint8_t r = ((pixel >> 11) & 0x1F) << 3;  // 5 bit -> 8 bit
            uint8_t g = ((pixel >> 5) & 0x3F) << 2;   // 6 bit -> 8 bit
            uint8_t b = (pixel & 0x1F) << 3;          // 5 bit -> 8 bit

            // Calcola luminosità (formula standard)
            uint8_t luminance = (uint8_t)(0.299 * r + 0.587 * g + 0.114 * b);
            totalLuminance += luminance;
            sampleCount++;
          }
        }

        // Calcola luminosità media
        if (sampleCount > 0) {
          uint8_t avgLuminance = totalLuminance / sampleCount;

          // Se lo sfondo è chiaro (luminosità > 128), usa testo nero
          // Se lo sfondo è scuro, usa testo bianco
          textColor = (avgLuminance > 128) ? BLACK : WHITE;
        }
      }

      analogClockOffscreenGfx->setFont(u8g2_font_helvR08_tr);  // Font piccolo
      analogClockOffscreenGfx->setTextColor(textColor);
      analogClockOffscreenGfx->setCursor(5, 15);  // In alto a sinistra
      analogClockOffscreenGfx->print(WiFi.localIP().toString() + ":8080/clock");
    }

    Serial.println("[ANALOG CLOCK] Rendering offscreen completato");

    // ===== FASE 3: TRASFERISCI TUTTO AL DISPLAY IN UN COLPO SOLO =====
    Serial.println("[ANALOG CLOCK] Trasferimento al display...");
    gfx->draw16bitRGBBitmap(0, 0, analogClockFrameBuffer, 480, 480);
    Serial.println("[ANALOG CLOCK] *** ZERO FLICKERING - FATTO! ***");

    // Salva gli angoli correnti per il prossimo aggiornamento
    lastHourAngle = hourAngle;
    lastMinuteAngle = minuteAngle;
    lastSecondAngle = secondAngle;
  }

  // Gestione touch per attivare/disattivare la data
  // Tocca la parte bassa dello schermo per toggle della data
  ts.read(); // Legge lo stato del touch screen

  if (ts.isTouched) {
    // Mappa le coordinate del primo punto di contatto
    int x = map(ts.points[0].x, TOUCH_MAP_X1, TOUCH_MAP_X2, 0, 480 - 1);
    int y = map(ts.points[0].y, TOUCH_MAP_Y1, TOUCH_MAP_Y2, 0, 480 - 1);

    // Area touch per toggle data: parte inferiore dello schermo (sotto y=360)
    if (y > 360) {
      // Toggle stato data
      showClockDate = !showClockDate;

      // Salva in EEPROM
      EEPROM.write(EEPROM_CLOCK_DATE_ADDR, showClockDate ? 1 : 0);
      EEPROM.commit();

      Serial.printf("[ANALOG CLOCK] Data %s\n", showClockDate ? "ABILITATA" : "DISABILITATA");

      // Forza aggiornamento immediato - il prossimo ciclo ridisegnerà tutto con double buffering
      lastDisplayedSecond = 255;

      // Aspetta il rilascio del touch
      do {
        delay(10);
        ts.read();
      } while (ts.isTouched);

      delay(200); // Debounce
    }
  }
}
