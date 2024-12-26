#include <string>

using namespace std;

class Order {
    enum { BUY, SELL } position;
    enum { MARKET, LIMIT } type;

    public:
        void placeOrder();

    private:
        void cancelOrder();
};