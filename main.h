/**
 * @brief
 * @author Lukáš Ježek
 * @author Rostislav Král
 */

#ifndef IMS_MAIN_H

#define pocetKutru 1
#define pocetNarazek 2
#define pocetUdiren 1
#define INPUT 400
#define pocetRezniku 2
#define DEBUG true
#define WorkDays 5

struct ProgramOptions {
    unsigned int CutterCapacity = 80;
    unsigned int FillerCapacity = 50;
    unsigned int SmokeHouseCapacity = 400;
    unsigned int MeatAgingFridge = 3500;
    unsigned int MeatAgingFridgeUsage = 1;
    unsigned int MeatIntakeFridge = 5000;
    unsigned int MeatIntakeFridgeUsage = 1;
    unsigned int ProductFridge = 5000;
    unsigned int ProductFridgeUsage = 1;
};

struct MachinesTiming {
    // in minutes
    unsigned int Cutter = 12 * 60;
    int SmokeHouse[2] = {72 * 60, 165 * 60};
    int SausageFiller = 48;
};

struct SimulationParams {
    unsigned int product = 0;
    bool working = false;
    double totalTime = 0;
};

class MeatStacking : public Process {
public:
    unsigned int Intake;
    double TotalTime;

    explicit MeatStacking(unsigned int intake);

    void Behavior();
};

class MeatPreparation : public Process {
public:
    unsigned int Load;
    double TotalTime;

    // Konstruktor s explicitním zadáním zatížení
    explicit MeatPreparation(unsigned int load, double vstup);

    // Chování procesu
    void Behavior();
};

class ProductPackaging : public Process {
public:
    unsigned int Load;

    // Konstruktor s explicitním zadáním zatížení
    explicit ProductPackaging(unsigned int load);

    // Chování procesu
    void Behavior();
};

class ProductCreation : public Process {
public:
    unsigned int Load;
    unsigned int FinalLoad;
    double TotalTime;

    // Konstruktor s explicitním zadáním zatížení
    explicit ProductCreation(unsigned int load, double vstup);

    // Chování procesu
    void Behavior();
};


class ProductExpedition : public Process {
public:
    unsigned int TotalTime;

    // Konstruktor s explicitním zadáním zatížení
    explicit ProductExpedition();

    // Chování procesu
    void Behavior();
};

class WorkingHours : public Process {
public:
    explicit WorkingHours();
    // Chování procesu
    void Behavior();
};

class CutterProcess : public Process {
public:
    int Todo, Load;
    double TotalTime;

    explicit CutterProcess(unsigned int todo, unsigned int load);
    // Chování procesu
    void Behavior();
};

class FillerProcess : public Process {
public:
    int Todo, Load;
    double Vstup;

    explicit FillerProcess(unsigned int todo, unsigned int load);
    // Chování procesu
    void Behavior();
};

class SmokeHouseProcess : public Process {
public:
    int Todo, Load;
    double TotalTime;

    explicit SmokeHouseProcess(unsigned int todo, unsigned int load);
    // Chování procesu
    void Behavior();
};


#define IMS_MAIN_H

#endif //IMS_MAIN_H
