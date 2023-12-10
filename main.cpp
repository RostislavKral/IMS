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
#define INPUT 150
#define pocetRezniku 1

double wdh = 0;
double inProcess = 0;
int inSmokeHouseTotal = 0;

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
Queue Q1("cutter reset Queue");
Queue Q2("filler reset Queue");
Queue Q3("smoke house reset Queue");

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
    if (workday >= 5) Wait(10 * 24 * 60 * 60);
    workday++;
    std::cerr << "---------------NEW WD ---------------" << Time << std::endl;
    SimParams.totalTime = Time;
    Q1.Output();
    if (Q1.Length() > 0)
        Q1.GetFirst()->Activate();
    (new MeatStacking((INPUT)))->Activate();
    dobaSkladani(Time - SimParams.totalTime);

    Wait(8 * 60 * 60);
    SimParams.working = false;

    double defaultTime = 16 * 60 * 60;
    double timeT = Time;
    std::cerr << "konec smeny" << std::endl;
    Enter(Butcher, pocetRezniku);
    defaultTime = defaultTime - (Time - timeT);
    wdh += (Time - inTime);
    std::cerr << "WDS Butcher  workday: " << workday << " Time: " << Time << " TimeV: " << (workday*86400) << " TimeN: " << (Time - (workday*86400)) << std::endl;

    if (defaultTime <= 0) {
        std::cerr << "Prekracovani 24h cyklu" << std::endl;
        exit(EXIT_FAILURE);
    };
    Wait(defaultTime);
    Leave(Butcher, pocetRezniku);
    std::cerr << "WDR Butcher  leave: " << workday << std::endl;

    (new WorkingHours)->Activate();
}

MeatStacking::MeatStacking(unsigned int intake) {
    Intake = intake;
    Activate();
    Priority = 2;
}

void MeatStacking::Behavior() {
    where = "stacking";

    Enter(Butcher, 1);
    Enter(MeatIntakeFridge, Intake);

    Wait(2.7 * Intake);

    Leave(Butcher, 1);

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

    Enter(Butcher, 1);
    where = "preparation";
    Leave(MeatIntakeFridge, Load);
    Enter(MeatAgingFridge, Load);

    // zpracovani
    Wait(Exponential(30 * 60));
    // presun do lednice
    Wait(20 * 60);

    Leave(Butcher, 1);
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
    Enter(Butcher);
    Wait(Timing.Cutter);
    CutteredMeat += Todo;

    if (CutterQueue.Length() > 0) {
        CutterQueue.GetFirst()->Activate();
    }

    std::cerr << "c: " << CutteredMeat << " L: " << Load << " T " << Todo <<  " Time " << Time <<  std::endl;
    if (CutteredMeat == Load) {
        CutteredMeat = 0;
        Q1.GetFirst()->Activate();
    }
    Release(Cutter[c]);
    Leave(Butcher);
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
    Enter(Butcher);

    Wait(Todo * Timing.SausageFiller);
    FilledMeat += Todo;

    if (FilledMeat == Load) {
        FilledMeat = 0;
        Q2.GetFirst()->Activate();
    }
    Leave(Butcher);
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

    Enter(Butcher, 1);
    std::cerr << "seized butcher SMOKE" << std::endl;

    Wait(5*60);

    Leave(Butcher,1);
    std::cerr << "released butcher SMOKE" << std::endl;

    Wait(Uniform(Timing.SmokeHouse[0], Timing.SmokeHouse[1]));
    SmokedMeat += Todo;
    // Enter(Butcher, 1);
    if (SmokedMeat == Load) {
        SmokedMeat = 0;
        Q3.GetFirst()->Activate();
    }
    Release(SmokeHouse[c]);
    // Leave(Butcher, 1);
    if (SmokeHouseQueue.Length() > 0) {
        SmokeHouseQueue.GetFirst()->Activate();
    }
}

void ProductCreation::Behavior() {
    where = "start creation";
    double StartTime = Time;

    retry:
    if (!SimParams.working) {
        Into(Q1);
        Passivate();
        goto retry;
    }
    Enter(Butcher, 1);
    Leave(MeatAgingFridge, Load);
    if (ProductFridge.Free() < Load)
        std::cerr << "Malo mista v lednici produktu" << std::endl;

    double ProcessTime = Time;

    // Kutrovani
    where = "start cutter block";
    std::cerr << "start cutter" << std::endl;

    Leave(Butcher);
    unsigned int inCutters = 0;
    for (int i = 0; i < std::ceil((Load*1.0) / (Options.CutterCapacity*1.0)); ++i) {
        int todo = (Options.CutterCapacity < (Load - inCutters)) ?
                        Options.CutterCapacity :
                        (Load - inCutters);
        inCutters += todo;
        (new CutterProcess(todo, Load))->Activate();
    }

    Into(Q1);
    Passivate();
    Enter(Butcher);
    where = "end cutter block";
    std::cerr << "end cutter" << std::endl;
    dobaKutrovani(Time - ProcessTime);


    // Narazeni
    where = "naraz";
    ProcessTime = Time;

    Leave(Butcher);
    unsigned int inFiller = 0;
    for (int i = 0; i < std::ceil((Load*1.0) / (Options.FillerCapacity*1.0)); ++i) {
        int todo = (Options.FillerCapacity < (Load - inFiller)) ?
                   Options.FillerCapacity :
                   (Load - inFiller);
        inFiller += todo;
        (new FillerProcess(todo, Load))->Activate();
    }
    Into(Q2);
    Passivate();
    Enter(Butcher);
    dobaNarazeni(Time - ProcessTime);

    // skladani do kose do udirny
    Wait(Load * 0.05);

    // uzeni
    where = "uzeni";

    ProcessTime = Time;

    Leave(Butcher, 1);

    unsigned int inSmokeHouse = 0;
    inSmokeHouseTotal += Load;
    for (int i = 0; i < std::ceil((Load*1.0) / (Options.SmokeHouseCapacity*1.0)); ++i) {
        int todo = (Options.SmokeHouseCapacity < (Load - inSmokeHouse)) ?
                   Options.SmokeHouseCapacity :
                   (Load - inSmokeHouse);
        inSmokeHouse += todo;
        (new SmokeHouseProcess(todo, Load))->Activate();
    }
    Into(Q3);
    Passivate();
    inSmokeHouseTotal -= Load;

    Enter(Butcher, 1);
    dobaUzeni(Time - ProcessTime);

    dobaVyroby(Time - StartTime);
    Enter(ProductFridge, Load*0.8);
    Leave(Butcher, 1);
    (new ProductPackaging(Load))->Activate();
}

ProductPackaging::ProductPackaging(unsigned int load) {
    Load = load;
    Activate();
    // Priority = 7;
}

void ProductPackaging::Behavior() {
    where = "pack";
    Enter(Butcher, 1);
    // chlazeni
    Wait(30 * 60);

    //baleni
    double tvstup = Time;
    Wait(Load * 1.2 * 60);
    dobaBaleni(Time - tvstup);

    Leave(Butcher, 1);

    dobaCelkem(Time - SimParams.totalTime);
    // dobaVSystemu(Time - SimParams.totalTime);
}

ProductExpedition::ProductExpedition() {
    Activate();
    Priority = 3;
}

void ProductExpedition::Behavior() {
    Enter(Butcher, 1);
    where = "exp";
    int exped = ProductFridge.Used();
    std::cerr << "expeduju: " << exped << std::endl;
    Wait(0.5*exped);

    //Wait(40 * 60);

    Leave(Butcher, 1);
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
        SausageFiller[i].SetName(name);
    }

    for (int i = 0; i < pocetKutru; ++i) {
        std::string name = "Kutr ";
        name += std::to_string(i);
        Cutter[i].SetName(name);
    }


    Init(0, 7 * 24 * 60 * 60);

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


    std::cout << "Kontrolni soucet: " << workday*INPUT << " Check: " << ((ProductFridge.Used()*1.0 + finalProduct) / 80 * 100) + MeatAgingFridge.Used() + MeatIntakeFridge.Used() + inProcess << " V udirne: " << inSmokeHouseTotal << std::endl
    << "Vyrobeno: " << ProductFridge.Used() + finalProduct << std::endl
    << "Expedovano: " << finalProduct << std::endl
    << "Pocet dni: " << workday << std::endl
    << "Hodin denne reznik: " << std::setprecision(2) << wdh/workday/3600/pocetRezniku << "h/den" <<  std::endl;
    std::cerr << where << std::endl;
    return 0;
}
