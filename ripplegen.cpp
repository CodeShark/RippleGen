///////////////////////////////////////////////////////////////////////////////////
//
// ripplegen.cpp 
//
// Copyright (c) 2013 Eric Lombrozo
//  all rights reserved
//
// A simple multithreaded vanity id generator for the ripple p2p network.
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
            cout << "  Thread " << n << ": " << count << " seeds." << endl;
            cout << "             Total Searched: " << total_searched << endl;
            cout << "             Total Time:     " << nSecs << "s" << endl;
            cout << "             Speed:          " << speed << " seeds/s" << endl << endl;
        }
        key++;
        boost::this_thread::yield();
    } while ((account_id.substr(0, pattern.size()) != pattern) && !fDone);

    boost::unique_lock<boost::mutex> lock(mutex);
    if (fDone) return;
    fDone = true;
    cout << endl << "Found by thread " << n << "." << endl;

    *pmaster_seed = naSeed.humanSeed();
    *pmaster_seed_hex = naSeed.getSeed().ToString();
    *paccount_id = account_id;
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        cout << endl << "Usage: " << argv[0] << " [pattern] [threads=1]";
        cout << endl << "   specify -1 for threads to use maximum available CPUs." << endl << endl;
        return 0;
    }

    string pattern = argv[1];

    cout << endl << "Generating seed...";
    cout << endl << "  pattern: " << pattern << endl << endl;;

    int cpus = boost::thread::hardware_concurrency();
    cout << "CPUs detected: " << cpus << endl << endl;

    int threads = (argc >= 3) ? strtol(argv[2], NULL, 0) : 1;
    if (threads == -1) threads = cpus;
    cout << "Launching " << threads << " threads..." << endl << endl;

    string master_seed, master_seed_hex, account_id;

    start_time = time(NULL);

    vector<boost::thread*> vpThreads;
    for (int i = 0; i < threads; i++) {
        vpThreads.push_back(new boost::thread(LoopThread, i, &pattern, &master_seed, &master_seed_hex, &account_id));
    }

    for (int i = 0; i < threads; i++) {
        vpThreads[i]->join();
    }
   
    for (int i = 0; i < threads; i++) {
        delete vpThreads[i];
    }
 
    cout << endl << "  master seed:     " << master_seed;
    cout << endl << "  master seed hex: " << master_seed_hex;
    cout << endl << "  account id:      " << account_id << endl << endl;

    return 0;
}
