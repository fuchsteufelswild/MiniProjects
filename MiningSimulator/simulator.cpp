#include "mylib.hpp"

std::vector<Miner *> miners;
std::vector<Transporter *> transporters;
std::vector<Producer> smelterProducers;
std::vector<Producer> foundryProducers;

std::vector<Smelter *> smelters;
std::vector<Foundry *> foundries;

std::map<Producer, void *> coalMap;
std::map<Producer, void *> copperMap;
std::map<Producer, void *> ironMap;

void Foundry::Init(unsigned int _capacity, int _interval)
{
    random = new RandomInt(_interval - (_interval / 100), _interval + (_interval / 100));
    mpIron = &ironMap;
    mpCoal = &coalMap;

    info = new FoundryInfo();
    FillFoundryInfo(info, FoundryCount++, _capacity, 0, 0, 0);

    sem_init(&mutexOre, 0, 1);
    sem_init(&transporterMutex, 0, 1);
    sem_init(&emptySem, 0, _capacity);
    sem_init(&ironSem, 0, 0);
    sem_init(&coalSem, 0, 0);
    sem_init(&emptySemIron, 0, _capacity);
    sem_init(&reservedIronSem, 0, 0);
    sem_init(&reservedCoalSem, 0, 0);
}

void Smelter::Init(OreType _ore, unsigned int _capacity, int _interval)
{
    random = new RandomInt(_interval - (_interval / 100), _interval + (_interval / 100));

    info = new SmelterInfo();
    FillSmelterInfo(info, SmelterCount++, _ore, _capacity, 0, 0);

    if (_ore == OreType::IRON)
    {
        mapMutexPointer = &mapIronCoal;
        mutexWaitPointer = &producerIronWaitMutex;
        mp = &ironMap;
    }
    else
    {
        mapMutexPointer = &producerSearchCopper;
        mutexWaitPointer = &producerCopperWaitMutex;
        mp = &copperMap;
    }

    sem_init(&mutexOre, 0, 1);
    sem_init(&transporterMutex, 0, 1);
    sem_init(&emptySem, 0, _capacity);
    sem_init(&oreSem, 0, 0);
    sem_init(&reservedOreSem, 0, 0);
}

// Update respective conainer map for changed producer
Producer UpdateMap(Producer producer, Foundry *foundry, Smelter *smelter, bool _halfway, bool _hasEmpty, bool _available, int type, bool _halfwayIron, bool _hasEmptyIron)
{
    if (smelter)
    {
        if (smelter->info->oreType == OreType::IRON)
            mapIndex = 0;
        else
            mapIndex = 1;
        auto itr = smelter->mp->find(producer);
        smelter->mp->erase(itr);

        smelter->halfway = _halfway;
        smelter->hasEmpty = _hasEmpty;
        smelter->available = _available;
        Producer last(smelter);
        (*smelter->mp)[last] = static_cast<void *>(smelter);

        return last;
    }
    else
    {
        mapIndex = 0;
        auto itr1 = ironMap.find(producer);
        ironMap.erase(itr1);

        mapIndex = 1;
        auto itr2 = coalMap.find(producer);
        coalMap.erase(itr2);

        foundry->halfwayIron = _halfwayIron;
        foundry->halfwayCoal = _halfway;
        foundry->hasEmpty = _hasEmpty;
        foundry->available = _available;
        foundry->hasEmptyIron = _hasEmptyIron;

        Producer last(foundry);

        mapIndex = 0;
        (*foundry->mpIron)[last] = static_cast<void *>(foundry);
        mapIndex = 1;
        (*foundry->mpCoal)[last] = static_cast<void *>(foundry);

        return last;
    }
}

// Find next available miner
unsigned int FindNextMiner(unsigned int index)
{
    while (1)
    {
        if (index == miners.size())
            index = 0;

        sem_wait(&(miners[index]->mutex));
        if (miners[index]->hasOre)
        {

            if (miners[index]->reservedOreCount == 1)
                miners[index]->hasOre = false;

            miners[index]->reservedOreCount--;
            sem_post(&(miners[index]->mutex));

            return index;
        }
        sem_post(&(miners[index]->mutex));
        ++index;
    }
}

// Pick a suitable producer according to priority conditions
Producer PickProducer(std::map<Producer, void *> *mp, sem_t *mapPointer, OreType type)
{
    Producer nextProducer;
    Producer temp = (mp->begin()->first);

    if (type == OreType::IRON)
    {
        if (temp.smelter)
        {

            sem_wait(&(temp.smelter->mutexOre));

            bool tHalfway, tHasEmpty = true;
            if (!(temp.smelter->reservedWaitingOreCount))
                tHalfway = true;
            else
                tHalfway = false;
            if (++(temp.smelter->reservedWaitingOreCount) == temp.smelter->info->loading_capacity)
                tHasEmpty = false;

            nextProducer = UpdateMap(temp, nullptr, temp.smelter, tHalfway, tHasEmpty, true, 1, false, false);
            sem_post(&(temp.smelter->reservedOreSem));
            sem_post(&mapSearch);
            sem_post(&(temp.smelter->mutexOre));
        }
        else
        {
            sem_wait(&(temp.foundry->mutexOre));
            bool hIron = temp.foundry->halfwayIron, hCoal = temp.foundry->halfwayCoal, hEmptyIron = temp.foundry->hasEmpty;
            if (!temp.foundry->reservedWaitingCoalOreCount && !temp.foundry->reservedWaitingIronOreCount)
                hCoal = true;
            if (temp.foundry->reservedWaitingCoalOreCount && !temp.foundry->reservedWaitingIronOreCount)
                hIron = false;
            if (temp.foundry->reservedWaitingIronOreCount == temp.foundry->info->loading_capacity - 1)
                hEmptyIron = false;

            temp.foundry->reservedWaitingIronOreCount++;
            nextProducer = UpdateMap(temp, temp.foundry, nullptr, hCoal, temp.foundry->hasEmpty, true, 2, hIron, hEmptyIron);
            sem_post(&(temp.foundry->reservedIronSem));
            sem_post(&mapSearch);
            sem_post(&(temp.foundry->mutexOre));
        }

        return nextProducer;
    }

    if (type == OreType::COPPER)
    {
        sem_wait(&(temp.smelter->mutexOre));
        bool tHalfway, tHasEmpty = true;
        if (!temp.smelter->reservedWaitingOreCount)
            tHalfway = true;
        else
            tHalfway = false;
        if (++temp.smelter->reservedWaitingOreCount == temp.smelter->info->loading_capacity)
            tHasEmpty = false;

        nextProducer = UpdateMap(temp, nullptr, temp.smelter, tHalfway, tHasEmpty, true, 1, false, false);
        sem_post(&(temp.smelter->reservedOreSem));
        sem_post(&mapSearch);
        sem_post(&(temp.smelter->mutexOre));
    }

    else
    {
        sem_wait(&(temp.foundry->mutexOre));
        bool hIron = temp.foundry->halfwayIron, hCoal = temp.foundry->halfwayCoal, hEmpty = temp.foundry->hasEmpty;
        if (!temp.foundry->reservedWaitingCoalOreCount && !temp.foundry->reservedWaitingIronOreCount)
            hIron = true;
        if (!temp.foundry->reservedWaitingCoalOreCount && temp.foundry->reservedWaitingIronOreCount)
            hCoal = false;
        if (temp.foundry->reservedWaitingCoalOreCount == temp.foundry->info->loading_capacity - 1)
            hEmpty = false;
        temp.foundry->reservedWaitingCoalOreCount++;
        nextProducer = UpdateMap(temp, temp.foundry, nullptr, hCoal, hEmpty, true, 2, hIron, temp.foundry->hasEmptyIron);
        sem_post(&(temp.foundry->reservedCoalSem));
        sem_post(&mapSearch);
        sem_post(&(temp.foundry->mutexOre));
    }

    return nextProducer;
}

void *TransporterRoutine(void *transporterVoid)
{
    Transporter *transporter = static_cast<Transporter *>(transporterVoid);
    WriteOutput(nullptr, transporter->info, nullptr, nullptr, Action::TRANSPORTER_CREATED);

    while (1)
    {
        // Check if all miners exited
        sem_wait(&minerActiveMutex);
        if (minerEnd)
        {
            sem_post(&minerWaitMutex);
            sem_post(&minerActiveMutex);
            break;
        }
        sem_post(&minerActiveMutex);

        transporter->info->carry = nullptr;
        MinerInfo _mInfo;

        sem_wait(&(minerWaitMutex)); // Wait for available ore
        sem_wait(&(minerSearch));
        sem_wait(&(minerActiveMutex));
        if (minerEnd)
        {
            sem_post(&minerWaitMutex);
            sem_post(&minerSearch);
            sem_post(&minerActiveMutex);
            break;
        }
        sem_post(&minerActiveMutex);

        unsigned int tIndex = FindNextMiner(transporter->lastMinerIndex + 1); // Find suitable miner
        transporter->lastMinerIndex = tIndex;
        Miner *nextMiner = miners[tIndex];
        sem_post(&(minerSearch));

        FillMinerInfo(&_mInfo, nextMiner->info->ID, static_cast<OreType>(0), 0, 0);
        WriteOutput(&_mInfo, transporter->info, nullptr, nullptr, Action::TRANSPORTER_TRAVEL);
        usleep((*(transporter->random))());

        sem_wait(&(nextMiner->mutex));
        // Check for ending of the miners
        if (!nextMiner->active && nextMiner->currentOreCount - 1 == 0)
        {
            sem_wait(&minerActiveMutex);
            if (--activeMinerCount == 0)
                minerEnd = true;
            sem_post(&minerActiveMutex);
        }

        transporter->info->carry = &(nextMiner->info->oreType);
        nextMiner->currentOreCount--;
        nextMiner->info->current_count = nextMiner->currentOreCount;
        WriteOutput(nextMiner->info, transporter->info, nullptr, nullptr, Action::TRANSPORTER_TAKE_ORE);
        sem_post(&(nextMiner->mutex));

        sem_wait(&(nextMiner->transporterMutex)); // Unload
        usleep((*(transporter->random))());
        sem_post(&(nextMiner->emptySem));
        sem_post(&(nextMiner->transporterMutex));

        sem_t *mapPointer;
        std::map<Producer, void *> *mp; // Pointer to respective map
        // Pick Producer

        switch (*(transporter->info->carry))
        {
        case OreType::IRON:
            mp = &ironMap;
            mapPointer = &mapSearch;
            sem_wait(&producerIronWaitMutex);
            break;
        case OreType::COPPER:
            mp = &copperMap;
            mapPointer = &mapSearch;
            sem_wait(&producerCopperWaitMutex);
            break;
        case OreType::COAL:
            mp = &coalMap;
            mapPointer = &mapSearch;
            sem_wait(&producerCoalWaitMutex);
            break;
        }
		
        if(producerEnd)
            break;

        Producer nextProducer;
        sem_wait(&mapSearch);
        if (*(transporter->info->carry) == OreType::IRON)
            mapIndex = 0;
        else
            mapIndex = 1;

        nextProducer = PickProducer(mp, mapPointer, *(transporter->info->carry));

        if (nextProducer.smelter) // Drop ore to smelter
        {
            sem_wait(&nextProducer.smelter->emptySem);
            if (!nextProducer.smelter->available)
                continue;

            SmelterInfo _sInfo;
            FillSmelterInfo(&_sInfo, nextProducer.smelter->info->ID, static_cast<OreType>(0), 0, 0, 0);
            WriteOutput(nullptr, transporter->info, &_sInfo, nullptr, Action::TRANSPORTER_TRAVEL);
            usleep((*(transporter->random))());

            sem_wait(&(nextProducer.smelter->transporterMutex));

            sem_wait(&(nextProducer.smelter->mutexOre));
            nextProducer.smelter->waitingOreCount += 1;
            nextProducer.smelter->info->waiting_ore_count = nextProducer.smelter->waitingOreCount;
            WriteOutput(nullptr, transporter->info, nextProducer.smelter->info, nullptr, Action::TRANSPORTER_DROP_ORE);
            sem_post(&(nextProducer.smelter->mutexOre));

            usleep((*(transporter->random))());

            sem_post(&(nextProducer.smelter->oreSem));

            sem_post(&(nextProducer.smelter->transporterMutex));
        }
        else
        {
            if(*(transporter->info->carry) == OreType::IRON)
                sem_wait(&nextProducer.foundry->emptySemIron);
            else
                sem_wait(&nextProducer.foundry->emptySem);
            if (!nextProducer.foundry->available)
                break;

            FoundryInfo _fInfo;
            FillFoundryInfo(&_fInfo, nextProducer.foundry->info->ID, 0, 0, 0, 0);
            WriteOutput(nullptr, transporter->info, nullptr, &_fInfo, Action::TRANSPORTER_TRAVEL);
            usleep((*(transporter->random))());

            sem_wait(&(nextProducer.foundry->transporterMutex));

            sem_wait(&(nextProducer.foundry->mutexOre));
            if(*(transporter->info->carry) == OreType::IRON)
                (nextProducer.foundry->waitingIronOreCount)++;
            else
                nextProducer.foundry->waitingCoalOreCount++;
            
            nextProducer.foundry->info->waiting_coal = nextProducer.foundry->waitingCoalOreCount;
            nextProducer.foundry->info->waiting_iron = (nextProducer.foundry->waitingIronOreCount);
            WriteOutput(nullptr, transporter->info, nullptr, nextProducer.foundry->info, Action::TRANSPORTER_DROP_ORE);
            sem_post(&(nextProducer.foundry->mutexOre));

            usleep((*(transporter->random))());

            if (*(transporter->info->carry) == OreType::IRON)
                sem_post(&(nextProducer.foundry->ironSem));
            else
                sem_post(&(nextProducer.foundry->coalSem));

            sem_post(&(nextProducer.foundry->transporterMutex));
        }
    }

    WriteOutput(nullptr, transporter->info, nullptr, nullptr, TRANSPORTER_STOPPED);

    sem_wait(&transporterActiveMutex);
    if (--activeTransporterCount == 0)
    {
        transporterEnd = true;
        for (int i = 0; i < miners.size(); ++i)
            sem_post(&(miners[i]->emptySem));
    }
    sem_post(&transporterActiveMutex);
}

// Miner thread routine
void *MinerRoutine(void *minerVoid)
{
    Miner *miner = static_cast<Miner *>(minerVoid);
    WriteOutput(miner->info, nullptr, nullptr, nullptr, Action::MINER_CREATED); // Inform that this miner has been created
    miner->active = true;

    while (miner->maxOreCount--)
    {
        sem_wait(&(miner->emptySem)); // Wait for empty space in store
        if(transporterEnd)
            break;

        sem_wait(&(miner->mutex));
        miner->info->current_count = miner->currentOreCount; // Update changes to minerinfo
        WriteOutput(miner->info, nullptr, nullptr, nullptr, Action::MINER_STARTED); // Inform that this miner has started production
        sem_post(&(miner->mutex));
        usleep((*(miner->random))()); // Sleep for a while (interval - interval / 100, interval + interval / 100)

        sem_wait(&(miner->mutex));
        miner->reservedOreCount++;                           // Increase reserved ore count
        miner->currentOreCount++;                            // Increase current ore
        miner->hasOre = true;                                // Inform that transporters can come to this miner
        miner->info->current_count = miner->currentOreCount; // Update changes to minerinfo
        WriteOutput(miner->info, nullptr, nullptr, nullptr, Action::MINER_FINISHED); // Inform that this miner has finished its production
        sem_post(&(miner->mutex));

        sem_post(&(minerWaitMutex));  // Inform that this miner has an ore
        usleep((*(miner->random))()); // Sleep for a while (interval - interval / 100, interval + interval / 100)
    }

    // MinerStopped
    sem_wait(&(miner->mutex));
    miner->info->current_count = miner->currentOreCount;
    miner->active = false;
    sem_wait(&minerActiveMutex);
    if (miner->currentOreCount == 0)
    {
        if (--activeMinerCount == 0)
        {
            minerEnd = true;
            sem_post(&(minerWaitMutex));
        }
    }
    sem_post(&minerActiveMutex);
    WriteOutput(miner->info, nullptr, nullptr, nullptr, Action::MINER_STOPPED);
    sem_post(&(miner->mutex));
}

void *SmelterRoutine(void *smelterVoid)
{
    int sem = 0;
    Smelter *smelter = static_cast<Smelter *>(smelterVoid);

    smelter->info->waiting_ore_count = smelter->waitingOreCount;
    WriteOutput(nullptr, nullptr, smelter->info, nullptr, Action::SMELTER_CREATED);
    {
        Producer temp(smelter);
        sem_wait(&mapSearch);
        UpdateMap(temp, nullptr, smelter, false, true, true, 1, false, false);
        sem_post(&mapSearch);
    }
    for (int i = 0; i < smelter->info->loading_capacity; ++i)
        sem_post(smelter->mutexWaitPointer);

    while (1)
    {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += 5;

        auto start = std::chrono::system_clock::now();
        sem = sem_timedwait(&(smelter->reservedOreSem), &ts);
        auto end = std::chrono::system_clock::now();
        if(sem == -1)
            break;
        std::chrono::duration<double> elapsedTime = end - start;
        ts.tv_sec -= elapsedTime.count();    
        sem = sem_timedwait(&(smelter->reservedOreSem), &ts);
        if (sem == -1)
            break;
        
        sem_wait(&smelter->oreSem);
        sem_wait(&smelter->oreSem);

        sem_wait(&mapSearch);
        sem_wait(&(smelter->mutexOre));
        smelter->waitingOreCount -= 2;
        smelter->reservedWaitingOreCount -= 2;
        smelter->info->waiting_ore_count = smelter->waitingOreCount;

        if (smelter->reservedWaitingOreCount == 1)
        {
            bool hEmpty = smelter->hasEmpty;
            if(!hEmpty)
                hEmpty = true;
            Producer temp(smelter);
            UpdateMap(temp, nullptr, smelter, true, hEmpty, true, 1, false, false);
        }
        WriteOutput(nullptr, nullptr, smelter->info, nullptr, Action::SMELTER_STARTED);
        sem_post(&mapSearch);
        sem_post(&(smelter->mutexOre));
        
        usleep((*(smelter->random))());

        smelter->producedIngotCount++;

        sem_wait(&mapSearch);
        Producer temp(smelter);

        UpdateMap(temp, nullptr, smelter, smelter->halfway, smelter->hasEmpty, true, 1, false, false);

        sem_post(&mapSearch);
        sem_post(&smelter->emptySem);
        sem_post(&smelter->emptySem);
        sem_post(smelter->mutexWaitPointer);
        sem_post(smelter->mutexWaitPointer);

        sem_wait(&(smelter->mutexOre));
        smelter->info->waiting_ore_count = smelter->waitingOreCount;
        smelter->info->total_produce = smelter->producedIngotCount;
        WriteOutput(nullptr, nullptr, smelter->info, nullptr, Action::SMELTER_FINISHED);
        sem_post(&(smelter->mutexOre));
        
    }

    sem_wait(&mapSearch);
    Producer temp(smelter);
    if (smelter->info->oreType == OreType::IRON)
        mapIndex = 0;
    else
        mapIndex = 1;
    auto itr = smelter->mp->find(temp);
    smelter->mp->erase(itr);
    sem_post(&mapSearch);

    sem_wait(&smelter->mutexOre);
    smelter->info->waiting_ore_count = smelter->waitingOreCount;
    smelter->info->total_produce = smelter->producedIngotCount;
    WriteOutput(NULL, NULL, smelter->info, NULL, Action::SMELTER_STOPPED);
    sem_post(&smelter->mutexOre);

    sem_wait(&producerActiveMutex);
    smelter->available = false;
    sem_post(&smelter->emptySem);
    if (--activeProducerCount == 0)
    {
        unsigned int sz = Transporter::transporterCount - 1;
        producerEnd = true;
        for (int i = 0; i < sz; ++i)
        {
            sem_post(&producerIronWaitMutex);
            sem_post(&producerCoalWaitMutex);
            sem_post(&producerCopperWaitMutex);
        }
    }
    sem_post(&producerActiveMutex);

}

void *FoundryRoutine(void *foundryVoid)
{
    int sem = 0;
    Foundry *foundry = static_cast<Foundry *>(foundryVoid);

    foundry->info->waiting_iron = foundry->waitingIronOreCount;
    foundry->info->waiting_coal = foundry->waitingCoalOreCount;

    WriteOutput(nullptr, nullptr, nullptr, foundry->info, Action::FOUNDRY_CREATED);

    {
        sem_wait(&mapSearch);
        Producer temp(foundry);
        UpdateMap(temp, foundry, nullptr, false, true, true, 2, false, true);
        sem_post(&mapSearch);
    }
    for (int i = 0; i < foundry->info->loading_capacity; ++i)
        sem_post(&producerCoalWaitMutex);
    for (int i = 0; i < foundry->info->loading_capacity; ++i)
        sem_post(&producerIronWaitMutex);
    while (1)
    {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += 5;

        auto start = std::chrono::system_clock::now();
        sem = sem_timedwait(&foundry->reservedIronSem, &ts);
        if(sem == -1)
            break;
        auto end = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsedTime = end - start;
        ts.tv_sec -= elapsedTime.count();
        sem = sem_timedwait(&foundry->reservedCoalSem, &ts);
        if(sem == -1)
            break;

        sem_wait(&foundry->ironSem);
        sem_wait(&foundry->coalSem);


        sem_wait(&mapSearch);
        sem_wait(&(foundry->mutexOre));
        foundry->waitingIronOreCount--;
        foundry->waitingCoalOreCount--;
        foundry->reservedWaitingIronOreCount--;
        foundry->reservedWaitingCoalOreCount--;
        foundry->info->waiting_iron = foundry->waitingIronOreCount;
        foundry->info->waiting_coal = foundry->waitingCoalOreCount;

        bool hIron = foundry->halfwayIron, hCoal = foundry->halfwayCoal, hEmpty = foundry->hasEmpty, hEmptyIron = foundry->hasEmptyIron;
        if (foundry->reservedWaitingIronOreCount && !foundry->reservedWaitingCoalOreCount)
            hCoal = true;
        else if (foundry->reservedWaitingCoalOreCount && !foundry->reservedWaitingIronOreCount)
            hIron = true;
        else
            hCoal = hIron = false;

        if (!hEmpty)
            hEmpty = true;
        if (!hEmptyIron)
            hEmptyIron = true;

        Producer temp(foundry);
        UpdateMap(temp, foundry, nullptr, hCoal, hEmpty, true, 2, hIron, hEmptyIron);

        WriteOutput(nullptr, nullptr, nullptr, foundry->info, Action::FOUNDRY_STARTED);
        sem_post(&(foundry->mutexOre));
        sem_post(&mapSearch);

        usleep((*(foundry->random))());

        foundry->producedIngotCount++;

        sem_wait(&(foundry->mutexOre));
        foundry->info->total_produce = foundry->producedIngotCount;
        foundry->info->waiting_coal = foundry->waitingCoalOreCount;
        foundry->info->waiting_iron = foundry->waitingIronOreCount;
        WriteOutput(nullptr, nullptr, nullptr, foundry->info, Action::FOUNDRY_FINISHED);
        sem_post(&(foundry->mutexOre));

        sem_post(&foundry->emptySem);
        sem_post(&foundry->emptySemIron);
        sem_post(&producerIronWaitMutex);
        sem_post(&producerCoalWaitMutex);
    }

    sem_wait(&mapSearch);
    Producer temp(foundry);
    mapIndex = 1;
    auto itr = foundry->mpCoal->find(temp);
    foundry->mpCoal->erase(itr);
    mapIndex = 0;
    itr = foundry->mpIron->find(temp);
    foundry->mpIron->erase(itr);
    sem_post(&mapSearch);

    sem_wait(&foundry->mutexOre);
    foundry->info->waiting_coal = foundry->reservedWaitingCoalOreCount;
    foundry->info->waiting_iron = foundry->reservedWaitingIronOreCount;
    foundry->info->total_produce = foundry->producedIngotCount;
    WriteOutput(nullptr, nullptr, nullptr, foundry->info, Action::FOUNDRY_STOPPED);
    sem_post(&foundry->mutexOre);

    sem_wait(&producerActiveMutex);
    sem_post(&foundry->emptySem);
    sem_post(&foundry->emptySemIron);
    foundry->available = false;
    if (--activeProducerCount == 0)
    {
        unsigned int sz = Transporter::transporterCount - 1;
        producerEnd = true;

        for (int i = 0; i < sz; ++i)
        {
            sem_post(&producerIronWaitMutex);
            sem_post(&producerCoalWaitMutex);
            sem_post(&producerCopperWaitMutex);
        }
    }
    sem_post(&producerActiveMutex);
}

void InitSem()
{
    sem_init(&minerWaitMutex, 0, 0);
    sem_init(&minerSearch, 0, 1);
    sem_init(&minerActiveMutex, 0, 1);
    sem_init(&transporterActiveMutex, 0, 1);

    sem_init(&producerCopperWaitMutex, 0, 0);
    sem_init(&producerCoalWaitMutex, 0, 0);
    sem_init(&producerIronWaitMutex, 0, 0);
    sem_init(&producerSearchCopper, 0, 1);
    sem_init(&producerActiveMutex, 0, 1);
    sem_init(&mapIronCoal, 0, 1);
    sem_init(&mapSearch, 0, 1);
}

int main(void)
{

    InitSem();
    int count = 0;
    int nMiners;

    std::cin >> nMiners;
    activeMinerCount = nMiners;

    while (count++ != nMiners)
    {
        int _inter, _cap, _type, _max;
        std::cin >> _inter >> _cap >> _type >> _max;
        Miner *_miner = new Miner();
        _miner->Init(static_cast<OreType>(_type), _cap, _inter, _max);
        miners.push_back(_miner);
    }

    int nTransporters;
    std::cin >> nTransporters;

    count = 0;
    while (count++ != nTransporters)
    {
        int _inter;
        std::cin >> _inter;
        Transporter *_transporter = new Transporter();
        _transporter->Init(_inter, nullptr);
        transporters.push_back(_transporter);
    }

    int nSmelters;

    std::cin >> nSmelters;
    
    count = 0;
    while (count++ != nSmelters)
    {
        int _inter, _cap, _ore;
        std::cin >> _inter >> _cap >> _ore;
        Smelter *_smelter = new Smelter();
        _smelter->Init(static_cast<OreType>(_ore), _cap, _inter);

        smelters.push_back(_smelter);
        Producer temp(_smelter);

        switch (static_cast<OreType>(_ore))
        {
        case OreType::IRON:
            mapIndex = 0;
            ironMap[temp] = static_cast<void *>(temp.smelter);
            break;
        case OreType::COPPER:
            mapIndex = 1;
            copperMap[temp] = static_cast<void *>(temp.smelter);
            break;
        case OreType::COAL:
            mapIndex = 1;
            coalMap[temp] = static_cast<void *>(temp.smelter);
            break;
        default:
            std::cout << "Error undefined type of ore \n";
            system("pause");
        }
    }

    int nFoundries;
    std::cin >> nFoundries;

    count = 0;
    while (count++ != nFoundries)
    {

        int _interval, _capacity;
        std::cin >> _interval >> _capacity;

        Foundry *_foundry = new Foundry();
        _foundry->Init(_capacity, _interval);

        foundries.push_back(_foundry);
        Producer temp(_foundry);

        mapIndex = 0;

        ironMap[temp] = static_cast<void *>(temp.foundry);

        mapIndex = 1;
        coalMap[temp] = static_cast<void *>(temp.foundry);
    }

    activeProducerCount = nSmelters + nFoundries;
    activeTransporterCount = nTransporters;
    pthread_t minerThreads[nMiners];
    pthread_t transporterThreads[nTransporters];
    pthread_t smelterThreads[nSmelters];
    pthread_t foundryThreads[nFoundries];


    for (int i = 0; i < nMiners; ++i)
        pthread_create(&minerThreads[i], nullptr, MinerRoutine, static_cast<void *>(miners[i]));
    for (int i = 0; i < nSmelters; ++i)
        pthread_create(&smelterThreads[i], nullptr, SmelterRoutine, static_cast<void *>(smelters[i]));
    for (int i = 0; i < nTransporters; ++i)
        pthread_create(&transporterThreads[i], nullptr, TransporterRoutine, static_cast<void *>(transporters[i]));
    for (int i = 0; i < nFoundries; ++i)
        pthread_create(&foundryThreads[i], nullptr, FoundryRoutine, static_cast<void *>(foundries[i]));

    for (int i = 0; i < nSmelters; ++i)
        pthread_join(smelterThreads[i], nullptr);
    for (int i = 0; i < nMiners; ++i)
        pthread_join(minerThreads[i], nullptr);
    for (int i = 0; i < nTransporters; ++i)
        pthread_join(transporterThreads[i], nullptr);
    for (int i = 0; i < nFoundries; ++i)
        pthread_join(foundryThreads[i], nullptr);

    for (int i = 0; i < nMiners; ++i)
        delete miners[i];
    for (int i = 0; i < nTransporters; ++i)
        delete transporters[i];
    for (int i = 0; i < nSmelters; ++i)
        delete smelters[i];
    for (int i = 0; i < nFoundries; ++i)
        delete foundries[i];

    miners.erase(miners.begin(), miners.end());
    miners.clear();

    transporters.erase(transporters.begin(), transporters.end());
    transporters.clear();

    smelters.erase(smelters.begin(), smelters.end());
    smelters.clear();

    foundries.erase(foundries.begin(), foundries.end());
    foundries.clear();
}
