
// ORAQUADRA nano, By Paolo Sambinello, Alessandro Spagnoletti, Davide Gatti (www.survivalhacking.it)
// Versione ridotta e semplificato del progetto ORAQUADRA 2: https://youtu.be/DiFU6ITK8QQ / https://github.com/SurvivalHacking/Oraquadra2 
// Tutti i file necessari per la stampa 3D e info varie le trovate qui:  https://github.com/SurvivalHacking/OraquadraNano  
// Si può programmare senza compilazione andando a questo LINK e collegando il dispositivo con un cavo USB al computer: https://davidegatti.altervista.org/sh/oraQuadraNano.html
// Qui potrete trovare il corretto modulo usato in questo progetto:  ESP32-4848S040  https://s.click.aliexpress.com/e/_onmPeo3  
//
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//   ATTENZIONE !!!!!   Per una corretta compilazione dovrete selezionare nel gestore schede il core ESP32 in versione 2.0.17  /////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//---------------------------------------------------------------------------------------------------------------------------------------
// Il quadrante è touch e se lo suddividete in 4 parti avrete 4 zone separate con diverse funzionalità.
//
// ----- Parte in alto a sinistra -----
// Cambio modalità:  FADE / LENTO / VELOCE / MATRIX / MATRIX2 / SNALE / GOCCIA
//
// ----- Parte in alto a destra -----
// Una serie di combinazioni di colori delle varie modalità
//
// ----- Parte in basso a destra -----
// Fa lampeggiare la lettera E separatore tra le ore e minuti
//
// ----- Parte in basso a sinistra -----
// Selettore colore. Tenendolo premuto è possibile ciclre in tutti i colori per poter scegliere quello preferito.
//
// Una volta selezionato un modo o preset con il vostro colore personalizzato, verrà memorizzato e quando riavvierete il dispositivo, ritornerà nell'ultima configurazione che avevate impostato
//
// ----- Tenendo premute con 4 dita tutte e 4 le zone durante la fase di avvio, verranno resettate le impostazioni WIFI
//---------------------------------------------------------------------------------------------------------------------------------------
//
//
// Un grande ringraziamento a Paolo e Alessandro per il grandissimo lavoro di porting e conversione




//#define MENU_SCROLL //ATTIVA MENU SCROLL COMMENTA QUESTA RIGA PER DISATTIVARE
//#define AUDIO  //ATTIVA AUDIO COMMENTA QUESTA RIGA PER DISATTIVARE

// ================== INCLUSIONE LIBRERIE ==================
#include <Arduino.h>             // Libreria base per la programmazione di schede Arduino (ESP32).
#include <WiFi.h>                // Libreria per la gestione della connessione Wi-Fi.
#include <ESPmDNS.h>             // Libreria per il supporto del servizio mDNS (Multicast DNS), che permette di accedere al dispositivo tramite un nome host sulla rete locale.
#include <ArduinoOTA.h>          // Libreria per l'Over-The-Air (OTA) update, che consente di aggiornare il firmware del dispositivo via Wi-Fi.
#include <ezTime.h>              // Libreria per la gestione di data e ora, inclusa la sincronizzazione con server NTP (Network Time Protocol) e la gestione dei fusi orari.
#include <Espalexa.h>            // Libreria per l'integrazione con Amazon Alexa, permettendo il controllo del dispositivo tramite comandi vocali.
#include <EEPROM.h>              // Libreria per la lettura e scrittura nella memoria EEPROM (Electrically Erasable Programmable Read-Only Memory) del chip, utilizzata per memorizzare dati persistenti.
#include <U8g2lib.h>             // Libreria per la gestione di display grafici monocromatici (anche se in questo codice sembra essere usato un display a colori).
#include <Arduino_GFX_Library.h> // Libreria generica per la gestione di display grafici, che supporta diversi tipi di display a colori.
#include <TAMC_GT911.h>          // Libreria specifica per la gestione del controller touch screen GT911.
#include <Wire.h>                // Libreria per la comunicazione tramite protocollo I2C (Inter-Integrated Circuit).
#include "SPI.h"                 // Libreria per la comunicazione tramite protocollo SPI (Serial Peripheral Interface).
#include <WiFiManager.h>         // Libreria per la gestione semplificata della connessione Wi-Fi, spesso tramite un captive portal (una pagina web che si apre automaticamente quando ci si connette a una rete Wi-Fi senza credenziali).
#include <HTTPClient.h>          // Libreria per effettuare richieste HTTP (Hypertext Transfer Protocol), utile per comunicare con server web.
#include <SPIFFS.h>              // Libreria per l'utilizzo del filesystem SPIFFS (SPI Flash File System) sulla memoria flash del chip ESP32, per memorizzare file (es. configurazioni, suoni).
#include "word_mappings.h"       // File di header locale (nello stesso progetto) che probabilmente contiene array o definizioni per mappare parole (stringhe di testo) a specifiche posizioni di LED sulla matrice del display.
#include "sh_logo_480x480new_black.h" // File di header locale che probabilmente contiene i dati binari (array di byte) per visualizzare un logo (in bianco e nero) sul display con risoluzione 480x480 pixel.

#ifdef AUDIO
#include <driver/i2s.h>           // Driver di basso livello per l'interfaccia I2S (Integrated Interchip Sound) dell'ESP32, utilizzata per la trasmissione e ricezione di dati audio digitali.
#include <AudioFileSourceSPIFFS.h> // Libreria per leggere file audio dal filesystem SPIFFS come sorgente per la riproduzione.
#include <AudioFileSourceBuffer.h>  // Libreria per utilizzare buffer di memoria come sorgente per la riproduzione audio.
#include <AudioGeneratorMP3.h>    // Libreria per decodificare file audio in formato MP3.
#include <AudioOutputI2S.h>       // Libreria per inviare l'audio decodificato all'interfaccia I2S per la riproduzione (tramite un amplificatore e altoparlante collegati).
#endif

// ================== CONFIGURAZIONE ==================
// Definizione dei pin
#define I2C_SDA_PIN 19   // Definisce il pin 19 dell'ESP32 come il pin SDA (Serial Data) per la comunicazione I2C.
#define I2C_SCL_PIN 45   // Definisce il pin 45 dell'ESP32 come il pin SCL (Serial Clock) per la comunicazione I2C.
#define TOUCH_INT -1     // Definisce il pin per l'interrupt del touch screen. Un valore di -1 indica che questa funzionalità potrebbe non essere utilizzata o gestita in modo diverso (polling).
#define TOUCH_RST -1     // Definisce il pin per il reset del touch screen. Un valore di -1 indica che il reset potrebbe non essere controllato via software.
#define MAX_PATH_MEMORY 256  // Definisce la dimensione massima dell'array utilizzato per memorizzare il percorso del "serpente" di LED.

#ifdef AUDIO
#define I2S_BCLK      1     // Definisce il pin 1 dell'ESP32 come il pin BCLK (Bit Clock) per l'interfaccia I2S.
#define I2S_LRC       2     // Definisce il pin 2 dell'ESP32 come il pin LRC (Left/Right Clock o Word Select) per l'interfaccia I2S.
#define I2S_DOUT      40    // Definisce il pin 40 dell'ESP32 come il pin DOUT (Data Out) per l'interfaccia I2S.
#define I2S_PIN_ENABLE -1    // Definisce un pin per abilitare/disabilitare l'alimentazione o l'amplificatore audio collegato all'I2S. Un valore di -1 indica che non è utilizzato.
#define VOLUME_LEVEL  1.0    // Definisce il livello di volume iniziale per la riproduzione audio (1.0 è il volume massimo).
#endif

// Configurazione PWM (Pulse Width Modulation) per il controllo della luminosità del display
#define GFX_BL 38            // Definisce il pin 38 dell'ESP32 come il pin utilizzato per controllare la retroilluminazione (BackLight) del display.
#define PWM_CHANNEL     0    // Definisce il canale PWM (da 0 a 15 sull'ESP32) che verrà utilizzato per controllare la luminosità.
#define PWM_FREQ        1000   // Definisce la frequenza del segnale PWM in Hertz (Hz). Una frequenza più alta di solito porta a uno sfarfallio meno percepibile.
#define PWM_RESOLUTION  8    // Definisce la risoluzione del segnale PWM in bit. 8 bit significa che ci sono 2^8 = 256 livelli di luminosità possibili (da 0 a 255).

// Configurazione touch screen
#define TOUCH_ROTATION ROTATION_NORMAL // Definisce l'orientamento del touch screen come "normale" (non ruotato). Altri valori potrebbero essere ROTATION_90, ROTATION_180, ROTATION_270.
#define TOUCH_MAP_X1 480      // Definisce il valore massimo letto sull'asse X dal sensore touch.
#define TOUCH_MAP_X2 0        // Definisce il valore minimo letto sull'asse X dal sensore touch.
#define TOUCH_MAP_Y1 480      // Definisce il valore massimo letto sull'asse Y dal sensore touch.
#define TOUCH_MAP_Y2 0        // Definisce il valore minimo letto sull'asse Y dal sensore touch. Questi valori vengono usati per mappare le coordinate del touch sulle coordinate del display.
#define TOUCH_DEBOUNCE_MS 300 // Definisce il tempo minimo in millisecondi che deve trascorrere tra due tocchi consecutivi per essere considerati distinti, evitando "rimbalzi" o letture multiple di un singolo tocco.

// Costanti per la pagina di setup (menu di configurazione)
#define SETUP_SCROLL_THRESHOLD 40      // Definisce la distanza minima in pixel che un tocco deve spostarsi per essere considerato uno "scroll" nel menu di setup.
#define SETUP_TIMEOUT 30000            // Definisce il tempo massimo in millisecondi (30 secondi) di inattività sulla pagina di setup prima che il sistema esca automaticamente.
#define SETUP_ITEMS_COUNT 5            // Definisce il numero totale di elementi presenti nel menu di setup.
#define LONG_PRESS_THRESHOLD 600      // Definisce la durata minima in millisecondi di un tocco continuo per essere considerato una "pressione lunga".

// Configurazione degli effetti di visualizzazione
#define SNAKE_SPEED 120          // Definisce la velocità iniziale dell'effetto "serpente" di LED, in millisecondi tra un movimento e l'altro (valore più basso = più veloce).
#define SNAKE_MIN_SPEED 40       // Definisce la velocità massima (più veloce) raggiungibile dall'effetto "serpente".
#define SNAKE_MAX_LENGTH 30      // Definisce la lunghezza massima che può raggiungere il "serpente" di LED.
#define SNAKE_ACCELERATION 5     // Definisce di quanto diminuisce l'intervallo di tempo (aumentando la velocità) del "serpente" ogni volta che "mangia una lettera".

// Configurazione dell'effetto "goccia d'acqua"
#define DROP_DURATION 3000      // Definisce la durata totale in millisecondi dell'effetto della "goccia d'acqua".
#define RIPPLE_SPEED 8.0         // Definisce la velocità con cui si espande il "cerchio" dell'onda generata dalla "goccia".
#define MAX_RIPPLE_RADIUS 20     // Definisce il raggio massimo in pixel che può raggiungere l'onda della "goccia".

#define MAX_TIME_LEDS 100  // Definisce il numero massimo di LED che possono essere utilizzati per visualizzare l'ora corrente in una determinata modalità.

// Definizione delle costanti per la gestione automatica della luminosità in base all'ora del giorno
#define BRIGHTNESS_DAY   250 // Definisce il livello di luminosità da utilizzare durante il giorno (valore PWM).
#define BRIGHTNESS_NIGHT 90  // Definisce il livello di luminosità da utilizzare durante la notte (valore PWM).
#define HOUR_DAY   7    // Definisce l'ora del giorno (in formato 24 ore) a partire dalla quale considerare "giorno".
#define HOUR_NIGHT 19   // Definisce l'ora del giorno (in formato 24 ore) a partire dalla quale considerare "notte".

// Definizione delle costanti per lo stato di visualizzazione della lettera "E"
#define E_DISABLED 0 // La lettera "E" non viene visualizzata.
#define E_ENABLED 1  // La lettera "E" è visualizzata in modo statico.
#define E_BLINK 2    // La lettera "E" lampeggia.

// Configurazione del display (matrice di LED virtuale, anche se il display fisico è diverso)
#define MATRIX_WIDTH  16  // Larghezza della matrice virtuale di LED.
#define MATRIX_HEIGHT 16  // Altezza della matrice virtuale di LED.
#define NUM_LEDS 256    // Numero totale di LED nella matrice virtuale (larghezza * altezza).
#define GFX_DEV_DEVICE ESP32_4848S040_86BOX_GUITION // Definisce il tipo specifico di dispositivo grafico (display) utilizzato.
#define RGB_PANEL          // Indica che il display è un pannello RGB a colori.

// Configurazione degli effetti "MATRIX" (pioggia di LED)
#define MATRIX_START_Y_MIN  -7.0f // Valore minimo per la posizione Y iniziale delle "gocce" di LED (può essere negativo per farle apparire dall'alto).
#define MATRIX_START_Y_MAX   0.0f  // Valore massimo per la posizione Y iniziale delle "gocce" di LED.
#define MATRIX_TRAIL_LENGTH  10   // Lunghezza della "scia" luminosa dietro ogni "goccia".
#define NUM_DROPS           32   // Numero totale di "gocce" di LED da visualizzare contemporaneamente.
#define MATRIX_BASE_SPEED   0.30f // Velocità base di caduta delle "gocce".
#define MATRIX_SPEED_VAR    0.15f // Variazione casuale della velocità delle "gocce".
#define MATRIX2_BASE_SPEED  0.30f // Velocità base per una seconda variante dell'effetto "MATRIX".
#define MATRIX2_SPEED_VAR   0.15f // Variazione casuale della velocità per la seconda variante dell'effetto "MATRIX".

// Configurazione della memoria EEPROM
#define EEPROM_SIZE 512          // Definisce la dimensione totale in byte della memoria EEPROM utilizzabile.
#define EEPROM_CONFIGURED_MARKER 0x55 // Un valore specifico (byte) scritto in una posizione della EEPROM per indicare se la configurazione iniziale è già stata eseguita.
#define EEPROM_PRESET_ADDR 1      // Indirizzo di memoria nella EEPROM dove è memorizzato il preset di configurazione corrente.
#define EEPROM_BLINK_ADDR 2       // Indirizzo di memoria nella EEPROM dove è memorato lo stato di lampeggio (probabilmente per qualche indicatore).
#define EEPROM_MODE_ADDR 3        // Indirizzo di memoria nella EEPROM dove è memorata la modalità di visualizzazione corrente.
#define EEPROM_WIFI_SSID_ADDR 10     // Indirizzo di memoria di inizio per memorizzare la SSID (nome) della rete Wi-Fi (32 byte riservati).
#define EEPROM_WIFI_PASS_ADDR 42     // Indirizzo di memoria di inizio per memorizzare la password della rete Wi-Fi (64 byte riservati).
#define EEPROM_WIFI_VALID_ADDR 106    // Indirizzo di memoria per un flag (1 byte) che indica se le credenziali Wi-Fi memorizzate sono valide.
#define EEPROM_WIFI_VALID_VALUE 0xAA // Valore (byte) che, se presente nell'indirizzo `EEPROM_WIFI_VALID_ADDR`, indica che le credenziali Wi-Fi sono valide.
#define EEPROM_SETUP_OPTIONS_ADDR 110 // Indirizzo di memoria per memorizzare le opzioni di configurazione del setup (probabilmente un byte che codifica diverse impostazioni).
#define EEPROM_WORD_E_VISIBLE_ADDR 107 // Indirizzo di memoria per memorizzare se la parola "E" deve essere visibile.
#define EEPROM_WORD_E_STATE_ADDR 108   // Indirizzo di memoria per memorizzare lo stato di visualizzazione della lettera "E" (es. abilitata, lampeggiante).
#define EEPROM_COLOR_R_ADDR 130       // Indirizzo di memoria per memorizzare la componente rossa (Red) del colore preferito dall'utente.
#define EEPROM_COLOR_G_ADDR 131       // Indirizzo di memoria per memorizzare la componente verde (Green) del colore preferito dall'utente.
#define EEPROM_COLOR_B_ADDR 132       // Indirizzo di memoria per memorizzare la componente blu (Blue) del colore preferito dall'utente.

String ino = __FILE__; // Variabile globale di tipo String che viene inizializzata con il nome del file corrente (.ino).

// ================== MODALITÀ DI VISUALIZZAZIONE ==================
enum DisplayMode {
  MODE_FADE = 0,   // Modalità di visualizzazione con effetto di dissolvenza.
  MODE_SLOW = 1,   // Modalità di visualizzazione lenta (definizione specifica dell'effetto non chiara qui).
  MODE_FAST = 2,   // Modalità di visualizzazione veloce (definizione specifica dell'effetto non chiara qui).
  MODE_MATRIX = 3, // Modalità di visualizzazione con l'effetto "matrice" (pioggia di LED).
  MODE_MATRIX2 = 4, // Una seconda modalità di visualizzazione con una variante dell'effetto "matrice".
  MODE_SNAKE = 5,  // Modalità di visualizzazione con l'effetto "serpente" di LED.
  MODE_WATER = 6,  // Modalità di visualizzazione con l'effetto "goccia d'acqua".
  NUM_MODES = 7    // Costante che indica il numero totale di modalità di visualizzazione definite nell'enum.
};

// ================== STRUTTURE DATI ==================
// Struttura per memorizzare i LED dell'orario corrente per l'effetto SNAKE
struct TimeDisplay {
  uint16_t positions[MAX_TIME_LEDS];  // Array di tipo uint16_t (unsigned integer a 16 bit) per memorizzare gli indici (posizioni) dei LED che rappresentano l'ora corrente nell'effetto "serpente".
  uint16_t count;                    // Variabile di tipo uint16_t per tenere traccia del numero di LED effettivamente utilizzati per visualizzare l'ora.
};

// Struttura per memorizzare il percorso del "serpente"
struct SnakePath {
  uint16_t positions[MAX_PATH_MEMORY];  // Array per memorizzare le posizioni
uint16_t length;                      // Variabile di tipo uint16_t per memorizzare la lunghezza attuale del percorso del serpente.
  bool reversed;                        // Variabile di tipo bool (booleano, vero o falso) per indicare se il serpente sta percorrendo il suo percorso all'indietro.
};

// Strutture dati per gestire il serpente (l'effetto visivo)
struct SnakeSegment {
  uint16_t ledIndex;             // Variabile di tipo uint16_t per memorizzare la posizione di un singolo segmento del serpente come indice del LED nella matrice.
};

struct Snake {
  SnakeSegment segments[SNAKE_MAX_LENGTH];  // Array di strutture `SnakeSegment` per rappresentare tutti i segmenti del serpente. La dimensione massima è definita da `SNAKE_MAX_LENGTH`.
  uint8_t length;                // Variabile di tipo uint8_t (unsigned integer a 8 bit) per memorizzare la lunghezza attuale del serpente (numero di segmenti).
  uint16_t speed;                // Variabile di tipo uint16_t per memorizzare la velocità attuale del serpente, espressa come intervallo di tempo in millisecondi tra un movimento e l'altro.
  bool gameOver;                 // Variabile di tipo bool per indicare se il "gioco" del serpente è finito (ad esempio, se ha colpito se stesso).
  bool wordCompleted;            // Variabile di tipo bool per indicare se tutte le parole da illuminare sono state "mangiate" dal serpente.
  uint32_t lastMove;             // Variabile di tipo uint32_t (unsigned integer a 32 bit) per memorizzare il timestamp dell'ultimo movimento del serpente (in millisecondi).
  bool isVertical;               // Variabile di tipo bool per indicare se la direzione principale di movimento del serpente è verticale.
};

// Struttura dati per l'effetto "goccia d'acqua"
struct WaterDrop {
  float centerX;                 // Variabile di tipo float (numero in virgola mobile a precisione singola) per memorizzare la coordinata X del centro della goccia.
  float centerY;                 // Variabile di tipo float per memorizzare la coordinata Y del centro della goccia.
  float currentRadius;           // Variabile di tipo float per memorizzare il raggio attuale dell'onda che si propaga dalla goccia.
  uint32_t startTime;            // Variabile di tipo uint32_t per memorizzare il timestamp (in millisecondi) in cui è iniziato l'effetto della goccia.
  bool active;                   // Variabile di tipo bool per indicare se l'effetto della goccia è attualmente attivo.
  bool completed;                // Variabile di tipo bool per indicare se l'effetto della goccia ha completato la sua animazione.
  bool cleanupDone;              // Variabile di tipo bool per indicare se le operazioni di "pulizia" (reset di variabili) dopo il completamento dell'effetto sono state eseguite.
};

// Struttura dati per gestire i colori RGB (Rosso, Verde, Blu)
struct RGB {
  uint8_t r; // Componente rossa del colore (valore da 0 a 255).
  uint8_t g; // Componente verde del colore (valore da 0 a 255).
  uint8_t b; // Componente blu del colore (valore da 0 a 255).

  // Costruttori
  RGB() : r(255), g(255), b(255) {} // Costruttore predefinito che inizializza il colore a bianco (tutte le componenti al massimo).
  RGB(uint8_t r_, uint8_t g_, uint8_t b_) : r(r_), g(g_), b(b_) {} // Costruttore che permette di specificare i valori delle componenti RGB all'atto della creazione dell'oggetto.

  // Operatori utili per confrontare oggetti RGB
  bool operator==(const RGB& other) const { // Operatore di uguaglianza (==) che confronta due oggetti RGB e restituisce true se tutte le loro componenti sono uguali.
    return r == other.r && g == other.g && b == other.b;
  }

  bool operator!=(const RGB& other) const { // Operatore di disuguaglianza (!=) che confronta due oggetti RGB e restituisce true se almeno una delle loro componenti è diversa.
    return !(*this == other); // Utilizza l'operatore di uguaglianza per definire la disuguaglianza.
  }
};

// Struttura dati per la gestione dei colori (utilizzata in modo simile a RGB ma potenzialmente con un nome più generico)
struct Color {
  uint8_t r;
  uint8_t g;
  uint8_t b;

  // Costruttori (identici a quelli di RGB)
  Color() : r(255), g(255), b(255) {}
  Color(uint8_t r_, uint8_t g_, uint8_t b_) : r(r_), g(g_), b(b_) {}

  // Conversione implicita a RGB
  operator RGB() const { // Operatore di conversione che permette di utilizzare un oggetto `Color` dove è atteso un oggetto `RGB`. Restituisce un nuovo oggetto `RGB` con le stesse componenti.
    return RGB(r, g, b);
  }
};

// Struttura dati per la gestione del ciclo dei colori (effetto arcobaleno)
struct colorCycleState {
  bool isActive;                // Variabile di tipo bool per indicare se l'effetto di ciclo dei colori è attivo.
  uint32_t lastColorChange;     // Variabile di tipo uint32_t per memorizzare il timestamp (in millisecondi) dell'ultimo cambio di colore nel ciclo.
  uint16_t hue = 0;         // Variabile di tipo uint16_t per memorizzare la tonalità (Hue) nel modello di colore HSV (Hue, Saturation, Value), con valori da 0 a 360 gradi.
  uint8_t saturation = 255; // Variabile di tipo uint8_t per memorizzare la saturazione nel modello HSV, con valori da 0 (grigio) a 255 (colore pieno).
  bool fadingToWhite = false; // Variabile di tipo bool per indicare se il colore sta attualmente sfumando verso il bianco.
  bool showingWhite = false;  // Variabile di tipo bool per indicare se attualmente viene visualizzato il colore bianco.
};

colorCycleState colorCycle; // Dichiarazione di una variabile globale di tipo `colorCycleState` chiamata `colorCycle`.

// Struttura dati per l'effetto "Matrix" (pioggia di LED)
struct Drop {
  uint8_t x;         // Coordinata X della "goccia" nella matrice virtuale.
  float y;         // Coordinata Y della "goccia" (può essere float per un movimento più preciso).
  float speed;     // Velocità di caduta della "goccia".
  uint8_t intensity; // Intensità luminosa della "goccia".
  bool active;     // Flag per indicare se la "goccia" è attualmente attiva e in movimento.
  bool isMatrix2;  // Flag per distinguere tra la prima e la seconda variante dell'effetto "Matrix".
};

// Struttura dati per un elemento del menu di setup
struct SetupMenuItem {
  const char* label;           // Puntatore a una stringa costante (memorizzata nella memoria flash) che rappresenta l'etichetta visualizzata per questa opzione del menu.
  bool* valuePtr;              // Puntatore a una variabile di tipo bool che rappresenta lo stato (on/off) di questa opzione. Utilizzato per modificare direttamente il valore quando l'utente interagisce.
  uint8_t* modeValuePtr;       // Puntatore a una variabile di tipo uint8_t che rappresenta la modalità selezionata (per le opzioni che permettono di scegliere tra diverse modalità).
  bool isModeSelector;         // Variabile di tipo bool per indicare se questo elemento del menu è un selettore di modalità (invece di un semplice on/off).
};

// Struttura dati per memorizzare le opzioni di configurazione del setup
struct SetupOptions {
  bool autoNightModeEnabled;    // Stato (abilitato/disabilitato) della modalità notte automatica (basata sull'ora).
  bool touchSoundsEnabled;      // Stato (abilitato/disabilitato) dei suoni al tocco dello schermo.
  bool powerSaveEnabled;        // Stato (abilitato/disabilitato) della modalità di risparmio energetico.
  bool rainbowSecondsEnabled;   // Stato (abilitato/disabilitato) dell'effetto arcobaleno per i secondi.
  uint8_t defaultDisplayMode;   // Valore che rappresenta la modalità di visualizzazione predefinita all'avvio. Fa riferimento ai valori definiti nell'enum `DisplayMode`.
  bool wifiEnabled;             // Stato (abilitato/disabilitato) del Wi-Fi (potrebbe essere sempre true in questo progetto).
  bool ntpEnabled;              // Stato (abilitato/disabilitato) della sincronizzazione con server NTP (potrebbe essere sempre true se il Wi-Fi è attivo).
  bool alexaEnabled;            // Stato (abilitato/disabilitato) dell'integrazione con Amazon Alexa.
};

// Struttura dati per rappresentare un pulsante nella pagina di setup
struct SetupButton {
  int x;           // Coordinata X dell'angolo superiore sinistro del pulsante.
  int y;           // Coordinata Y dell'angolo superiore sinistro del pulsante.
  int width;       // Larghezza del pulsante in pixel.
  int height;      // Altezza del pulsante in pixel.
  const char* label; // Puntatore a una stringa costante (etichetta) visualizzata sul pulsante.
  uint16_t color;    // Colore di sfondo del pulsante (in formato colore del display, es. RGB565).
  uint16_t textColor; // Colore del testo visualizzato sul pulsante.
};

// Array di pulsanti per la pagina di setup
#define NUM_SETUP_BUTTONS 5 // Definisce il numero totale di pulsanti nella pagina di setup.
SetupButton setupButtons[NUM_SETUP_BUTTONS] = { // Inizializzazione dell'array di strutture `SetupButton` con le proprietà di ciascun pulsante.
  {60, 120, 360, 50, "CAMBIA COLORE", BLUE, WHITE},
  {60, 190, 360, 50, "REGOLA LUMINOSITA", GREEN, BLACK},
  {60, 260, 360, 50, "IMPOSTAZIONI WIFI", CYAN, BLACK},
  {60, 330, 360, 50, "RESET IMPOSTAZIONI", RED, WHITE},
  {60, 400, 360, 50, "ESCI", YELLOW, BLACK}
};

// ================== DICHIARAZIONE VARIABILI GLOBALI ==================
// Hardware
Arduino_DataBus *bus = new Arduino_SWSPI( // Creazione di un oggetto puntatore alla classe `Arduino_DataBus` per la comunicazione con il display tramite SPI software (SWSPI).
    GFX_NOT_DEFINED /* DC */, 39 /* CS */, // DC (Data/Command), CS (Chip Select)
    48 /* SCK */, 47 /* MOSI */, GFX_NOT_DEFINED /* MISO */); // SCK (Serial Clock), MOSI (Master Out Slave In), MISO (Master In Slave Out) - MISO non definito perché il display è solo in uscita.

Arduino_ESP32RGBPanel *rgbpanel = new Arduino_ESP32RGBPanel( // Creazione di un oggetto puntatore alla classe `Arduino_ESP32RGBPanel` per gestire il pannello RGB del display.
    18 /* DE */, 17 /* VSYNC */, 16 /* HSYNC */, 21 /* PCLK */, // DE (Data Enable), VSYNC (Vertical Sync), HSYNC (Horizontal Sync), PCLK (Pixel Clock)
    11 /* R0 */, 12 /* R1 */, 13 /* R2 */, 14 /* R3 */, 0 /* R4 */, // Pin per i bit del colore Rosso
    8 /* G0 */, 20 /* G1 */, 3 /* G2 */, 46 /* G3 */, 9 /* G4 */, 10 /* G5 */, // Pin per i bit del colore Verde
    4 /* B0 */, 5 /* B1 */, 6 /* B2 */, 7 /* B3 */, 15 /* B4 */, // Pin per i bit del colore Blu
    1 /* hsync_polarity */, 10 /* hsync_front_porch */, 8 /* hsync_pulse_width */, 50 /* hsync_back_porch */, // Timing orizzontale
    1 /* vsync_polarity */, 10 /* vsync_front_porch */, 8 /* vsync_pulse_width */, 20 /* vsync_back_porch */, // Timing verticale
    0 /* pclk_active_neg */, 12000000 /* prefer_speed */, false /* useBigEndian */, // Configurazione del clock dei pixel, velocità preferita, ordine dei byte
    0 /* de_idle_high */, 0 /* pclk_idle_high */, 0 /* bounce_buffer_size_px */); // Stato idle dei segnali DE e PCLK, dimensione del buffer per l'anti-rimbalzo (non usato qui).

Arduino_RGB_Display *gfx = new Arduino_RGB_Display( // Creazione di un oggetto puntatore alla classe `Arduino_RGB_Display`, che fornisce le funzioni di disegno sul display.
    480 /* width */, 480 /* height */, rgbpanel, 0 /* rotation */, true /* auto_flush */, // Dimensioni del display, oggetto del pannello, rotazione, auto-flush
    bus, GFX_NOT_DEFINED /* RST */, st7701_type9_init_operations, sizeof(st7701_type9_init_operations)); // Bus di comunicazione, pin di reset (non usato), sequenza di inizializzazione specifica per il controller del display ST7701.

// Touch
TAMC_GT911 ts = TAMC_GT911(I2C_SDA_PIN, I2C_SCL_PIN, TOUCH_INT, TOUCH_RST, max(TOUCH_MAP_X1, TOUCH_MAP_X2), max(TOUCH_MAP_Y1, TOUCH_MAP_Y2)); // Creazione di un oggetto della classe `TAMC_GT911` per interagire con il touch screen tramite I2C. I valori `max` sono usati per assicurare che l'intervallo di mappatura sia corretto.
int touch_last_x = 0, touch_last_y = 0; // Variabili globali per memorizzare le coordinate dell'ultimo tocco rilevato.

// Networking
Timezone myTZ; // Oggetto della classe `Timezone` per gestire il fuso orario locale.
Espalexa espalexa; // Oggetto della classe `Espalexa` per gestire l'integrazione con Amazon Alexa.
#ifdef AUDIO
AudioOutputI2S *output = nullptr; // Puntatore all'oggetto per l'output audio I2S, inizializzato a null.
AudioGeneratorMP3 *mp3 = nullptr;   // Puntatore all'oggetto per la decodifica MP3, inizializzato a null.
AudioFileSourceSPIFFS *file = nullptr; // Puntatore all'oggetto per la sorgente audio da SPIFFS, inizializzato a null.
AudioFileSourceBuffer *buff = nullptr;  // Puntatore all'oggetto per la sorgente audio da buffer, inizializzato a null.
bool isPlaying = false; // Flag per indicare se l'audio è attualmente in riproduzione.

// Buffer per tono di test
const int sampleRate = 16000; // Frequenza di campionamento per il tono di test (16000 campioni al secondo).
const int bufferLen = sampleRate/4; // Lunghezza del buffer per il tono di test (un quarto di secondo di dati).
int16_t sineBuffer[bufferLen]; // Array di interi a 16 bit per memorizzare i campioni del tono sinusoidale.

// Prototipi delle funzioni relative all'audio
void generateSineWave(); // Prototipo della funzione per generare un'onda sinusoidale.
void playTone(int frequency, int duration_ms); // Prototipo della funzione per riprodurre un tono a una data frequenza per una data durata.
void playFrequencySweep(); // Prototipo della funzione per riprodurre una variazione continua di frequenza (sweep).
bool playTTS(const String& text, const String& language); // Prototipo della funzione per riprodurre testo tramite sintesi vocale (Text-to-Speech).
void cleanupAudio(); // Prototipo della funzione per rilasciare le risorse utilizzate per l'audio.
String myUrlEncode(const String& msg); // MODIFICATO: rinominato da urlEncode a myUrlEncode - Prototipo della funzione per codificare una stringhe per l'URL.
bool announceTime(); // Prototipo della funzione per annunciare l'ora corrente tramite sintesi vocale.
void checkTimeAndAnnounce(); // Prototipo della funzione per verificare se è il momento di annunciare l'ora.
static unsigned long lastAnnounceTime = 0; // Variabile statica per memorizzare il timestamp dell'ultimo annuncio dell'ora.
static uint8_t lastMinuteChecked = 255; // Variabile statica per tenere traccia dell'ultimo minuto in cui è stato verificato l'annuncio dell'ora (inizializzata a un valore non valido).
#endif

// VARIABILI GLOBALI PER GESTIONE LETTERA E
uint8_t word_E_state = 0;           // Variabile globale di tipo uint8_t per memorizzare lo stato di visibilità della lettera "E" (0=disabilitata, 1=abilitata, 2=lampeggiante).
DisplayMode currentMode = MODE_FAST; // Variabile globale di tipo `DisplayMode` per memorizzare la modalità di visualizzazione corrente, inizializzata a `MODE_FAST`.
DisplayMode userMode = MODE_FAST;    // Variabile globale di tipo `DisplayMode` per memorizzare la modalità di visualizzazione preferita dall'utente, inizializzata a `MODE_FAST`.
static DisplayMode prevMode = MODE_FAST;  // Variabile statica di tipo `DisplayMode` per memorizzare la modalità di visualizzazione precedente, inizializzata a `MODE_FAST`.

uint8_t currentPreset = 0;   // Variabile globale di tipo uint8_t per memorizzare il preset di configurazione corrente (probabilmente un indice).
uint8_t currentHour = 0;     // Variabile globale di tipo uint8_t per memorizzare l'ora corrente (in formato 24 ore).
uint8_t currentMinute = 0;   // Variabile globale di tipo uint8_t per memorizzare il minuto corrente.
uint8_t currentSecond = 0;   // Variabile globale di tipo uint8_t per memorizzare il secondo corrente.
uint8_t lastHour = 255;      // Variabile globale di tipo uint8_t per memorizzare l'ora precedente (inizializzata a un valore non valido per forzare l'aggiornamento all'inizio).
uint8_t lastMinute = 255;    // Variabile globale di tipo uint8_t per memorizzare il minuto precedente (inizializzata a un valore non valido).
uint8_t lastSecond = 255;    // Variabile globale di tipo uint8_t per memorizzare il secondo precedente (inizializzata a un valore non valido).
uint8_t alexaOff = 0;        // Variabile globale di tipo uint8_t, probabilmente un flag per indicare se l'output del display è stato temporaneamente disattivato tramite Alexa (0=attivo, 1=disattivo).
uint32_t lastTouchTime = 0; // Variabile globale di tipo uint32_t per memorizzare il timestamp dell'ultimo tocco rilevato (in millisecondi).

// Variabili per l'effetto "Snake"
Snake snake;                     // Variabile globale di tipo `Snake` (la struttura definita in precedenza) per gestire lo stato e il comportamento del serpente.
bool snakeInitNeeded = true;     // Variabile globale di tipo bool per indicare se è necessaria l'inizializzazione della struttura `snake` (ad esempio, all'avvio o al cambio modalità).

// Variabili per l'effetto "Water Drop"
WaterDrop waterDrop;             // Variabile globale di tipo `WaterDrop` per gestire lo stato e il comportamento della goccia d'acqua.
bool waterDropInitNeeded = true; // Variabile globale di tipo bool per indicare se è necessaria l'inizializzazione della struttura `waterDrop`.

// Variabili per l'effetto "Matrix"
Drop drops[NUM_DROPS];       // Array globale di strutture `Drop` per memorizzare lo stato di ciascuna "goccia" nell'effetto matrice.
bool matrixInitialized = false; // Variabile globale di tipo bool per indicare se l'effetto matrice è stato inizializzato (ad esempio, con la posizione iniziale delle gocce).

// Variabili per la modalità "Slow" (definizione specifica dell'effetto non chiara qui)
bool slowInitialized = false; // Variabile globale di tipo bool per indicare se la modalità "Slow" è stata inizializzata.
bool fadeDone = false;        // Variabile globale di tipo bool, probabilmente usata nella modalità "Slow" per indicare se una fase di dissolvenza è completata.
uint32_t fadeStartTime = 0;   // Variabile globale di tipo uint32_t per memorizzare il timestamp di inizio di una dissolvenza nella modalità "Slow".
const uint16_t fadeDuration = 3000; // Costante di tipo uint16_t per definire la durata di una dissolvenza in millisecondi (3 secondi).

// Variabili per la modalità "Fade" (dissolvenza delle parole dell'ora)
enum FadePhase {
  FADE_SONO_LE,        // Fase di dissolvenza per la parola "SONO LE".
  FADE_ORA,            // Fase di dissolvenza per la parola dell'ora.
  FADE_E,              // Fase di dissolvenza per la parola "E".
  FADE_MINUTI_NUMERO,  // Fase di dissolvenza per il numero dei minuti.
  FADE_MINUTI_PAROLA,  // Fase di dissolvenza per la parola "MINUTI".
  FADE_DONE            // Fase che indica che la sequenza di dissolvenza è completata.
};

// Struct per le parole dei minuti (decine e unità)
struct MinuteWords {
  const uint8_t* tens; // Puntatore a un array di byte (probabilmente indici nella mappa delle parole) per la decina dei minuti.
  const uint8_t* ones; // Puntatore a un array di byte per l'unità dei minuti.
};

static FadePhase fadePhase = FADE_SONO_LE; // Variabile statica di tipo `FadePhase` per memorizzare la fase attuale della dissolvenza, inizializzata a `FADE_SONO_LE`.
static bool fadeInitialized = false;    // Variabile statica di tipo bool per indicare se la sequenza di dissolvenza è stata inizializzata.
static uint8_t fadeStep = 0;           // Variabile statica di tipo uint8_t per tenere traccia del passo corrente nell'animazione di dissolvenza.
static uint32_t lastFadeUpdate = 0;     // Variabile statica di tipo uint32_t per memorizzare il timestamp dell'ultimo aggiornamento dell'animazione di dissolvenza.
static const uint8_t fadeSteps = 20;    // Costante statica di tipo uint8_t per definire il numero totale di passi nell'animazione di dissolvenza.
static const uint16_t wordFadeDuration = 500; // Costante statica di tipo uint16_t per definire la durata in millisecondi della dissolvenza di una singola parola.

// Buffer per il display e gestione dei pixel
uint16_t displayBuffer[NUM_LEDS]; // Array di tipo uint16_t per memorizzare lo stato del colore di ciascun LED nella matrice virtuale (probabilmente in formato RGB565).
bool pixelChanged[NUM_LEDS] = {false}; // Array di tipo bool per indicare se il colore di un pixel è cambiato dall'ultimo aggiornamento parziale del display.
uint32_t lastPartialUpdate = 0;    // Variabile globale di tipo uint32_t per memorizzare il timestamp dell'ultimo aggiornamento parziale del display.

bool targetPixels[NUM_LEDS] = {false};   // Array di tipo bool per memorizzare quali LED devono essere accesi in un determinato momento (usato probabilmente per effetti semplici on/off).
bool targetPixels_1[NUM_LEDS] = {false}; // Array simile a `targetPixels`, probabilmente usato per un secondo layer di animazione o per memorizzare uno stato intermedio.
bool targetPixels_2[NUM_LEDS] = {false}; // Altro array simile, per un terzo layer o stato.
bool activePixels[NUM_LEDS] = {false};   // Array di tipo bool per memorizzare quali LED sono attualmente attivi (accesi).

// Opzioni e stati per la pagina di setup
SetupOptions setupOptions;                // Variabile globale di tipo `SetupOptions` per memorizzare le impostazioni di configurazione.
SetupMenuItem setupMenuItems[SETUP_ITEMS_COUNT]; // Array di strutture `SetupMenuItem` per definire gli elementi del menu di setup.
uint8_t setupCurrentScroll = 0;         // Variabile globale di tipo uint8_t per memorizzare la posizione corrente dello scroll nel menu di setup.
unsigned long setupPageTimeout = 60000;  // Variabile globale di tipo unsigned long per definire il timeout (in millisecondi) per uscire automaticamente dalla pagina di setup (60 secondi).
bool exitingSetupPage = false;          // Variabile globale di tipo bool per indicare se si sta per uscire dalla pagina di setup.
bool setupPageActive = false;           // Variabile globale di tipo bool per indicare se la pagina di setup è attualmente attiva e visualizzata.
bool setupScrollActive = false;         // Variabile globale di tipo bool per indicare se lo scroll nel menu di setup è attivo (l'utente sta scorrendo).
bool checkingSetupScroll = false;       // Variabile globale di tipo bool per indicare se si sta verificando un movimento per determinare se è uno scroll.
uint32_t setupLastActivity = 0;         // Variabile globale di tipo uint32_t per memorizzare il timestamp dell'ultima interazione dell'utente con la pagina di setup.
int16_t colorTextX = 0;                // Variabile globale di tipo int16_t per memorizzare la coordinata X del testo relativo alla selezione del colore nella pagina di setup.
int16_t colorTextY = 240;               // Variabile globale di tipo int16_t per memorizzare la coordinata Y del testo relativo alla selezione del colore.
extern bool menuActive;                 // Dichiarazione di una variabile esterna (definita in un altro file) di tipo bool per indicare lo stato di attività del menu (potrebbe essere la stessa `setupPageActive`).

// Colore del testo dell'orologio
Color currentColor = Color(255, 255, 255);  // Variabile globale di tipo `Color` per memorizzare il colore attuale utilizzato per visualizzare l'orologio (inizializzato a bianco).
Color userColor = Color(255, 255, 255);     // Variabile globale di tipo `Color` per memorizzare il colore preferito dall'utente (inizializzato a bianco e potenzialmente caricato dalla EEPROM).
uint8_t BackColor = 40;                    // Variabile globale di tipo uint8_t per memorizzare un valore di "sfondo" per i colori (potrebbe essere un'intensità o un indice di colore).
Color TextBackColor = Color(40, 40, 40);    // Variabile globale di tipo `Color` per memorizzare il colore di sfondo del testo (inizializzato a un grigio scuro).
Color snake_E_color = Color(255, 255, 255); // Variabile globale di tipo `Color` per memorizzare il colore temporaneo per la visualizzazione e il lampeggio della lettera "E" nell'effetto "Snake".
Color hsvToRgb(uint8_t h, uint8_t s, uint8_t v); // Prototipo della funzione per convertire un colore dal modello HSV (Hue, Saturation, Value) al modello RGB (Red, Green, Blue).

// RGB leds[NUM_LEDS]; // Dichiarazione (commentata) di un array di strutture `RGB` per memorizzare il colore di ciascun LED (potrebbe essere stato sostituito da `displayBuffer`).
bool isFadeFirstEntry = true;      // Variabile globale di tipo bool per indicare se è la prima volta che si entra nella modalità "Fade" (potrebbe essere usata per un'inizializzazione specifica).
bool backgroundInitialized = false; // Variabile globale di tipo bool per indicare se lo sfondo del display è stato inizializzato.

// Strutture dati globali
TimeDisplay timeDisplay; // Variabile globale di tipo `TimeDisplay` per memorizzare i LED che rappresentano l'ora corrente.
SnakePath snakePath;     // Variabile globale di tipo `SnakePath` per memorizzare il percorso del serpente.

uint32_t debugTime = 0;            // Variabile globale di tipo uint32_t per scopi di debug (memorizzazione di timestamp).
static uint32_t menuDisplayTime = 0;   // Variabile statica di tipo uint32_t per memorizzare il timestamp dell'ultima visualizzazione del menu.
bool menuActive = false;        // Variabile globale di tipo bool per indicare lo stato di attività del menu (potrebbe essere la stessa `setupPageActive`).
bool resetCountdownStarted = false; // Variabile globale di tipo bool per indicare se è iniziato un conto alla rovescia per un reset.

// Touch e Ciclo colori
uint32_t touchStartTime = 0;          // Variabile globale di tipo uint32_t per memorizzare il timestamp dell'inizio di un tocco.

//===================================================================//
//                        GESTIONE COLORI                            //
//===================================================================//
void updateColorCycling(uint32_t currentMillis) { // Funzione per aggiornare lo stato del ciclo dei colori (effetto arcobaleno).
  const uint16_t interval = 100;  // Costante per definire l'intervallo di tempo in millisecondi tra un cambio di colore e l'altro.
  static uint8_t whiteSteps = 0; // Variabile statica per contare i passi durante la fase di visualizzazione del bianco.

  if (!colorCycle.isActive) return; // Se il ciclo dei colori non è attivo, la funzione termina.

  if (currentMillis - colorCycle.lastColorChange >= interval) { // Se è trascorso l'intervallo di tempo necessario dall'ultimo cambio di colore.
    colorCycle.lastColorChange = currentMillis; // Aggiorna il timestamp dell'ultimo cambio di colore.

    //  Fase 1: Ciclo colore HSV
    if (!colorCycle.fadingToWhite && !colorCycle.showingWhite) { // Se non si sta sfumando verso il bianco e non si sta mostrando il bianco.
      //Serial.println("FASE 1");
      colorCycle.hue += 3; // Incrementa la tonalità (hue) per passare al colore successivo nello spettro.

      // Dopo un giro completo, inizia la sfumatura verso il bianco
      if (colorCycle.hue >= 360) { // Se la tonalità ha completato un ciclo (360 gradi).
        colorCycle.fadingToWhite = true; // Inizia la fase di sfumatura verso il bianco.
      }
    }

    // Fase 2: Sfumatura verso bianco (saturazione)
    if (colorCycle.fadingToWhite) { // Se si sta sfumando verso il bianco.
      //Serial.println("FASE 2");
      if (colorCycle.saturation > 0) { // Se la saturazione non è ancora zero (bianco).
        colorCycle.saturation -= 5; // Diminuisce la saturazione per avvicinarsi al bianco.
        if (colorCycle.saturation == 0) { // Se la saturazione raggiunge zero (bianco).
          colorCycle.fadingToWhite = false; // Termina la fase di sfumatura.
          colorCycle.showingWhite = true;  // Inizia la fase di visualizzazione del bianco.
          whiteSteps = 0;                 // Resetta il contatore dei passi del bianco.
        }
      }
    }

    // Fase 3: Mostra bianco per un po'
    if (colorCycle.showingWhite) { // Se si sta mostrando il bianco.
      //Serial.println("FASE 3");
      whiteSteps++; // Incrementa il contatore dei passi del bianco.
      if (whiteSteps >= 30) {  // Circa 3 secondi (30 * 100ms).
        colorCycle.showingWhite = false; // Termina la fase di visualizzazione del bianco.
        colorCycle.saturation = 255;    // Resetta la saturazione al massimo per il ciclo successivo.
        colorCycle.hue = 0;              // Resetta la tonalità all'inizio del ciclo.
      }
    }

    // 🖍 Calcolo colore attuale
    uint8_t scaledHue = (colorCycle.hue % 360) * 255 / 360; // Scala la tonalità da 0-360 a 0-255 per compatibilità con `hsvToRgb`.
    Color c = hsvToRgb(scaledHue, colorCycle.saturation, 255); // Converte il valore HSV corrente in un oggetto `Color` (RGB).
    currentColor = c; // Aggiorna il colore corrente globale con il nuovo colore del ciclo.

    // ✨ Aggiorna parole con il colore attuale
    if (currentHour == 0) { // Se l'ora è mezzanotte.
      displayWord(WORD_MEZZANOTTE, currentColor);
    } else { // Altrimenti (se non è mezzanotte).
      displayWord(WORD_SONO_LE, currentColor); // Visualizza la parola "SONO LE".
      const uint8_t* hourWord = (const uint8_t*)pgm_read_ptr(&HOUR_WORDS[currentHour]); // Ottiene un puntatore alla parola dell'ora corrente dalla memoria flash.
      displayWord(hourWord, currentColor); // Visualizza la parola dell'ora.
    }

    if (currentMinute > 0) { // Se il minuto corrente è maggiore di zero.
      displayWord(WORD_E, currentColor); // Visualizza la parola "E".
      showMinutes(currentMinute, currentColor); // Visualizza il numero e la parola dei minuti.
      displayWord(WORD_MINUTI, currentColor); // Visualizza la parola "MINUTI".
    }
  }
}

void rgbToHsv(uint8_t r, uint8_t g, uint8_t b, uint16_t &h, uint8_t &s, uint8_t &v) {
  float r_norm = r / 255.0f;
  float g_norm = g / 255.0f;
  float b_norm = b / 255.0f;

  float max_c = max(r_norm, max(g_norm, b_norm));
  float min_c = min(r_norm, min(g_norm, b_norm));
  float delta = max_c - min_c;

  // Valore massimo = luminosità (Value)
  v = round(max_c * 255.0f);

  // Saturazione (Saturation)
  if (max_c == 0.0f) {
    s = 0;
  } else {
    s = round((delta / max_c) * 255.0f);
  }

  // Tonalità (Hue)
  float hue_deg = 0.0f;
  if (delta > 0.0f) {
    if (max_c == r_norm) {
      hue_deg = 60.0f * fmodf((g_norm - b_norm) / delta, 6.0f);
    } else if (max_c == g_norm) {
      hue_deg = 60.0f * (((b_norm - r_norm) / delta) + 2.0f);
    } else { // max_c == b_norm
      hue_deg = 60.0f * (((r_norm - g_norm) / delta) + 4.0f);
    }

    if (hue_deg < 0.0f) {
      hue_deg += 360.0f;
    }
  }

  // Scala hue su 0–255 (compatibile con hsvToRgb)
  h = round(hue_deg * 255.0f / 360.0f);
}

// Funzione per convertire RGB a RGB565 (formato colore a 16 bit utilizzato da molti display)
uint16_t RGBtoRGB565(const RGB& color) {
  return ((color.r & 0xF8) << 8) | ((color.g & 0xFC) << 3) | (color.b >> 3);
}

// Funzione per convertire Color a RGB565
uint16_t convertColor(const Color& color) {
  return ((color.r & 0xF8) << 8) | ((color.g & 0xFC) << 3) | (color.b >> 3);
}

// Conversione da HSV a RGB
Color hsvToRgb(uint8_t h, uint8_t s, uint8_t v) {
  // Implementazione ottimizzata della conversione HSV a RGB
  uint8_t region, remainder, p, q, t;

  if (s == 0) {
    return Color(v, v, v); // Se la saturazione è 0, il colore è una scala di grigi (r=g=b=v).
  }

  region = h / 43;
  remainder = (h - (region * 43)) * 6;

  p = (v * (255 - s)) >> 8;
  q = (v * (255 - ((s * remainder) >> 8))) >> 8;
  t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;

  switch (region) {
    case 0:
      return Color(v, t, p);
    case 1:
      return Color(q, v, p);
    case 2:
      return Color(p, v, t);
    case 3:
      return Color(p, q, v);
    case 4:
      return Color(t, p, v);
    default:
      return Color(v, p, q);
  }
}

// ================== SETUP PRINCIPALE ==================
void setup() {

  // RICAVO NOME SKETCH DA STAMPARE A VIDEO
  int lastSlash = ino.lastIndexOf("/");
  if (lastSlash == -1) {
      lastSlash = ino.lastIndexOf("\\");
  }
  ino = ino.substring(lastSlash + 1);  // MyClock.ino
  
  int dotIndex = ino.lastIndexOf(".");
  if (dotIndex > 0) {
      ino = ino.substring(0, dotIndex);  // MyClock
  }

  Serial.begin(115200);
  delay(1000);

  Serial.println("\n\nOraQuadra2 Display 480x480 - Inizializzazione");
  
  // Inizializza SPIFFS (SPI Flash File System)
  if (!SPIFFS.begin(true)) {
    Serial.println("Errore inizializzazione SPIFFS - formattazione...");
    SPIFFS.format(); // Tenta di formattare il filesystem se l'inizializzazione fallisce.
    if (!SPIFFS.begin(true)) {
      Serial.println("ERRORE CRITICO: SPIFFS non inizializzabile!");
      while(1) delay(1000); // Blocca l'esecuzione se SPIFFS non può essere inizializzato.
    }
  }
  
  Serial.printf("SPIFFS: %d KB totali, %d KB usati\n", 
                SPIFFS.totalBytes()/1024, SPIFFS.usedBytes()/1024);
                
  // SETUP COMPONENTI IN SEQUENZA
  setup_eeprom();  // Inizializza e carica le impostazioni dalla EEPROM.
  setup_display(); // Inizializza il display.
  setup_touch();   // Inizializza il touch screen.

   // VERIFICA SE I 4 ANGOLI SONO PREMUTI ALL'AVVIO (per triggerare un reset delle impostazioni WiFi)
  int cornerCheckAttempts = 0;
  while (cornerCheckAttempts < 5) { // Facciamo più tentativi per stabilizzare il touch
    if (checkCornersAtBoot()) {
      // Mostra feedback e conferma
      gfx->fillScreen(YELLOW);
      gfx->setTextColor(BLACK);
      gfx->setCursor(80, 180);
      gfx->println(F("RESET WIFI AVVIO"));
      gfx->setCursor(120, 240);
      gfx->println(F("RILEVATO"));
      delay(2000);
      
      // Esegui il reset WiFi (funzione definita altrove)
      resetWiFiSettings();
      // Non ritorna mai da qui (il dispositivo si riavvia dopo il reset WiFi)
    }
    cornerCheckAttempts++;
    delay(50); // Piccola pausa tra i controlli
  }

  // Setup WiFi e servizi correlati (solo se necessario)
  setup_wifi();

  if (WiFi.status() == WL_CONNECTED) {
    setup_OTA();   // Inizializza l'aggiornamento Over-The-Air.
    setup_alexa(); // Inizializza l'integrazione con Alexa.
    setup_NTP();   // Inizializza la sincronizzazione dell'ora tramite NTP.
    #ifdef AUDIO
    setup_audio(); // Inizializza il sistema audio (solo se la macro AUDIO è definita).
    #endif
  } else {
    Serial.println("WiFi non connesso, utilizzo ora predefinita");
    currentHour = 12;
    currentMinute = 0;
    currentSecond = 0;
  }
  // Inizializzazione delle opzioni di setup (caricamento dalla EEPROM o valori predefiniti)
  initSetupOptions();
  initSetupMenu(); // Inizializza la struttura del menu di setup.

  // Applicazione del preset corrente (caricato dalla EEPROM) e aggiornamento iniziale del display.
  applyPreset(currentPreset);
}

void setup_NTP() {
  Serial.println("Inizializzazione NTP...");
  
  // Configura server NTP (Network Time Protocol)
  setServer("pool.ntp.org");
  
  // Imposta il fuso orario locale - Italia (GMT+1 con ora legale)
  myTZ.setLocation("Europe/Rome");
  
  // Attendi prima sincronizzazione (con un timeout di 10 secondi)
  waitForSync(10);
  
  // Semplice verifica che controlla se l'ora è nell'intervallo valido (0-23)
  if (myTZ.hour() <= 23) {
    Serial.println("NTP sincronizzato. Ora corrente: " + myTZ.dateTime("H:i:s"));
    
    // Imposta l'ora corrente per le variabili del sistema
    currentHour = myTZ.hour();
    currentMinute = myTZ.minute();
    currentSecond = myTZ.second();
    
    // Regola la luminosità del display in base all'ora del giorno
    if (currentHour < HOUR_DAY || currentHour >= HOUR_NIGHT) {
      ledcWrite(PWM_CHANNEL, BRIGHTNESS_NIGHT); // Imposta la luminosità notturna.
    } else {
      ledcWrite(PWM_CHANNEL, BRIGHTNESS_DAY);   // Imposta la luminosità diurna.
    }
    
    Serial.printf("Impostazione ora: %02d:%02d:%02d\n", 
                  currentHour, currentMinute, currentSecond);
  } else {
    Serial.println("Sincronizzazione NTP non riuscita, utilizzo ora predefinita");
    // Imposta un'ora predefinita in caso di fallimento della sincronizzazione
    currentHour = 12;
    currentMinute = 0;
    currentSecond = 0;
  }
}

void setup_audio() {
  #ifdef AUDIO
  Serial.println("Inizializzazione sistema audio...");
  
  // Configura il pin di abilitazione I2S (se definito)
  if (I2S_PIN_ENABLE != -1) {
    pinMode(I2S_PIN_ENABLE, OUTPUT);
    digitalWrite(I2S_PIN_ENABLE, LOW);  // Disabilita l'amplificatore all'inizio
    delay(5);
    digitalWrite(I2S_PIN_ENABLE, HIGH); // Abilita l'amplificatore
    delay(10);
  }
  
  // Attendi un momento per la stabilizzazione dell'hardware audio
  delay(50);
  
  // Inizializza l'output I2S con la libreria Audio
  output = new AudioOutputI2S();
  output->SetPinout(I2S_BCLK, I2S_LRC, I2S_DOUT); // Imposta i pin per BCLK, LRC e DOUT.
  output->SetGain(VOLUME_LEVEL); // Imposta il livello di volume.
  output->SetChannels(1);  // Configura l'output come mono (1 canale).
  
  Serial.println("Sistema audio inizializzato");
  
  // Test audio iniziale (solo se connesso al WiFi)
  if (WiFi.status() == WL_CONNECTED) {
    String greeting = "Sistema audio inizializzato. L'ora attuale è ";
    
    // Verifica se l'ora è valida (0-23)
    if (myTZ.hour() <= 23) {
      if (myTZ.hour() == 0 || myTZ.hour() == 24) {
        greeting += "mezzanotte";
      } else if (myTZ.hour() == 12) {
        greeting += "mezzogiorno";
      } else if (myTZ.hour() == 1 || myTZ.hour() == 13) {
        greeting += "l'una";
      } else {
        greeting += "le " + String(myTZ.hour() > 12 ? myTZ.hour() - 12 : myTZ.hour());
      }
      
      if (myTZ.minute() > 0) {
        greeting += myTZ.minute() == 1 ? " e un minuto" : " e " + String(myTZ.minute()) + " minuti";
      }
    } else {
      greeting += "ora non disponibile";
    }
    
    Serial.println("Messaggio di benvenuto: " + greeting);
    playTTS(greeting, "it"); // Riproduce il messaggio di benvenuto tramite TTS (Text-to-Speech).
    
    // Attendi la fine della riproduzione
    while (mp3 && mp3->isRunning()) {
      if (!mp3->loop()) break; // Interrompe il loop se la riproduzione è finita o in errore.
      delay(10);
    }
    cleanupAudio(); // Libera le risorse audio dopo la riproduzione.
  } else {
    // Test audio base se non c'è WiFi
    playTone(440, 500); // Riproduce un tono a 440 Hz per 500 ms.
    delay(300);
    playTone(880, 500); // Riproduce un tono a 880 Hz per 500 ms.
  }
   // Disabilita e riabilita l'amplificatore audio (sicurezza/reset)
  if (I2S_PIN_ENABLE != -1) {
    digitalWrite(I2S_PIN_ENABLE, LOW);
    delay(5);
    digitalWrite(I2S_PIN_ENABLE, HIGH);
    delay(10);
  }
  
  isPlaying = false; // Resetta il flag di riproduzione.
  Serial.println("Setup audio completato - in attesa del cambio ora...");
 #endif 
}

// ================== LOOP PRINCIPALE ==================
void loop() {
  static uint32_t lastUpdate = 0;     // Timestamp dell'ultimo aggiornamento dell'ora.
  static uint32_t lastUpdate_sec = 0; // Timestamp dell'ultimo aggiornamento dei secondi (potrebbe non essere strettamente necessario).
  static uint32_t lastButtonCheck = 0; // Timestamp dell'ultima verifica dei tocchi/pulsanti.
  static uint32_t lastEffectUpdate = 0; // Timestamp dell'ultimo aggiornamento degli effetti visivi.
  static bool lastEState = false;    // Stato precedente della lettera 'E' (probabilmente per il lampeggio).
  static uint8_t lastSecondFast = 255; // Ultimo secondo visualizzato in modalità FAST (per aggiornare solo al cambio di secondo).
  uint32_t currentMillis = millis();  // Ottiene il tempo attuale in millisecondi dall'avvio.

  // Gestione OTA (Over-The-Air update) e network events (ezTime)
  if (WiFi.status() == WL_CONNECTED) {
    ArduinoOTA.handle(); // Gestisce le eventuali richieste di aggiornamento OTA.
    events(); // Gestisce gli eventi della libreria ezTime (es. cambio di ora).
    espalexa.loop(); // Gestisce le comunicazioni con Alexa.
  }

  // Check dei tocchi ogni 50ms
  if (currentMillis - lastButtonCheck > 50) {
    checkButtons(); // Funzione per leggere e gestire gli eventi del touch screen.
    lastButtonCheck = currentMillis;
  }

  // Verifica timeout della pagina di setup
  if (setupPageActive && currentMillis - setupLastActivity > setupPageTimeout) {
    hideSetupPage(); // Nasconde la pagina di setup se non c'è stata attività per un certo periodo.
  }

  // Gestione audio
  #ifdef AUDIO
  if (mp3 && mp3->isRunning()) { // Se un file MP3 è in riproduzione.
    if (!mp3->loop()) { // E se il loop di riproduzione è terminato (fine del file o errore).
      mp3->stop();     // Ferma la riproduzione.
      cleanupAudio();  // Libera le risorse audio.
      Serial.println("Riproduzione completata");
    }
  }
  // Controllo dell'ora per l'annuncio vocale (ogni 10 secondi, solo se connesso e non in riproduzione)
  static unsigned long lastTimeCheck = 0;
  if (millis() - lastTimeCheck > 10000 && !isPlaying && WiFi.status() == WL_CONNECTED) {
    lastTimeCheck = millis();
    checkTimeAndAnnounce(); // Verifica se è il momento di annunciare l'ora vocalmente.
  }
  #endif
  
  // Aggiornamento orario ogni secondo se connesso al WiFi
  if (WiFi.status() == WL_CONNECTED && currentMillis - lastUpdate > 1000) {
    currentHour = myTZ.hour();     // Ottiene l'ora dal fuso orario.
    currentMinute = myTZ.minute();   // Ottiene i minuti.
    currentSecond = myTZ.second();   // Ottiene i secondi.
    lastUpdate = currentMillis;    // Aggiorna il timestamp dell'ultimo aggiornamento.
    
    // Regola la luminosità in base all'ora corrente
    if(currentHour < HOUR_DAY || currentHour >= HOUR_NIGHT) {
      ledcWrite(PWM_CHANNEL, BRIGHTNESS_NIGHT);      
    } else {
      ledcWrite(PWM_CHANNEL, BRIGHTNESS_DAY);
    }
  } else if (currentMillis - lastUpdate > 1000) {
    // Senza WiFi, incrementa manualmente i secondi (per un orologio di fallback)
    uint8_t oldSecond = currentSecond;
    currentSecond++;
    if (currentSecond >= 60) {
      currentSecond = 0;
      currentMinute++;
      if (currentMinute >= 60) {
        currentMinute = 0;
        currentHour = (currentHour + 1) % 24; // Incrementa l'ora (0-23).      
      }
    }
    // Regola la luminosità anche in modalità offline
    if(currentHour < HOUR_DAY || currentHour >= HOUR_NIGHT) {
      ledcWrite(PWM_CHANNEL, BRIGHTNESS_NIGHT);      
    } else {
      ledcWrite(PWM_CHANNEL, BRIGHTNESS_DAY);
    }
    lastUpdate = currentMillis;
  }

  // ===== GESTIONE PRIORITARIA DEL CICLAGGIO COLORI =====
  if (colorCycle.isActive) {
    updateColorCycling(currentMillis);    // Aggiorna l'animazione del ciclo dei colori.
  }
  
  else if (!setupPageActive && !resetCountdownStarted) {
    // Gestione del display in base alla modalità corrente (se non è attiva la pagina di setup e non è iniziato un reset)
    if (alexaOff == 0) {  // Se il display non è stato spento tramite Alexa.
      switch (currentMode) {
        case MODE_MATRIX:
          if (currentMillis - lastEffectUpdate > 20) {
            updateMatrix();       // Aggiorna l'effetto "Matrix".
            lastEffectUpdate = currentMillis;
          }
          blinkWord_E();  // Gestisce il lampeggio della lettera 'E' (se abilitato).
          break;
          
        case MODE_MATRIX2:
          if (currentMillis - lastEffectUpdate > 20) {
            updateMatrix2();      // Aggiorna la seconda variante dell'effetto "Matrix".
            lastEffectUpdate = currentMillis;
          }
          blinkWord_E();
          break;
          
        case MODE_SNAKE:
          if (currentMillis - lastEffectUpdate > 20) {
            updateSnake();        // Aggiorna l'effetto "Snake".
            lastEffectUpdate = currentMillis;
          }
          blinkWord_E();
          break;
          
        case MODE_WATER:
          if (currentMillis - lastEffectUpdate > 20) {
            updateWaterDrop();    // Aggiorna l'effetto "Goccia d'acqua".
            lastEffectUpdate = currentMillis;
          }
          blinkWord_E();
          break;
          
        case MODE_FADE:
          if (currentMillis - lastEffectUpdate > 20) {
            updateFadeMode();     // Aggiorna l'animazione di dissolvenza.
            lastEffectUpdate = currentMillis;           
          }
          blinkWord_E();                    
          break;
          
        case MODE_SLOW:
          if (currentMillis - lastEffectUpdate > 20) {
            updateSlowMode();     // Aggiorna la modalità "Slow".
            lastEffectUpdate = currentMillis;           
          }
          blinkWord_E();                    
          break;
          
        case MODE_FAST:
          // Aggiorna la modalità "Fast" solo al cambio di ora o minuto
          if (lastHour != currentHour || lastMinute != currentMinute) {
            updateFastMode();     // Aggiorna la visualizzazione veloce dell'ora.
          } else {
            blinkWord_E();
          }                              
          break;
      }
    }
  }

  // Permette al watchdog timer dell'ESP32 di non resettare il dispositivo
  yield();
}

// GESTIONE LETTERA E con lampeggio basato sui secondi
void blinkWord_E(){
  if (currentMinute > 0) { // Lampeggia 'E' solo se ci sono i minuti.
    static bool E_on = false;
    static bool E_off = false;     
    uint8_t pos = 116; // Posizione della lettera 'E' nella mappa delle parole.
    // E Blink
    if (word_E_state == 1 ) { // Se lo stato di 'E' è impostato per il lampeggio.
      if (currentSecond % 2 == 0) { // Se il secondo è pari.         
        E_off = false;
        if(!E_on){
          E_on = true;    
          gfx->setTextColor(convertColor(currentColor));       
          gfx->setCursor(pgm_read_word(&TFT_X[pos]), pgm_read_word(&TFT_Y[pos]));
          gfx->write(pgm_read_byte(&TFT_L[pos]));             
        }                  
      } else { // Se il secondo è dispari.
        E_on = false;
        if(!E_on){
          E_off = true;
          gfx->setTextColor(convertColor(TextBackColor));
          gfx->setCursor(pgm_read_word(&TFT_X[pos]), pgm_read_word(&TFT_Y[pos]));
          gfx->write(pgm_read_byte(&TFT_L[pos]));
        }
      }
    }
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

// Funzione per verificare se tutti e quattro gli angoli del touch screen sono premuti all'avvio
bool checkCornersAtBoot() {
  // Leggi lo stato del touch screen
  ts.read();
  
  // Se non ci sono almeno 4 tocchi, ritorna falso
  if (ts.touches < 4) return false;
  
  // Variabili per tenere traccia se ogni angolo è stato toccato
  bool tl = false, tr = false, bl = false, br = false; // Top-Left, Top-Right, Bottom-Left, Bottom-Right
  
  // Controlla tutti i punti di contatto rilevati
  for (int i = 0; i < ts.touches && i < 10; i++) {
    // Mappa le coordinate del touch screen alle coordinate del display (0-479)
    int x = map(ts.points[i].x, TOUCH_MAP_X1, TOUCH_MAP_X2, 0, 480 - 1);
    int y = map(ts.points[i].y, TOUCH_MAP_Y1, TOUCH_MAP_Y2, 0, 480 - 1);
    
    // Verifica se il punto di contatto rientra in una delle zone degli angoli (con un margine di 100 pixel)
    if (x < 100 && y < 100) tl = true;
    else if (x > 380 && y < 100) tr = true;
    else if (x < 100 && y > 380) bl = true;
    else if (x > 380 && y > 380) br = true;
  }
  
  // Ritorna vero solo se tutti e quattro gli angoli sono stati toccati contemporaneamente
  return (tl && tr && bl && br);
}

/////////////////////////////////////////////////MENU///////////////////////////////////////////////// 