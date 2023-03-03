#include <Arduino.h>


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

int turnAttack;   //tell that who attack this turn -> 0=tong, 1=MK

void initGame(Player *P1, Player *P2){
  turnAttack = 0;

  (P1 -> attack) = 10;
  (P1 -> HP) = 500;
  (P1 -> xPosition) = 0;
  (P1 -> yPosition) = 0;

  (P2 -> attack) = 10;
  (P2 -> HP) = 500;
  (P2 -> xPosition) = 0;
  (P2 -> yPosition) = 0;

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
    Serial.println("enemy near at tb");
    return 1;
  }
  //check left&right
  if(((P1->yPosition) == (P2->yPosition)) && (abs((P1->xPosition)-(P2->xPosition)) == 1)){
    Serial.println("enemy near at lr");
    return 1;
  }
  return 0;
}

void setup() {
  Serial.begin(115200);

  //variable for test isNear
  // tong.xPosition = 2;
  // tong.yPosition = 3;
  // mk.xPosition = 2;
  // mk.yPosition = 0;
}

int count = 0;

void loop() {
  // Serial.print("tong atk : ");
  // Serial.print(tong.attack);
  // Serial.print("     tong HP : ");
  // Serial.print(tong.HP);
  // Serial.println(" ");


  /*
  //test getItem
  getItem(2,&tong);

  //test InitGame
  count++;
  if(count == 10){
    initGame(&tong,&mk);
    count = 0;
  }

  //test is attacked
  if(count % 2 == 0){
    attacked(&tong,20);
  }
  */

  /*
  // test isNear
  mk.yPosition++;
  Serial.print("tong x: ");
  Serial.print(tong.xPosition);
  Serial.print("    y: ");
  Serial.print(tong.yPosition);
  Serial.println("");
  Serial.print("mk x: ");
  Serial.print(mk.xPosition);
  Serial.print("    y: ");
  Serial.print(mk.yPosition);
  Serial.println("");
  int b = isNearPlayer(&tong,&mk);
  */

  delay(1000);

}
