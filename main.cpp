/**
 * @authors xkralr06 - Rostislav Kral, xjezek19 - Lukas Jezek
*/

#include <simlib.h>
#include "main.h"
#include <getopt.h>
#include <iostream>
#include <cmath>
#include <iomanip>

double wdh = 0;
double inProcess = 0;
int inSmokeHouseTotal = 0;
int inSmokeHouseActive = 0;
MachinesTiming Timing;
ProgramOptions Options;
SimulationParams SimParams;

Stat dobaCelkem("Doba celkem");

Stat dobaSkladani("Doba skladani");

Stat dobaPripravy("Doba pripravy");

Stat dobaZrani("Doba zrani");

Stat dobaVyroby("Doba vyroby");
Stat dobaKutrovani("Doba kutrovani");
Stat dobaNarazeni("Doba narazeni");
Stat dobaUzeni("Doba uzeni");

Stat dobaBaleni("Doba baleni");

Stat dobaExpedice("Doba expedice");

Histogram dobaVSystemuHist("Celkova doba v systemu", 0, 40, 20);

Queue Q1("cutter reset Queue");
Queue Q2("filler reset Queue");
Queue Q3("smoke house reset Queue");

Store Butcher("Reznik", pocetRezniku);

Queue CutterQueue("Fronta na kutr");
Facility Cutter[pocetKutru];

Queue SmokeHouseQueue("Fronty na udirnu");
Facility SmokeHouse[pocetUdiren];

Queue SausageFillerQueue("Fronta na narazecku");
Facility SausageFiller[pocetNarazek];

Store MeatIntakeFridge("Lednice pro prijem masa", Options.MeatIntakeFridge * Options.MeatIntakeFridgeUsage);

Store MeatAgingFridge("Lednice pro zrani", Options.MeatAgingFridge * Options.MeatAgingFridgeUsage);

Store ProductFridge("Lednice pro hotove produkty", Options.ProductFridge * Options.ProductFridgeUsage);

int finalProduct = 0;
int workday = 0;
int CutteredMeat = 0;
int FilledMeat = 0;
int SmokedMeat = 0;
int inSmokeHouseNum = 0;
bool error = false;
double totalVyrobaCustom = 0;
int pocetCyklu = 0;
std::string where;


WorkingHours::WorkingHours() {
    Priority = 1;
}

void WorkingHours::Behavior() {
    double inTime = Time;

    SimParams.working = true;
    workday++;

    if (workday == WorkDays+1) {
        workday--;
        if (DEBUG) std::cerr << "!!!!!!!!!!!!!EOS!!!!!!!!" << std::endl;
        Stop();
    }
    if (DEBUG) std::cerr << "---------------NEW WD -----" << workday << "---------- Actual Time:" << Time << " - " << workday*86400 << "    " << Time << " - " << Time + 8*60*60 << std::endl;

    SimParams.totalTime = Time;

    (new MeatStacking((INPUT)))->Activate();
    dobaSkladani(Time - SimParams.totalTime);

    Wait(8 * 60 * 60);
    SimParams.working = false;

    double defaultTime = 16 * 60 * 60;
    double timeT = Time;

    if (DEBUG) std::cerr << " @@@ konec smeny t: " << Time<< std::endl;

    Enter(Butcher, pocetRezniku);

    if (DEBUG) std::cerr << "Butcher IN 1  " << Time << std::endl;

    defaultTime = defaultTime - (Time - timeT);
    // wdh += (Time - inTime);

    if (DEBUG) std::cerr << "WDS Butcher  workday: " << workday << " Time: " << Time  << std::endl;

    if (defaultTime <= 0) {
        std::cerr << "Prekracovani 24h cyklu" << std::endl;
        error = true;
        Stop();
    };

    Wait(defaultTime);

    Leave(Butcher, pocetRezniku);

    if (DEBUG) {
        std::cerr << "Butcher OUT " << Time << std::endl;
        std::cerr << "WDR Butcher  leave: " << workday << "  T: " << Time << std::endl;
        std::cerr << "---------------RECALL  WD ----------------" << std::endl;
    }

    (new WorkingHours)->Activate();
}

MeatStacking::MeatStacking(unsigned int intake) {
    Intake = intake;
    Activate();
    Priority = 2;
}

void MeatStacking::Behavior() {
    if (DEBUG) std::cerr << "---- start stacking ---" << Time << std::endl;
    where = "Prijem zbozi";

    TotalTime = Time;

    Enter(Butcher, 1);
    double btime = Time;

    if (DEBUG) std::cerr << "Butcher IN  2 " << Time << std::endl;

    if (MeatIntakeFridge.Free() < Intake) {
        std::cerr << "Nedostatečná kapacita vstupní lednice, konec simulace" << std::endl;
        error = true;
        Stop();
    }
    Enter(MeatIntakeFridge, Intake);

    Wait(2.7 * Intake);

    Leave(Butcher, 1);
    wdh += (Time - btime);

    if (DEBUG) std::cerr << "Butcher OUT " << Time << std::endl;

    (new ProductExpedition)->Activate();
    ((new MeatPreparation(Intake, (Time - TotalTime)))->Activate());

    dobaVSystemuHist(Time - TotalTime);

    if (DEBUG) std::cerr << "---- end stacking ---" << Time << std::endl;
}

MeatPreparation::MeatPreparation(unsigned int load, double vstup) {
    Load = load;
    Activate();
    TotalTime = vstup;
    // Priority = 5;
}

void MeatPreparation::Behavior() {
    where = "Priprava masa";
    double tvstup = Time;
    double in = Time;

    Enter(Butcher);
    double btime = Time;

    if (DEBUG) std::cerr << "Butcher IN  3 " << Time << std::endl;

    if (MeatAgingFridge.Free() < Load) {
        std::cerr << "Nedostatečná kapacita zrací lednice, ukončuji simulaci" << std::endl;
        error = true;
        Stop();
    }
    Enter(MeatAgingFridge, Load);
    Leave(MeatIntakeFridge, Load);

    // zpracovani
    Wait(Exponential(30 * 60));

    // presun do lednice
    Wait(20 * 60);

    Leave(Butcher);
    wdh += (Time - btime);
    if (DEBUG) std::cerr << "Butcher OUT " << Time << std::endl;

    dobaPripravy(Time - tvstup);
    tvstup = Time;

    TotalTime += Time - in;
    // zrani masa
    Wait(2 * 60 * 60 * 24);

    if (DEBUG) std::cerr << "---- end zrani ---" << Time << std::endl;

    dobaZrani(Time - tvstup);
    dobaVSystemuHist(Time - in);

    (new ProductCreation(Load, (TotalTime)))->Activate();
}

ProductCreation::ProductCreation(unsigned int load, double vstup) {
    Load = load;
    FinalLoad = 0;
    Activate();
    TotalTime = vstup;
}

CutterProcess::CutterProcess (unsigned int todo, unsigned int load) {
    Todo = todo;
    Load = load;
};

void CutterProcess::Behavior() {
    double in = Time;

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
    double btime = Time;

    if (DEBUG) std::cerr << "Butcher IN CutterProcess " << Time << std::endl;

    // Prace na kutru
    Wait(Timing.Cutter);

    // Hotove maso celkem ze vsech kutru
    CutteredMeat += Todo;

    if (CutterQueue.Length() > 0) {
        CutterQueue.GetFirst()->Activate();
    }

    if (DEBUG) std::cerr << "c: " << CutteredMeat << " L: " << Load << " T " << Todo <<  " Time " << Time <<  std::endl;

    // Pokud se dokoncila varka, pokracuj dal
    if (CutteredMeat == Load) {
        CutteredMeat = 0;
        Q1.GetFirst()->Activate();
    }

    // Uvolneni zdroju
    Release(Cutter[c]);
    Leave(Butcher);
    wdh += (Time - btime);

    dobaVSystemuHist(Time - in);

    if (DEBUG) std::cerr << "Butcher OUT CutterProcess " << Time << std::endl;
}

FillerProcess::FillerProcess (unsigned int todo, unsigned int load) {
    Todo = todo;
    Load = load;
};

void FillerProcess::Behavior() {
    double in = Time;

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
    // Zabrani zdroju
    Seize(SausageFiller[c]);
    Enter(Butcher);
    double btime = Time;

    if (DEBUG) std::cerr << "Butcher IN 5  " << Time << std::endl;

    // Narazeni parku
    Wait(Todo * Timing.SausageFiller);

    FilledMeat += Todo;

    // Dokoncena varka, pokracuj ve vyrobe
    if (FilledMeat == Load) {
        FilledMeat = 0;
        Q2.GetFirst()->Activate();
    }

    // uvolneni
    Leave(Butcher);
    Release(SausageFiller[c]);
    wdh += (Time - btime);

    dobaVSystemuHist(Time - in);

    if (DEBUG) std::cerr << "Butcher OUT " << Time << std::endl;

    if (SausageFillerQueue.Length() > 0)
        SausageFillerQueue.GetFirst()->Activate();
}

SmokeHouseProcess::SmokeHouseProcess (unsigned int todo, unsigned int load) {
    Todo = todo;
    Load = load;
    Priority = 4;
};

void SmokeHouseProcess::Behavior() {
    double in = Time;

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

    // alokace zdroju
    Seize(SmokeHouse[c]);
    Enter(Butcher, 1);
    double btime = Time;

    if (DEBUG) std::cerr << "Butcher IN SMOKE " << Time << std::endl;

    // pouzivanych udiren
    inSmokeHouseNum++;
    inSmokeHouseActive += Todo;

    // presun do udirny
    Wait(5*60);

    // uvolneni reznika, udirna pracuje sama
    Leave(Butcher,1);
    wdh += (Time - btime);

    if (DEBUG) std::cerr << "Butcher OUT SMOKE " << Time << std::endl;

    // uzeni
    Wait(Uniform(Timing.SmokeHouse[0], Timing.SmokeHouse[1]));

    SmokedMeat += Todo;

    Enter(Butcher, 1);
    btime = Time;

    // vytazeni kose z udirny
    Wait(5*60);

    if (DEBUG) std::cerr << "Butcher IN   " << Time << std::endl;

    if (SmokedMeat == Load) {
        SmokedMeat = 0;
        Q3.GetFirst()->Activate();
    }

    // Uvolneni
    Release(SmokeHouse[c]);
    Leave(Butcher, 1);
    wdh += (Time - btime);

    inSmokeHouseActive -= Todo;
    inSmokeHouseNum--;

    dobaVSystemuHist(Time - in);

    if (DEBUG) std::cerr << "Butcher OUT " << Time << std::endl;

    if (SmokeHouseQueue.Length() > 0) {
        SmokeHouseQueue.GetFirst()->Activate();
    }
}

void ProductCreation::Behavior() {
    where = "Zacatek produkce";
    if (DEBUG) std::cerr << "---- start creation ---" << Time << std::endl;

    double StartTime = Time;

    Enter(Butcher);
    double btime = Time;

    if (DEBUG) std::cerr << "Butcher IN   " << Time << std::endl;

    if (ProductFridge.Free() < Load) {
        std::cerr << "Malo mista v lednici produktů, konec simulace" << std::endl;
        error = true;
        Stop();
        Wait(100);
    }

    Leave(MeatAgingFridge, Load);

    double ProcessTime = Time;

    // START BLOK Kutrovani
    where = "Zacatek kutrovani";
    if (DEBUG) std::cerr << "start cutter" << std::endl;

    // Uvolneni reznika, pro moznost paralelismu
    Leave(Butcher);
    wdh += (Time - btime);

    if (DEBUG) std::cerr << "Butcher OUT " << Time << std::endl;

    // Rozdeleni kutrovani podle kapacit stroju
    unsigned int inCutters = 0;
    for (int i = 0; i < std::ceil((Load*1.0) / (Options.CutterCapacity*1.0)); ++i) {
        int todo = (Options.CutterCapacity < (Load - inCutters)) ?
                        Options.CutterCapacity :
                        (Load - inCutters);
        inCutters += todo;
        (new CutterProcess(todo, Load))->Activate();
    }

    where = "Konec kutrovani";

    // Cekani na dokonceni kutrovani
    Into(Q1);
    Passivate();
    dobaKutrovani(Time - ProcessTime);

    // END BLOK kutrovani

    // START BLOK Narazeni
    where = "Zacatek narazeni";

    ProcessTime = Time;

    // Procesy narazeni
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
    dobaNarazeni(Time - ProcessTime);
    where = "Konec narazeni";
    // END BLOK Narazeni

    Enter(Butcher);
    btime = Time;
    if (DEBUG) std::cerr << "Butcher IN skladani do kose  " << Time << std::endl;

    // skladani do kose do udirny
    Wait(Load * 0.05);

    // uzeni
    where = "Zacatek uzeni";

    ProcessTime = Time;

    Leave(Butcher, 1);
    wdh += (Time - btime);
    if (DEBUG) std::cerr << "Butcher OUT pred udirna" << Time << std::endl;

    unsigned int inSmokeHouseLocal = 0;
    inSmokeHouseTotal += Load;
    for (int i = 0; i < std::ceil((Load*1.0) / (Options.SmokeHouseCapacity*1.0)); ++i) {
        int todo = (Options.SmokeHouseCapacity < (Load - inSmokeHouseLocal)) ?
                   Options.SmokeHouseCapacity :
                   (Load - inSmokeHouseLocal);
        inSmokeHouseLocal += todo;
        (new SmokeHouseProcess(todo, Load))->Activate();
    }
    Into(Q3);
    Passivate();
    where = "Konec uzeni";
    dobaUzeni(Time - ProcessTime);
    inSmokeHouseTotal -= Load;

    Enter(Butcher, 1);
    btime = Time;
    if (DEBUG) std::cerr << "Butcher IN udirna end " << Time << std::endl;

    // vytazeni z udiren
    Wait(5*60);

    Leave(Butcher);
    wdh += (Time - btime);
    if (DEBUG) std::cerr << "Butcher OUT udirna end" << Time << std::endl;

    // chlazeni produktu
    Wait(30 * 60);

    Enter(Butcher, 1);
    btime = Time;
    if (DEBUG) std::cerr << "Butcher IN chlaz   " << Time << std::endl;

    // Ulozeni do lednice, ztrana na hmotnosti 20%
    Enter(ProductFridge, Load*0.8);

    Leave(Butcher, 1);
    wdh += (Time - btime);
    if (DEBUG) std::cerr << "Butcher OUT chlaz" << Time << std::endl;

    // paralelni baleni
    for (int i = 0; i < pocetRezniku; ++i) {
        (new ProductPackaging(Load/pocetRezniku))->Activate();
    }

    TotalTime += ((Load*36)/pocetRezniku);
    TotalTime += (Time - StartTime);
    totalVyrobaCustom += TotalTime;
    pocetCyklu++;
    dobaVyroby(TotalTime);
    dobaVSystemuHist(Time - StartTime);
}

ProductPackaging::ProductPackaging(unsigned int load) {
    Load = load;
    Activate();
    // Priority = 7;
}

void ProductPackaging::Behavior() {
    if (DEBUG) std::cerr << "-------- Start pack ---------"<< Time << std::endl;
    where = "Baleni";

    Enter(Butcher, 1);
    double btime = Time;

    if (DEBUG) std::cerr << "Butcher IN pack  " << Time << std::endl;

    double tvstup = Time;
    //baleni
    Wait(Load * 0.6 * 60);

    dobaBaleni(Time - tvstup);

    Leave(Butcher, 1);
    wdh += (Time - btime);
    if (DEBUG) std::cerr << "Butcher OUT pack " << Time << std::endl;

    dobaCelkem(Time - SimParams.totalTime);

    // dobaVSystemuHist(Time - SimParams.totalTime);
    if (DEBUG) std::cerr << "-------- End pack ---------"<< Time << std::endl;
}

ProductExpedition::ProductExpedition() {
    Activate();
    Priority = 3;
}

void ProductExpedition::Behavior() {
    if (DEBUG) std::cerr << "---- start exp ---"<< Time << std::endl;
    where = "Expedice";

    Enter(Butcher, 1);
    double btime = Time;
    if (DEBUG) std::cerr << "Butcher IN  exp " << Time << std::endl;

    // Expeduji se vzdy vsechny vyrobky
    int exped = ProductFridge.Used();

    if (DEBUG) std::cerr << "expeduji: " << exped << std::endl;

    Wait(0.5*exped);

    Leave(Butcher, 1);
    wdh += (Time - btime);
    if (DEBUG) std::cerr << "Butcher OUT exp" << Time << std::endl;

    finalProduct += exped;
    ProductFridge.Leave(ProductFridge.Used());

    if (DEBUG) std::cerr << "---- end exp ---" << Time << std::endl;
}


int main(int argc, char *argv[]) {

    // SetOutput("multiexp.dat");
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


    Init(0, (WorkDays+1)* 24 * 60 * 60);

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

    // dobaVSystemuHist.Output();
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

    std::string state;
    if (error) state = "Chyba";
    else if (inSmokeHouseNum > 0) state = "OK - ! Udirna aktivni !";
    else if ((((ProductFridge.Used()*1.0 + finalProduct) / 80 * 100) + MeatAgingFridge.Used() + MeatIntakeFridge.Used() + inProcess) != workday*INPUT) state = "Nestihnuta vyroba, konec v: " +where;
    else state = "OK";

    std::cout << "Stav simulace: " <<state<< std::endl
    << "Kontrolni soucet: " << workday*INPUT << " Check: " << ((ProductFridge.Used()*1.0 + finalProduct) / 80 * 100) + MeatAgingFridge.Used() + MeatIntakeFridge.Used() + inProcess << std::endl
    << " V udirne: " << inSmokeHouseActive << std::endl
    << " Celkem k uzeni: "<< inSmokeHouseTotal << std::endl
    << " pocet aktivnich udiren: "<< inSmokeHouseNum << std::endl
    << "Vyrobeno: " << ProductFridge.Used() + finalProduct << " Kg, Použito: " << (((ProductFridge.Used()*1.0 + finalProduct) / 80 * 100)) << " Kg Masa" << std::endl
    << "Expedovano: " << finalProduct << std::endl
    << "Pocet dni: " << workday << std::endl
    << "Hodin denne cista prace bez cekani: " << std::setprecision(2) << wdh/workday/3600 << "h/den" <<  std::endl
    << "Prumerna doba 1 cyklus vyroby bez zrani a baleni: " << (totalVyrobaCustom*1.0)/3600/pocetCyklu << " h" << std::endl
    << "Prumerna doba vyroby 1KG bez zrani a baleni: " << (totalVyrobaCustom*1.0)/60/INPUT << " min" << std::endl;
    if (DEBUG) std::cerr << where << std::endl;
    return 0;
}
