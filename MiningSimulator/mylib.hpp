#ifndef _MYLIB_H
#define _MYLIB_H

#include <iostream>
#include <string>
#include <random>
#include <vector>
#include <map>
#include <chrono>
#include <ctime>
#include <unistd.h>
#include <semaphore.h>
#include <time.h>
#include "writeOutput.h"

class Miner;
class Transporter;
class Smelter;
class Foundry;
struct Producer;

sem_t transporterActiveMutex;
int activeTransporterCount;
bool transporterEnd = false;

sem_t minerActiveMutex;
int activeMinerCount;
bool minerEnd = false;

sem_t producerActiveMutex;
int activeProducerCount;
bool producerEnd = false;

short mapIndex = 0;

sem_t minerWaitMutex;
sem_t minerSearch;

sem_t mapSearch;
sem_t producerSearchCopper;
sem_t mapIronCoal;
sem_t producerCopperWaitMutex;
sem_t producerIronWaitMutex;
sem_t producerCoalWaitMutex;

class RandomInt
{
  private:
    std::default_random_engine eng;
    std::uniform_int_distribution<> dist;

  public:
    RandomInt(int _l, int _h) : dist(_l, _h) {}
    int operator()() { return dist(eng); }
};

class Transporter
{
  public:
    TransporterInfo *info;
    RandomInt *random;

    static unsigned int transporterCount;

    unsigned int lastMinerIndex = 0;

  public:
    Transporter() = default;
    ~Transporter()
    {
        delete info;
        delete random;
    }

  public:
    void Init(int _interval, OreType *_ore)
    {
        random = new RandomInt(_interval - (_interval / 100), _interval + (_interval / 100));

        info = new TransporterInfo();

        FillTransporterInfo(info, transporterCount++, _ore);
    }

    friend std::ostream &operator<<(std::ostream &out, const Transporter &agent);
};

class Miner
{
  public:
    Miner() = default;
    ~Miner()
    {
        delete info;
        delete random;
        sem_destroy(&mutex);
        sem_destroy(&emptySem);
        sem_destroy(&transporterMutex);
    }

  public:
    MinerInfo *info;
    RandomInt *random;

    static unsigned int MinerCount;

    unsigned int currentOreCount = 0;
    unsigned int maxOreCount;
    unsigned int reservedOreCount = 0;

    bool active = false;
    bool avaliable = false;
    bool hasOre = false;

  public:
    void Init(OreType _ore, unsigned int _capacity, int _interval, int _maxOre);
    friend std::ostream &operator<<(std::ostream &out, const Miner &miner);

  public:
    sem_t emptySem;
    sem_t mutex;
    sem_t transporterMutex;
};

void Miner::Init(OreType _ore, unsigned int _capacity, int _interval, int _maxOre)
{
    random = new RandomInt(_interval - (_interval / 100), _interval + (_interval / 100));
    maxOreCount = _maxOre;

    info = new MinerInfo();
    FillMinerInfo(info, MinerCount++, _ore, _capacity, 0);

    sem_init(&mutex, 0, 1);
    sem_init(&emptySem, 0, _capacity);
    sem_init(&transporterMutex, 0, 1);
}

class Smelter
{
  public:
    Smelter() = default;
    ~Smelter()
    {
        delete info;
        delete random;
        sem_destroy(&mutexOre);
        sem_destroy(&transporterMutex);
        sem_destroy(&emptySem);
    }

  public:
    SmelterInfo *info;
    RandomInt *random;
    static unsigned int SmelterCount;

    unsigned int producedIngotCount = 0;
    unsigned int waitingOreCount = 0;
    unsigned int reservedWaitingOreCount = 0;

    bool available = false;
    bool halfway = false;
    bool hasEmpty = true;
    bool waiting = false;

    std::map<Producer, void *> *mp;

  public:
    void Init(OreType _ore, unsigned int _capacity, int _interval);
    friend std::ostream &operator<<(std::ostream &out, const Smelter &smelter);

  public:
    sem_t *mapMutexPointer;
    sem_t *mutexWaitPointer;
    sem_t mutexOre;
    sem_t transporterMutex;
    sem_t emptySem;
    sem_t oreSem;
    sem_t reservedOreSem;
};

class Foundry
{
  public:
    Foundry() = default;
    ~Foundry()
    {
        delete info;
        delete random;
        sem_destroy(&transporterMutex);
        sem_destroy(&mutexOre);
        sem_destroy(&emptySem);
        sem_destroy(&emptySemIron);
    }

  public:
    FoundryInfo *info;
    RandomInt *random;

    static unsigned int FoundryCount;

    unsigned int producedIngotCount = 0;
    unsigned int maxProduceCount;
    unsigned int waitingIronOreCount = 0;
    unsigned int waitingCoalOreCount = 0;
    unsigned int reservedWaitingIronOreCount = 0;
    unsigned int reservedWaitingCoalOreCount = 0;

    bool available = false;
    bool halfwayIron = false;
    bool halfwayCoal = false;
    bool hasEmpty = true;
    bool hasEmptyIron = true;
    bool waiting = false;

    std::map<Producer, void *> *mpIron;
    std::map<Producer, void *> *mpCoal;

  public:
    void Init(unsigned int _capacity, int _interval);

    friend std::ostream &operator<<(std::ostream &out, const Foundry &foundry);

  public:
    sem_t transporterMutex;
    sem_t mutexOre;
    sem_t emptySem;
    sem_t emptySemIron;
    sem_t ironSem;
    sem_t coalSem;
    sem_t reservedIronSem;
    sem_t reservedCoalSem;
};

struct Producer
{
    Smelter *smelter = nullptr;
    Foundry *foundry = nullptr;
    short type = 0;
    unsigned int id = -1;
    bool available = true;
    bool halfway = false;
    bool hasEmpty = true;

    void Init(Smelter *_smelter)
    {
        smelter = _smelter;
        type = 1;
        id = _smelter->info->ID;
        available = smelter->available;
        halfway = smelter->halfway;
        hasEmpty = smelter->hasEmpty;
    }
    void Init(Foundry *_foundry)
    {
        foundry = _foundry;
        type = 2;
        id = _foundry->info->ID;
        available = foundry->available;
        halfway = foundry->halfwayCoal;
        hasEmpty = foundry->hasEmpty;
    }
    Producer() {}
    Producer(Smelter *_smelter) : smelter(_smelter)
    {
        type = 1;
        id = _smelter->info->ID;
        available = smelter->available;
        halfway = smelter->halfway;
        hasEmpty = smelter->hasEmpty;
    }
    Producer(Foundry *_foundry) : foundry(_foundry)
    {
        type = 2;
        id = _foundry->info->ID;
        available = foundry->available;
        halfway = foundry->halfwayCoal;
        hasEmpty = foundry->hasEmpty;
    }
    Producer(const Producer &producer) : smelter(producer.smelter), foundry(producer.foundry), type(producer.type), id(producer.id), available(producer.available),
                                         halfway(producer.halfway), hasEmpty(producer.hasEmpty) {}
    bool operator==(const Producer &producer) const { return producer.id == this->id; }
    bool operator!=(const Producer &producer) const { return !(*this == producer); }
    friend bool operator<(const Producer &producer1, const Producer &producer2);
    friend bool operator>(const Producer &producer1, const Producer &producer2);
    friend bool operator<=(const Producer &producer1, const Producer &producer2);
    friend bool operator>=(const Producer &producer1, const Producer &producer2);
    friend std::ostream &operator<<(std::ostream &out, const Producer &producer);
};

unsigned int Miner::MinerCount = 1;
unsigned int Smelter::SmelterCount = 1;
unsigned int Foundry::FoundryCount = 1;
unsigned int Transporter::transporterCount = 1;

std::ostream &operator<<(std::ostream &out, const Miner &agent)
{
    out << "Miner card: \n"
        << "Miner ID: " << agent.info->ID << "\n"
        << "Miner Ore: " << agent.info->oreType << "\n"
        << "Miner capacity: " << agent.info->capacity << "\n"
        << "Miner current count: " << agent.info->current_count << "\n"
        << "Miner max produce count: " << agent.maxOreCount << "\n";
    return out;
}

std::ostream &operator<<(std::ostream &out, const Smelter &agent)
{
    out << "Smelter card: \n"
        << "Smelter ID: " << agent.info->ID << "\n"
        << "Smelter Ore: " << agent.info->oreType << "\n"
        << "Smelter capacity: " << agent.info->loading_capacity << "\n"
        << "Smelter current count: " << agent.info->waiting_ore_count << "\n"
        << "Smelter max produce: " << agent.info->total_produce << "\n";
    return out;
}

std::ostream &operator<<(std::ostream &out, const Foundry &agent)
{
    out << "Foundry card: \n"
        << "Foundry ID: " << agent.info->ID << "\n"
        << "Foundry waiting iron ore: " << agent.info->waiting_iron << "\n"
        << "Foundry waiting coal ore: " << agent.info->waiting_coal << "\n"
        << "Foundry capacity: " << agent.info->loading_capacity << "\n"
        << "Foundry produced so far: " << agent.info->total_produce << "\n";
    return out;
}

std::ostream &operator<<(std::ostream &out, const Transporter &agent)
{
    out << "Transporter card: \n"
        << "Transporter ID: " << agent.info->ID << "\n";
    if (agent.info->carry)
        out << "Transporter Ore: " << *(agent.info->carry) << "\n";
    else
        out << "Transporter Ore: None "
            << "\n";
    return out;
}

std::ostream &operator<<(std::ostream &out, const Producer &producer)
{
    if (producer.smelter)
        out << *(producer.smelter);
    else
        out << *(producer.foundry);

    return out;
}
bool operator<(const Producer &producer1, const Producer &producer2)
{
    if (!mapIndex)
    {
        if (producer1.foundry)
        {
            if (producer1.foundry->hasEmptyIron)
            {
                if (producer2.foundry)
                {
                    if (!producer2.foundry->hasEmptyIron)
                        return true;
                }
                else if (!producer2.hasEmpty)
                    return true;
            }
            else
            {
                if (producer2.foundry)
                {
                    if (producer2.foundry->hasEmptyIron)
                        return false;
                }
                else if (producer2.hasEmpty)
                    return false;
            }

            if (producer1.foundry->halfwayIron)
            {
                if (producer2.foundry)
                {
                    if (producer2.foundry->halfwayIron)
                        return producer1.id < producer2.id;
                    return true;
                }
                if (producer2.halfway)
                    return true;
                return true;
            }
            if (producer2.foundry)
            {
                if (producer2.foundry->halfwayIron)
                    return false;
                return producer1.id < producer2.id;
            }
            if (producer2.halfway)
                return false;
            return true;
        }
        else
        {
            if (producer1.hasEmpty)
            {
                if (producer2.foundry)
                {
                    if (!producer2.foundry->hasEmptyIron)
                        return true;
                }
                else if (!producer2.hasEmpty)
                    return true;
            }
            else
            {
                if (producer2.foundry)
                {
                    if (producer2.foundry->hasEmptyIron)
                        return false;
                }
                else if (producer2.hasEmpty)
                    return false;
            }
        }

        if (producer1.halfway)
        {
            if (producer1.hasEmpty)
            {
                if (producer2.foundry)
                {
                    if (!producer2.foundry->hasEmptyIron)
                        return true;
                }
                else if (!producer2.hasEmpty)
                    return true;
            }
            else
            {
                if (producer2.foundry)
                {
                    if (producer2.foundry->hasEmptyIron)
                        return false;
                }
                else if (producer2.hasEmpty)
                    return false;
            }

            if (producer2.foundry)
            {
                if (producer2.foundry->halfwayIron)
                    return false;
                return true;
            }
            if (producer2.halfway)
                return producer1.id < producer2.id;
            return true;
        }

        if (producer2.foundry)
        {
            if (producer2.foundry->halfwayIron)
                return false;
            return false;
        }

        if (!producer2.halfway)
            return producer1.id < producer2.id;
        return false;
    }

    if (producer1.hasEmpty)
    {
        if (!producer2.hasEmpty)
            return true;
    }
    else if (producer2.hasEmpty)
        return false;

    if (producer1.halfway)
    {
        if (producer2.halfway)
            return producer1.id < producer2.id;
        return true;
    }
    if (!producer2.halfway)
        return producer1.id < producer2.id;
    return false;
}

bool operator>(const Producer &producer1, const Producer &producer2)
{
    if (producer1 < producer2 || producer1 == producer2)
        return false;
    return true;
}

bool operator<=(const Producer &producer1, const Producer &producer2)
{
    if (producer1 < producer2 || producer1 == producer2)
        return true;
    else
        return false;
}
bool operator>=(const Producer &producer1, const Producer &producer2)
{
    if (producer1 > producer2 || producer1 == producer2)
        return true;
    else
        return false;
}


#endif // _MYLIB_H
