///////////////////////////////////////////////////////////////////////////////////
//
// ripplegen.cpp 
//
// Copyright (c) 2013 Eric Lombrozo
//  all rights reserved
//
// A simple multithreaded vanity id generator for the ripple p2p network.
//
//
// Portions of the source were taken from the ripple and bitcoin reference
// implementations, but they have been refactored as to minimize linkage
// dependencies.
//
// Further changes are possible to dramatically improve performance.
//

#include "RippleAddress.h"
#include <iostream>
#include <stdint.h>
#include <boost/thread.hpp>
#include <openssl/rand.h>

#define UPDATE_ITERATIONS 1000

using namespace std;

boost::mutex mutex;
bool fDone = false;

uint64_t start_time;
uint64_t total_searched;

const char* ALPHABET = "rpshnaf39wBUDNEGHJKLM4PQRST7VWXYZ2bcdeCg65jkm8oFqi1tuvAxyz";

char charHex(int iDigit)
{
    return iDigit < 10 ? '0' + iDigit : 'A' - 10 + iDigit;
}

void getRand(unsigned char *buf, int num)
{
    if (RAND_bytes(buf, num) != 1)
    {
        assert(false);
        throw std::runtime_error("Entropy pool not seeded");
    }
}

void LoopThread(unsigned int n, string* ppattern, string* pmaster_seed, string* pmaster_seed_hex, string* paccount_id)
{
    RippleAddress naSeed;
    RippleAddress naAccount;
    string        pattern = *ppattern;
    string        account_id;

    uint128 key;
    getRand(key.begin(), key.size());

    uint64_t count = 0;
    uint64_t last_count = 0;
    do {
        naSeed.setSeed(key);
        RippleAddress naGenerator = createGeneratorPublic(naSeed);
        naAccount.setAccountPublic(naGenerator.getAccountPublic(), 0);
        account_id = naAccount.humanAccountID();
        count++;
        if (count % UPDATE_ITERATIONS == 0) {
            boost::unique_lock<boost::mutex> lock(mutex);
            total_searched += count - last_count;
            last_count = count;
            uint64_t nSecs = time(NULL) - start_time;
            double speed = (1.0 * total_searched)/nSecs;

            cout << "# Thread " << n << ": " << count << " seeds." << endl
                 << "#           Account ID      " << account_id << endl
                 << "#           Pattern:        \"" << pattern << "\"" << endl
                 << "#           Total Searched: " << total_searched << endl
                 << "#           Total Time:     " << nSecs << "s" << endl
                 << "#           Speed:          " << speed << " seeds/s" << endl
                 << "#" << endl;
        }
        key++;
        boost::this_thread::yield();
    } while ((account_id.substr(0, pattern.size()) != pattern) && !fDone);

    boost::unique_lock<boost::mutex> lock(mutex);
    if (fDone) return;
    fDone = true;

    cout << "#    *** Found by thread " << n << ". ***" << endl
         << "#" << endl;

    *pmaster_seed = naSeed.humanSeed();
    *pmaster_seed_hex = naSeed.getSeed().ToString();
    *paccount_id = account_id;
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        cout << "# Usage: " << argv[0] << " [pattern] [threads=cpus available]" << endl
             << "#" << endl;
        return 0;
    }

    // TODO: Validate pattern

    unsigned int cpus = boost::thread::hardware_concurrency();
    unsigned int threads = (argc >= 3) ? strtoul(argv[2], NULL, 0) : cpus;
    if (threads == 0) {
        cout << "# You must run at least one thread." << endl
             << "#" << endl;
        return -1;
    }

    cout << "# CPUs detected: " << cpus << endl;
    cout << "# Running " << threads << " thread" << (threads == 1 ? "" : "s") << "..." << endl;

    string pattern = argv[1];
    cout << "#" << endl
         << "# Generating seed for pattern \"" << pattern << "\"" << endl
         << "#" << endl;

    string master_seed, master_seed_hex, account_id;

    start_time = time(NULL);

    vector<boost::thread*> vpThreads;
    for (int i = 0; i < (int)threads; i++)
        vpThreads.push_back(new boost::thread(LoopThread, i, &pattern, &master_seed, &master_seed_hex, &account_id));

    for (int i = 0; i < (int)threads; i++)
        vpThreads[i]->join();
   
    for (int i = 0; i < (int)threads; i++)
        delete vpThreads[i];
 
    cout << "#    master seed:     " << master_seed << endl
         << "#    master seed hex: " << master_seed_hex << endl
         << "#    account id:      " << account_id << endl
         << "#" << endl;

    return 0;
}
