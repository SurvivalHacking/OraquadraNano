// ================== FUNZIONI UTILITY PER FLIP CLOCK ==================

#ifdef EFFECT_FLIP_CLOCK

// Calcola l'orario test attuale basandosi sul tempo trascorso dall'impostazione
void calculateTestTime(uint8_t &hour, uint8_t &minute, uint8_t &second) {
  // Calcola l'orario test attuale basandosi sul tempo trascorso dall'impostazione

  if (WiFi.status() != WL_CONNECTED) {
    // Senza WiFi, usa solo il tempo trascorso da millis()
    uint32_t elapsedMillis = millis() - testModeStartTime;
    uint32_t elapsedSeconds = elapsedMillis / 1000;

    // Calcola ore, minuti e secondi totali dall'orario base (inclusi i secondi impostati)
    uint32_t totalSeconds = (testHour * 3600) + (testMinute * 60) + testSecond + elapsedSeconds;

    hour = (totalSeconds / 3600) % 24;
    minute = (totalSeconds / 60) % 60;
    second = totalSeconds % 60;
  } else {
    // Con WiFi, usa i secondi NTP per maggiore precisione
    uint8_t currentSecondNTP = myTZ.second();

    // Calcola secondi trascorsi (gestendo il wrap a 60)
    int secondsPassed;
    if (currentSecondNTP >= testSecond) {
      secondsPassed = currentSecondNTP - testSecond;
    } else {
      secondsPassed = (60 - testSecond) + currentSecondNTP;
    }

    // Aggiungi i secondi trascorsi all'orario base
    uint32_t totalSeconds = (testHour * 3600) + (testMinute * 60) + secondsPassed;

    hour = (totalSeconds / 3600) % 24;
    minute = (totalSeconds / 60) % 60;
    second = totalSeconds % 60;
  }
}

// Funzione stub per audio flip clock (da implementare se necessario)
bool playClackViaWiFi() {
  // AUDIO NON IMPLEMENTATO IN V1_2_A
  // Questa è una versione stub per evitare errori di compilazione
  // Se vuoi l'audio, devi:
  // 1. Copiare il file 7_AUDIO_WIFI.ino dalla V1_3
  // 2. Configurare ESP32C3 per l'audio

  #ifdef AUDIO
  Serial.println("[FLIP CLOCK] Audio non implementato in questa versione");
  #endif

  return false;
}

#endif // EFFECT_FLIP_CLOCK
