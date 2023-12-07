/**
 * Authors xkralr06 - Rostislav Kral, xjezek19 - Lukas Jezek
*/

#include <simlib.h>
#include <getopt.h>



class Meat: Process {



};

class Generator: Event {
  


};


Queue  ButcherQueue("Rada na reznika");
Facility Butcher("Reznik");

Queue CutterQueue("Fronta na kutr");
Facility Cutter("Kutr");

Queue SmokeHouseQueue("Fronty na udirnu");
Facility SmokeHouse("Udirna");

Queue SausageFillerQueue("Fronta na narazecku");
Facility SausageFiller("Narazecka");

Store MeatAgingFridge("Lednice pro zrani", 3500);

Store MeatIntakeFridge("Lednice pro prijem masa", 5000);

Store ProductFridge("Lednice pro hotove produkty", 5000);

int main() {  

  Init(0, 10000);
  Run();

  ButcherQueue.Output();

  return 0;
}
