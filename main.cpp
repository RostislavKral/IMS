/**
 * Authors xkralr06 - Rostislav Kral, xjezek19 - Lukas Jezek
*/

#include <simlib.h>
#include "main.h"
#include <getopt.h>
#include <vector>
#include <iostream>
#include <cmath>
#include <iomanip>

#define pocetKutru 1
#define pocetNarazek 1
#define pocetUdiren 1
#define INPUT 200
#define pocetRezniku 5

double wdh = 0;
double inProcess = 0;

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

Queue Q;

Store Butcher("Reznik", pocetRezniku);
//Facility Butcher("Reznik");

Queue CutterQueue("Fronta na kutr");
Facility Cutter[pocetKutru];

Queue SmokeHouseQueue("Fronty na udirnu");
Facility SmokeHouse[pocetUdiren];

Queue SausageFillerQueue("Fronta na narazecku");
Facility SausageFiller[pocetNarazek];

Store MeatIntakeFridge("Lednice pro prijem masa", 5000);

Store MeatAgingFridge("Lednice pro zrani", 3500);

Store ProductFridge("Lednice pro hotove produkty", 5000);

int finalProduct = 0;
int workday = 0;
int CutteredMeat = 0;
int FilledMeat = 0;
int SmokedMeat = 0;
int inSmokeHouseNum = 0;

std::string where;
WorkingHours::WorkingHours() {
    Priority = 1;
}

void WorkingHours::Behavior() {
    double inTime = Time;
    SimParams.working = true;
    if (workday >= 5) Wait(Exponential(10000000));
    workday++;
    std::cerr << "NEW WD//////////////////" << std::endl;
    SimParams.totalTime = Time;
    (new MeatStacking((INPUT)))->Activate();
    dobaSkladani(Time - SimParams.totalTime);

    Wait(8 * 60 * 60);
    SimParams.working = false;

    double defaultTime = 16 * 60 * 60;
    Enter(Butcher, pocetRezniku);

    wdh += (Time - inTime);
    std::cerr << "WDS Butcher  WD: " << workday << std::endl;

    Wait(defaultTime);
    Leave(Butcher, pocetRezniku);
    std::cerr << "WDR Butcher  WD: " << workday << std::endl;

    (new WorkingHours)->Activate();
}

MeatStacking::MeatStacking(unsigned int intake) {
    Intake = intake;
    Activate();
    Priority = 2;
}

void MeatStacking::Behavior() {
    where = "stacking";

    Enter(Butcher);
    Enter(MeatIntakeFridge, Intake);

    Wait(2.7 * Intake);

    Leave(Butcher);

    (new ProductExpedition)->Activate();
    ((new MeatPreparation(Intake))->Activate());
}

MeatPreparation::MeatPreparation(unsigned int load) {
    Load = load;
    Activate();
    // Priority = 5;
}

void MeatPreparation::Behavior() {
    double tvstup = Time;

    Enter(Butcher);
    where = "preparation";
    Leave(MeatIntakeFridge, Load);
    Enter(MeatAgingFridge, Load);

    // zpracovani
    Wait(Exponential(30 * 60));
    // presun do lednice
    Wait(20 * 60);

    Leave(Butcher);
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
    // Priority = 6;
}

CutterProcess::CutterProcess (unsigned int todo, unsigned int load) {
    Todo = todo;
    Load = load;
};

void CutterProcess::Behavior() {
    int c = -1;
    zpet:
    for (int i = 0; i < pocetKutru; ++i) {
        if(!Cutter[i].Busy()) {
            c = i;
            break;
        }
    }

    if (c == -1){
        Into(CutterQueue);
        Passivate();
        goto zpet;
    }
    Seize(Cutter[c]);

    Wait(Timing.Cutter);
    CutteredMeat += Todo;

    // std::cerr << "c: " << CutteredMeat << " L: " << Load << "T " << Todo <<  std::endl;
    if (CutteredMeat == Load) {
        CutteredMeat = 0;
        Q.GetFirst()->Activate();
    }
    Release(Cutter[c]);
    if (CutterQueue.Length() > 0) {
        CutterQueue.GetFirst()->Activate();
    }
}

FillerProcess::FillerProcess (unsigned int todo, unsigned int load) {
    Todo = todo;
    Load = load;
};

void FillerProcess::Behavior() {
    int c = -1;
    zpet:
    for (int i = 0; i < pocetNarazek; ++i) {
        if(!SausageFiller[i].Busy()) { c = i; break; }
    }

    if (c == -1){
        Into(SausageFillerQueue);
        Passivate();
        goto zpet;
    }
    Seize(SausageFiller[c]);

    Wait(Load * Timing.SausageFiller);
    FilledMeat += Todo;

    if (FilledMeat == Load) {
        FilledMeat = 0;
        Q.GetFirst()->Activate();
    }

    Release(SausageFiller[c]);
    if (SausageFillerQueue.Length() > 0)
        SausageFillerQueue.GetFirst()->Activate();
}

SmokeHouseProcess::SmokeHouseProcess (unsigned int todo, unsigned int load) {
    Todo = todo;
    Load = load;
    Priority = 4;
};

void SmokeHouseProcess::Behavior() {
    int c = -1;
    zpet:
    for (int i = 0; i < pocetUdiren; ++i) {
        if(!SmokeHouse[i].Busy()) {
            c = i;
            break;
        }
    }

    if (c == -1){
        Into(SmokeHouseQueue);
        Passivate();
        goto zpet;
    }
    Seize(SmokeHouse[c]);
    inSmokeHouseNum++;
    Enter(Butcher);
    std::cerr << "seized butcher smote" << std::endl;
    Wait(5*60);
    Leave(Butcher);
    std::cerr << "released butcher smote" << std::endl;

    Wait(Uniform(Timing.SmokeHouse[0], Timing.SmokeHouse[1]));
    SmokedMeat += Todo;
    // Enter(Butcher);
    if (SmokedMeat == Load) {
        SmokedMeat = 0;
        Q.GetFirst()->Activate();
    }
    Release(SmokeHouse[c]);
    // Leave(Butcher);
    if (SmokeHouseQueue.Length() > 0) {
        SmokeHouseQueue.GetFirst()->Activate();
    }
}

void ProductCreation::Behavior() {
    double StartTime = Time;

    Enter(Butcher);
    Leave(MeatAgingFridge, Load);
    if (ProductFridge.Free() < Load)
        std::cerr << "Malo mista v lednici produktu" << std::endl;

    double ProcessTime = Time;

    // Kutrovani
    where = "cutter";

    unsigned int inCutters = 0;
    for (int i = 0; i < std::ceil((Load*1.0) / (Options.CutterCapacity*1.0)); ++i) {
        int todo = (Options.CutterCapacity < (Load - inCutters)) ?
                        Options.CutterCapacity :
                        (Load - inCutters);
        inCutters += todo;
        (new CutterProcess(todo, Load))->Activate();
    }
    Into(Q);
    Passivate();
    dobaKutrovani(Time - ProcessTime);


    // Narazeni
    where = "naraz";
    ProcessTime = Time;

    unsigned int inFiller = 0;
    for (int i = 0; i < std::ceil((Load*1.0) / (Options.FillerCapacity*1.0)); ++i) {
        int todo = (Options.FillerCapacity < (Load - inFiller)) ?
                   Options.FillerCapacity :
                   (Load - inFiller);
        inCutters += todo;
        (new FillerProcess(todo, Load))->Activate();
    }
    Into(Q);
    Passivate();
    dobaNarazeni(Time - ProcessTime);

    // skladani do kose do udirny
    Wait(Load * 0.05);

    // uzeni
    where = "uzeni";

    ProcessTime = Time;

    Leave(Butcher);

    unsigned int inSmokeHouse = 0;
    for (int i = 0; i < std::ceil((Load*1.0) / (Options.SmokeHouseCapacity*1.0)); ++i) {
        int todo = (Options.SmokeHouseCapacity < (Load - inSmokeHouse)) ?
                   Options.SmokeHouseCapacity :
                   (Load - inSmokeHouse);
        inSmokeHouse += todo;
        (new SmokeHouseProcess(todo, Load))->Activate();
    }
    Into(Q);
    Passivate();

    Enter(Butcher);
    dobaUzeni(Time - ProcessTime);

    dobaVyroby(Time - StartTime);
    Enter(ProductFridge, Load*0.8);
    Leave(Butcher);
    (new ProductPackaging(Load))->Activate();
}

ProductPackaging::ProductPackaging(unsigned int load) {
    Load = load;
    Activate();
    // Priority = 7;
}

void ProductPackaging::Behavior() {
    where = "pack";
    Enter(Butcher);
    // chlazeni
    Wait(30 * 60);

    //baleni
    double tvstup = Time;
    Wait(Load * 1.2 * 60);
    dobaBaleni(Time - tvstup);

    Leave(Butcher);

    dobaCelkem(Time - SimParams.totalTime);
    dobaVSystemu(Time - SimParams.totalTime);
}

ProductExpedition::ProductExpedition() {
    Activate();
    Priority = 3;
}

void ProductExpedition::Behavior() {
    Enter(Butcher);
    where = "exp";
    int exped = ProductFridge.Used();
    std::cerr << "expeduju: " << exped << std::endl;
    Wait(5*exped);

    //Wait(40 * 60);

    Leave(Butcher);
    finalProduct += exped;
    ProductFridge.Leave(ProductFridge.Used());
}


int main(int argc, char *argv[]) {
    int c;

    // Zpracování vstupních přepínačů a argumentů při spuštění
    while ((c = getopt(argc, argv, "i:o:")) != -1) {
        switch (c) {
            case 'i':
                Options.Butchers = std::stoi(optarg);
                break;
            case 'o':
                Options.Cutter = std::stoi(optarg);
                break;
            default:
                abort();
        }
    }

    for (int i = 0; i < pocetUdiren; ++i) {
        std::string name = "Udirna ";
        name += std::to_string(i);
        SmokeHouse[i].SetName(name);
    }

    for (int i = 0; i < pocetNarazek; ++i) {
        std::string name = "Narazka ";
        name += std::to_string(i);
        SmokeHouse[i].SetName(name);
    }

    for (int i = 0; i < pocetKutru; ++i) {
        std::string name = "Kutr ";
        name += std::to_string(i);
        SmokeHouse[i].SetName(name);
    }


    Init(0, 5 * 24 * 60 * 60);

    (new WorkingHours)->Activate();

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

    for (const auto & i : Cutter) {
        i.Output();
    }
    CutterQueue.Output();

    for (const auto & i : SmokeHouse) {
        i.Output();
    }
    SmokeHouseQueue.Output();

    for (const auto & i : SausageFiller) {
        i.Output();
    }
    SausageFillerQueue.Output();

    MeatIntakeFridge.Output();
    MeatAgingFridge.Output();
    ProductFridge.Output();

    std::cout << "Kontrolni soucet: " << workday*INPUT << " Check: " << ((ProductFridge.Used() + finalProduct) / 80 * 100) + MeatAgingFridge.Used() + MeatIntakeFridge.Used() + inProcess << std::endl
    << "Vyrobeno: " << ProductFridge.Used() + finalProduct << std::endl
    << "Expedovano: " << finalProduct << std::endl
    << "Pocet dni: " << workday << std::endl
    << "Hodin denne reznik: " << std::setprecision(2) << wdh/workday/3600/pocetRezniku << "h/den" <<  std::endl;
    std::cerr << where << std::endl;
    return 0;
}
