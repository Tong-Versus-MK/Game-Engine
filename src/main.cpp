#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>

#define BUTTON 19
//Detail Character
struct Player{
  char characterName[5];
  int attack,HP;
  int xPosition,yPosition; //Current Position in Axis
};

Player tong = {"Tong", 10, 500,0,0};
Player mk = {"MK", 10, 500,0,0};

int gameMap[6][6] = {
  {0,0,0,0,0,0},
  {0,0,0,0,0,0},
  {0,0,0,0,0,0},
  {0,0,0,0,0,0},
  {0,0,0,0,0,0},
  {0,0,0,0,0,0},
};

int turn = 0;   //tell that who attack this turn -> 0=tong, 1=MK
typedef struct struct_message {
  int player;
  int x;
  int y;
  int wall_hit;
  int move_count;
} struct_message;

struct_message income_mess;
int player_id;
int pos_x;
int pos_y;
int wall_hit;
int move_count;

volatile int waiting = 1;

String success;
esp_now_peer_info_t peerInfo;

uint8_t broadcastAddress[] = { 0x24, 0x6F, 0x28, 0x50, 0xA6, 0x78 };

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
  turn = 0;

  (P1 -> attack) = 10;
  (P1 -> HP) = 500;
  (P1 -> xPosition) = 0;
  (P1 -> yPosition) = 0;

  (P2 -> attack) = 10;
  (P2 -> HP) = 500;
  (P2 -> xPosition) = 2;
  (P2 -> yPosition) = 3;

}

void getItem(int item,Player *P){
  
  // Serial.println(P -> characterName);
  if(item == 2){
    (P -> attack) += 10;
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
  //variable for test isNear
  // tong.xPosition = 2;
  // tong.yPosition = 5;
  // mk.xPosition = 2;
  // mk.yPosition = 0;
  initGame(&tong, &mk);
  WiFi.mode(WIFI_MODE_STA);
  Serial.println(WiFi.macAddress());

  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
  esp_now_register_recv_cb(OnDataRecv);
}

int tile_id;
int isMainPhase = 1;


void loop() {

  Player *player[] = {&tong, &mk};

  if (isMainPhase) {  
    /* Main Phase */
    waiting = 1;
    while (waiting) ;
    /* Update Position */  
    player[turn] -> xPosition = pos_x;
    player[turn] -> yPosition = pos_y;
    /* Wall Collision Damage */
    if (wall_hit) attacked(player[turn], wall_hit);

    /* Check For Item */
    tile_id = gameMap[pos_y][pos_x];
    getItem(tile_id, player[turn]);

    /* Check for Duel Phase */
    if (isNearPlayer(player[turn], player[!turn])) {
      isMainPhase = 0;
    }

    if (move_count == 0 && isMainPhase) turn = !turn;
  } 
  else {
    /* Duel Mode */
    Serial.printf("In Duel Session! Player %d's Turn\n", turn);
    waiting = 1;
    while (waiting);
    Serial.printf("Player %d hit\n", turn);
    delay(1000);
  }
  // if (isMainPhase && turn == 0) tong.yPosition--;
  
  /* Wait For Player's Turn to End*/

}
