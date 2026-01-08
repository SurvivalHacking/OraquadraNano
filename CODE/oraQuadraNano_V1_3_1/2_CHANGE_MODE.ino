// Funzione per forzare un aggiornamento completo del display
void forceDisplayUpdate() {
  // Pulizia completa dello schermo impostando tutti i pixel a nero.
  gfx->fillScreen(BLACK);

  // Reset completo degli array di stato utilizzati per la gestione dei pixel.
  memset(activePixels, 0, sizeof(activePixels));   // Imposta tutti i pixel attivi a false (spenti).
  memset(targetPixels, 0, sizeof(targetPixels));   // Imposta tutti i pixel target a false (nessun cambiamento in corso).
  memset(pixelChanged, 1, sizeof(pixelChanged));   // Imposta tutti i pixel come "cambiati" per forzare un aggiornamento.

  // Aggiornamento specifico in base alla modalità di visualizzazione corrente.
  switch (currentMode) {
    case MODE_MATRIX:
      matrixInitialized = false; // Forza la reinizializzazione della modalità matrix.
      updateMatrix();            // Chiama la funzione per aggiornare la modalità matrix.
      break;
    case MODE_MATRIX2:
      matrixInitialized = false; // Forza la reinizializzazione della modalità matrix continua.
      updateMatrix2();           // Chiama la funzione per aggiornare la modalità matrix continua.
      break;
    case MODE_SNAKE:
      snakeInitNeeded = true;    // Forza la reinizializzazione della modalità snake.
      updateSnake();             // Chiama la funzione per aggiornare la modalità snake.
      break;
    case MODE_WATER:
      waterDropInitNeeded = true; // Forza la reinizializzazione della modalità goccia d'acqua.
      updateWaterDrop();          // Chiama la funzione per aggiornare la modalità goccia d'acqua.
      break;
    case MODE_MARIO:
      marioInitNeeded = true;     // Forza la reinizializzazione della modalità Mario Bros.
      updateMarioMode();          // Chiama la funzione per aggiornare la modalità Mario Bros.
      break;
    case MODE_TRON:
      tronInitialized = false;    // Forza la reinizializzazione della modalità Tron.
      updateTron();               // Chiama la funzione per aggiornare la modalità Tron.
      break;
#ifdef EFFECT_GALAGA
    case MODE_GALAGA:
      galagaInitNeeded = true;    // Forza la reinizializzazione della modalità Galaga.
      updateGalagaMode();         // Chiama la funzione per aggiornare la modalità Galaga.
      break;
#endif
#ifdef EFFECT_ANALOG_CLOCK
    case MODE_ANALOG_CLOCK:
      analogClockInitNeeded = true; // Forza la reinizializzazione dell'orologio analogico.
      updateAnalogClock();        // Chiama la funzione per aggiornare l'orologio analogico.
      break;
#endif
#ifdef EFFECT_FLIP_CLOCK
    case MODE_FLIP_CLOCK:
      flipClockInitialized = false; // Forza la reinizializzazione del flip clock.
      updateFlipClock();          // Chiama la funzione per aggiornare il flip clock.
      break;
#endif
#ifdef EFFECT_BTTF
    case MODE_BTTF:
      bttfInitialized = false;    // Forza la reinizializzazione della modalità BTTF.
      updateBTTF();               // Chiama la funzione per aggiornare la modalità BTTF.
      break;
#endif
#ifdef EFFECT_LED_RING
    case MODE_LED_RING:
      ledRingInitialized = false; // Forza la reinizializzazione della modalità LED Ring.
      updateLedRingClock();       // Chiama la funzione per aggiornare la modalità LED Ring.
      break;
#endif
    case MODE_FADE:
      fadeInitialized = false;   // Forza la reinizializzazione della modalità dissolvenza.
      updateFadeMode();          // Chiama la funzione per aggiornare la modalità dissolvenza.
      break;
    case MODE_SLOW:
      slowInitialized = false;   // Forza la reinizializzazione della modalità lenta.
      updateSlowMode();          // Chiama la funzione per aggiornare la modalità lenta.
      break;
    case MODE_FAST:
      updateFastMode();          // Chiama la funzione per aggiornare la modalità veloce.
      break;
  }
}


void applyPreset(uint8_t preset) {

  const char* presetName = ""; // Stringa per memorizzare il nome del preset.
  const char* title = "PRESET:"; // Titolo da mostrare sul display.
  uint16_t presetColor = WHITE; // Colore predefinito per il testo del preset.

  // Salva la modalità precedente per verificare se è cambiata.
  DisplayMode previousMode = currentMode;

  switch (preset) {
    case 0:  // Preset 0: Normale - Solo orario con colori casuali.
      presetName = "RANDOM";
      presetColor = WHITE;
      currentMode = (DisplayMode)random(NUM_MODES);           // Imposta una modalità casuale.
      currentColor = Color(random(256), random(256), random(256));  // Imposta un colore casuale.
      break;
    case 1:  // Preset 1: Veloce con colore acqua (ciano).
      presetName = "VELOCE ACQUA";
      presetColor = CYAN;
      currentMode = MODE_FAST;
      currentColor = Color(0, 255, 255);
      break;
    case 2: // Preset 2: Lento con colore viola.
      presetName = "LENTO VIOLA";
      presetColor = PURPLE;
      currentMode = MODE_SLOW;
      currentColor = Color(255, 0, 255);
      break;
    case 3: // Preset 3: Lento con arancione.
      presetName = "LENTO ARANCIONE";
      presetColor = ORANGE;
      currentMode = MODE_SLOW;
      currentColor = Color(255, 165, 0);
      break;
    case 4: // Preset 4: Fade con rosso.
      presetName = "FADE ROSSO";
      presetColor = RED;
      currentMode = MODE_FADE;
      currentColor = Color(255, 0, 0);
      break;
    case 5: // Preset 5: Fade con verde.
      presetName = "FADE VERDE";
      presetColor = GREEN;
      currentMode = MODE_FADE;
      currentColor = Color(0, 255, 0);
      break;
    case 6: // Preset 6: Fade con colore blu.
      presetName = "FADE BLU";
      presetColor = BLUE;
      currentMode = MODE_FADE;
      currentColor = Color(0, 0, 255);
      break;
    case 7: // Preset 7: Matrix con parole gialle.
      presetName = "MATRIX GIALLO";
      presetColor = YELLOW;
      currentMode = MODE_MATRIX;
      currentColor = Color(255, 255, 0);
      break;
    case 8: // Preset 8: Matrix con parole ciano.
      presetName = "MATRIX CIANO";
      presetColor = CYAN;
      currentMode = MODE_MATRIX;
      currentColor = Color(0, 255, 255);
      break;
    case 9: // Preset 9: Matrix continua con parole verdi.
      presetName = "MATRIX CONTINUO VERDE";
      presetColor = GREEN;
      currentMode = MODE_MATRIX2;
      currentColor = Color(0, 255, 0);
      break;
    case 10: // Preset 10: Matrix continua con parole bianche.
      presetName = "MATRIX CONTINUO BIANCO";
      presetColor = WHITE;
      currentMode = MODE_MATRIX2;
      currentColor = Color(255, 255, 255);
      break;
    case 11:  // Preset 11: Effetto serpente.
      presetName = "SNAKE";
      presetColor = YELLOW;
      currentMode = MODE_SNAKE;
      currentColor = Color(255, 255, 0);
      break;
    case 12:  // Preset 12: Effetto goccia d'acqua.
      presetName = "GOCCIA ACQUA";
      presetColor = CYAN;
      currentMode = MODE_WATER;
      currentColor = Color(0, 150, 255);
      break;
    case 13:  // Preset 13: Effetto personalizzato (come impostato dall'utente).
      presetName = "COME PIACE A ME!";
      presetColor = RED;
      currentMode = userMode;
      currentColor = userColor;
      break;
    case 14:  // Preset 14: Effetto Mario Bros.
      presetName = "SUPER MARIO";
      presetColor = RED;
      currentMode = MODE_MARIO;
      currentColor = Color(255, 200, 0);  // Giallo oro
      break;
    case 15:  // Preset 15: Effetto Tron.
      presetName = "TRON";
      presetColor = CYAN;
      currentMode = MODE_TRON;
      currentColor = Color(0, 150, 255);  // Blu ciano classico Tron
      break;
    case 16:  // Preset 16: Effetto Galaga.
      presetName = "GALAGA";
      presetColor = YELLOW;  // Giallo
      currentMode = MODE_GALAGA;
      currentColor = Color(0, 255, 255);  // Ciano brillante
      break;
    case 17:  // Preset 17: Orologio Analogico.
      presetName = "OROLOGIO";
      presetColor = WHITE;
      currentMode = MODE_ANALOG_CLOCK;
      currentColor = Color(255, 255, 255);  // Bianco
      break;
    case 18:  // Preset 18: Ritorno al Futuro (BTTF).
      presetName = "RITORNO AL FUTURO";
      presetColor = GREEN;
      currentMode = MODE_BTTF;
      currentColor = Color(0, 255, 0);  // Verde brillante
      break;
    case 19:  // Preset 19: Orologio LED Ring.
      presetName = "LED RING CLOCK";
      presetColor = CYAN;
      currentMode = MODE_LED_RING;
      currentColor = Color(0, 200, 255);  // Ciano brillante
      break;
    default:
      presetName = "PRESET DEFAULT";
      presetColor = WHITE;
      currentMode = MODE_FAST;
      currentColor = Color(255, 255, 255);
      break;
  }

  // Mostra il nome del preset sul display.
  gfx->fillScreen(BLACK);
  gfx->setFont(u8g2_font_maniac_te);
  gfx->setTextColor(presetColor);

  int textWidth;
  int xPos;

  // Calcola la larghezza approssimativa del testo del titolo per centrarlo.
  textWidth = strlen(title) * 18;  // Stima 18 pixel per carattere.
  xPos = (480 - textWidth) / 2;
  if (xPos < 0) xPos = 0;  // Evita posizioni negative.
  // Mostra il titolo.
  gfx->setCursor(xPos, 210);
  gfx->println(title);


  // Calcola la larghezza approssimativa del testo del nome del preset per centrarlo.
  textWidth = strlen(presetName) * 18;  // Stima 18 pixel per carattere.
  xPos = (480 - textWidth) / 2;
  if (xPos < 0) xPos = 0;  // Evita posizioni negative.
  // Mostra il nome del preset.
  gfx->setCursor(xPos, 240);
  gfx->println(presetName);


  // Pausa per mostrare il nome del preset all'utente.
  delay(1000);

  gfx->fillScreen(BLACK);
  gfx->setFont(u8g2_font_inb21_mr);
  delay(100);

  // Salva il preset corrente nella EEPROM.
  EEPROM.write(EEPROM_PRESET_ADDR, preset);

  // Salva il colore corrente nella EEPROM.
  EEPROM.write(EEPROM_COLOR_R_ADDR, currentColor.r);
  EEPROM.write(EEPROM_COLOR_G_ADDR, currentColor.g);
  EEPROM.write(EEPROM_COLOR_B_ADDR, currentColor.b);

  // Salva la modalità corrente nella EEPROM.
  EEPROM.write(EEPROM_MODE_ADDR, currentMode);

  // Scrive i dati dalla cache della EEPROM alla memoria fisica.
  EEPROM.commit();

  // Forza un aggiornamento immediato del display per mostrare il nuovo preset.
  forceDisplayUpdate();

}



void handleModeChange() {

  // Passa alla modalità di visualizzazione successiva nel ciclo.
  currentMode = (DisplayMode)((currentMode + 1) % NUM_MODES);
  userMode = currentMode; // Aggiorna anche la modalità utente.

  // Salva la modalità corrente, reimposta il preset a personalizzato (13) e salva il colore corrente nella EEPROM.
  EEPROM.write(EEPROM_MODE_ADDR, currentMode);
  EEPROM.write(EEPROM_PRESET_ADDR, 13);
  EEPROM.write(EEPROM_COLOR_R_ADDR, currentColor.r);
  EEPROM.write(EEPROM_COLOR_G_ADDR, currentColor.g);
  EEPROM.write(EEPROM_COLOR_B_ADDR, currentColor.b);

  EEPROM.commit();


  // Visualizza il nome della modalità corrente sul display.
  gfx->fillScreen(BLACK);
  gfx->setFont(u8g2_font_maniac_te);

  // Determina il nome e il colore del testo in base alla modalità corrente.
  const char* title = "MODE:";
  const char* modeName = "";
  uint16_t modeColor = WHITE;

  switch (currentMode) {
    case MODE_FADE:
      modeName = "MODO FADE";
      modeColor = BLUE;
      break;
    case MODE_SLOW:
      modeName = "MODO LENTO";
      modeColor = PURPLE;
      break;
    case MODE_FAST:
      modeName = "MODO VELOCE";
      modeColor = CYAN;
      break;
    case MODE_MATRIX:
      modeName = "MODO MATRIX";
      modeColor = GREEN;
      break;
    case MODE_MATRIX2:
      modeName = "MODO MATRIX 2";
      modeColor = BLUE;
      break;
    case MODE_SNAKE:
      modeName = "MODO SNAKE";
      modeColor = YELLOW;
      break;
    case MODE_WATER:
      modeName = "MODO GOCCIA";
      modeColor = CYAN;
      break;
    case MODE_MARIO:
      modeName = "MODO MARIO";
      modeColor = RED;
      break;
    case MODE_TRON:
      modeName = "MODO TRON";
      modeColor = CYAN;
      break;
#ifdef EFFECT_GALAGA
    case MODE_GALAGA:
      modeName = "MODO GALAGA";
      modeColor = CYAN;  // Ciano brillante
      break;
#endif
#ifdef EFFECT_ANALOG_CLOCK
    case MODE_ANALOG_CLOCK:
      modeName = "OROLOGIO ANALOGICO";
      modeColor = WHITE;
      break;
#endif
#ifdef EFFECT_FLIP_CLOCK
    case MODE_FLIP_CLOCK:
      modeName = "OROLOGIO A PALETTE";
      modeColor = CYAN;
      break;
#endif
#ifdef EFFECT_BTTF
    case MODE_BTTF:
      modeName = "RITORNO AL FUTURO";
      modeColor = GREEN;
      break;
#endif
#ifdef EFFECT_LED_RING
    case MODE_LED_RING:
      modeName = "OROLOGIO LED RING";
      modeColor = CYAN;
      break;
#endif
    default:
      modeName = "MODO SCONOSCIUTO";
      modeColor = WHITE;
      break;
  }

  // Mostra il nome della modalità sul display.
  gfx->fillScreen(BLACK);
  gfx->setFont(u8g2_font_maniac_te);
  gfx->setTextColor(modeColor);

  int textWidth;
  int xPos;

  // Calcola la larghezza approssimativa del testo del titolo per centrarlo.
  textWidth = strlen(title) * 18;  // Stima 18 pixel per carattere.
  xPos = (480 - textWidth) / 2;
  if (xPos < 0) xPos = 0;  // Evita posizioni negative.
  // Mostra il titolo.
  gfx->setCursor(xPos, 210);
  gfx->println(title);

  // Calcola la larghezza approssimativa del testo del nome della modalità per centrarlo.
  textWidth = strlen(modeName) * 18;  // Stima 18 pixel per carattere.
  xPos = (480 - textWidth) / 2;
  if (xPos < 0) xPos = 0;  // Evita posizioni negative.
  // Mostra il nome della modalità.
  gfx->setCursor(xPos, 240);
  gfx->println(modeName);


  // Attende un secondo per mostrare il nome della modalità all'utente.
  delay(1000);

  // Esegue un reset completo del display quando si cambia modalità.
  gfx->setFont(u8g2_font_inb21_mr);
  gfx->fillScreen(BLACK);

  // Forza un aggiornamento immediato del display per avviare la nuova modalità.
  forceDisplayUpdate();
}