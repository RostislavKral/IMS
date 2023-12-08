/**
 * Authors xkralr06 - Rostislav Kral, xjezek19 - Lukas Jezek
*/

#include <simlib.h>
#include "main.h"
#include <getopt.h>
#include <vector>
#include <iostream>
#include <cmath>

MachinesTiming Timing;
ProgramOptions Options;
SimulationParams SimParams;

Stat dobaCelkem("Doba celkem");

Stat dobaSkladani("Doba skladani");

Stat dobaPripravy("Doba pripravy");

Stat dobaZrani("Doba zrami");

Stat dobaVyroby("Doba vyroby");
Stat dobaKutrovani("Doba kutrovani");
Stat dobaNarazeni("Doba narazeni");
Stat dobaUzeni("Doba uzeni");

Stat dobaBaleni("Doba baleni");

Stat dobaExpedice("Doba expedice");

Histogram dobaVSystemu("Celkova doba v systemu", 0, 40, 20);

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

int n = 50;
int finalProduct = 0;
int workday = 0;


WorkingHours::WorkingHours() {
    Priority = 1;
}

void WorkingHours::Behavior() {
    start:
    SimParams.working = true;
    workday++;
    SimParams.totalTime = Time;

    (new MeatStacking(120))->Activate();
    dobaSkladani(Time - SimParams.totalTime);

    double exp = Time;
    (new ProductExpedition)->Activate();
    dobaExpedice(Time - exp);

    Wait(8 * 60 * 60);
    SimParams.working = false;
    Seize(Butcher);

    Wait(16 * 60 * 60);
    Release(Butcher);
    // goto start;
    // Activate(Time + Exponential(2));
    (new WorkingHours)->Activate();
}

MeatStacking::MeatStacking(unsigned int intake) {
    Intake = intake;
    Activate();
    Priority = 3;
}

void MeatStacking::Behavior() {
    Enter(MeatIntakeFridge, Intake);

    Seize(Butcher);

    Wait(2.7 * Intake);

    Release(Butcher);

    ((new MeatPreparation(Intake))->Activate());
}

MeatPreparation::MeatPreparation(unsigned int load) {
    Load = load;
    Activate();
    Priority = 4;
}

void MeatPreparation::Behavior() {
    double tvstup = Time;

    Seize(Butcher);

    Leave(MeatIntakeFridge, Load);
    Enter(MeatAgingFridge, Load);

    // zpracovani
    Wait(Exponential(30 * 60));
    // presun do lednice
    Wait(20 * 60);

    Release(Butcher);
    dobaPripravy(Time - tvstup);

    tvstup = Time;
    // zrani masa
    Wait(2 * 60 * 60 * 24);

    dobaZrani(Time - tvstup);
    (new ProductCreation(Load))->Activate();
}

ProductCreation::ProductCreation(unsigned int load) {
    Load = load;
    FinalLoad = 0;
    Activate();
    Priority = 5;
}

void ProductCreation::Behavior() {
    double StartTime = Time;
    double ProcessTime = 0;

    MeatIntakeFridge.Output();
    Seize(Butcher);
    Leave(MeatAgingFridge, Load);
    Enter(ProductFridge, Load);



    repeatCutter:
    ProcessTime = Time;
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
    ProcessTime = Time;
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
    dobaNarazeni(Time - ProcessTime);

    // skladani do kose do udirny
    Wait(CutteredMeat * 0.05);

    repeatSmoke:
    ProcessTime = Time;
    if (SmokeHouse.Busy()) {
        Into(SmokeHouseQueue);
        Passivate();
        goto repeatSmoke;
    }

    Seize(SmokeHouse);
    Release(Butcher);

    unsigned int SmokedMeat = 0;

    // uzeni
    while (CutteredMeat > SmokedMeat) {
        Seize(Butcher);
        Wait(5*60);
        Release(Butcher);
        Wait(Uniform(Timing.SmokeHouse[0], Timing.SmokeHouse[1]));
        SmokedMeat += (Options.SmokeHouseCapacity < (Load - SmokedMeat)) ?
                      Options.SmokeHouseCapacity :
                      (Load - SmokedMeat);
    }
    Release(SmokeHouse);
    if (SmokeHouseQueue.Length() > 0)
        SmokeHouseQueue.GetFirst()->Activate();
    dobaUzeni(Time - ProcessTime);

    dobaVyroby(Time - StartTime);

    (new ProductPackaging(Load * 0.8))->Activate();
}

ProductPackaging::ProductPackaging(unsigned int load) {
    Load = load;
    Activate();
    Priority = 5;
}

void ProductPackaging::Behavior() {
    Seize(Butcher);
    // chlazeni
    Wait(30 * 60);

    //baleni
    double tvstup = Time;
    Wait(Load * 1.2 * 60);
    dobaBaleni(Time - tvstup);

    Release(Butcher);

    dobaCelkem(Time - SimParams.totalTime);
    dobaVSystemu(Time - SimParams.totalTime);
}

ProductExpedition::ProductExpedition() {
    Activate();
    Priority = 2;
}

void ProductExpedition::Behavior() {
    Wait(Exponential(2 * 60 * 60));
    Seize(Butcher);

    Wait(40 * 60);

    Release(Butcher);
    finalProduct += ProductFridge.Used();
    ProductFridge.Leave(ProductFridge.Used());
}

//Generator::Generator(unsigned int load) {
//    Load = load;
//}
//
//void Generator::Behavior() {
//        for (int i = 0; i < n; i++) {
//            auto meat = new MeatStacking(loa);
//            meat->Activate(Time);
//        }
//    }
//};



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


    Init(0, 5 * 24 * 60 * 60);
    /* (new MeatStacking(40))->Activate();
         (new MeatStacking(40))->Activate();*/

    (new WorkingHours)->Activate();
    // (new Generator)->Activate();

    // (new ProductExpedition(1 * 24 * 60 * 60))->Activate();
    Run();

    dobaCelkem.Output();

    dobaSkladani.Output();

    dobaPripravy.Output();

    dobaZrani.Output();

    dobaVyroby.Output();
    dobaKutrovani.Output();
    dobaNarazeni.Output();
    dobaUzeni.Output();

    dobaBaleni.Output();

    dobaExpedice.Output();

    dobaVSystemu.Output();
    Butcher.Output();
    Cutter.Output();
    CutterQueue.Output();
    SmokeHouse.Output();
    SmokeHouseQueue.Output();
    SausageFiller.Output();
    SausageFillerQueue.Output();
    MeatIntakeFridge.Output();
    MeatAgingFridge.Output();
    ProductFridge.Output();
    Print(finalProduct);
    Print(workday);

    return 0;
}
