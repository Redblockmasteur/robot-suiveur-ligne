// Robot Suiveur de Ligne - Arduino Mega
// Basé sur l'algorithme algo.md et la nomenclature composants.pdf

// propriete du robot (en m)
#define ROBOT_L 0.2   // entraxe des roues
#define ROBOT_r 0.07  // diametre d'une roue
#define PPR 11        // ticks de l'encodeur (je sort cette valeur parce que j'ai pas mieux et c generalement celle la)

// Configuration des broches
const int LEFT_SENSOR_PIN = 50;
const int RIGHT_SENSOR_PIN = 52;
const int LEFT_MOTOR_PIN = 10;
const int RIGHT_MOTOR_PIN = 11;
const int LEFT_ENCODER_PIN = 30; // a verifier donc prions
const int RIGHT_ENCODER_PIN = 31;
const int START_BUTTON_PIN = 2;  // À confirmer
const int OBSTACLE_SENSOR_PIN = A0;  // Capteur IR obstacle

// Variables d'état
bool started = false;
bool obstacleDetected = false;

// Variables associées aux encodeurs et aux interruptions
#define TIMEOUT 100000 // si valeur superieur, roues a l'arret (0.1s)
volatile unsigned long dt_L = 0;
volatile unsigned long lastTimeL = 0;
volatile int sens_l = 1;

volatile unsigned long dt_R = 0;
volatile unsigned long lastTimeR = 0;
volatile int sens_R = 1;

struct q {
  float v;
  float omega;
}; // Format que renvoie la lecture des encodeurs

void setup() {
  // Initialisation des broches
  pinMode(LEFT_SENSOR_PIN, INPUT);
  pinMode(RIGHT_SENSOR_PIN, INPUT);
  pinMode(START_BUTTON_PIN, INPUT_PULLUP);
  pinMode(OBSTACLE_SENSOR_PIN, INPUT);
  pinMode(LEFT_MOTOR_PIN, OUTPUT);
  pinMode(RIGHT_MOTOR_PIN, OUTPUT);

  // Arrêt initial des moteurs
  digitalWrite(LEFT_MOTOR_PIN, LOW);
  digitalWrite(RIGHT_MOTOR_PIN, LOW);

  // setup des interruptions pour l'encodeur
  attachInterrupt(digitalPinToInterrupt(LEFT_ENCODER_PIN), handleLeftEncoder, RISING);
  attachInterrupt(digitalPinToInterrupt(RIGHT_ENCODER_PIN), handleRightEncoder, RISING);

  Serial.begin(9600);
  Serial.println("Robot Suiveur de Ligne - Initialisation");
}

void loop() {


  // Vérification du bouton start
  if (!started && digitalRead(START_BUTTON_PIN) == LOW) {
    started = true;
    Serial.println("Début du suivi de ligne...");
    delay(500);  // Anti-rebond
  }

  if (!started) {
    return;  // Attendre le bouton start
  }

  // Détection d'obstacle
  obstacleDetected = (digitalRead(OBSTACLE_SENSOR_PIN) == LOW);

  if (obstacleDetected) {
    stopMotors();
    Serial.println("Obstacle détecté - Arrêt");
    delay(100);
    return;
  }

  // Lecture des capteurs de ligne
  int leftSensor = digitalRead(LEFT_SENSOR_PIN);
  int rightSensor = digitalRead(RIGHT_SENSOR_PIN);

  Serial.print("Capteurs: Gauche=");
  Serial.print(leftSensor);
  Serial.print(" Droit=");
  Serial.println(rightSensor);

  // Logique de suivi de ligne
  if (leftSensor == LOW && rightSensor == LOW) {
    // Les deux capteurs sur la ligne noire - Arrêt
    stopMotors();
    Serial.println("Arrêt - Les deux capteurs sur la ligne");
  } else if (leftSensor == LOW) {
    // Capteur gauche sur la ligne - Tourner à gauche
    turnLeft();
    Serial.println("Tourner à gauche");
  } else if (rightSensor == LOW) {
    // Capteur droit sur la ligne - Tourner à droite
    turnRight();
    Serial.println("Tourner à droite");
  } else {
    // Les deux capteurs sur le blanc - Avancer tout droit
    moveForward();
    Serial.println("Avancer tout droit");
  }

  delay(50);  // Petit délai pour stabilité
}

// Fonctions de contrôle des moteurs
void moveForward() {
  analogWrite(LEFT_MOTOR_PIN, 200);  // PWM pour vitesse
  analogWrite(RIGHT_MOTOR_PIN, 200);
}

void turnLeft() {
  analogWrite(LEFT_MOTOR_PIN, 100);  // Ralentir le moteur gauche
  analogWrite(RIGHT_MOTOR_PIN, 200);
}

void turnRight() {
  analogWrite(LEFT_MOTOR_PIN, 200);
  analogWrite(RIGHT_MOTOR_PIN, 100);  // Ralentir le moteur droit
}

void stopMotors() {
  digitalWrite(LEFT_MOTOR_PIN, LOW);
  digitalWrite(RIGHT_MOTOR_PIN, LOW);
}


// Envoie de consignes de vitesses au robot
void moveRobot(float Wl, float Wr) {
  // A CHANGER
  digitalWrite(LEFT_MOTOR_PIN, LOW);
  digitalWrite(RIGHT_MOTOR_PIN, LOW);
}


// Convertir des donnees de la forme v, omega en vitesses de moteur droit/gauche
void commandToSpeed(float v, float omega) {
  float w_L = (2 * v - ROBOT_L * omega) / (2 * ROBOT_r);
  float w_R = (2 * v + ROBOT_L * omega) / (2 * ROBOT_r);
  moveRobot(w_L, w_R);
}

// Conversion inverse -> vitesse des roues into v, omega (a noter qu'on a pas le sens du robot genre avant/arriere, a deduire de la commande)
q GetQFromWheel() {
  // Calcul de la vitesse des deux roues
  float vL, vR;
  if (dt_L > TIMEOUT) {
    vL = 0.0;
  } else {
    vL = (2.0 * PI * ROBOT_r * 1000000.0) / (PPR * (float)dt_L);
  }

  if (dt_R > TIMEOUT) {
    vR = 0.0;
  } else {
    vR = (2.0 * PI * ROBOT_r * 1000000.0) / (PPR * (float)dt_R);
  }

  // Calcul de la vitesse lineaire et angulair globale du robot
  float Vrobot = (vL + vR) / 2;
  float Omegarobot = (vR - vL) / ROBOT_L;

  return q{Vrobot, Omegarobot};
}



// fonctions qui actualisent a chaque interruptions le delai entre chaques ticks d'encodeurs (pour déduire la vitesse des roues)
void handleLeftEncoder() {
  unsigned long current_time = micros();
  dt_L = current_time - lastTimeL; // Temps écoulé depuis le dernier tick
  lastTimeL = current_time;
}

void handleRightEncoder() {
  unsigned long current_time = micros();
  dt_R = current_time - lastTimeR; // Temps écoulé depuis le dernier tick
  lastTimeR = current_time;
}
