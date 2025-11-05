// ================== FLIP CLOCK - OROLOGIO A PALETTE FLIP ==================
/*
 * Simulazione di orologio a palette meccaniche stile vintage anni '70
 * Display tipo aeroporti/stazioni ferroviarie
 *
 * Features:
 * - 4 palette per HH:MM
 * - Animazione flip 3D quando cambiano le cifre
 * - Suono click meccanico (via ESP32C3)
 * - Design retrò con ombreggiature
 */

#ifdef EFFECT_FLIP_CLOCK

// ================== CONFIGURAZIONE FLIP CLOCK ==================
/*
 * FONT DISPONIBILI PER FLIP CLOCK (stile meccanico vintage):
 *
 * ✓ ATTIVO: u8g2_font_logisoso42_tn / u8g2_font_logisoso16_tn
 *   - Stile meccanico/industriale PERFETTO per flip clock vintage
 *   - Numeri ben spaziati e chiari
 *   - Aspect ratio ideale per palette
 *   - HH:MM: logisoso42 (grande e ben visibile)
 *   - SS: logisoso16 (piccolo, compatto)
 *
 * Alternative da testare (modificare in drawDigit e drawDigitHalf):
 *   - u8g2_font_logisoso32_tn (versione più piccola)
 *   - u8g2_font_fub42_tn / u8g2_font_fub20_tn
 *     (Free Universal Bold - molto chiaro e leggibile)
 *   - u8g2_font_profont29_tn / u8g2_font_profont17_tn
 *     (Programmers Font - stile terminale/display)
 *   - u8g2_font_inb42_mn / u8g2_font_inb19_mn
 *     (Inconsolata Bold - font precedente, più classico)
 */

// Palette HH:MM (grandi, in alto)
#define FLAP_WIDTH      90      // Larghezza palette ore/minuti
#define FLAP_HEIGHT     140     // Altezza palette ore/minuti
#define FLAP_GAP        20      // Spazio tra palette

// Palette SS (piccole, sotto, centrate)
#define FLAP_SEC_WIDTH      50      // Larghezza palette secondi (molto più piccole)
#define FLAP_SEC_HEIGHT     80      // Altezza palette secondi (molto più piccole)
#define FLAP_SEC_GAP        10      // Spazio tra palette secondi

// Animazione
#define FLIP_FRAMES     18      // Frame totali animazione flip (aumentato per più fluidità)
#define FLIP_DURATION   600     // Durata flip in ms (come web: 0.3s top + 0.6s bottom max)

// Timing animazione a 2 fasi (ispirato al flip clock web)
#define FLIP_PHASE1_FRAMES  6   // Fase 1: metà superiore scende (0.3s)
#define FLIP_PHASE2_FRAMES  12  // Fase 2: metà inferiore sale (0.6s)

// Lampeggio separatori
#define BLINK_INTERVAL  500     // Intervallo lampeggio separatori in ms

// Display digitale (LCD/LED style)
#define DIGITAL_WIDTH       200     // Larghezza box display digitale
#define DIGITAL_HEIGHT      60      // Altezza box display digitale
#define DIGITAL_Y           60      // Posizione Y (sopra flip clock)

// Colori design vintage con maggiore profondità
#define COLOR_FLAP_BG       0x0000  // Nero
#define COLOR_FLAP_TEXT     0xFFFF  // Bianco
#define COLOR_FLAP_SHADOW   0x2104  // Grigio più scuro per profondità
#define COLOR_FLAP_BORDER   0x8410  // Grigio medio
#define COLOR_BG_FLIP       0x1082  // Blu scuro vintage
#define COLOR_FLAP_DIVIDER  0x5ACB  // Grigio chiaro per linea divisoria

// Colori display digitale (stile LCD/LED verde classico)
#define COLOR_DIGITAL_BG    0x0320  // Verde molto scuro (LCD spento)
#define COLOR_DIGITAL_TEXT  0x07E0  // Verde brillante (LED/LCD acceso)
#define COLOR_DIGITAL_BORDER 0x2104 // Grigio scuro per bordo
#define COLOR_DIGITAL_FRAME  0x4208 // Grigio medio per cornice esterna

// ================== STRUTTURA PALETTE ==================
struct Flap {
  int16_t x, y;              // Posizione
  uint8_t currentDigit;      // Cifra attuale (0-9)
  uint8_t targetDigit;       // Cifra destinazione
  bool isFlipping;           // Animazione in corso
  uint8_t flipFrame;         // Frame corrente (0-FLIP_FRAMES)
  unsigned long flipStartTime;
  bool isSecond;             // true se è una palette dei secondi (più piccola)
};

// 6 palette: 4 per HH:MM + 2 per SS
Flap flaps[6];

// Stato precedente per rilevare cambiamenti
uint8_t lastHourTens = 99;
uint8_t lastHourOnes = 99;
uint8_t lastMinTens = 99;
uint8_t lastMinOnes = 99;
uint8_t lastSecTens = 99;
uint8_t lastSecOnes = 99;

// Stato lampeggio separatori
unsigned long lastBlinkTime = 0;
bool separatorsVisible = true;

// Stato display digitale
uint8_t lastDigitalHour = 99;
uint8_t lastDigitalMinute = 99;

// Stato data
uint8_t lastDay = 99;
uint8_t lastMonth = 99;
uint16_t lastYear = 9999;
uint8_t lastDayOfWeek = 99;

// Array nomi in italiano
const char* daysOfWeek[] = {"Domenica", "Lunedì", "Martedì", "Mercoledì", "Giovedì", "Venerdì", "Sabato"};
const char* months[] = {"Gennaio", "Febbraio", "Marzo", "Aprile", "Maggio", "Giugno",
                        "Luglio", "Agosto", "Settembre", "Ottobre", "Novembre", "Dicembre"};

// ================== PROTOTIPI FUNZIONI ==================
// NOTA: initFlipClock() e updateFlipClock() sono dichiarati nel file principale
void drawFlipClock();
void drawFlap(Flap &flap, bool drawStatic);
void drawFlapBorders(Flap &flap);  // Disegna SOLO bordi (per init secondi)
void drawFlapContent(Flap &flap);  // Disegna SOLO contenuto numeri (per update secondi)
void drawFlipAnimation(Flap &flap);
void drawFlipAnimationContent(Flap &flap);  // Animazione SOLO contenuto (per secondi)
void drawDigit(int16_t x, int16_t y, uint8_t digit, int16_t w, int16_t h, bool inverted, bool isSecond);
void drawDigitHalf(int16_t x, int16_t y, uint8_t digit, int16_t w, int16_t fullHeight, int16_t halfHeight, bool topHalf, bool isSecond);
void startFlip(uint8_t flapIndex, uint8_t newDigit);
void drawSeparator();  // Un solo separatore (HH:MM)
void redrawSeparatorOnly();  // Ridisegna SOLO il separatore (per lampeggio)
void playFlipSound(uint8_t numClacks, uint8_t flapIndex);

// Display digitale
void drawDigitalDisplay(uint8_t hour, uint8_t minute, bool redrawAll);
void drawDigitalFrame();  // Disegna SOLO la cornice (una volta all'init)

// Data completa
void drawDate(uint8_t dayOfWeek, uint8_t day, uint8_t month, uint16_t year, bool redrawAll);

// Funzioni di easing (ispirate al flip clock web)
float easingTopFlip(float t);    // Per metà superiore: cubic-bezier(0.37, 0.01, 0.94, 0.35)
float easingBottomFlip(float t); // Per metà inferiore: cubic-bezier(0.15, 0.45, 0.28, 1)

// ================== INIZIALIZZAZIONE ==================
void initFlipClock() {
  if (flipClockInitialized) {
    return;
  }

  Serial.println("=== INIT FLIP CLOCK ===");

  // Sfondo
  gfx->fillScreen(COLOR_BG_FLIP);

  // ===== LAYOUT =====
  // HH:MM in alto (grandi)
  // SS sotto centrato (piccole)

  // Calcola posizione HH:MM (4 palette + separatore)
  int totalWidthHHMM = (FLAP_WIDTH * 4) + (FLAP_GAP * 3) + 20; // +20 per separatore :
  int startXHHMM = (480 - totalWidthHHMM) / 2;
  int startYHHMM = 150;  // Più in alto per far spazio ai secondi sotto

  // Calcola posizione SS (2 palette piccole sotto, centrate)
  int totalWidthSS = (FLAP_SEC_WIDTH * 2) + FLAP_SEC_GAP;
  int startXSS = (480 - totalWidthSS) / 2;
  int startYSS = startYHHMM + FLAP_HEIGHT + 30;  // 30px sotto HH:MM

  // Leggi ora corrente per inizializzare le palette
  uint8_t hour = currentHour;
  uint8_t minute = currentMinute;
  uint8_t second = currentSecond;

  if (testModeEnabled) {
    calculateTestTime(hour, minute, second);
  }

  uint8_t hourTens = hour / 10;
  uint8_t hourOnes = hour % 10;
  uint8_t minTens = minute / 10;
  uint8_t minOnes = minute % 10;
  uint8_t secTens = second / 10;
  uint8_t secOnes = second % 10;

  // ===== PALETTE HH:MM (grandi, in alto) =====
  // Palette ore (decine)
  flaps[0].x = startXHHMM;
  flaps[0].y = startYHHMM;
  flaps[0].currentDigit = hourTens;
  flaps[0].targetDigit = hourTens;
  flaps[0].isFlipping = false;
  flaps[0].flipFrame = 0;
  flaps[0].isSecond = false;

  // Palette ore (unità)
  flaps[1].x = startXHHMM + FLAP_WIDTH + FLAP_GAP;
  flaps[1].y = startYHHMM;
  flaps[1].currentDigit = hourOnes;
  flaps[1].targetDigit = hourOnes;
  flaps[1].isFlipping = false;
  flaps[1].flipFrame = 0;
  flaps[1].isSecond = false;

  // Palette minuti (decine) - dopo separatore
  flaps[2].x = startXHHMM + (FLAP_WIDTH * 2) + (FLAP_GAP * 2) + 20;
  flaps[2].y = startYHHMM;
  flaps[2].currentDigit = minTens;
  flaps[2].targetDigit = minTens;
  flaps[2].isFlipping = false;
  flaps[2].flipFrame = 0;
  flaps[2].isSecond = false;

  // Palette minuti (unità)
  flaps[3].x = startXHHMM + (FLAP_WIDTH * 3) + (FLAP_GAP * 3) + 20;
  flaps[3].y = startYHHMM;
  flaps[3].currentDigit = minOnes;
  flaps[3].targetDigit = minOnes;
  flaps[3].isFlipping = false;
  flaps[3].flipFrame = 0;
  flaps[3].isSecond = false;

  // ===== PALETTE SS (piccole, sotto, centrate) =====
  // Palette secondi (decine)
  flaps[4].x = startXSS;
  flaps[4].y = startYSS;
  flaps[4].currentDigit = secTens;
  flaps[4].targetDigit = secTens;
  flaps[4].isFlipping = false;
  flaps[4].flipFrame = 0;
  flaps[4].isSecond = true;  // PALETTE SECONDI

  // Palette secondi (unità)
  flaps[5].x = startXSS + FLAP_SEC_WIDTH + FLAP_SEC_GAP;
  flaps[5].y = startYSS;
  flaps[5].currentDigit = secOnes;
  flaps[5].targetDigit = secOnes;
  flaps[5].isFlipping = false;
  flaps[5].flipFrame = 0;
  flaps[5].isSecond = true;  // PALETTE SECONDI

  // Inizializza anche le variabili last* per evitare flip immediato
  lastHourTens = hourTens;
  lastHourOnes = hourOnes;
  lastMinTens = minTens;
  lastMinOnes = minOnes;
  lastSecTens = secTens;
  lastSecOnes = secOnes;

  // Inizializza stato lampeggio
  lastBlinkTime = millis();
  separatorsVisible = true;

  // Disegna stato iniziale
  drawFlipClock();

  // Display digitale LCD/LED (inizializzazione completa)
  lastDigitalHour = hour;
  lastDigitalMinute = minute;
  drawDigitalDisplay(hour, minute, true);  // redrawAll=true per init

  // Data completa (inizializzazione)
  uint8_t dayOfWeek = myTZ.weekday();  // 1=Domenica, 2=Lunedì, ..., 7=Sabato
  uint8_t day = myTZ.day();
  uint8_t month = myTZ.month();
  uint16_t year = myTZ.year();

  // Converti dayOfWeek da 1-7 a indice array 0-6
  if (dayOfWeek >= 1 && dayOfWeek <= 7) {
    dayOfWeek = dayOfWeek - 1;
  }

  drawDate(dayOfWeek, day, month, year, true);  // redrawAll=true per init

  flipClockInitialized = true;
  Serial.println("Flip Clock inizializzato (con display digitale LCD/LED + data)");
}

// ================== UPDATE ==================
void updateFlipClock() {
  // Ottieni ora corrente
  uint8_t hour = currentHour;
  uint8_t minute = currentMinute;
  uint8_t second = currentSecond;

  if (testModeEnabled) {
    calculateTestTime(hour, minute, second);
  }

  // Calcola cifre
  uint8_t hourTens = hour / 10;
  uint8_t hourOnes = hour % 10;
  uint8_t minTens = minute / 10;
  uint8_t minOnes = minute % 10;
  uint8_t secTens = second / 10;
  uint8_t secOnes = second % 10;

  // Rileva cambiamenti e avvia flip
  if (hourTens != lastHourTens) {
    startFlip(0, hourTens);
    lastHourTens = hourTens;
  }
  if (hourOnes != lastHourOnes) {
    startFlip(1, hourOnes);
    lastHourOnes = hourOnes;
  }
  if (minTens != lastMinTens) {
    startFlip(2, minTens);
    lastMinTens = minTens;
  }
  if (minOnes != lastMinOnes) {
    startFlip(3, minOnes);
    lastMinOnes = minOnes;
  }
  if (secTens != lastSecTens) {
    startFlip(4, secTens);
    lastSecTens = secTens;
  }
  if (secOnes != lastSecOnes) {
    startFlip(5, secOnes);
    lastSecOnes = secOnes;
  }

  // Aggiorna display digitale (solo se cambia ora o minuto)
  if (hour != lastDigitalHour || minute != lastDigitalMinute) {
    drawDigitalDisplay(hour, minute, false);  // redrawAll=false per update
  }

  // Aggiorna data (solo se cambia)
  uint8_t dayOfWeek = myTZ.weekday();  // 1=Domenica, 2=Lunedì, ..., 7=Sabato
  uint8_t day = myTZ.day();
  uint8_t month = myTZ.month();
  uint16_t year = myTZ.year();

  // Converti dayOfWeek da 1-7 a indice array 0-6
  if (dayOfWeek >= 1 && dayOfWeek <= 7) {
    dayOfWeek = dayOfWeek - 1;
  }

  // Ridisegna solo se la data è cambiata
  if (day != lastDay || month != lastMonth || year != lastYear || dayOfWeek != lastDayOfWeek) {
    drawDate(dayOfWeek, day, month, year, false);  // redrawAll=false per update
  }

  // Gestione lampeggio separatori (ridisegna SOLO i separatori)
  unsigned long currentTime = millis();
  static bool lastSeparatorState = true;
  if (currentTime - lastBlinkTime >= BLINK_INTERVAL) {
    separatorsVisible = !separatorsVisible;
    lastBlinkTime = currentTime;

    // Ridisegna SOLO i separatori (flip clock + display digitale)
    if (separatorsVisible != lastSeparatorState) {
      redrawSeparatorOnly();  // Separatore flip clock
      drawDigitalDisplay(hour, minute, false);  // Aggiorna separatore digitale
      lastSeparatorState = separatorsVisible;
    }
  }

  // Aggiorna animazioni flip - ridisegna SOLO le palette che cambiano
  for (int i = 0; i < 6; i++) {
    if (flaps[i].isFlipping) {
      unsigned long elapsed = millis() - flaps[i].flipStartTime;

      if (elapsed >= FLIP_DURATION) {
        // Flip completato
        flaps[i].currentDigit = flaps[i].targetDigit;
        flaps[i].isFlipping = false;
        flaps[i].flipFrame = 0;

        // Ridisegna SOLO questa palette (stato finale)
        drawFlap(flaps[i], true);
      } else {
        // Calcola frame corrente
        uint8_t newFrame = (elapsed * FLIP_FRAMES) / FLIP_DURATION;

        // Ridisegna SOLO se il frame è effettivamente cambiato
        if (newFrame != flaps[i].flipFrame) {
          flaps[i].flipFrame = newFrame;

          // Ridisegna SOLO questa palette in animazione
          drawFlipAnimation(flaps[i]);
        }
      }
    }
  }
}

// ================== DISEGNA FLIP CLOCK (solo per inizializzazione) ==================
void drawFlipClock() {
  // Disegna tutte e 6 le palette (stato iniziale)
  for (int i = 0; i < 6; i++) {
    drawFlap(flaps[i], true);
  }

  // Disegna separatore iniziale (sempre visibile all'inizio)
  drawSeparator();
}

// ================== DISEGNA SINGOLA PALETTE STATICA ==================
void drawFlap(Flap &flap, bool drawStatic) {
  // Disegna bordi + contenuto
  drawFlapBorders(flap);
  drawFlapContent(flap);
}

// ================== DISEGNA SOLO BORDI PALETTE ==================
void drawFlapBorders(Flap &flap) {
  int16_t x = flap.x;
  int16_t y = flap.y;

  // Usa dimensioni diverse per secondi
  int16_t w = flap.isSecond ? FLAP_SEC_WIDTH : FLAP_WIDTH;
  int16_t h = flap.isSecond ? FLAP_SEC_HEIGHT : FLAP_HEIGHT;
  int16_t halfHeight = h / 2;

  // Ombra esterna (più piccola per i secondi)
  int16_t shadowSize = flap.isSecond ? 2 : 4;
  gfx->fillRoundRect(x - shadowSize, y - shadowSize, w + shadowSize*2, h + shadowSize*2,
                     flap.isSecond ? 4 : 7, 0x1082);
  gfx->fillRoundRect(x - shadowSize/2, y - shadowSize/2, w + shadowSize, h + shadowSize,
                     flap.isSecond ? 3 : 6, COLOR_FLAP_SHADOW);

  // Sfondo palette
  gfx->fillRoundRect(x, y, w, h, flap.isSecond ? 3 : 5, COLOR_FLAP_BG);

  // Bordi laterali con gradiente simulato (effetto 3D)
  gfx->drawRoundRect(x, y, w, h, flap.isSecond ? 3 : 5, COLOR_FLAP_BORDER);
  gfx->drawRoundRect(x + 1, y + 1, w - 2, h - 2, flap.isSecond ? 2 : 4, 0x3186);

  // Linea divisoria centrale (split delle due metà palette)
  int16_t lineMargin = flap.isSecond ? 2 : 3;
  gfx->drawFastHLine(x + lineMargin, y + halfHeight - 1, w - lineMargin*2, COLOR_FLAP_SHADOW);
  gfx->drawFastHLine(x + lineMargin, y + halfHeight, w - lineMargin*2, COLOR_FLAP_DIVIDER);
  gfx->drawFastHLine(x + lineMargin, y + halfHeight + 1, w - lineMargin*2, COLOR_FLAP_SHADOW);

  // Piccole ombre ai lati della linea divisoria per profondità
  if (!flap.isSecond) {  // Solo per palette grandi
    gfx->drawFastVLine(x + 2, y + halfHeight - 2, 4, COLOR_FLAP_SHADOW);
    gfx->drawFastVLine(x + w - 3, y + halfHeight - 2, 4, COLOR_FLAP_SHADOW);
  }
}

// ================== DISEGNA SOLO CONTENUTO PALETTE (senza bordi) ==================
void drawFlapContent(Flap &flap) {
  int16_t x = flap.x;
  int16_t y = flap.y;
  int16_t w = flap.isSecond ? FLAP_SEC_WIDTH : FLAP_WIDTH;
  int16_t h = flap.isSecond ? FLAP_SEC_HEIGHT : FLAP_HEIGHT;

  // Cancella contenuto precedente (solo area interna)
  int16_t margin = flap.isSecond ? 3 : 5;
  gfx->fillRect(x + margin, y + margin, w - margin*2, h - margin*2, COLOR_FLAP_BG);

  // Disegna numero
  drawDigit(x, y, flap.currentDigit, w, h, false, flap.isSecond);
}

// ================== ANIMAZIONE FLIP SOLO CONTENUTO (per secondi - senza ridisegnare bordi) ==================
void drawFlipAnimationContent(Flap &flap) {
  // Questa funzione è ottimizzata per i secondi:
  // - NON ridisegna mai i bordi/ombre/linee divisorie
  // - Manipola SOLO l'area contenuto interno
  // - Stessa animazione 2-fasi di drawFlipAnimation()

  int16_t x = flap.x;
  int16_t y = flap.y;
  int16_t w = FLAP_SEC_WIDTH;
  int16_t h = FLAP_SEC_HEIGHT;
  int16_t halfHeight = h / 2;

  // Margine per evitare di toccare i bordi
  int16_t margin = 3;

  // FASE 1: Metà superiore del numero vecchio scende (frame 0-5)
  // FASE 2: Metà inferiore del numero nuovo sale (frame 6-17)

  if (flap.flipFrame < FLIP_PHASE1_FRAMES) {
    // ========== FASE 1: Metà superiore scende ==========
    float rawProgress = (float)flap.flipFrame / (float)FLIP_PHASE1_FRAMES;
    float progress = easingTopFlip(rawProgress);

    // Metà inferiore SEMPRE visibile (numero vecchio)
    gfx->fillRect(x + margin, y + halfHeight + margin, w - margin*2, halfHeight - margin*2, COLOR_FLAP_BG);
    drawDigitHalf(x, y + halfHeight, flap.currentDigit, w, h, halfHeight, false, flap.isSecond);

    // Metà superiore con "rotazione" simulata
    int16_t scaledHeight = halfHeight * (1.0 - progress);

    if (scaledHeight > 2) {
      // Disegna metà superiore che si chiude dall'alto
      gfx->fillRect(x + margin, y + margin, w - margin*2, halfHeight - margin, COLOR_FLAP_BG);
      drawDigitHalf(x, y, flap.currentDigit, w, h, halfHeight, true, flap.isSecond);

      // Copri la parte che "ruota via"
      int16_t coverHeight = halfHeight - scaledHeight - margin;
      if (coverHeight > 0) {
        gfx->fillRect(x + margin, y + margin, w - margin*2, coverHeight, COLOR_FLAP_BG);
      }
    } else {
      // Metà superiore completamente chiusa
      gfx->fillRect(x + margin, y + margin, w - margin*2, halfHeight - margin, COLOR_FLAP_BG);
    }

  } else {
    // ========== FASE 2: Metà inferiore sale ==========
    float rawProgress = (float)(flap.flipFrame - FLIP_PHASE1_FRAMES) / (float)FLIP_PHASE2_FRAMES;
    float progress = easingBottomFlip(rawProgress);

    // Metà superiore SEMPRE visibile (numero NUOVO)
    gfx->fillRect(x + margin, y + margin, w - margin*2, halfHeight - margin, COLOR_FLAP_BG);
    drawDigitHalf(x, y, flap.targetDigit, w, h, halfHeight, true, flap.isSecond);

    // Metà inferiore con "rotazione" simulata
    int16_t scaledHeight = halfHeight * progress;

    if (scaledHeight < halfHeight - 2) {
      // Disegna metà inferiore
      gfx->fillRect(x + margin, y + halfHeight + margin, w - margin*2, halfHeight - margin*2, COLOR_FLAP_BG);
      drawDigitHalf(x, y + halfHeight, flap.targetDigit, w, h, halfHeight, false, flap.isSecond);

      // Copri la parte che non è ancora "ruotata"
      int16_t coverHeight = halfHeight - scaledHeight - margin*2;
      if (coverHeight > 0) {
        gfx->fillRect(x + margin, y + h - coverHeight - margin, w - margin*2, coverHeight, COLOR_FLAP_BG);
      }
    } else {
      // Animazione completata - mostra numero nuovo completo
      gfx->fillRect(x + margin, y + halfHeight + margin, w - margin*2, halfHeight - margin*2, COLOR_FLAP_BG);
      drawDigitHalf(x, y + halfHeight, flap.targetDigit, w, h, halfHeight, false, flap.isSecond);
    }
  }

  // NON ridisegna la linea divisoria centrale - è già lì dai bordi statici
}

// ================== ANIMAZIONE FLIP (2 FASI) ==================
void drawFlipAnimation(Flap &flap) {
  // Per i secondi: usa versione ottimizzata senza bordi
  if (flap.isSecond) {
    drawFlipAnimationContent(flap);
    return;
  }

  // Per HH:MM: disegna tutto (bordi + animazione)
  int16_t x = flap.x;
  int16_t y = flap.y;
  int16_t w = FLAP_WIDTH;
  int16_t h = FLAP_HEIGHT;
  int16_t halfHeight = h / 2;

  // Disegna base della palette con bordi
  gfx->fillRoundRect(x - 3, y - 3, w + 6, h + 6, 6, COLOR_FLAP_SHADOW);
  gfx->fillRoundRect(x, y, w, h, 5, COLOR_FLAP_BG);
  gfx->drawRoundRect(x, y, w, h, 5, COLOR_FLAP_BORDER);

  // FASE 1: Metà superiore del numero vecchio scende (frame 0-5)
  // FASE 2: Metà inferiore del numero nuovo sale (frame 6-17)

  if (flap.flipFrame < FLIP_PHASE1_FRAMES) {
    // ========== FASE 1: Metà superiore scende ==========
    // Progress con easing (0.0 a 1.0)
    float rawProgress = (float)flap.flipFrame / (float)FLIP_PHASE1_FRAMES;
    float progress = easingTopFlip(rawProgress);

    // Metà inferiore SEMPRE visibile (numero vecchio)
    drawDigitHalf(x, y + halfHeight, flap.currentDigit, w, h, halfHeight, false, flap.isSecond);

    // Metà superiore con "rotazione" simulata (scala verticale)
    int16_t scaledHeight = halfHeight * (1.0 - progress);

    if (scaledHeight > 2) {
      // Disegna metà superiore che si chiude dall'alto
      // Clip region per mostrare solo metà superiore
      gfx->fillRect(x, y, w, halfHeight, COLOR_FLAP_BG);
      drawDigitHalf(x, y, flap.currentDigit, w, h, halfHeight, true, flap.isSecond);

      // Copri la parte che "ruota via" con sfondo che scende
      int16_t coverHeight = halfHeight - scaledHeight;
      if (coverHeight > 0) {
        gfx->fillRect(x, y, w, coverHeight, COLOR_FLAP_BG);
        // Ombra progressiva per effetto 3D
        int16_t shadowMargin = flap.isSecond ? 3 : 5;
        gfx->drawFastHLine(x + shadowMargin, y + coverHeight, w - shadowMargin*2, COLOR_FLAP_SHADOW);
      }
    } else {
      // Metà superiore completamente chiusa
      gfx->fillRect(x, y, w, halfHeight, COLOR_FLAP_BG);
    }

  } else {
    // ========== FASE 2: Metà inferiore sale ==========
    float rawProgress = (float)(flap.flipFrame - FLIP_PHASE1_FRAMES) / (float)FLIP_PHASE2_FRAMES;
    float progress = easingBottomFlip(rawProgress);

    // Metà superiore SEMPRE visibile (numero NUOVO)
    drawDigitHalf(x, y, flap.targetDigit, w, h, halfHeight, true, flap.isSecond);

    // Metà inferiore con "rotazione" simulata (scala verticale dal basso)
    int16_t scaledHeight = halfHeight * progress;

    if (scaledHeight < halfHeight - 2) {
      // Disegna metà inferiore
      gfx->fillRect(x, y + halfHeight, w, halfHeight, COLOR_FLAP_BG);
      drawDigitHalf(x, y + halfHeight, flap.targetDigit, w, h, halfHeight, false, flap.isSecond);

      // Copri la parte che non è ancora "ruotata"
      int16_t coverHeight = halfHeight - scaledHeight;
      if (coverHeight > 0) {
        gfx->fillRect(x, y + h - coverHeight, w, coverHeight, COLOR_FLAP_BG);
        // Ombra progressiva per effetto 3D
        int16_t shadowMargin = flap.isSecond ? 3 : 5;
        gfx->drawFastHLine(x + shadowMargin, y + h - coverHeight - 1, w - shadowMargin*2, COLOR_FLAP_SHADOW);
      }
    } else {
      // Animazione completata - mostra numero nuovo completo
      drawDigitHalf(x, y + halfHeight, flap.targetDigit, w, h, halfHeight, false, flap.isSecond);
    }
  }

  // Linea divisoria centrale (sempre visibile)
  int16_t lineMargin = flap.isSecond ? 2 : 3;
  gfx->drawFastHLine(x + lineMargin, y + halfHeight, w - lineMargin*2, COLOR_FLAP_DIVIDER);
  gfx->drawFastHLine(x + lineMargin, y + halfHeight + 1, w - lineMargin*2, COLOR_FLAP_SHADOW);
}

// ================== DISEGNA CIFRA ==================
void drawDigit(int16_t x, int16_t y, uint8_t digit, int16_t w, int16_t h, bool inverted, bool isSecond) {
  // Font stile FLIP CLOCK meccanico vintage
  // Alternative testate per stile flip clock:
  // - u8g2_font_logisoso32_tn (stile meccanico/industriale) ✓ MIGLIORE
  // - u8g2_font_fub42_tn (bold universale, molto chiaro)
  // - u8g2_font_inb42_mn (inconsolata bold - quello attuale)
  // - u8g2_font_fur42_tn (universal regular)

  if (isSecond) {
    gfx->setFont(u8g2_font_logisoso16_tn); // Font meccanico piccolo per secondi
  } else {
    gfx->setFont(u8g2_font_logisoso42_tn); // Font meccanico GRANDE per ore/minuti - STILE FLIP CLOCK VINTAGE
  }
  gfx->setTextColor(inverted ? COLOR_FLAP_BG : COLOR_FLAP_TEXT);

  // Converti digit a stringa
  char digitStr[2];
  digitStr[0] = '0' + digit;
  digitStr[1] = '\0';

  // Calcola posizione centrata in base al font
  int16_t textX, textY;
  if (isSecond) {
    // Font u8g2_font_logisoso16_tn: altezza ~20px, larghezza ~13px
    textX = x + (w - 13) / 2;
    textY = y + (h + 16) / 2;
  } else {
    // Font u8g2_font_logisoso42_tn: altezza ~50px, larghezza ~32px
    textX = x + (w - 32) / 2;
    textY = y + (h + 42) / 2;
  }

  gfx->setCursor(textX, textY);
  gfx->print(digitStr);
}

// ================== DISEGNA METÀ CIFRA ==================
void drawDigitHalf(int16_t x, int16_t y, uint8_t digit, int16_t w, int16_t fullHeight, int16_t halfHeight, bool topHalf, bool isSecond) {
  // Questa funzione disegna solo metà della cifra (superiore o inferiore)
  // usando una tecnica di clipping manuale

  // Font meccanico FLIP CLOCK
  int16_t fontHeight, fontWidth;
  if (isSecond) {
    gfx->setFont(u8g2_font_logisoso16_tn); // Font meccanico piccolo per secondi
    fontHeight = 16;
    fontWidth = 13;
  } else {
    gfx->setFont(u8g2_font_logisoso42_tn); // Font meccanico GRANDE per ore/minuti
    fontHeight = 42;
    fontWidth = 32;
  }
  gfx->setTextColor(COLOR_FLAP_TEXT);

  char digitStr[2];
  digitStr[0] = '0' + digit;
  digitStr[1] = '\0';

  // Posizione centrata della cifra INTERA (come se fosse nella palette completa)
  // Ma disegniamo solo in questa metà
  int16_t textX = x + (w - fontWidth) / 2;

  if (topHalf) {
    // Metà superiore: disegna la cifra centrata rispetto alla palette COMPLETA
    // ma limita il rendering alla metà superiore
    int16_t textY = y + (fullHeight + fontHeight) / 2;  // Centro della palette intera
    gfx->setCursor(textX, textY);
    gfx->print(digitStr);
  } else {
    // Metà inferiore: stessa logica ma y è già spostato
    // Compensiamo per far apparire la metà inferiore della cifra
    int16_t textY = y + (fontHeight / 2) - (fullHeight / 2) + fullHeight / 2;
    gfx->setCursor(textX, textY);
    gfx->print(digitStr);
  }
}

// ================== DISEGNA SEPARATORE : ==================
void drawSeparator() {
  // Separatore tra ore e minuti (flaps[1] e flaps[2])
  int16_t sepX = flaps[1].x + FLAP_WIDTH + FLAP_GAP + 5;
  int16_t sepY = flaps[0].y + FLAP_HEIGHT / 2;

  // Due punti con effetto 3D
  // Punto superiore
  gfx->fillCircle(sepX + 1, sepY - 19, 7, COLOR_FLAP_SHADOW); // Ombra
  gfx->fillCircle(sepX, sepY - 20, 7, COLOR_FLAP_TEXT);       // Punto principale
  gfx->fillCircle(sepX - 1, sepY - 21, 3, 0xFFFF);            // Highlight per effetto 3D

  // Punto inferiore
  gfx->fillCircle(sepX + 1, sepY + 21, 7, COLOR_FLAP_SHADOW); // Ombra
  gfx->fillCircle(sepX, sepY + 20, 7, COLOR_FLAP_TEXT);       // Punto principale
  gfx->fillCircle(sepX - 1, sepY + 19, 3, 0xFFFF);            // Highlight per effetto 3D
}

// ================== RIDISEGNA SOLO SEPARATORE (anti-flickering) ==================
void redrawSeparatorOnly() {
  // Calcola area del separatore
  int16_t sepX = flaps[1].x + FLAP_WIDTH + FLAP_GAP + 5;
  int16_t sepY = flaps[0].y + FLAP_HEIGHT / 2;

  if (separatorsVisible) {
    // Disegna il separatore
    drawSeparator();
  } else {
    // Cancella il separatore (copri con sfondo)
    // Area da cancellare: cerchi con raggio 8 (7+1 per margine)
    gfx->fillCircle(sepX + 1, sepY - 19, 8, COLOR_BG_FLIP);
    gfx->fillCircle(sepX, sepY - 20, 8, COLOR_BG_FLIP);
    gfx->fillCircle(sepX - 1, sepY - 21, 8, COLOR_BG_FLIP);
    gfx->fillCircle(sepX + 1, sepY + 21, 8, COLOR_BG_FLIP);
    gfx->fillCircle(sepX, sepY + 20, 8, COLOR_BG_FLIP);
    gfx->fillCircle(sepX - 1, sepY + 19, 8, COLOR_BG_FLIP);
  }
}

// ================== AVVIA FLIP ==================
void startFlip(uint8_t flapIndex, uint8_t newDigit) {
  if (flapIndex >= 6) return;

  uint8_t oldDigit = flaps[flapIndex].currentDigit;

  flaps[flapIndex].targetDigit = newDigit;
  flaps[flapIndex].isFlipping = true;
  flaps[flapIndex].flipFrame = 0;
  flaps[flapIndex].flipStartTime = millis();

  // Suono click meccanico SOLO per ore e minuti (palette 0-3)
  // NON per i secondi (palette 4-5) per evitare troppo rumore
  if (flapIndex < 4) {
    // In un flip clock reale, il tempo scorre linearmente un minuto alla volta
    // Quindi ogni cambio di cifra produce sempre 1 solo clack
    uint8_t numClacks = 1; // Sempre 1 clack per flip
    playFlipSound(numClacks, flapIndex);
    Serial.printf("Flip palette %d: %d -> %d (%d clack)\n", flapIndex, oldDigit, newDigit, numClacks);
  } else {
    // Secondi: flip silenzioso
    Serial.printf("Flip palette secondi %d: %d -> %d (SILENZIOSO)\n", flapIndex, oldDigit, newDigit);
  }
}

// ================== DISPLAY DIGITALE LCD/LED ==================

// Disegna la cornice del display digitale (chiamata solo all'init)
void drawDigitalFrame() {
  int16_t x = (480 - DIGITAL_WIDTH) / 2;  // Centrato orizzontalmente
  int16_t y = DIGITAL_Y;

  // Cornice esterna (effetto plastica grigia)
  gfx->fillRoundRect(x - 8, y - 8, DIGITAL_WIDTH + 16, DIGITAL_HEIGHT + 16, 8, COLOR_DIGITAL_FRAME);
  gfx->fillRoundRect(x - 6, y - 6, DIGITAL_WIDTH + 12, DIGITAL_HEIGHT + 12, 6, COLOR_DIGITAL_BORDER);

  // Display LCD interno (verde scuro)
  gfx->fillRoundRect(x, y, DIGITAL_WIDTH, DIGITAL_HEIGHT, 4, COLOR_DIGITAL_BG);

  // Effetto vetro LCD (riflesso superiore)
  gfx->drawFastHLine(x + 5, y + 3, DIGITAL_WIDTH - 10, 0x0860);  // Linea riflesso
  gfx->drawFastHLine(x + 5, y + 4, DIGITAL_WIDTH - 10, 0x0440);  // Linea riflesso attenuata
}

// Disegna il contenuto del display digitale HH:MM
void drawDigitalDisplay(uint8_t hour, uint8_t minute, bool redrawAll) {
  int16_t x = (480 - DIGITAL_WIDTH) / 2;
  int16_t y = DIGITAL_Y;

  // Se redrawAll, ridisegna anche la cornice
  if (redrawAll) {
    drawDigitalFrame();
  }

  // Font a 7 segmenti (LED/LCD style)
  gfx->setFont(u8g2_font_7Segments_26x42_mn);
  gfx->setTextColor(COLOR_DIGITAL_TEXT);

  // Converti in stringhe separate (HH e MM separati)
  char hourStr[3];   // "HH\0"
  char minuteStr[3]; // "MM\0"
  sprintf(hourStr, "%02d", hour);
  sprintf(minuteStr, "%02d", minute);

  // Font 7Segments_26x42: ogni carattere ~26px larghezza, 42px altezza
  int16_t charWidth = 26;
  int16_t charHeight = 42;
  int16_t colonWidth = 12;  // I due punti sono più stretti
  int16_t gapHourColon = 8;   // Spazio tra HH e :
  int16_t gapColonMinute = 8; // Spazio tra : e MM

  // Calcola posizione centrata per l'intero gruppo HH:MM (con spazi allargati)
  int16_t totalWidth = (charWidth * 4) + colonWidth + gapHourColon + gapColonMinute;
  int16_t startX = x + (DIGITAL_WIDTH - totalWidth) / 2;
  int16_t startY = y + (DIGITAL_HEIGHT + charHeight) / 2 - 8 + 10;  // +10px abbassa i numeri dentro il display

  // Posizioni separate con spazi allargati
  int16_t hourX = startX;
  int16_t colonX = startX + (charWidth * 2) + gapHourColon;
  int16_t minuteX = colonX + colonWidth + gapColonMinute;

  // Ridisegna area contenuto (solo se cambia)
  if (redrawAll || hour != lastDigitalHour || minute != lastDigitalMinute) {
    // Cancella area contenuto
    gfx->fillRect(x + 2, y + 6, DIGITAL_WIDTH - 4, DIGITAL_HEIGHT - 8, COLOR_DIGITAL_BG);

    // Disegna HH
    gfx->setFont(u8g2_font_7Segments_26x42_mn);
    gfx->setTextColor(COLOR_DIGITAL_TEXT);
    gfx->setCursor(hourX, startY);
    gfx->print(hourStr);

    // Disegna MM
    gfx->setCursor(minuteX, startY);
    gfx->print(minuteStr);

    // Disegna ":" (se visibile al momento del redraw)
    if (separatorsVisible) {
      gfx->setCursor(colonX, startY);
      gfx->print(":");
    }

    // Aggiorna stato
    lastDigitalHour = hour;
    lastDigitalMinute = minute;
  } else {
    // Separatore lampeggiante (sincronizzato con flip clock)
    // Solo ridisegna i ":" in base a separatorsVisible
    if (separatorsVisible) {
      // Disegna i due punti
      gfx->setFont(u8g2_font_7Segments_26x42_mn);
      gfx->setTextColor(COLOR_DIGITAL_TEXT);
      gfx->setCursor(colonX, startY);
      gfx->print(":");
    } else {
      // Cancella i due punti (area precisa)
      gfx->fillRect(colonX - 2, y + 10, colonWidth + 4, DIGITAL_HEIGHT - 20, COLOR_DIGITAL_BG);
    }
  }
}

// ================== DATA COMPLETA ==================
void drawDate(uint8_t dayOfWeek, uint8_t day, uint8_t month, uint16_t year, bool redrawAll) {
  // Posizione sotto i secondi
  int16_t dateY = 420;  // Sotto i secondi (che sono a Y=320 + 80px altezza)

  // Solo ridisegna se cambia o è init
  if (!redrawAll && day == lastDay && month == lastMonth && year == lastYear && dayOfWeek == lastDayOfWeek) {
    return;  // Nessun cambiamento
  }

  // Font medio per la data
  gfx->setFont(u8g2_font_helvB10_tr);  // Helvetica Bold 10pt
  gfx->setTextColor(COLOR_FLAP_TEXT);

  // Formatta stringa data: "Giovedì 20 Ottobre"
  char dateStr[40];
  sprintf(dateStr, "%s %d %s",
          daysOfWeek[dayOfWeek],
          day,
          months[month - 1]);  // month è 1-12, array è 0-11

  // Calcola larghezza per centrare
  int16_t x1, y1;
  uint16_t w, h;
  gfx->getTextBounds(dateStr, 0, 0, &x1, &y1, &w, &h);
  int16_t dateX = (480 - w) / 2;

  // Cancella area precedente
  if (!redrawAll) {
    gfx->fillRect(0, dateY - 15, 480, 20, COLOR_BG_FLIP);
  }

  // Disegna data
  gfx->setCursor(dateX, dateY);
  gfx->print(dateStr);

  // Anno su riga sotto
  char yearStr[10];
  sprintf(yearStr, "%d", year);

  gfx->getTextBounds(yearStr, 0, 0, &x1, &y1, &w, &h);
  int16_t yearX = (480 - w) / 2;
  int16_t yearY = dateY + 18;  // 18px sotto la data

  // Cancella area anno
  if (!redrawAll) {
    gfx->fillRect(0, yearY - 15, 480, 20, COLOR_BG_FLIP);
  }

  // Disegna anno
  gfx->setCursor(yearX, yearY);
  gfx->print(yearStr);

  // Aggiorna stato
  lastDay = day;
  lastMonth = month;
  lastYear = year;
  lastDayOfWeek = dayOfWeek;
}

// ================== SUONO CLICK ==================
void playFlipSound(uint8_t numClacks, uint8_t flapIndex) {
  #ifdef AUDIO
  // CONTROLLO PRESENZA: non inviare comandi se display spento
  extern bool presenceDetected;
  if (!presenceDetected) {
    // Display spento = nessun suono (risparmio energetico + nessun disturbo)
    return;
  }

  if (audioSlaveConnected) {
    Serial.printf("Flip clock: invio %d CLACK per palette %d\n", numClacks, flapIndex);

    // Riproduce clack.mp3 il numero di volte corrispondente al movimento
    // NOTA: L'ESP32C3 gestisce anche il controllo dell'ora (giorno/notte)
    for (uint8_t i = 0; i < numClacks; i++) {
      playClackViaWiFi();

      // Pausa tra i clack per dare tempo all'ESP32C3 di completare la riproduzione
      // (durata clack.mp3 + tempo di elaborazione)
      if (i < numClacks - 1) {
        delay(400); // Pausa di 400ms tra un clack e l'altro
      }
    }
  } else {
    Serial.println("Flip clock: ESP32C3 non connesso, audio non disponibile");
  }
  #else
  Serial.println("Flip clock: AUDIO non definito");
  #endif
}

// ================== FUNZIONI DI EASING ==================
// Ispirate alle curve cubic-bezier del flip clock web

// Per metà superiore: cubic-bezier(0.37, 0.01, 0.94, 0.35)
// Decelerazione rapida - l'animazione parte veloce e rallenta bruscamente
float easingTopFlip(float t) {
  // Approssimazione della curva cubic-bezier con funzione polinomiale
  // per performance migliori su embedded
  if (t <= 0.0) return 0.0;
  if (t >= 1.0) return 1.0;

  // Curva che simula decelerazione rapida
  // Combinazione di funzioni per approssimare la bezier
  float t2 = t * t;
  float t3 = t2 * t;

  // Coefficienti approssimati dalla curva bezier originale
  return 3.0 * t2 - 2.0 * t3;  // Smooth step con bias verso rallentamento
}

// Per metà inferiore: cubic-bezier(0.15, 0.45, 0.28, 1)
// Movimento più fluido con leggero bounce
float easingBottomFlip(float t) {
  // Approssimazione della curva cubic-bezier
  if (t <= 0.0) return 0.0;
  if (t >= 1.0) return 1.0;

  // Curva più dolce con accelerazione e decelerazione graduate
  float t2 = t * t;
  float t3 = t2 * t;

  // Formula che approssima la curva bezier con movimento più fluido
  // Include un leggero "ease-out" per naturalezza
  return 1.0 - pow(1.0 - t, 3);  // Ease-out cubic
}

#endif // EFFECT_FLIP_CLOCK
