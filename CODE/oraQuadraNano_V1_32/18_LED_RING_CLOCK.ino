//===================================================================//
//                   MODALITÀ OROLOGIO LED RING                       //
//===================================================================//
// Questa modalità visualizza l'orario con un anello di LED circolare
// - 60 LED per i minuti disposti in cerchio
// - 12 LED per le ore nelle posizioni classiche dell'orologio
// - Display digitale al centro
//===================================================================//

#ifdef EFFECT_LED_RING

// ================== CONFIGURAZIONE LED RING ==================
#define LED_RING_CENTER_X 240      // Centro X del cerchio (metà del display 480x480)
#define LED_RING_CENTER_Y 240      // Centro Y del cerchio
#define LED_RING_SECOND_RADIUS 165 // Raggio per i LED dei SECONDI (60 LED) - cerchio interno
#define LED_RING_MINUTE_RADIUS 190 // Raggio per i LED dei MINUTI (60 LED) - cerchio medio
#define LED_RING_HOUR_RADIUS 215   // Raggio per i LED delle ORE (12 LED fissi) - cerchio esterno
#define LED_RING_CARDINAL_RADIUS 230 // Raggio per i punti cardinali (12, 3, 6, 9)
#define LED_RING_SECOND_SIZE 3     // Dimensione dei LED dei secondi (più piccoli)
#define LED_RING_MINUTE_SIZE 4     // Dimensione dei LED dei minuti
#define LED_RING_HOUR_SIZE 8       // Dimensione dei LED delle ore (più grandi, fissi)
#define LED_RING_ACTIVE_SIZE 6     // Dimensione dei LED attivi (secondo/minuto corrente)
#define LED_RING_CARDINAL_SIZE 8   // Dimensione dei punti cardinali

// ================== DOUBLE BUFFERING ==================
#define LED_RING_SCREEN_WIDTH  480
#define LED_RING_SCREEN_HEIGHT 480

uint16_t *ledRingFrameBuffer = nullptr;
OffscreenGFX *ledRingOffscreenGfx = nullptr;

// Variabili globali per la modalità LED Ring
bool ledRingInitialized = false;    // Flag per controllare l'inizializzazione
int ledRingLastMinute = -1;         // Ultimo minuto visualizzato
int ledRingLastHour = -1;           // Ultima ora visualizzata
int ledRingLastSecond = -1;         // Ultimo secondo visualizzato
bool ledRingFirstDraw = true;       // Flag per il primo disegno completo
int ledRingLastHourX = -1;          // Ultima posizione X del LED dell'ora
int ledRingLastHourY = -1;          // Ultima posizione Y del LED dell'ora

// ================== FUNZIONI DI UTILITÀ ==================

// Calcola la posizione X di un LED sul cerchio dato l'angolo
int getLedRingX(float angle, int radius) {
  return LED_RING_CENTER_X + (int)(cos(angle) * radius);
}

// Calcola la posizione Y di un LED sul cerchio dato l'angolo
int getLedRingY(float angle, int radius) {
  return LED_RING_CENTER_Y + (int)(sin(angle) * radius);
}

// Converte i minuti in angolo (0 minuti = -90° = top, senso orario)
float minuteToAngle(int minute) {
  // L'angolo parte da -90° (top) e va in senso orario
  // 0 minuti = -90°, 15 minuti = 0°, 30 minuti = 90°, 45 minuti = 180°
  return (minute * 6.0 - 90.0) * PI / 180.0;  // 6° per minuto (360° / 60)
}

// Converte le ore (0-11) in angolo FLUIDO considerando anche i minuti
// Come in un orologio analogico, la lancetta delle ore si sposta gradualmente
float hourToAngle(int hour, int minute) {
  int hour12 = hour % 12;
  // Ogni ora occupa 30 gradi (360° / 12 = 30°)
  // Ogni minuto sposta l'ora di 0.5 gradi (30° / 60 = 0.5°)
  float hourAngle = (hour12 * 30.0) + (minute * 0.5);
  // -90° per far partire le ore 12 dalla posizione in alto
  return (hourAngle - 90.0) * PI / 180.0;
}

// Disegna un singolo LED sul cerchio (con double buffering)
void drawLedRingLed(Arduino_GFX *targetGfx, int x, int y, int size, uint16_t color) {
  targetGfx->fillCircle(x, y, size, color);
  // Aggiungi un effetto glow (alone luminoso)
  targetGfx->drawCircle(x, y, size + 1, color);
}

// Cancella un LED (ridisegna in nero con dimensione maggiore per coprire il glow)
void eraseLedRingLed(Arduino_GFX *targetGfx, int x, int y, int maxSize) {
  targetGfx->fillCircle(x, y, maxSize + 2, BLACK);  // +2 per cancellare anche il glow
}

// ================== FUNZIONE PRINCIPALE DI AGGIORNAMENTO ==================

void updateLedRingClock() {

  // Inizializzazione
  if (!ledRingInitialized) {
    Serial.println("[LED RING] Inizializzazione modalità LED Ring Clock con DOUBLE BUFFERING");

    // Alloca frameBuffer
    if (ledRingFrameBuffer == nullptr) {
      ledRingFrameBuffer = (uint16_t*)heap_caps_malloc(
        LED_RING_SCREEN_WIDTH * LED_RING_SCREEN_HEIGHT * sizeof(uint16_t),
        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT
      );

      if (ledRingFrameBuffer == nullptr) {
        ledRingFrameBuffer = (uint16_t*)ps_malloc(LED_RING_SCREEN_WIDTH * LED_RING_SCREEN_HEIGHT * sizeof(uint16_t));
        if (ledRingFrameBuffer == nullptr) {
          Serial.println("[LED RING] ERRORE: Impossibile allocare frameBuffer!");
          return;
        }
      }
      Serial.println("[LED RING] FrameBuffer allocato");
    }

    // Crea GFX offscreen
    if (ledRingOffscreenGfx == nullptr) {
      ledRingOffscreenGfx = new OffscreenGFX(ledRingFrameBuffer, LED_RING_SCREEN_WIDTH, LED_RING_SCREEN_HEIGHT);
      Serial.println("[LED RING] OffscreenGFX creato");
    }

    gfx->fillScreen(BLACK);
    ledRingInitialized = true;
    ledRingLastMinute = -1;
    ledRingLastHour = -1;
    ledRingLastSecond = -1;
    ledRingLastHourX = -1;
    ledRingLastHourY = -1;
    ledRingFirstDraw = true;

    Serial.print("[LED RING] Ora iniziale: ");
    Serial.print(currentHour);
    Serial.print(":");
    Serial.println(currentMinute);
  }

  // Controlla se è necessario aggiornare il display
  bool minuteChanged = (ledRingLastMinute != currentMinute);
  bool hourChanged = (ledRingLastHour != currentHour);
  bool secondChanged = (ledRingLastSecond != currentSecond);

  // MEZZANOTTE: Azzera tutti i LED quando è esattamente 00:00:00
  if ((currentHour == 0 && currentMinute == 0 && currentSecond == 0) && secondChanged) {
    Serial.println("[LED RING] MEZZANOTTE (00:00:00) - Azzero TUTTI i LED circolari");

    // Prepara buffer nero
    for (int i = 0; i < LED_RING_SCREEN_WIDTH * LED_RING_SCREEN_HEIGHT; i++) {
      ledRingFrameBuffer[i] = BLACK;
    }

    ledRingFirstDraw = true;        // Forza ridisegno completo dei LED
    ledRingLastMinute = -1;         // Reset variabili
    ledRingLastHour = -1;
    ledRingLastHourX = -1;
    ledRingLastHourY = -1;
    // NON resetto ledRingLastSecond per non cancellare il display digitale
  }

  // Se non serve aggiornamento, esci
  if (!minuteChanged && !hourChanged && !secondChanged && !ledRingFirstDraw) {
    return;
  }

  // ===== FASE 1: RENDERING OFFSCREEN (su frameBuffer) =====
  Serial.println("[LED RING] *** RENDERING OFFSCREEN - INIZIO ***");

  // Memorizza se è il primo disegno (prima di azzerare il flag)
  bool isFirstDraw = ledRingFirstDraw;

  // Se è il primo disegno, inizializza tutto lo sfondo nero
  if (isFirstDraw) {
    Serial.println("[LED RING] Primo disegno - disegno tutti i LED");
    for (int i = 0; i < LED_RING_SCREEN_WIDTH * LED_RING_SCREEN_HEIGHT; i++) {
      ledRingFrameBuffer[i] = BLACK;
    }
    ledRingFirstDraw = false;
  }

  // ========== AGGIORNA I LED DELLE ORE (12 LED BLU FISSI + 1 LED ARANCIONE MOBILE) ==========
  if (hourChanged || minuteChanged || isFirstDraw) {
    int currentHour12 = currentHour % 12;

    if (isFirstDraw) {
      Serial.print("[LED RING] Prima visualizzazione ore: ");
      Serial.print(currentHour12);
      Serial.print(":");
      Serial.println(currentMinute);
    }

    // Calcola la nuova posizione del LED ARANCIONE dell'ora (considerando anche i minuti)
    float angle = hourToAngle(currentHour, currentMinute);
    int newX = getLedRingX(angle, LED_RING_HOUR_RADIUS);
    int newY = getLedRingY(angle, LED_RING_HOUR_RADIUS);

    // STEP 1: Cancella il LED arancione dalla posizione precedente (se diversa)
    if (!isFirstDraw && (newX != ledRingLastHourX || newY != ledRingLastHourY)) {
      if (ledRingLastHourX != -1 && ledRingLastHourY != -1) {
        eraseLedRingLed(ledRingOffscreenGfx, ledRingLastHourX, ledRingLastHourY, LED_RING_HOUR_SIZE);
      }
    }

    // STEP 2: RIDISEGNA SEMPRE tutti i 12 LED BLU FISSI delle ore
    // Questo garantisce che non vengano mai cancellati e ripristina l'area cancellata
    for (int hour = 0; hour < 12; hour++) {
      float fixedAngle = hourToAngle(hour, 0);  // Posizioni fisse (minuto 0)
      int x = getLedRingX(fixedAngle, LED_RING_HOUR_RADIUS);
      int y = getLedRingY(fixedAngle, LED_RING_HOUR_RADIUS);
      // LED blu fisso di dimensione media
      drawLedRingLed(ledRingOffscreenGfx, x, y, 5, RGB565(0, 100, 255));  // Blu medio
    }

    // STEP 3: DISEGNA il LED ARANCIONE nella nuova posizione (SOPRA i LED blu)
    if (isFirstDraw || newX != ledRingLastHourX || newY != ledRingLastHourY) {
      int redValue = 200 + (currentHour12 * 55 / 12);
      int greenValue = 100 - (currentHour12 * 80 / 12);
      uint16_t hourColor = RGB565(redValue, greenValue, 0);  // Arancione/rosso
      drawLedRingLed(ledRingOffscreenGfx, newX, newY, LED_RING_HOUR_SIZE, hourColor);

      // Salva la nuova posizione
      ledRingLastHourX = newX;
      ledRingLastHourY = newY;

      if (!isFirstDraw) {
        Serial.print("[LED RING] LED ora arancione spostato a: ");
        Serial.print(currentHour);
        Serial.print(":");
        Serial.println(currentMinute);
      }
    }

    ledRingLastHour = currentHour;
  }

  // ========== AGGIORNA I LED DEI MINUTI (cerchio medio - 60 LED) ==========
  if (minuteChanged || isFirstDraw) {

    // AZZERA i LED dei minuti quando i minuti tornano a 0 (cambio ora)
    if (currentMinute == 0 && hourChanged) {
      Serial.println("[LED RING] Minuti a 0 - Azzero tutti i LED dei minuti");
      // Cancella tutti i LED dei minuti uno a uno
      for (int minute = 1; minute < 60; minute++) {  // Da 1 a 59 (lo 0 verrà disegnato dopo)
        float angle = minuteToAngle(minute);
        int x = getLedRingX(angle, LED_RING_MINUTE_RADIUS);
        int y = getLedRingY(angle, LED_RING_MINUTE_RADIUS);
        eraseLedRingLed(ledRingOffscreenGfx, x, y, LED_RING_ACTIVE_SIZE);
      }

      // Disegna immediatamente il minuto 0
      float angle = minuteToAngle(0);
      int x = getLedRingX(angle, LED_RING_MINUTE_RADIUS);
      int y = getLedRingY(angle, LED_RING_MINUTE_RADIUS);
      drawLedRingLed(ledRingOffscreenGfx, x, y, LED_RING_ACTIVE_SIZE, RGB565(0, 200, 255));
      ledRingLastMinute = 0;

      // Trasferisci al display
      gfx->draw16bitRGBBitmap(0, 0, ledRingFrameBuffer, LED_RING_SCREEN_WIDTH, LED_RING_SCREEN_HEIGHT);
      return;  // Esci, già gestito
    }

    // Se è il primo disegno, disegna tutti i LED dei minuti
    if (isFirstDraw) {
      for (int minute = 0; minute < 60; minute++) {
        float angle = minuteToAngle(minute);
        int x = getLedRingX(angle, LED_RING_MINUTE_RADIUS);
        int y = getLedRingY(angle, LED_RING_MINUTE_RADIUS);

        if (minute <= currentMinute) {
          // LED acceso - gradiente blu
          int blueValue = 150 + (minute * 105 / 60);  // Da 150 a 255
          int greenValue = 50 + (minute * 100 / 60);  // Da 50 a 150
          uint16_t ledColor = RGB565(0, greenValue, blueValue);

          if (minute == currentMinute) {
            drawLedRingLed(ledRingOffscreenGfx, x, y, LED_RING_ACTIVE_SIZE, RGB565(0, 200, 255));  // Blu brillante
          } else {
            drawLedRingLed(ledRingOffscreenGfx, x, y, LED_RING_MINUTE_SIZE, ledColor);
          }
        } else {
          // LED spento - piccolo punto scuro ogni 5 minuti
          if (minute % 5 == 0) {
            ledRingOffscreenGfx->fillCircle(x, y, 2, RGB565(0, 20, 40));
          }
        }
      }
    } else {
      // Aggiornamento normale: ridisegna solo il minuto precedente e quello corrente

      // Ridisegna il minuto precedente con dimensione normale
      if (ledRingLastMinute != -1 && ledRingLastMinute != currentMinute) {
        float angle = minuteToAngle(ledRingLastMinute);
        int x = getLedRingX(angle, LED_RING_MINUTE_RADIUS);
        int y = getLedRingY(angle, LED_RING_MINUTE_RADIUS);

        // PRIMA: Cancella il LED grande precedente
        eraseLedRingLed(ledRingOffscreenGfx, x, y, LED_RING_ACTIVE_SIZE);

        // POI: Ridisegna con dimensione normale
        int blueValue = 150 + (ledRingLastMinute * 105 / 60);
        int greenValue = 50 + (ledRingLastMinute * 100 / 60);
        uint16_t ledColor = RGB565(0, greenValue, blueValue);
        drawLedRingLed(ledRingOffscreenGfx, x, y, LED_RING_MINUTE_SIZE, ledColor);
      }

      // Disegna il nuovo minuto corrente più grande e luminoso
      float angle = minuteToAngle(currentMinute);
      int x = getLedRingX(angle, LED_RING_MINUTE_RADIUS);
      int y = getLedRingY(angle, LED_RING_MINUTE_RADIUS);

      // PRIMA: Cancella eventuale LED vecchio
      eraseLedRingLed(ledRingOffscreenGfx, x, y, LED_RING_ACTIVE_SIZE);

      // POI: Disegna il nuovo LED grande
      drawLedRingLed(ledRingOffscreenGfx, x, y, LED_RING_ACTIVE_SIZE, RGB565(0, 200, 255));
    }

    ledRingLastMinute = currentMinute;
  }

  // ========== AGGIORNA I LED DEI SECONDI (cerchio interno - 60 LED) ==========
  if (secondChanged || isFirstDraw) {
    static int lastSecond = -1;

    // AZZERA i LED dei secondi quando i secondi tornano a 0 (cambio minuto)
    if (currentSecond == 0 && minuteChanged) {
      Serial.println("[LED RING] Secondi a 0 - Azzero tutti i LED dei secondi");
      // Cancella tutti i LED dei secondi uno a uno
      for (int second = 1; second < 60; second++) {  // Da 1 a 59 (lo 0 verrà disegnato dopo)
        float angle = minuteToAngle(second);
        int x = getLedRingX(angle, LED_RING_SECOND_RADIUS);
        int y = getLedRingY(angle, LED_RING_SECOND_RADIUS);
        eraseLedRingLed(ledRingOffscreenGfx, x, y, LED_RING_ACTIVE_SIZE);
      }
      lastSecond = -1;  // Reset

      // Disegna immediatamente il secondo 0
      float angle = minuteToAngle(0);
      int x = getLedRingX(angle, LED_RING_SECOND_RADIUS);
      int y = getLedRingY(angle, LED_RING_SECOND_RADIUS);
      drawLedRingLed(ledRingOffscreenGfx, x, y, LED_RING_ACTIVE_SIZE, RGB565(0, 255, 255));
      lastSecond = 0;

      // Trasferisci al display
      gfx->draw16bitRGBBitmap(0, 0, ledRingFrameBuffer, LED_RING_SCREEN_WIDTH, LED_RING_SCREEN_HEIGHT);
      return;  // Esci, già gestito
    }

    // Se è il primo disegno, disegna tutti i LED dei secondi
    if (isFirstDraw) {
      for (int second = 0; second < 60; second++) {
        float angle = minuteToAngle(second);
        int x = getLedRingX(angle, LED_RING_SECOND_RADIUS);
        int y = getLedRingY(angle, LED_RING_SECOND_RADIUS);

        if (second <= currentSecond) {
          // LED acceso - gradiente verde/ciano
          int greenValue = 255 - (second * 155 / 60);  // Da 255 a 100
          int blueValue = 100 + (second * 155 / 60);   // Da 100 a 255
          uint16_t ledColor = RGB565(0, greenValue, blueValue);

          if (second == currentSecond) {
            drawLedRingLed(ledRingOffscreenGfx, x, y, LED_RING_ACTIVE_SIZE, RGB565(0, 255, 255));  // Ciano brillante
          } else {
            drawLedRingLed(ledRingOffscreenGfx, x, y, LED_RING_SECOND_SIZE, ledColor);
          }
        }
      }
      lastSecond = currentSecond;
    } else {
      // Aggiornamento normale: controlla se ci sono secondi saltati e riempili

      // RILEVAMENTO SECONDI SALTATI
      if (lastSecond != -1 && lastSecond != currentSecond) {
        int missedCount = 0;

        // Calcola quanti secondi sono stati saltati
        if (currentSecond > lastSecond) {
          // Caso normale: da 48 a 50 (saltato 49)
          missedCount = currentSecond - lastSecond - 1;
        }

        if (missedCount > 0) {
          Serial.print("[LED RING] ⚠️ SECONDI SALTATI RILEVATI! Da ");
          Serial.print(lastSecond);
          Serial.print(" a ");
          Serial.print(currentSecond);
          Serial.print(" (saltati: ");
          Serial.print(missedCount);
          Serial.println(")");

          // Riempi tutti i secondi mancanti
          for (int second = lastSecond + 1; second < currentSecond; second++) {
            float angle = minuteToAngle(second);
            int x = getLedRingX(angle, LED_RING_SECOND_RADIUS);
            int y = getLedRingY(angle, LED_RING_SECOND_RADIUS);

            // Disegna il LED mancante con dimensione normale
            int greenValue = 255 - (second * 155 / 60);
            int blueValue = 100 + (second * 155 / 60);
            uint16_t ledColor = RGB565(0, greenValue, blueValue);
            drawLedRingLed(ledRingOffscreenGfx, x, y, LED_RING_SECOND_SIZE, ledColor);

            Serial.print("[LED RING] 🔧 Riempito secondo mancante: ");
            Serial.println(second);
          }
        }

        // Ridisegna il secondo precedente con dimensione normale
        float angle = minuteToAngle(lastSecond);
        int x = getLedRingX(angle, LED_RING_SECOND_RADIUS);
        int y = getLedRingY(angle, LED_RING_SECOND_RADIUS);

        // PRIMA: Cancella il LED grande precedente
        eraseLedRingLed(ledRingOffscreenGfx, x, y, LED_RING_ACTIVE_SIZE);

        // POI: Ridisegna con dimensione normale
        int greenValue = 255 - (lastSecond * 155 / 60);
        int blueValue = 100 + (lastSecond * 155 / 60);
        uint16_t ledColor = RGB565(0, greenValue, blueValue);
        drawLedRingLed(ledRingOffscreenGfx, x, y, LED_RING_SECOND_SIZE, ledColor);
      }

      // Disegna il nuovo secondo corrente più grande e luminoso
      float angle = minuteToAngle(currentSecond);
      int x = getLedRingX(angle, LED_RING_SECOND_RADIUS);
      int y = getLedRingY(angle, LED_RING_SECOND_RADIUS);

      // PRIMA: Cancella eventuale LED vecchio (solo se non è il precedente appena ridisegnato)
      if (lastSecond != currentSecond) {
        eraseLedRingLed(ledRingOffscreenGfx, x, y, LED_RING_ACTIVE_SIZE);
      }

      // POI: Disegna il nuovo LED grande
      drawLedRingLed(ledRingOffscreenGfx, x, y, LED_RING_ACTIVE_SIZE, RGB565(0, 255, 255));

      lastSecond = currentSecond;
    }
  }

  // ========== DISPLAY DIGITALE AL CENTRO ==========
  // Aggiorna SEMPRE (per mostrare i secondi che cambiano)

  // Definisci l'area del display digitale
  int displayWidth = 160;
  int displayHeight = 80;
  int displayX = LED_RING_CENTER_X - displayWidth / 2;
  int displayY = LED_RING_CENTER_Y - displayHeight / 2;

  // CANCELLA completamente l'area del display con un rettangolo nero
  ledRingOffscreenGfx->fillRect(displayX, displayY, displayWidth, displayHeight, BLACK);

  // Prepara la stringa dell'orario
  char timeStr[6];
  sprintf(timeStr, "%02d:%02d", currentHour, currentMinute);

  // Configura il font per il display digitale
  ledRingOffscreenGfx->setFont(u8g2_font_logisoso32_tn);   // Font digitale grande

  // Calcola la posizione per centrare il testo
  int textX = LED_RING_CENTER_X - 62;  // Aggiustato per centrare meglio
  int textY = LED_RING_CENTER_Y + 10;

  // Disegna l'orario digitale
  ledRingOffscreenGfx->setTextColor(RGB565(0, 255, 200));  // Ciano verdastro
  ledRingOffscreenGfx->setCursor(textX, textY);
  ledRingOffscreenGfx->print(timeStr);

  // Aggiungi secondi in piccolo sotto l'orario principale
  char secStr[3];
  sprintf(secStr, "%02d", currentSecond);

  ledRingOffscreenGfx->setFont(u8g2_font_logisoso16_tn);  // Font più piccolo per i secondi
  int secTextX = LED_RING_CENTER_X - 12;  // Centrato
  int secTextY = textY + 28;

  ledRingOffscreenGfx->setTextColor(RGB565(100, 150, 200));  // Blu più tenue per i secondi
  ledRingOffscreenGfx->setCursor(secTextX, secTextY);
  ledRingOffscreenGfx->print(secStr);

  // Ripristina il font predefinito
  ledRingOffscreenGfx->setFont(u8g2_font_inb21_mr);

  ledRingLastSecond = currentSecond;

  if (secondChanged) {
    Serial.print("[LED RING] Aggiornato: ");
    Serial.print(timeStr);
    Serial.print(":");
    Serial.println(secStr);
  }

  Serial.println("[LED RING] Rendering offscreen completato");

  // ===== FASE 2: TRASFERISCI TUTTO AL DISPLAY IN UN COLPO SOLO =====
  Serial.println("[LED RING] Trasferimento al display...");
  gfx->draw16bitRGBBitmap(0, 0, ledRingFrameBuffer, LED_RING_SCREEN_WIDTH, LED_RING_SCREEN_HEIGHT);
  Serial.println("[LED RING] *** ZERO FLICKERING - FATTO! ***");
}

#endif // EFFECT_LED_RING
