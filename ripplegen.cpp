#include <RippleAddress.h>
#include <iostream>
#include <stdint.h>

using namespace std;

int main(int argc, char* argv[])
{
    if (argc < 2) {
        cout << endl << "Usage: " << argv[0] << " [pattern]" << endl << endl;
        return 0;
    }

    string pattern = argv[1];
    uint64_t count = 0;

    cout << endl << "Generating seed...";
    cout << endl << "  pattern: " << pattern << endl << endl;

    RippleAddress naSeed;
    RippleAddress naAccount;
    string        account_id;

    uint128 key;
    getRand(key.begin(), key.size());

    do {
        naSeed.setSeed(key);
        RippleAddress naGenerator = RippleAddress::createGeneratorPublic(naSeed);
        naAccount.setAccountPublic(naGenerator, 0);
        account_id = naAccount.humanAccountID();
        count++;
        if (count % 10000 == 0)
            cout << "  Tested " << count << " seeds so far." << endl;
        key++;
    } while (account_id.substr(0, pattern.size()) != pattern);

    cout << endl << endl << "Done!";
    cout << endl << "  master seed:     " << naSeed.humanSeed();
    cout << endl << "  master seed hex: " << naSeed.getSeed().ToString();
    cout << endl << "  account id:      " << account_id << endl << endl;

    return 0;
}
