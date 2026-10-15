//12.10.2026
//Sketch voll funktionsfähig
#include <Wire.h>
#include <U8g2lib.h>

// U8g2 Konfiguration mit Seitenpuffer (_1_) für den Arduino Uno
U8G2_SH1106_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

// Pin-Definitionen
const int dirPin = 8;         // DIR-Pin des Treibers an Pin 8
const int stepPin = 9;        // STEP-Pin des Treibers an Pin 9
const int buttonForward = 2;  // Taster für Vorwärtsfahrt an Pin 2
const int buttonBackward = 3; // Taster für Rückwärtsfahrt an Pin 3
const int homeButton = 5;     // Homing-Sensor / Endschalter an Pin 5

// Taster für definierte Positionen (Gleise)
const int buttonPos1 = 10;    // Taster für Gleis 1 (Pin 10)
const int buttonPos2 = 11;    // Taster für Gleis 2 (Pin 11)
const int buttonPos3 = 12;    // Taster für Gleis 3 (Pin 12)

// Definierte Zielpositionen
const long pos1 = 0;
const long pos2 = 50;
const long pos3 = 100;

// Geschwindigkeit
const int stepDelay = 8000; 

// Variablen für Position und Gleis-Status
long currentPosition = 0;             
const char* currentGleis = "Unbekannt"; 

unsigned long lastPrintTime = 0;      
const unsigned long printInterval = 100; 

// --- HILFSFUNKTIONEN ---

void updateGleisName() {
  if (currentPosition == pos1) {
    currentGleis = "Gleis 1";
  } else if (currentPosition == pos2) {
    currentGleis = "Gleis 2";
  } else if (currentPosition == pos3) {
    currentGleis = "Gleis 3";
  } else {
    currentGleis = "Unterwegs..."; 
  }
}

// Funktion für Statusmeldungen auf dem Display (während Setup / Homing)
void showStatusMessage(const char* line1, const char* line2) {
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.setCursor(0, 10);
    u8g2.print("Schrittmotor System");
    u8g2.drawHLine(0, 13, 128);
    
    u8g2.setCursor(0, 30);
    u8g2.print(line1);
    
    if (line2 != nullptr) {
      u8g2.setCursor(0, 50);
      u8g2.print(line2);
    }
  } while (u8g2.nextPage());
}

void updateDisplay() {
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.setCursor(0, 10);
    u8g2.print("Schrittmotor Status");
    u8g2.drawHLine(0, 13, 128);
    
    u8g2.setCursor(0, 30);
    u8g2.print("Status: ");
    u8g2.print(currentGleis);

    u8g2.setCursor(0, 50);
    u8g2.print("Pos: ");
    u8g2.print(currentPosition);
    u8g2.print(" stp");
  } while (u8g2.nextPage());
}

void moveToPosition(long targetPos, const char* gleisName) {
  long stepsToMove = targetPos - currentPosition;
  if (stepsToMove == 0) return;

  Serial.print("Fahre zu ");
  Serial.print(gleisName);
  Serial.print(" (Position: ");
  Serial.print(targetPos);
  Serial.println(")");

  if (stepsToMove > 0) {
    digitalWrite(dirPin, HIGH);
    for (long i = 0; i < stepsToMove; i++) {
      digitalWrite(stepPin, HIGH);
      delayMicroseconds(stepDelay);
      digitalWrite(stepPin, LOW);
      delayMicroseconds(stepDelay);
      currentPosition++;
    }
  } else {
    digitalWrite(dirPin, LOW);
    stepsToMove = -stepsToMove;
    for (long i = 0; i < stepsToMove; i++) {
      digitalWrite(stepPin, HIGH);
      delayMicroseconds(stepDelay);
      digitalWrite(stepPin, LOW);
      delayMicroseconds(stepDelay);
      currentPosition--;
    }
  }
  
  updateGleisName();
  Serial.println("Zielposition erreicht.");
}

// --- SETUP ---



void setup() {
  Serial.begin(115200);
  delay(1000); 

  // I2C und Display starten
  Wire.begin();
  u8g2.begin();
  
  // Pins konfigurieren
  pinMode(buttonForward, INPUT_PULLUP);
  pinMode(buttonBackward, INPUT_PULLUP);
  pinMode(homeButton, INPUT);
  pinMode(buttonPos1, INPUT_PULLUP);
  pinMode(buttonPos2, INPUT_PULLUP);
  pinMode(buttonPos3, INPUT_PULLUP);
  
  pinMode(stepPin, OUTPUT);
  pinMode(dirPin, OUTPUT);
  
  delay(500);

  // NEU: Zeigt jetzt auch auf dem Display an, dass das System startet
  showStatusMessage("System gestartet","");
  delay(1000); // 1 Sekunde anzeigen

  showStatusMessage("Starte Homing...", "Referenzfahrt laeuft");
  delay(500);

  // --- HOMING-ROUTINE (KORRIGIERTE RICHTUNGEN) ---
  Serial.println("Starte Homing (Referenzfahrt)...");
  
  // 1. FREIFAHREN: Wenn der Sensor beim Start aktiv ist, mit HIGH wegfahren
  if (digitalRead(homeButton) == HIGH) {
    Serial.println("Sensor ist aktiv - fahre frei...");
    showStatusMessage("Sensor aktiv", "Fahre frei...");
    
    digitalWrite(dirPin, HIGH); // Richtung WEG vom Sensor
    delay(2); 
    
    int freeSteps = 0;
    while (digitalRead(homeButton) == LOW && freeSteps < 400) {
      digitalWrite(stepPin, HIGH);
      delayMicroseconds(stepDelay);
      digitalWrite(stepPin, LOW);
      delayMicroseconds(stepDelay);
      freeSteps++;
    }
    delay(300);
  }

  // 2. HOMING: Zum Sensor hin mit LOW fahren
  showStatusMessage("Starte Homing...", "Referenzfahrt laeuft");
  digitalWrite(dirPin, LOW); // Richtung ZUM Sensor hin
  delay(2); 

  long maxSteps = 2500; 
  long stepsTaken = 0;

  while (stepsTaken < maxSteps) {
    int sensorStatus = digitalRead(homeButton);
    if (sensorStatus == LOW) {
      Serial.println("Sensor erkannt! Stoppe Homing.");
      showStatusMessage("Sensor erkannt!", "Stoppe Homing...");
      break;
    }

    digitalWrite(stepPin, HIGH);
    delayMicroseconds(stepDelay);
    digitalWrite(stepPin, LOW);
    delayMicroseconds(stepDelay);
    stepsTaken++;
  }

  if (stepsTaken >= maxSteps) {
    Serial.println("FEHLER: Homing-Sensor nicht gefunden!");
    currentGleis = "Homing Fehler";
    showStatusMessage("FEHLER:", "Sensor nicht gefunden");
  } else {
    currentPosition = 0; 
    currentGleis = "Gleis 1";
    Serial.println("Homing erfolgreich abgeschlossen!");
    showStatusMessage("Homing erfolgreich!", "Gleis 1 erreicht");
  }
  
  delay(1500);
}

// --- LOOP ---

void loop() {
  // Nicht blockierende Tasterabfrage
  
  // Taster Vorwärts (Pin 2)
  if (digitalRead(buttonForward) == LOW) {
    delay(15);
    if (digitalRead(buttonForward) == LOW) {
      digitalWrite(dirPin, HIGH);
      digitalWrite(stepPin, HIGH);
      delayMicroseconds(stepDelay);
      digitalWrite(stepPin, LOW);
      delayMicroseconds(stepDelay);
      currentPosition++;
      updateGleisName();
    }
  }

  // Taster Rückwärts (Pin 3)
  if (digitalRead(buttonBackward) == LOW) {
    delay(15);
    if (digitalRead(buttonBackward) == LOW) {
      digitalWrite(dirPin, LOW);
      digitalWrite(stepPin, HIGH);
      delayMicroseconds(stepDelay);
      digitalWrite(stepPin, LOW);
      delayMicroseconds(stepDelay);
      currentPosition--;
      updateGleisName();
    }
  }

  // Taster Gleis 1 (Pin 10)
  if (digitalRead(buttonPos1) == LOW) {
    delay(15);
    if (digitalRead(buttonPos1) == LOW) {
      moveToPosition(pos1, "Gleis 1");
    }
  }

  // Taster Gleis 2 (Pin 11)
  if (digitalRead(buttonPos2) == LOW) {
    delay(15);
    if (digitalRead(buttonPos2) == LOW) {
      moveToPosition(pos2, "Gleis 2");
    }
  }

  // Taster Gleis 3 (Pin 12)
  if (digitalRead(buttonPos3) == LOW) {
    delay(15);
    if (digitalRead(buttonPos3) == LOW) {
      moveToPosition(pos3, "Gleis 3");
    }
  }

  // Display-Aktualisierung im regelmäßigen Intervall
  if (millis() - lastPrintTime >= printInterval) {
    lastPrintTime = millis();
    updateDisplay();
  }
}
