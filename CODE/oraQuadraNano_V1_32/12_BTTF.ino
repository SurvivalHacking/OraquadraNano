//http://TUO IP:8080/settime?year=2025&month=1&day=1&hour=0&minute=0&second=0
//PER TESTARE CAPODANNO MA FUNZIONA UNA SOLA VOLTA PER RIPROVARLO RIAVVIA ESP E RISCRIVI LA RIGA SOPRA
#ifdef EFFECT_BTTF
// ================== MODALITÀ RITORNO AL FUTURO (BTTF) con VERO DOUBLE BUFFERING ==================

#include "font/DS_DIGI20pt7b.h"
#include "font/DS_DIGIB20pt7b.h"

// ================== VERO DOUBLE BUFFER ==================
#define BTTF_SCREEN_WIDTH  480
#define BTTF_SCREEN_HEIGHT 480

// Buffer principale dove disegnare tutto PRIMA di trasferirlo al display
uint16_t *frameBuffer = nullptr;

// OffscreenGFX ora è definito nel file principale .ino (condiviso tra tutti i moduli)
OffscreenGFX *offscreenGfx = nullptr;

// Date configurabili
BTTFDate destinationTime = {10, 26, 1985, 1, 20, "AM"};
BTTFDate lastDeparted = {11, 5, 1955, 6, 0, "AM"};

// ================== COORDINATE ==================
#define BTTF_PANEL1_BASE_Y  78
#define BTTF_PANEL2_BASE_Y  248
#define BTTF_PANEL3_BASE_Y  406

#define BTTF_PANEL1_MONTH_X   35
#define BTTF_PANEL1_DAY_X     133
#define BTTF_PANEL1_YEAR_X    200
#define BTTF_PANEL1_AMPM_X    303
#define BTTF_PANEL1_HOUR_X    335
#define BTTF_PANEL1_COLON_X   395
#define BTTF_PANEL1_MIN_X     420

#define BTTF_PANEL2_MONTH_X   35
#define BTTF_PANEL2_DAY_X     133
#define BTTF_PANEL2_YEAR_X    200
#define BTTF_PANEL2_AMPM_X    300
#define BTTF_PANEL2_HOUR_X    330
#define BTTF_PANEL2_COLON_X   390
#define BTTF_PANEL2_MIN_X     415

#define BTTF_PANEL3_MONTH_X   35
#define BTTF_PANEL3_DAY_X     133
#define BTTF_PANEL3_YEAR_X    200
#define BTTF_PANEL3_AMPM_X    300
#define BTTF_PANEL3_HOUR_X    330
#define BTTF_PANEL3_COLON_X   390
#define BTTF_PANEL3_MIN_X     415

#define BTTF_AM_Y_OFFSET  -20
#define BTTF_PM_Y_OFFSET  +11

#define BTTF_COLON_Y_OFFSET_TOP   -15
#define BTTF_COLON_Y_SPACING       13
#define BTTF_COLON_RADIUS           3

// ================== SISTEMA SVEGLIA ==================
#define BUZZER_PIN -1

bool alarmDestinationEnabled = false;
bool alarmLastDepartedEnabled = false;
bool alarmDestinationTriggered = false;
bool alarmLastDepartedTriggered = false;

volatile bool bttfNeedsRedraw = false;

// Colori
#define BTTF_BG_COLOR       0x5ACB
#define BTTF_TEXT_COLOR     0x07E0
#define BTTF_RED_COLOR      0xF800
#define BTTF_AMBER_COLOR    0xFC00

const char* monthNames[] = {
  "JAN", "FEB", "MAR", "APR", "MAY", "JUN",
  "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"
};

// Stato tracking
static uint8_t lastDisplayedHour = 255;
static uint8_t lastDisplayedMinute = 255;
static uint8_t lastDisplayedDay = 255;
static uint8_t lastDisplayedMonth = 255;
static uint16_t lastDisplayedYear = 0;

// Lampeggio
static unsigned long lastColonBlinkTime = 0;
static bool showColon = true;
static bool lastColonState = true;
#define BTTF_COLON_BLINK_INTERVAL 500

// Buffer sfondo
uint16_t *bttfBackgroundBuffer = nullptr;

// ================== FUNZIONI ==================

// Disegna pannello su GFX generico (può essere offscreen o display)
void drawTimePanelGeneric(Arduino_GFX *targetGfx, int16_t baseY, 
                         uint8_t month, uint8_t day, uint16_t year,
                         uint8_t hour, uint8_t minute, const char* ampm, uint16_t labelColor,
                         int16_t monthX, int16_t dayX, int16_t yearX,
                         int16_t ampmX, int16_t hourX, int16_t colonX, int16_t minX, 
                         bool showColon) {

  char monthStr[4], dayStr[3], yearStr[5], hourStr[3], minStr[3];
  strcpy(monthStr, monthNames[month - 1]);
  sprintf(dayStr, "%02d", day);
  sprintf(yearStr, "%04d", year);
  sprintf(hourStr, "%02d", hour);
  sprintf(minStr, "%02d", minute);

  targetGfx->setFont(&DS_DIGIB20pt7b);
  targetGfx->setTextColor(labelColor);
  
  targetGfx->setCursor(monthX, baseY);
  targetGfx->print(monthStr);
  
  targetGfx->setCursor(dayX, baseY);
  targetGfx->print(dayStr);
  
  targetGfx->setCursor(yearX, baseY);
  targetGfx->print(yearStr);

  // Spia AM/PM
  targetGfx->fillCircle(ampmX, baseY + BTTF_AM_Y_OFFSET, 6, 0x0000);
  targetGfx->fillCircle(ampmX, baseY + BTTF_PM_Y_OFFSET, 6, 0x0000);

  if (strcmp(ampm, "PM") == 0) {
    targetGfx->fillCircle(ampmX, baseY + BTTF_PM_Y_OFFSET, 5, labelColor);
  } else {
    targetGfx->fillCircle(ampmX, baseY + BTTF_AM_Y_OFFSET, 5, labelColor);
  }

  targetGfx->setCursor(hourX, baseY);
  targetGfx->print(hourStr);

  // Due punti
  if (showColon) {
    int16_t colonTopY = baseY + BTTF_COLON_Y_OFFSET_TOP;
    targetGfx->fillCircle(colonX, colonTopY, BTTF_COLON_RADIUS, labelColor);
    targetGfx->fillCircle(colonX, colonTopY + BTTF_COLON_Y_SPACING, BTTF_COLON_RADIUS, labelColor);
  }

  targetGfx->setCursor(minX, baseY);
  targetGfx->print(minStr);
}

// Callback JPEG che scrive nel buffer invece che sul display
int bttfJpegDrawCallback(JPEGDRAW *pDraw) {
  if (bttfBackgroundBuffer != nullptr) {
    for (int y = 0; y < pDraw->iHeight; y++) {
      for (int x = 0; x < pDraw->iWidth; x++) {
        int bufferIndex = (pDraw->y + y) * BTTF_SCREEN_WIDTH + (pDraw->x + x);
        if (bufferIndex < BTTF_SCREEN_WIDTH * BTTF_SCREEN_HEIGHT) {
          bttfBackgroundBuffer[bufferIndex] = pDraw->pPixels[y * pDraw->iWidth + x];
        }
      }
    }
  }
  return 1;
}

bool loadBTTFBackgroundToBuffer() {
  String filepath = "/bttf.jpg";

  Serial.printf("[BTTF] Caricamento sfondo: '%s'\n", filepath.c_str());

  if (bttfBackgroundBuffer == nullptr) {
    bttfBackgroundBuffer = (uint16_t*)heap_caps_malloc(
      BTTF_SCREEN_WIDTH * BTTF_SCREEN_HEIGHT * sizeof(uint16_t), 
      MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT
    );
    
    if (bttfBackgroundBuffer == nullptr) {
      bttfBackgroundBuffer = (uint16_t*)ps_malloc(BTTF_SCREEN_WIDTH * BTTF_SCREEN_HEIGHT * sizeof(uint16_t));
      if (bttfBackgroundBuffer == nullptr) {
        Serial.println("[BTTF] ERRORE: Memoria insufficiente!");
        return false;
      }
    }
  }

  if (!SD.exists(filepath.c_str())) {
    Serial.println("[BTTF] File non trovato, uso colore piatto");
    for (int i = 0; i < BTTF_SCREEN_WIDTH * BTTF_SCREEN_HEIGHT; i++) {
      bttfBackgroundBuffer[i] = BTTF_BG_COLOR;
    }
    return false;
  }

  File jpegFile = SD.open(filepath.c_str(), FILE_READ);
  if (!jpegFile) return false;

  size_t fileSize = jpegFile.size();
  uint8_t *jpegBuffer = (uint8_t *)malloc(fileSize);
  if (!jpegBuffer) {
    jpegFile.close();
    return false;
  }

  jpegFile.read(jpegBuffer, fileSize);
  jpegFile.close();

  int result = jpeg.openRAM(jpegBuffer, fileSize, bttfJpegDrawCallback);
  if (result == 1) {
    jpeg.setPixelType(RGB565_LITTLE_ENDIAN);
    jpeg.decode(0, 0, 0);
    jpeg.close();
    Serial.println("[BTTF] Sfondo caricato!");
  }

  free(jpegBuffer);
  return (result == 1);
}

extern bool loadBTTFConfigFromSD();

void initBTTF() {
  if (bttfInitialized) return;

  Serial.println("=== INIT BTTF MODE - VERO DOUBLE BUFFERING ===");

  // Alloca frameBuffer per double buffering
  if (frameBuffer == nullptr) {
    frameBuffer = (uint16_t*)heap_caps_malloc(
      BTTF_SCREEN_WIDTH * BTTF_SCREEN_HEIGHT * sizeof(uint16_t),
      MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT
    );
    
    if (frameBuffer == nullptr) {
      frameBuffer = (uint16_t*)ps_malloc(BTTF_SCREEN_WIDTH * BTTF_SCREEN_HEIGHT * sizeof(uint16_t));
      if (frameBuffer == nullptr) {
        Serial.println("[BTTF] ERRORE: Impossibile allocare frameBuffer!");
        return;
      }
    }
    Serial.println("[BTTF] FrameBuffer allocato");
  }

  // Crea GFX offscreen
  if (offscreenGfx == nullptr) {
    offscreenGfx = new OffscreenGFX(frameBuffer, BTTF_SCREEN_WIDTH, BTTF_SCREEN_HEIGHT);
    Serial.println("[BTTF] OffscreenGFX creato");
  }

  loadBTTFConfigFromSD();
  loadBTTFBackgroundToBuffer();

  lastDisplayedHour = 255;
  lastDisplayedMinute = 255;
  lastDisplayedDay = 255;
  lastDisplayedMonth = 255;
  lastDisplayedYear = 0;

  bttfInitialized = true;
}

void updateBTTF() {
  uint8_t currentHour = myTZ.hour();
  uint8_t currentMinute = myTZ.minute();
  uint8_t currentDay = myTZ.day();
  uint8_t currentMonth = myTZ.month();
  uint16_t currentYear = myTZ.year();

  // Lampeggio
  unsigned long currentTime = millis();
  if (currentTime - lastColonBlinkTime >= BTTF_COLON_BLINK_INTERVAL) {
    showColon = !showColon;
    lastColonBlinkTime = currentTime;
  }

  // Converti 12h
  const char* currentAMPM = "AM";
  uint8_t displayHour = currentHour;

  if (currentHour == 0) {
    displayHour = 12;
  } else if (currentHour == 12) {
    currentAMPM = "PM";
  } else if (currentHour > 12) {
    displayHour = currentHour - 12;
    currentAMPM = "PM";
  }

  if (!bttfInitialized) {
    initBTTF();
  }

  // Aggiornamento COMPLETO
  bool needsFullUpdate = (bttfNeedsRedraw ||
                          lastDisplayedHour != displayHour ||
                          lastDisplayedMinute != currentMinute ||
                          lastDisplayedDay != currentDay ||
                          lastDisplayedMonth != currentMonth ||
                          lastDisplayedYear != currentYear);

  // Aggiornamento PARZIALE (solo due punti)
  bool needsColonUpdate = (showColon != lastColonState);

  if (needsFullUpdate && offscreenGfx != nullptr && frameBuffer != nullptr) {
    Serial.println("[BTTF] *** RENDERING OFFSCREEN - INIZIO ***");

    // ===== FASE 1: DISEGNA TUTTO SUL FRAMEBUFFER (OFFSCREEN) =====
    
    // Copia sfondo nel frameBuffer
    if (bttfBackgroundBuffer != nullptr) {
      memcpy(frameBuffer, bttfBackgroundBuffer, BTTF_SCREEN_WIDTH * BTTF_SCREEN_HEIGHT * sizeof(uint16_t));
    } else {
      for (int i = 0; i < BTTF_SCREEN_WIDTH * BTTF_SCREEN_HEIGHT; i++) {
        frameBuffer[i] = BTTF_BG_COLOR;
      }
    }

    // Disegna i 3 pannelli SULL'OFFSCREEN GFX
    drawTimePanelGeneric(offscreenGfx, BTTF_PANEL1_BASE_Y,
                        destinationTime.month, destinationTime.day, destinationTime.year,
                        destinationTime.hour, destinationTime.minute, destinationTime.ampm,
                        BTTF_RED_COLOR,
                        BTTF_PANEL1_MONTH_X, BTTF_PANEL1_DAY_X, BTTF_PANEL1_YEAR_X,
                        BTTF_PANEL1_AMPM_X, BTTF_PANEL1_HOUR_X, BTTF_PANEL1_COLON_X,
                        BTTF_PANEL1_MIN_X, showColon);

    drawTimePanelGeneric(offscreenGfx, BTTF_PANEL2_BASE_Y,
                        currentMonth, currentDay, currentYear,
                        displayHour, currentMinute, currentAMPM,
                        BTTF_TEXT_COLOR,
                        BTTF_PANEL2_MONTH_X, BTTF_PANEL2_DAY_X, BTTF_PANEL2_YEAR_X,
                        BTTF_PANEL2_AMPM_X, BTTF_PANEL2_HOUR_X, BTTF_PANEL2_COLON_X,
                        BTTF_PANEL2_MIN_X, showColon);

    drawTimePanelGeneric(offscreenGfx, BTTF_PANEL3_BASE_Y,
                        lastDeparted.month, lastDeparted.day, lastDeparted.year,
                        lastDeparted.hour, lastDeparted.minute, lastDeparted.ampm,
                        BTTF_AMBER_COLOR,
                        BTTF_PANEL3_MONTH_X, BTTF_PANEL3_DAY_X, BTTF_PANEL3_YEAR_X,
                        BTTF_PANEL3_AMPM_X, BTTF_PANEL3_HOUR_X, BTTF_PANEL3_COLON_X,
                        BTTF_PANEL3_MIN_X, showColon);

    // Mostra indirizzo IP in basso a sinistra
    offscreenGfx->setFont((const GFXfont *)NULL);  // Font piccolo di default
    offscreenGfx->setTextColor(WHITE);
    offscreenGfx->setCursor(5, 470);
    String ipAddress = WiFi.localIP().toString() + ":8080/bttf";
    offscreenGfx->print(ipAddress);

    Serial.println("[BTTF] Rendering offscreen completato");

    // ===== FASE 2: TRASFERISCI TUTTO AL DISPLAY IN UN COLPO SOLO =====
    Serial.println("[BTTF] Trasferimento al display...");
    
    gfx->draw16bitRGBBitmap(0, 0, frameBuffer, BTTF_SCREEN_WIDTH, BTTF_SCREEN_HEIGHT);
    
    Serial.println("[BTTF] *** ZERO FLICKERING - FATTO! ***");

    bttfNeedsRedraw = false;
    lastDisplayedHour = displayHour;
    lastDisplayedMinute = currentMinute;
    lastDisplayedDay = currentDay;
    lastDisplayedMonth = currentMonth;
    lastDisplayedYear = currentYear;
    lastColonState = showColon;

  } else if (needsColonUpdate) {
    // Aggiorna solo i due punti (veloce)
    gfx->startWrite();
    
    if (showColon) {
      int16_t y1 = BTTF_PANEL1_BASE_Y + BTTF_COLON_Y_OFFSET_TOP;
      gfx->fillCircle(BTTF_PANEL1_COLON_X, y1, BTTF_COLON_RADIUS, BTTF_RED_COLOR);
      gfx->fillCircle(BTTF_PANEL1_COLON_X, y1 + BTTF_COLON_Y_SPACING, BTTF_COLON_RADIUS, BTTF_RED_COLOR);

      int16_t y2 = BTTF_PANEL2_BASE_Y + BTTF_COLON_Y_OFFSET_TOP;
      gfx->fillCircle(BTTF_PANEL2_COLON_X, y2, BTTF_COLON_RADIUS, BTTF_TEXT_COLOR);
      gfx->fillCircle(BTTF_PANEL2_COLON_X, y2 + BTTF_COLON_Y_SPACING, BTTF_COLON_RADIUS, BTTF_TEXT_COLOR);

      int16_t y3 = BTTF_PANEL3_BASE_Y + BTTF_COLON_Y_OFFSET_TOP;
      gfx->fillCircle(BTTF_PANEL3_COLON_X, y3, BTTF_COLON_RADIUS, BTTF_AMBER_COLOR);
      gfx->fillCircle(BTTF_PANEL3_COLON_X, y3 + BTTF_COLON_Y_SPACING, BTTF_COLON_RADIUS, BTTF_AMBER_COLOR);
    } else {
      int16_t y1 = BTTF_PANEL1_BASE_Y + BTTF_COLON_Y_OFFSET_TOP;
      gfx->fillCircle(BTTF_PANEL1_COLON_X, y1, BTTF_COLON_RADIUS, 0x0000);
      gfx->fillCircle(BTTF_PANEL1_COLON_X, y1 + BTTF_COLON_Y_SPACING, BTTF_COLON_RADIUS, 0x0000);

      int16_t y2 = BTTF_PANEL2_BASE_Y + BTTF_COLON_Y_OFFSET_TOP;
      gfx->fillCircle(BTTF_PANEL2_COLON_X, y2, BTTF_COLON_RADIUS, 0x0000);
      gfx->fillCircle(BTTF_PANEL2_COLON_X, y2 + BTTF_COLON_Y_SPACING, BTTF_COLON_RADIUS, 0x0000);

      int16_t y3 = BTTF_PANEL3_BASE_Y + BTTF_COLON_Y_OFFSET_TOP;
      gfx->fillCircle(BTTF_PANEL3_COLON_X, y3, BTTF_COLON_RADIUS, 0x0000);
      gfx->fillCircle(BTTF_PANEL3_COLON_X, y3 + BTTF_COLON_Y_SPACING, BTTF_COLON_RADIUS, 0x0000);
    }
    
    gfx->endWrite();
    lastColonState = showColon;
  }

  checkBTTFAlarms(currentHour, currentMinute, currentDay, currentMonth, currentYear);
}

void forceBTTFRedraw() {
  Serial.println("[BTTF] Ridisegno forzato");
  bttfNeedsRedraw = true;
}

// ================== ALLARMI ==================

void triggerBTTFAlarm(const char* alarmName) {
  Serial.printf("[BTTF ALARM] 🔔 %s\n", alarmName);
  if (BUZZER_PIN >= 0) {
    for (int i = 0; i < 3; i++) {
      tone(BUZZER_PIN, 1000, 200);
      delay(300);
    }
    tone(BUZZER_PIN, 800, 1000);
    delay(1100);
  }
}

void checkBTTFAlarms(uint8_t currentHour, uint8_t currentMinute, uint8_t currentDay,
                    uint8_t currentMonth, uint16_t currentYear) {
  
  if (alarmDestinationEnabled && !alarmDestinationTriggered) {
    if (destinationTime.year == currentYear &&
        destinationTime.month == currentMonth &&
        destinationTime.day == currentDay &&
        destinationTime.hour == currentHour &&
        destinationTime.minute == currentMinute) {
      triggerBTTFAlarm("DESTINATION TIME");
      alarmDestinationTriggered = true;
    }
  }

  if (alarmLastDepartedEnabled && !alarmLastDepartedTriggered) {
    if (lastDeparted.year == currentYear &&
        lastDeparted.month == currentMonth &&
        lastDeparted.day == currentDay &&
        lastDeparted.hour == currentHour &&
        lastDeparted.minute == currentMinute) {
      triggerBTTFAlarm("LAST TIME DEPARTED");
      alarmLastDepartedTriggered = true;
    }
  }

  static uint8_t lastCheckedMinute = 255;
  if (currentMinute != lastCheckedMinute) {
    lastCheckedMinute = currentMinute;
    if (currentMinute != destinationTime.minute) {
      alarmDestinationTriggered = false;
    }
    if (currentMinute != lastDeparted.minute) {
      alarmLastDepartedTriggered = false;
    }
  }
}

#endif // EFFECT_BTTF
// ================== GUIDA PERSONALIZZAZIONE DISPLAY BTTF ==================
//
// FONT UTILIZZATI:
// - DS_DIGIB20pt7b - Font DS-Digital Bold 20pt (stile LED display a 7 segmenti)
//
// COORDINATE DA MODIFICARE PER ALLINEARE ALLA TUA SKIN:
//
// Linee 23-25: Baseline Y dei 3 pannelli
//  - BTTF_PANEL1_BASE_Y = 78   (DESTINATION TIME - pannello alto)
//  - BTTF_PANEL2_BASE_Y = 248  (PRESENT TIME - pannello centro)
//  - BTTF_PANEL3_BASE_Y = 406  (LAST TIME DEPARTED - pannello basso)
//
// Linee 32-57: Coordinate X COMPLETAMENTE SEPARATE per TUTTI i campi di ogni pannello
// Ogni pannello ha controllo completo e indipendente della posizione orizzontale di ogni elemento!
//
//  PANNELLO 1 (DESTINATION TIME - rosso):
//    - BTTF_PANEL1_MONTH_X = 35   (mese 3 lettere)
//    - BTTF_PANEL1_DAY_X = 133    (giorno 2 cifre)
//    - BTTF_PANEL1_YEAR_X = 200   (anno 4 cifre)
//    - BTTF_PANEL1_AMPM_X = 300   (spia LED AM/PM)
//    - BTTF_PANEL1_HOUR_X = 335   (ore 2 cifre)
//    - BTTF_PANEL1_COLON_X = 395  (due punti ":")
//    - BTTF_PANEL1_MIN_X = 420    (minuti 2 cifre)
//
//  PANNELLO 2 (PRESENT TIME - verde):
//    - BTTF_PANEL2_MONTH_X = 35   (mese 3 lettere)
//    - BTTF_PANEL2_DAY_X = 133    (giorno 2 cifre)
//    - BTTF_PANEL2_YEAR_X = 200   (anno 4 cifre)
//    - BTTF_PANEL2_AMPM_X = 300   (spia LED AM/PM)
//    - BTTF_PANEL2_HOUR_X = 330   (ore 2 cifre)
//    - BTTF_PANEL2_COLON_X = 390  (due punti ":")
//    - BTTF_PANEL2_MIN_X = 415    (minuti 2 cifre)
//
//  PANNELLO 3 (LAST TIME DEPARTED - ambra):
//    - BTTF_PANEL3_MONTH_X = 35   (mese 3 lettere)
//    - BTTF_PANEL3_DAY_X = 133    (giorno 2 cifre)
//    - BTTF_PANEL3_YEAR_X = 200   (anno 4 cifre)
//    - BTTF_PANEL3_AMPM_X = 300   (spia LED AM/PM)
//    - BTTF_PANEL3_HOUR_X = 330   (ore 2 cifre)
//    - BTTF_PANEL3_COLON_X = 390  (due punti ":")
//    - BTTF_PANEL3_MIN_X = 415    (minuti 2 cifre)
//
// Linee 60-61: Offset Y per spia AM/PM (separati per AM e PM)
//  - BTTF_AM_Y_OFFSET = -20 (offset AM rispetto al baseline Y)
//  - BTTF_PM_Y_OFFSET = +11 (offset PM rispetto al baseline Y)
//
// Linee 63-65: Parametri DUE PUNTI ":" (completamente personalizzabili)
//  - BTTF_COLON_Y_OFFSET_TOP = -15  (offset Y pallino superiore rispetto al baseline)
//  - BTTF_COLON_Y_SPACING = 13      (distanza verticale tra i due pallini)
//  - BTTF_COLON_RADIUS = 3          (raggio dei pallini)
//
// NOTA: I due punti ":" sono sempre visibili su tutti e 3 i pannelli.
// Il lampeggio è stato disabilitato per evitare flickering del display.
//
// ✅ PERSONALIZZAZIONE COMPLETA ✅
// Ogni pannello ha ora coordinate X completamente indipendenti per TUTTI i campi:
// MONTH, DAY, YEAR, AMPM, HOUR, COLON, MIN
// Puoi posizionare ogni elemento singolarmente in orizzontale per ogni pannello!
//
// Per modificare le date via web: http://<IP_ESP32>:8080/bttf
