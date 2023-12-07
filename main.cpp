/**
 * Authors xkralr06 - Rostislav Kral, xjezek19 - Lukas Jezek
*/

#include <simlib.h>
#include "main.h"
#include <getopt.h>

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

class Meat:public Process {

  void Behavior()
  {
    Seize(Butcher);
    Wait(2.7);
    Release(Butcher);
  }

};

class Generator:public Event{

 /*Generator::Generator(){
   // Activate();
  }*/

  void Behavior()
  {
    auto meat = new Meat();

  
    meat->Activate(Exponential(11));
  }

};



int main(int argc, char *argv[]) {
    int c;
    ProgramOptions input;

    // Zpracování vstupních přepínačů a argumentů při spuštění
    while ((c = getopt(argc, argv, "hrx6s:p:")) != -1) {
        switch (c) {
            default:
                abort();
        }
    }
  Init(0, 100000);
  (new Generator)->Activate();
  Run();

  ButcherQueue.Output();
  Butcher.Output();

  return 0;
}
