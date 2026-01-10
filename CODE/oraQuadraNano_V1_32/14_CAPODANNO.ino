// ================== EFFETTO SPECIALE CAPODANNO ==================
// Questo effetto si attiva automaticamente alle 00:00 del 01/01 di ogni anno
// Mostra "BUON ANNO" con animazione di fuochi d'artificio per 2 minuti
// Sovrasta qualsiasi modalità attiva e ripristina lo stato precedente dopo 2 minuti

// NOTA: Il font DS_DIGIB20pt7b è già incluso in 12_BTTF.ino, non serve includerlo di nuovo

// ================== VARIABILI ESTERNE (dichiarate in oraQuadraNano_V1_3.ino) ==================
extern bool capodannoActive;
extern uint32_t capodannoStartTime;
extern DisplayMode savedMode;
extern bool capodannoTriggeredThisYear;
extern uint16_t capodannoLastYear;

// ===== DURATA EFFETTO =====
// 📅 MODALITÀ PRODUZIONE: Durata completa di 2 minuti
#define CAPODANNO_DURATION 120000       // Durata PRODUZIONE: 2 minuti (120000 ms)
// 🔧 PER TEST: Usa 30 secondi per test veloci (commentare la riga sopra e decommentare questa)
//#define CAPODANNO_DURATION 30000      // Durata TEST: 30 secondi (30000 ms)

#define MAX_FIREWORKS 15                // Numero massimo di fuochi d'artificio attivi contemporaneamente

// ================== DATI LOCALI ==================
// NOTA: La struct Firework è definita in oraQuadraNano_V1_3.ino
// Array di fuochi d'artificio
Firework fireworks[MAX_FIREWORKS];

// Colori vivaci per i fuochi d'artificio (RGB565)
const uint16_t fireworkColors[] = {
  0xF800,  // Rosso
  0x07E0,  // Verde
  0x001F,  // Blu
  0xFFE0,  // Giallo
  0xF81F,  // Magenta
  0x07FF,  // Cyan
  0xFD20,  // Arancione
  0xFFFF,  // Bianco
  0xFC00,  // Arancione brillante
  0x7FE0   // Verde chiaro
};

// ================== FUNZIONI CAPODANNO ==================

// Inizializza l'effetto Capodanno
void initCapodanno() {
  Serial.println("[CAPODANNO] 🎆 BUON ANNO! Attivazione effetto speciale...");

  capodannoActive = true;
  capodannoStartTime = millis();
  savedMode = currentMode;  // Salva la modalità corrente

  // Inizializza tutti i fuochi come inattivi
  for (int i = 0; i < MAX_FIREWORKS; i++) {
    fireworks[i].active = false;
  }

  // Pulisce lo schermo con sfondo nero
  gfx->fillScreen(0x0000);

  Serial.printf("[CAPODANNO] Modalità salvata: %d - Durata: %d secondi\n", savedMode, CAPODANNO_DURATION / 1000);
}

// Crea un nuovo fuoco d'artificio in posizione casuale
void createFirework() {
  // Trova uno slot libero
  for (int i = 0; i < MAX_FIREWORKS; i++) {
    if (!fireworks[i].active) {
      fireworks[i].x = random(60, 420);          // Posizione X casuale (margine dai bordi)
      fireworks[i].y = random(60, 420);          // Posizione Y casuale
      fireworks[i].radius = 0;
      fireworks[i].maxRadius = random(40, 100);  // Raggio massimo casuale
      fireworks[i].speed = random(15, 35) / 10.0; // Velocità casuale (1.5 - 3.5 pixel/frame)
      fireworks[i].color = fireworkColors[random(10)]; // Colore casuale
      fireworks[i].active = true;
      fireworks[i].startTime = millis();
      fireworks[i].sparkles = random(8, 16);     // Numero di scintille casuali
      break;
    }
  }
}

// Disegna un singolo fuoco d'artificio
void drawFirework(Firework &fw) {
  if (!fw.active) return;

  // Calcola alpha (trasparenza simulata) in base al progresso
  float progress = fw.radius / fw.maxRadius;

  // Colore che sfuma verso il nero mentre si espande
  uint16_t currentColor = fw.color;
  if (progress > 0.7) {
    // Fade out negli ultimi 30% dell'espansione
    float fade = 1.0 - ((progress - 0.7) / 0.3);
    // Scurisce il colore
    uint8_t r = ((fw.color >> 11) & 0x1F) * fade;
    uint8_t g = ((fw.color >> 5) & 0x3F) * fade;
    uint8_t b = (fw.color & 0x1F) * fade;
    currentColor = (r << 11) | (g << 5) | b;
  }

  // Disegna scintille radiali
  for (int i = 0; i < fw.sparkles; i++) {
    float angle = (2.0 * PI * i) / fw.sparkles;
    int16_t x1 = fw.x + cos(angle) * fw.radius;
    int16_t y1 = fw.y + sin(angle) * fw.radius;

    // Disegna linea dalla posizione precedente
    float prevRadius = fw.radius - fw.speed;
    if (prevRadius < 0) prevRadius = 0;
    int16_t x0 = fw.x + cos(angle) * prevRadius;
    int16_t y0 = fw.y + sin(angle) * prevRadius;

    gfx->drawLine(x0, y0, x1, y1, currentColor);

    // Aggiungi puntino luminoso alla fine
    gfx->fillCircle(x1, y1, 2, currentColor);
  }

  // Disegna cerchio esterno
  gfx->drawCircle(fw.x, fw.y, (int16_t)fw.radius, currentColor);
}

// Aggiorna tutti i fuochi d'artificio
void updateFireworks() {
  for (int i = 0; i < MAX_FIREWORKS; i++) {
    if (fireworks[i].active) {
      // Espandi il raggio
      fireworks[i].radius += fireworks[i].speed;

      // Disattiva se ha raggiunto il raggio massimo
      if (fireworks[i].radius >= fireworks[i].maxRadius) {
        fireworks[i].active = false;
      }
    }
  }
}

// Disegna il testo "BUON ANNO" al centro
void drawBuonAnno() {
  // Usa il font DS-Digital Bold per un effetto moderno
  gfx->setFont(&DS_DIGIB20pt7b);
  gfx->setTextColor(0xFFFF);  // Bianco

  // Calcola posizione centrata per "BUON"
  const char* buon = "BUON";
  int16_t x1, y1;
  uint16_t w, h;
  gfx->getTextBounds(buon, 0, 0, &x1, &y1, &w, &h);
  int16_t xBuon = (480 - w) / 2;
  int16_t yBuon = 200;

  // Disegna "BUON" con ombra
  gfx->setTextColor(0x7BEF);  // Grigio per ombra
  gfx->setCursor(xBuon + 2, yBuon + 2);
  gfx->print(buon);

  gfx->setTextColor(0xFFE0);  // Giallo brillante
  gfx->setCursor(xBuon, yBuon);
  gfx->print(buon);

  // Calcola posizione centrata per "ANNO"
  const char* anno = "ANNO";
  gfx->getTextBounds(anno, 0, 0, &x1, &y1, &w, &h);
  int16_t xAnno = (480 - w) / 2;
  int16_t yAnno = 260;

  // Disegna "ANNO" con ombra
  gfx->setTextColor(0x7BEF);  // Grigio per ombra
  gfx->setCursor(xAnno + 2, yAnno + 2);
  gfx->print(anno);

  gfx->setTextColor(0xF81F);  // Magenta brillante
  gfx->setCursor(xAnno, yAnno);
  gfx->print(anno);

  // Anno corrente sotto in formato più piccolo
  char yearStr[10];
  sprintf(yearStr, "%d", myTZ.year());

  gfx->setFont(&DS_DIGI20pt7b);
  gfx->getTextBounds(yearStr, 0, 0, &x1, &y1, &w, &h);
  int16_t xYear = (480 - w) / 2;
  int16_t yYear = 320;

  gfx->setTextColor(0x07FF);  // Cyan
  gfx->setCursor(xYear, yYear);
  gfx->print(yearStr);
}

// Loop principale dell'effetto Capodanno
void updateCapodanno() {
  uint32_t currentMillis = millis();
  uint32_t elapsed = currentMillis - capodannoStartTime;

  // Verifica se sono passati 2 minuti
  if (elapsed >= CAPODANNO_DURATION) {
    // Termina l'effetto e ripristina la modalità precedente
    Serial.println("[CAPODANNO] Effetto terminato. Ripristino modalità precedente...");
    capodannoActive = false;
    currentMode = savedMode;

    // Reset del flag per permettere test multipli (utile in fase di test)
    // In produzione, questo permetterà comunque la riattivazione solo al prossimo anno
    capodannoTriggeredThisYear = false;
    Serial.println("[CAPODANNO] Flag reset - pronto per nuovo trigger");

    // Forza ridisegno completo dello schermo
    gfx->fillScreen(0x0000);
    forceDisplayUpdate();

    Serial.printf("[CAPODANNO] Modalità ripristinata: %d\n", currentMode);
    return;
  }

  // Pulisce lo schermo con dissolvenza (sfondo nero semi-trasparente)
  // Invece di fillScreen, disegna un rettangolo semi-trasparente per effetto scia
  for (int y = 0; y < 480; y += 10) {
    for (int x = 0; x < 480; x += 10) {
      gfx->drawPixel(x, y, 0x0000);
    }
  }

  // Crea nuovi fuochi d'artificio casualmente (più frequenti all'inizio)
  // Usa percentuale invece di tempo fisso per adattarsi alla durata configurata
  float progress = (float)elapsed / CAPODANNO_DURATION;

  if (progress < 0.75) {  // Primi 75% del tempo con fuochi frequenti
    if (random(100) < 15) {  // 15% di probabilità ogni frame
      createFirework();
    }
  } else {  // Ultimi 25% del tempo con fuochi più rari
    if (random(100) < 5) {   // 5% di probabilità
      createFirework();
    }
  }

  // Aggiorna e disegna tutti i fuochi d'artificio
  updateFireworks();
  for (int i = 0; i < MAX_FIREWORKS; i++) {
    drawFirework(fireworks[i]);
  }

  // Disegna "BUON ANNO" sopra i fuochi
  drawBuonAnno();
}

// Controlla se è il momento di attivare l'effetto Capodanno
void checkCapodannoTrigger() {
  // Solo se WiFi connesso per avere ora corretta
  if (WiFi.status() != WL_CONNECTED) return;

  // Se l'effetto è già attivo, non fare nulla
  if (capodannoActive) return;

  // Ottieni data e ora correnti
  uint16_t year = myTZ.year();
  uint8_t month = myTZ.month();
  uint8_t day = myTZ.day();
  uint8_t hour = myTZ.hour();
  uint8_t minute = myTZ.minute();

  // Reset del flag se è un nuovo anno
  if (year != capodannoLastYear) {
    capodannoTriggeredThisYear = false;
    capodannoLastYear = year;
    Serial.printf("[CAPODANNO] Nuovo anno rilevato: %d - Flag reset\n", year);
  }

  // Verifica se è 01/01 alle 00:00
  if (month == 1 && day == 1 && hour == 0 && minute == 0 && !capodannoTriggeredThisYear) {
    Serial.printf("[CAPODANNO] ⚡ TRIGGER! Data: %02d/%02d/%d %02d:%02d\n", day, month, year, hour, minute);
    capodannoTriggeredThisYear = true;
    initCapodanno();
  }
}
