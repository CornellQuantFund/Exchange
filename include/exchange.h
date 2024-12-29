#include <vector>
#include <map>
#include <string>
#include "orderBook.h" 
#include "trader.h"

using namespace std;

class Exchange {
    unordered_map<int, Trader> traders;
    vector<shared_ptr<Orderbook>> orderBooks; // Use shared_ptr

    public:

        Exchange(int numOrderBooks) {
            for (int i = 0; i < numOrderBooks; ++i) {
                orderBooks.push_back(make_shared<Orderbook>());
            }
        }

        void addTrader(Trader trader) {
            int id = trader.getId();
            traders[id] = trader;
        }

        vector<shared_ptr<Orderbook>>& getOrderBooks() {
            return orderBooks;
        }

        Orderbook& getOrderBook(int index) {
            if (index < 0 || index >= orderBooks.size()) {
                throw out_of_range("Invalid order book index");
            }
            return *orderBooks[index];
        }

        int getNumOrderBooks() { return orderBooks.size(); }

        // Place an order with multiple orderBooks active
        void placeOrder(const int contract, const string& orderType, const string& order_side, double price, int quantity, int traderID);

        Trader getTrader(int id) { return traders[id]; }
};;
