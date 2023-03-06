#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>

#define BUTTON 19
//Detail Character
struct Player{
  char characterName[5];
  int ATK_plus,HP;
  int xPosition,yPosition; //Current Position in Axis
};

Player tong = {"Tong", 10, 500,0,0};
Player mk = {"MK", 10, 500,0,0};
Player *player[] = {&tong, &mk};

int gameMap[6][6] = {
  {0,1,0,0,2,0},
  {0,1,3,1,1,0},
  {0,0,0,1,1,0},
  {1,1,0,0,0,0},
  {0,0,0,0,1,0},
  {2,0,1,2,1,0}
};

int backupMap[6][6] = {
  {0,1,0,0,2,0},
  {0,1,3,1,1,0},
  {0,0,0,1,1,0},
  {1,1,0,0,0,0},
  {0,0,0,0,1,0},
  {2,0,1,2,1,0}
};

typedef struct struct_message {
  int player;
  int x;
  int y;
  int wall_hit;
  int move_count;
} struct_message;

typedef struct control_t {
  int turn; // 0 : Tong, 1 : MK
  int mode; // 0 : walk, 1 : Duel, 2: In-Reset
} control_t;

struct_message income_mess;
int player_id;
int pos_x;
int pos_y;
int wall_hit;
int move_count;

volatile int waiting = 1;

String success;
esp_now_peer_info_t peerInfo;

uint8_t broadcastAddress1[] = { 0x24, 0x6F, 0x28, 0x50, 0xA6, 0x78 };
uint8_t broadcastAddress2[] = { 0x24, 0x6F, 0x28, 0x28, 0x15, 0x94};

control_t control_msg;
control_t * control = &control_msg;

void OnDataRecv(const uint8_t* mac, const uint8_t* incomingData, int len) {
  memcpy(&income_mess, incomingData, sizeof(income_mess));
  Serial.print("Bytes received: ");
  Serial.println(len);
  player_id = income_mess.player;
  pos_x = income_mess.x;
  pos_y = income_mess.y;
  wall_hit = income_mess.wall_hit;
  move_count = income_mess.move_count;
  waiting = !waiting;
  Serial.print("Player: ");
  Serial.println(player_id);
  Serial.print("Pos_x: ");
  Serial.println(pos_x);
  Serial.print("Pos_y: ");
  Serial.println(pos_y);
  Serial.print("Wall_Hit: ");
  Serial.println(wall_hit);
  Serial.print("Move_count: ");
  Serial.println(move_count);
}

void initGame(Player *P1, Player *P2){
  
  control_msg.turn = 0;
  control_msg.mode = 0;

  (P1 -> ATK_plus) = 0;
  (P1 -> HP) = 30;
  (P1 -> xPosition) = 0;
  (P1 -> yPosition) = 0;

  (P2 -> ATK_plus) = 0;
  (P2 -> HP) = 30;
  (P2 -> xPosition) = 5;
  (P2 -> yPosition) = 5;

  for (int y = 0; y < 6; y++) {
    for (int x = 0; x < 6; x++) {
      gameMap[y][x] = backupMap[y][x];
    }
  }

  Serial.println("=-=-=-=-= Game Start! =-=-=-=-=");

}

void getItem(int x, int y, Player *P){
  // Serial.println(P -> characterName);
  
  /* +1 ATK+ */
  if (gameMap[y][x] == 2) {
    (P -> ATK_plus) += 1;
    gameMap[y][x] = 0;
    Serial.printf("%s Recieved +1 ATK+\n", P->characterName);
    Serial.println("=========================");
    Serial.printf("| Tong : %d | MK : %d |\n", player[0]->ATK_plus, player[1]->ATK_plus);
    Serial.println("=========================");
  }

  /* +5 HP  */
  if (gameMap[y][x] == 3) {
    (P -> HP) += 5;
    gameMap[y][x] = 0;
    Serial.printf("%s Recieved +5 HP\n", P->characterName);
    Serial.println("=========================");
    Serial.printf("| Tong : %d | MK : %d |\n", player[0]->HP, player[1]->HP);
    Serial.println("=========================");
  }
}

void attacked(Player *P, int dmg){
  (P -> HP) -= dmg;
}

int isNearPlayer(Player *P1, Player *P2) {
  //check top&bottom
  if(((P1->xPosition) == (P2->xPosition)) && (abs((P1->yPosition)-(P2->yPosition)) == 1)){
    // Serial.println("enemy near at tb");
    return 1;
  }
  //check left&right
  if(((P1->yPosition) == (P2->yPosition)) && (abs((P1->xPosition)-(P2->xPosition)) == 1)){
    // Serial.println("enemy near at lr");
    return 1;
  }
  return 0;
}

void setup() {
  Serial.begin(115200);

  initGame(&tong, &mk);
  WiFi.mode(WIFI_MODE_STA);
  Serial.println(WiFi.macAddress());

  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  memcpy(peerInfo.peer_addr, broadcastAddress1, 6);
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

  memcpy(peerInfo.peer_addr, broadcastAddress2, 6);
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }

  esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
  
  if (control->mode == 0) {  
    /* Main Phase */
    waiting = 1;
    while (waiting) ;
    
    /* Update Position */  
    player[control->turn] -> xPosition = pos_x;
    player[control->turn] -> yPosition = pos_y;

    /* Wall Collision Damage */
    if (wall_hit) {
      Serial.printf("WALL HIT!!\n", wall_hit);
      attacked(player[control->turn], wall_hit*5);
      Serial.printf("Player : %d , HP : %d\n", control->turn, player[control->turn]->HP);
    };
    /* Check Game Over Caused by wall hits */
    if (player[control->turn]->HP <= 0) {
      control->mode = 2;
      control->turn = !control->turn;
    } 
    else {
      /* Check For Item & Take if there is an item */
      getItem(pos_y, pos_x, player[control->turn]);

      /* Check for Duel Phase */
      if (isNearPlayer(player[control->turn], player[!control->turn])) {
        control->mode = 1;
      }

      /* Change Turn */
      if (move_count == 0 && control->mode == 0) {
        control->turn = !control->turn;
        esp_err_t result = esp_now_send(0, (uint8_t *) &control_msg, sizeof(control_t));
        // Serial.println("Duel Session BEGIN!!!");
      }
    }
  } 
  else if (control->mode == 1) {
    /* Duel Mode */
    Serial.printf("In Duel Session! Player %d's Turn\n", control->turn);
    /* Broadcast Turn to All Controllers */
    esp_err_t result = esp_now_send(0, (uint8_t *) &control_msg, sizeof(control_t));
    
    /* Show Both HP Status */
    Serial.println("=========================");
    Serial.printf("| Tong : %d | MK : %d |\n", player[0]->HP, player[1]->HP);
    Serial.println("=========================");

    waiting = 1;
    while (waiting);
    
    attacked(player[!control->turn], player[control->turn]->ATK_plus + move_count);
    
    Serial.printf("Player %d hit with %d DMG\n", 
      control->turn, 
      player[control->turn]->ATK_plus + move_count);

    /* Check Game Over or Continue*/
    if (player[!control->turn]->HP <= 0) {
      control->mode = 2;
    } else {
      control->turn = !control->turn;
    }
  } else {
    /* Reset Mode (No Reset Yet) */
    esp_err_t result = esp_now_send(0, (uint8_t *) &control_msg, sizeof(control_t));
    
    Serial.println("o=o=o=o GAME OVER! o=o=o=o");
    Serial.printf("        Player %d WIN!\n", control->turn);
    Serial.println("o=o=o=o=o=o=o=o=o=o=o=o=o");

    /* Reset */
    initGame(&tong, &mk);

  }
  // if (isMainPhase && turn == 0) tong.yPosition--; 
}
