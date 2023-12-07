/**
 * Authors xkralr06 - Rostislav Kral, xjezek19 - Lukas Jezek
*/

#include <simlib.h>
#include "main.h"
#include <getopt.h>
#include <vector>

MachinesTiming Timing;
ProgramOptions Options;
SimulationParams SimParams;

Stat dobaObsluhy("Doba obsluhy na lince");
Stat dobaObsluhy2("Doba obsluhy na lince2");
Histogram dobaVSystemu("Celkova doba v systemu", 0, 40, 2);

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



MeatStacking::MeatStacking(unsigned int intake) {
    Intake = intake;
    Activate();
    Priority = 2;
}

void MeatStacking::Behavior() {
    double tvstup = Time;
    double obsluha;
    test:
    if (MeatIntakeFridge.Empty()) {
        // Todo wait

    }
    Enter(MeatIntakeFridge, Intake);

    Seize(Butcher);
    Wait(10*60);

    dobaObsluhy2(obsluha);

    Release(Butcher);

    ((new MeatPreparation(Intake))->Activate());

    dobaVSystemu(Time - tvstup);
}

MeatPreparation::MeatPreparation(unsigned int load) {
    Load = load;
    Activate();
    Priority = 3;
}

void MeatPreparation::Behavior()
{
     double tvstup = Time;
    double obsluha;

    Enter(MeatAgingFridge, Load);

    Seize(Butcher);
    Wait(Exponential(30*60));
    Wait(20*60);
    Release(Butcher);

    Wait(2*60*60*24);

    dobaVSystemu(Time - tvstup);
    (new ProductCreation(Load))->Activate();
}

ProductPackaging::ProductPackaging(unsigned int load) {
    Load = load;
    Activate();
    Priority = 5;
}

void ProductPackaging::Behavior()
{
    double tvstup = Time;
    double obsluha;
    Seize(Butcher);
    Wait(30*60);
    Wait(0.8*Load*1.2*60);


    Release(Butcher);

    //TODO: Expedition process
    dobaVSystemu(Time - tvstup);
    (new ProductExpedition(Load*0.8))->Activate();
}

ProductExpedition::ProductExpedition(unsigned int load) {
    Load = load;
    Activate();
    Priority = 1;
}

void ProductExpedition::Behavior()
{
    double tvstup = Time;
    double obsluha;
    Wait(Uniform(12*60*60, 72*60*60));
    Seize(Butcher);
    Wait(40*60);


    Release(Butcher);
    finalProduct += Load;
    ProductFridge.Leave(ProductFridge.Used());
    //TODO: Expedition process
    dobaVSystemu(Time - tvstup);

}


class Generator: public Event{
public:

  void Behavior()
  {
    for(int i = 0; i < n; i++){
    auto meat = new MeatStacking(40);


    meat->Activate(Time);
    }
  }
};


ProductCreation::ProductCreation(unsigned int load) {
    Load = load;
    FinalLoad = 0;
    Activate();
    Priority = 4;
}

void ProductCreation::Behavior() {
    double StartTime = Time;
    double Service;

    Seize(Butcher);

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
    (new ProductPackaging(Load))->Activate();
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


  Init(0,2*8*60*60);
   /* (new MeatStacking(40))->Activate();
        (new MeatStacking(40))->Activate();*/

    (new Generator)->Activate();
    Run();

    dobaObsluhy.Output();
    dobaObsluhy2.Output();
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

  return 0;
}
