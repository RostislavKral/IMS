/**
 * Authors xkralr06 - Rostislav Kral, xjezek19 - Lukas Jezek
*/

#include <simlib.h>
#include "main.h"
#include <getopt.h>

Stat dobaObsluhy("Doba obsluhy na lince");
Stat dobaObsluhy2("Doba obsluhy na lince2");
Histogram dobaVSystemu("Celkova doba v systemu", 0, 40, 2);

Queue  ButcherQueue("Rada na reznika");
Facility Butcher("Reznik");

Queue CutterQueue("Fronta na kutr");
Facility Cutter("Kutr");

Queue SmokeHouseQueue("Fronty na udirnu");
Facility SmokeHouse("Udirna");

Queue SausageFillerQueue("Fronta na narazecku");
Facility SausageFiller("Narazecka");

Store MeatIntakeFridge("Lednice pro prijem masa", 5000);

Store MeatAgingFridge("Lednice pro zrani", 3500);

Store ProductFridge("Lednice pro hotove produkty", 5000);

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


MealStacking::MealStacking(unsigned int intake) {
    Intake = intake;
    Activate();
}

void MealStacking::Behavior() {
    double tvstup = Time;
    double obsluha;

    test:
    if (MeatIntakeFridge.Empty()) {
        // Todo wait

    }
    Enter(MeatIntakeFridge, Intake);

    if (Butcher.Busy()) {
        Into(ButcherQueue);
        Passivate();
        goto test;
    }
    Seize(Butcher);
    Wait(10);

    dobaObsluhy2(obsluha);

    Release(Butcher);
    if (ButcherQueue.Length() > 0) {
        ButcherQueue.GetFirst()->Activate();
    }

    ((new MeatPreparation(Intake))->Activate());

    dobaVSystemu(Time - tvstup);
}

MeatPreparation::MeatPreparation(unsigned int load) {
    Load = load;
    Activate();
}

void MeatPreparation::Behavior() {

}

int main(int argc, char *argv[]) {
    int c;
    ProgramOptions input;

    // Zpracování vstupních přepínačů a argumentů při spuštění
    while ((c = getopt(argc, argv, "b:c:t:s:a:i:p:")) != -1) {
        switch (c) {
            case 'b':
                input.Butchers = std::stoi(optarg);
                break;
            case 'c':
                input.Cutter = std::stoi(optarg);
                break;
            case 't':
                input.CutterCapacity = std::stoi(optarg);
                break;
            case 's':
                input.SmokeHouse = std::stoi(optarg);
                break;
            case 'a':
                input.MeatAgingFridge = std::stoi(optarg);
                break;
            case 'i':
                input.MeatIntakeFridge = std::stoi(optarg);
                break;
            case 'p':
                input.ProductFridge = std::stoi(optarg);
                break;
            default:
                abort();
        }
    }
    Init(0, 10000);
    (new MealStacking(40))->Activate();
    (new Generator)->Activate();
    Run();

    dobaObsluhy.Output();
    dobaObsluhy2.Output();
    dobaVSystemu.Output();
    Butcher.Output();
    ButcherQueue.Output();
    MeatIntakeFridge.Output();

  return 0;
}
