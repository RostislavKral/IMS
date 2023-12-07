/**
 * Authors xkralr06 - Rostislav Kral, xjezek19 - Lukas Jezek
*/

#include <simlib.h>
#include "main.h"
#include <getopt.h>

MachinesTiming Timing;
ProgramOptions Options;
SimulationParams SimParams;

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
        auto meat = new MealStacking(40);


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

ProductCreation::ProductCreation(unsigned int load) {
    Load = load;
    FinalLoad = 0;
    Activate();
}

void ProductCreation::Behavior() {
    double StartTime = Time;
    double Service;

    repeat:
    if (Butcher.Busy()) {
        Into(ButcherQueue);
        Passivate();
        goto repeat;
    }
    Seize(Butcher, 3);

    Enter(ProductFridge, Load);

    repeatCutter:
    if (Cutter.Busy()) {
        Into(CutterQueue);
        Passivate();
        goto repeatCutter;
    }
    Seize(Cutter);

    unsigned int CutteredMeat = 0;
    while (CutteredMeat < Load) {
        Wait(Timing.Cutter);
        CutteredMeat += (Options.CutterCapacity < (Load - CutteredMeat)) ?
                Options.CutterCapacity :
                (Load - CutteredMeat);
    }
    Release(Cutter);
    if (CutterQueue.Length() > 0)
        CutterQueue.GetFirst()->Activate();

    repeatFiller:
    if (SausageFiller.Busy()) {
        Into(SausageFillerQueue);
        Passivate();
        goto repeatFiller;
    }
    // narazka parku
    Seize(SausageFiller);

    Wait(CutteredMeat * 48);

    Release(SausageFiller);
    if (SausageFillerQueue.Length() > 0)
        SausageFillerQueue.GetFirst()->Activate();

    // skladani do kose do udirny
    Wait(CutteredMeat * 0.05);

    repeatSmoke:
    if (SmokeHouse.Busy()){
        Into(SmokeHouseQueue);
        Passivate();
        goto repeatSmoke;
    }
    Seize(SmokeHouse);

    Release(Butcher);
    if (ButcherQueue.Length() > 0)
        ButcherQueue.GetFirst()->Activate();

    unsigned int SmokedMeat = 0;
    while (CutteredMeat > SmokedMeat) {
        // uzeni
        Wait(Uniform(Timing.SmokeHouse[0],Timing.SmokeHouse[1]));
        SmokedMeat += (Options.SmokeHouseCapacity < (Load - SmokedMeat)) ?
                        Options.SmokeHouseCapacity :
                        (Load - SmokedMeat);
    }

    Release(SmokeHouse);
    if (SmokeHouseQueue.Length() > 0)
        SmokeHouseQueue.GetFirst()->Activate();

}

int main(int argc, char *argv[]) {
    int c;

    // Zpracování vstupních přepínačů a argumentů při spuštění
    while ((c = getopt(argc, argv, "b:c:t:s:a:i:p:")) != -1) {
        switch (c) {
            case 'b':
                Options.Butchers = std::stoi(optarg);
                break;
            case 'c':
                Options.Cutter = std::stoi(optarg);
                break;
            case 't':
                Options.CutterCapacity = std::stoi(optarg);
                break;
            case 's':
                Options.SmokeHouse = std::stoi(optarg);
                break;
            case 'a':
                Options.MeatAgingFridge = std::stoi(optarg);
                break;
            case 'i':
                Options.MeatIntakeFridge = std::stoi(optarg);
                break;
            case 'p':
                Options.ProductFridge = std::stoi(optarg);
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
