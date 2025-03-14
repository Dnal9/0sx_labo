#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define LCD_ADDR 0x27  // Adresse I2C de l'écran LCD1602
#define BTN_JOYSTICK 2 // Broche du bouton du joystick
#define LED 8          // LED pour la gestion des phares
#define CAPTEUR A0     // Broche du capteur analogique (LDR)
#define JOY_X A1       // Axe X du joystick
#define JOY_Y A2       // Axe Y du joystick

LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);

int numEtudiant = 2413335; // Numéro d'étudiant
bool pharesOn = false; // État des phares
int page = 0; 
unsigned long lastScreenToggle = 0;
unsigned long lastCapteurCheck = 0;
unsigned long lastSerialSend = 0;
bool boutonEtatPrecedent = HIGH;
unsigned long timerStart = 0;
bool timerEnCours = false;
unsigned long affichageNomDebut = 0;
bool afficherNom = true;


byte customChar3[8] = {
  B01110,
  B00010,
  B00010,
  B01110,
  B00010,
  B00010,
  B00010,
  B01110
};

byte customChar5[8] = {
  B11110,
  B10000,
  B10000,
  B11110,
  B00010,
  B00010,
  B11110,
  B00000
};

void setup() {
  Serial.begin(115200);
  lcd.init();
  lcd.backlight();
  pinMode(LED, OUTPUT);
  pinMode(BTN_JOYSTICK, INPUT_PULLUP);

  // Création des caractères spéciaux
  lcd.createChar(0, customChar3);
  lcd.createChar(1, customChar5);

  // Afficher le nom et DA (méthode non bloquante)
  afficherNomEtDA();
}

void loop() {
  unsigned long currentMillis = millis();

  // Affichage du nom pendant 3 secondes sans bloquer
  if (afficherNom) {
    if (currentMillis - affichageNomDebut >= 3000) {
      afficherNom = false;
      lcd.clear();
    }
    return; // Évite d'exécuter le reste du loop pendant cet affichage
  }

  // Lecture du capteur toutes les 100ms
  if (currentMillis - lastCapteurCheck >= 100) {
    lastCapteurCheck = currentMillis;
    lireCapteur();
  }

  // Vérification du bouton pour changer d’écran (anti-rebond)
  bool boutonPresse = (digitalRead(BTN_JOYSTICK) == LOW);
  if (boutonPresse && !boutonEtatPrecedent && currentMillis - lastScreenToggle >= 300) {
    lastScreenToggle = currentMillis;
    page = (page + 1) % 3;
    lcd.clear();
  }
  boutonEtatPrecedent = boutonPresse;

  // Affichage LCD
  afficherLCD();

  // Transmission série toutes les 100ms
  if (currentMillis - lastSerialSend >= 100) {
    lastSerialSend = currentMillis;
    envoyerTrameSerie();
  }
}

void afficherNomEtDA() {
  lcd.setCursor(0, 0);
  lcd.print("ONDO");
  lcd.setCursor(0, 1);
  lcd.print("DA: ");
  lcd.write(byte(0));  // Affichage du "3" personnalisé
  lcd.write(byte(1));  // Affichage du "5" personnalisé
  affichageNomDebut = millis();
}

void lireCapteur() {
  int valeur = analogRead(CAPTEUR);
  int luminosite = map(valeur, 0, 1023, 0, 100);

  if (!pharesOn && luminosite < 50) {
    if (!timerEnCours) {
      timerStart = millis();
      timerEnCours = true;
    }
    if (millis() - timerStart >= 5000) {
      pharesOn = true;
      digitalWrite(LED, HIGH);
      timerEnCours = false;
    }
  } else if (pharesOn && luminosite > 50) {
    if (!timerEnCours) {
      timerStart = millis();
      timerEnCours = true;
    }
    if (millis() - timerStart >= 5000) {
      pharesOn = false;
      digitalWrite(LED, LOW);
      timerEnCours = false;
    }
  } else {
    timerEnCours = false;
  }
}

void afficherLCD() {
  lcd.setCursor(0, 0);
  if (page == 0) {
    lcd.print("ONDO");
    lcd.setCursor(0, 1);
    lcd.print("DA: 35");
  } else if (page == 1) {
    int luminosite = map(analogRead(CAPTEUR), 0, 1023, 0, 100);
    lcd.print("Lumiere: ");
    lcd.print(luminosite);
    lcd.print("%");
    lcd.setCursor(0, 1);
    lcd.print(pharesOn ? "Phare : ON " : "Phare : OFF");
  } else {
    int joyX = analogRead(JOY_X);
    int joyY = analogRead(JOY_Y);
    int vitesse = map(joyY, 0, 1023, -120, 120);
    
    // Mise à 0 si joystick au repos
    if (joyY > 490 && joyY < 530) {
      vitesse = 0;
    }
    
    char direction = '-';
    if (joyX < 480) direction = 'G';
    else if (joyX > 544) direction = 'D';

    lcd.print(vitesse >= 0 ? "Avance: " : "Recule: ");
    lcd.print(abs(vitesse));
    lcd.print("km/h");
    lcd.setCursor(0, 1);
    lcd.print("Dir: ");
    lcd.print(direction);
  }
}

void envoyerTrameSerie() {
  int joyX = analogRead(JOY_X);
  int joyY = analogRead(JOY_Y);
  Serial.print("etd:2413335;x:");
  Serial.print(joyX);
  Serial.print(";y:");
  Serial.print(joyY);
  Serial.print(";sys:");
  Serial.println(pharesOn ? 1 : 0);
}
