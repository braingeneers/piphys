#include <atomic>

using namespace std;

class Redisqueue{
public:
    vector<char> mydata;
    int count = 1;
    bool stopQ = false;
    
    Redisqueue();
    int pushData(const char &data);
    void consumeAll(int num);
    
private:
    atomic<int>ano;

};




