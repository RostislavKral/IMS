/**
 * @brief
 * @author Lukáš Ježek
 * @author Rostislav Král
 */

#ifndef IMS_MAIN_H

struct ProgramOptions {
    unsigned int Butchers = 1;
    unsigned int Cutter = 1;
    unsigned int CutterCapacity = 25;
    unsigned int Filler = 1;
    unsigned int FillerCapacity = 50;
    unsigned int SmokeHouse = 1;
    unsigned int SmokeHouseCapacity = 80;
    unsigned int MeatAgingFridge = 3500;
    unsigned int MeatAgingFridgeUsage = 10;
    unsigned int MeatIntakeFridge = 5000;
    unsigned int MeatIntakeFridgeUsage = 10;
    unsigned int ProductFridge = 5000;
    unsigned int ProductFridgeUsage = 10;
    unsigned int ProductQuantity = 40;
    unsigned int Batch = 40;
};

struct MachinesTiming {
    // in minutes
    unsigned int Cutter = 12;
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

    explicit MeatStacking(unsigned int intake);

    void Behavior();
};

class MeatPreparation : public Process {
public:
    unsigned int Load;

    // Konstruktor s explicitním zadáním zatížení
    explicit MeatPreparation(unsigned int load);

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

    // Konstruktor s explicitním zadáním zatížení
    explicit ProductCreation(unsigned int load);

    // Chování procesu
    void Behavior();
};


class ProductExpedition : public Process {
public:
    unsigned int Load;

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
    explicit CutterProcess(unsigned int todo, unsigned int load);
    // Chování procesu
    void Behavior();
};

class FillerProcess : public Process {
public:
    int Todo, Load;
    explicit FillerProcess(unsigned int todo, unsigned int load);
    // Chování procesu
    void Behavior();
};

class SmokeHouseProcess : public Process {
public:
    int Todo, Load;
    explicit SmokeHouseProcess(unsigned int todo, unsigned int load);
    // Chování procesu
    void Behavior();
};


#define IMS_MAIN_H

#endif //IMS_MAIN_H
