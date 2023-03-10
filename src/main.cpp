#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <Bounce2.h>

#define BUTTON 19
//Detail Character
Bounce debouncer_btn = Bounce();

struct Player {
  int pid;
  int ATK_plus, HP;
  int xPosition, yPosition; //Current Position in Axis
};

Player tong = { 0, 10, 500,0,0 };
Player mk = { 1, 10, 500,0,0 };
Player* player[] = { &tong, &mk };


/* 0 : Space, 1 : Wall, 2 : +1 ATK, 3 : +5 HP */
int gameMap[8][8];

double damageModifier[] = { 0,0.5,0.75,1,1.25,1.5,2 };

/* 0 : Space, 1 : Wall, 2 : Unknown Item */
int backupMap[8][8] = {
  {0,1,2,0,0,0,0,2},
  {0,1,1,1,0,0,1,0},
  {0,0,0,1,0,0,1,1},
  {1,1,0,2,2,0,0,0},
  {0,0,0,2,2,0,0,0},
  {0,0,1,0,0,1,0,0},
  {0,1,1,1,0,1,1,0},
  {2,0,0,2,0,0,1,0}
};

/* [RECV] Message Received from Controllers */
typedef struct struct_message {
  int player;
  int x;
  int y;
  int wall_hit;
  int move_count;
} struct_message;

/* [SEND] Store Data that need to send to other peers */
typedef struct control_t {
  /* --- Global Control --- */
  int turn; // 0 : Tong, 1 : MK
  int mode; // 0 : walk, 1 : Duel, 2: In-Reset
  /* ---- OLED Control ---- */
  int stat_owner;
  int stat_hp;
  int stat_atk;
  /* ---------------------- */
} control_t;


typedef struct display_mess {
  int mode;
  int turn;
  int x;
  int y;
  int board[8][8];
} display_mess;

display_mess dmess;

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
uint8_t broadcastAddress2[] = { 0x24, 0x6F, 0x28, 0x28, 0x15, 0x94 };
uint8_t displayAddress[] = { 0xA4, 0xCF, 0x12, 0x8f, 0xD1, 0x30 };

control_t control_msg;
control_t* control = &control_msg;

void SendToDisplay() {
  dmess.mode = control->mode;
  dmess.turn = control->turn;
  dmess.x = player[control->turn]->xPosition;
  dmess.y = player[control->turn]->yPosition;
  esp_err_t result = esp_now_send(displayAddress, (uint8_t*)&dmess, sizeof(display_mess));
}

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

void initGame(Player* P1, Player* P2) {

  control->turn = 0;
  control->mode = 0;
  control->stat_atk = 4;
  control->stat_hp = 30;
  control->stat_owner = 2; /* Broadcast ID */

  esp_err_t result1 = esp_now_send(broadcastAddress1, (uint8_t*)&control_msg, sizeof(control_t));
  esp_err_t result1 = esp_now_send(broadcastAddress2, (uint8_t*)&control_msg, sizeof(control_t));
  // control->stat_owner = 1;

  (P1->ATK_plus) = 4;
  (P1->HP) = 30;
  (P1->xPosition) = 0;
  (P1->yPosition) = 0;

  (P2->ATK_plus) = 4;
  (P2->HP) = 30;
  (P2->xPosition) = 7;
  (P2->yPosition) = 7;

  /* Copy Backup Data */
  for (int y = 0; y < 8; y++) {
    for (int x = 0; x < 8; x++) {
      if (backupMap[y][x] == 2) {
        gameMap[y][x] = esp_random() % 2 + 2;
      }
      else {
        gameMap[y][x] = backupMap[y][x];
      }
      dmess.board[y][x] = gameMap[y][x];
    }
  }

  dmess.turn = 0;
  dmess.mode = 0;
  dmess.x = 0;
  dmess.y = 0;
  esp_err_t result = esp_now_send(displayAddress, (uint8_t*)&dmess, sizeof(display_mess));


  Serial.println("=-=-=-=-= Game Start! =-=-=-=-=");

}

void getItem(int x, int y, Player* P) {
  // Serial.println(P -> characterName);

  /* +1 ATK+ */
  if (gameMap[y][x] == 2) {
    (P->ATK_plus) += 1;
    gameMap[y][x] = 0;
    control->stat_owner = P->pid;
    control->stat_hp = P->HP;
    control->stat_atk = P->ATK_plus;
    // Serial.printf("%s Recieved +1 ATK+\n", P->characterName);
    Serial.println("=========================");
    Serial.printf("| Tong : %d | MK : %d |\n", player[0]->ATK_plus, player[1]->ATK_plus);
    Serial.println("=========================");
    SendToDisplay();
  }

  /* +5 HP  */
  if (gameMap[y][x] == 3) {
    (P->HP) += 5;
    gameMap[y][x] = 0;
    control->stat_owner = P->pid;
    control->stat_hp = P->HP;
    control->stat_atk = P->ATK_plus;
    // Serial.printf("%s Recieved +5 HP\n", P->characterName);
    Serial.println("=========================");
    Serial.printf("| Tong : %d | MK : %d |\n", player[0]->HP, player[1]->HP);
    Serial.println("=========================");
    SendToDisplay();
  }

  esp_err_t result = esp_now_send(broadcastAddress1, (uint8_t*)&control_msg, sizeof(control_t));
  esp_err_t result = esp_now_send(broadcastAddress2, (uint8_t*)&control_msg, sizeof(control_t));
}

void attacked(Player* P, int dmg) {
  (P->HP) -= dmg;
  /* Update HP Stat For OLED Control */
  control->stat_owner = P->pid;
  control->stat_hp = P->HP;
  control->stat_atk = P->ATK_plus;
  esp_err_t result = esp_now_send(broadcastAddress1, (uint8_t*)&control_msg, sizeof(control_t));
  esp_err_t result = esp_now_send(broadcastAddress2, (uint8_t*)&control_msg, sizeof(control_t));
}

int isNearPlayer(Player* P1, Player* P2) {
  //check top&bottom
  if (((P1->xPosition) == (P2->xPosition)) && (abs((P1->yPosition) - (P2->yPosition)) == 1)) {
    // Serial.println("enemy near at tb");
    return 1;
  }
  //check left&right
  if (((P1->yPosition) == (P2->yPosition)) && (abs((P1->xPosition) - (P2->xPosition)) == 1)) {
    // Serial.println("enemy near at lr");
    return 1;
  }
  return 0;
}

void setup() {
  Serial.begin(115200);

  debouncer_btn.attach(BUTTON, INPUT_PULLUP);
  debouncer_btn.interval(25);

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
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

  initGame(&tong, &mk);

  esp_now_register_recv_cb(OnDataRecv);
}

void loop() {

  if (control->mode == 0) {
    /* Main Phase */
    waiting = 1;
    while (waiting);

    /* Update Position */
    player[control->turn]->xPosition = pos_x;
    player[control->turn]->yPosition = pos_y;

    /* Wall Collision Damage */
    if (wall_hit) {
      // Serial.printf("WALL HIT!!\n", wall_hit);
      attacked(player[control->turn], wall_hit * 5);
      // Serial.printf("Player : %d , HP : %d\n", control->turn, player[control->turn]->HP);

    }
    /* Check Game Over Caused by wall hits */
    if (player[control->turn]->HP <= 0) {
      control->mode = 2;
      control->turn = !control->turn;
      //SendToDisplay();
    }
    else {
      /* Check For Item & Take if there is an item */
      getItem(pos_x, pos_y, player[control->turn]);

      /* Check for Duel Phase */
      if (isNearPlayer(player[control->turn], player[!control->turn])) {
        control->mode = 1;
        SendToDisplay();
      }

      /* Change Turn */
      if (move_count == 0 && control->mode == 0) {
        control->turn = !control->turn;
        esp_err_t result = esp_now_send(broadcastAddress1, (uint8_t*)&control_msg, sizeof(control_t));
        esp_err_t result = esp_now_send(broadcastAddress2, (uint8_t*)&control_msg, sizeof(control_t));
        SendToDisplay();
        // Serial.println("Duel Session BEGIN!!!");
      }
    }
  }
  else if (control->mode == 1) {
    /* Duel Mode */
    Serial.printf("In Duel Session! Player %d's Turn\n", control->turn);
    /* Broadcast Turn to All Controllers */
    esp_err_t result = esp_now_send(broadcastAddress1, (uint8_t*)&control_msg, sizeof(control_t));
    esp_err_t result = esp_now_send(broadcastAddress2, (uint8_t*)&control_msg, sizeof(control_t));

    /* Show Both HP Status */
    Serial.println("=========================");
    Serial.printf("| Tong : %d | MK : %d |\n", player[0]->HP, player[1]->HP);
    Serial.println("=========================");

    waiting = 1;
    while (waiting);

    attacked(player[!control->turn], ceil(player[control->turn]->ATK_plus * damageModifier[move_count]));

    Serial.printf("Player %d hit with %d DMG\n",
      control->turn,
      player[control->turn]->ATK_plus + move_count);

    /* Check Game Over or Continue*/
    if (player[!control->turn]->HP <= 0) {
      control->mode = 2;
    }
    else {
      control->turn = !control->turn;
    }
    SendToDisplay();
  }
  else {
    /* Reset Mode (No Reset Yet) */
    // debouncer_btn.update(); 

    esp_err_t result = esp_now_send(broadcastAddress1, (uint8_t*)&control_msg, sizeof(control_t));
    esp_err_t result = esp_now_send(broadcastAddress2, (uint8_t*)&control_msg, sizeof(control_t));
    Serial.println("o=o=o=o GAME OVER! o=o=o=o");
    Serial.printf("        Player %d WIN!\n", control->turn);
    Serial.println("o=o=o=o=o=o=o=o=o=o=o=o=o");

    delay(5000);
    /* Reset */
    // while (!debouncer_btn.fell());
    initGame(&tong, &mk);

  }
  //SendToDisplay();
  // if (isMainPhase && turn == 0) tong.yPosition--; 
}
