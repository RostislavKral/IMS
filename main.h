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
    unsigned int SmokeHouse = 1;
    unsigned int MeatAgingFridge = 3500;
    unsigned int MeatAgingFridgeUsage = 10;
    unsigned int MeatIntakeFridge = 5000;
    unsigned int MeatIntakeFridgeUsage = 10;
    unsigned int ProductFridge = 5000;
    unsigned int ProductFridgeUsage = 10;
};

class MealStacking : public Process {
public:
    unsigned int Intake;

    explicit MealStacking(unsigned int intake);

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

#define IMS_MAIN_H

#endif //IMS_MAIN_H
