#include <vector>
#include <map>
#include <string>
#include "orderBook.h" 
#include "trader.h"

using namespace std;

class Exchange {
    unordered_map<int, Trader> traders;
    vector<shared_ptr<Orderbook>> orderBooks; // Use shared_ptr
    vector<double> contractSettlements;

    public:

        Exchange(int numOrderBooks) {
            for (int i = 0; i < numOrderBooks; ++i) {
                orderBooks.push_back(make_shared<Orderbook>());
                contractSettlements.push_back(100); // FOR TESTING ONLY
            }
            // contractSettlements.resize(numOrderBooks);
        }

        void addContractSettlement(int contract, double settlePrice) { contractSettlements[contract] = settlePrice; }
        void addContractSettlement(vector<double>& settlePrices) {
            if (settlePrices.size() != contractSettlements.size()) {
                throw invalid_argument("Invalid number of settlement prices");
            }
            copy(settlePrices.begin(), settlePrices.end(), contractSettlements.begin());
        }

        void addTrader(Trader trader) {
            int id = trader.getId();
            traders[id] = trader;
        }

        vector<shared_ptr<Orderbook>>& getOrderBooks() { return orderBooks; }
        vector<double>& getContractSettlements() { return contractSettlements; }
        Orderbook& getOrderBook(int index) {
            if (index < 0 || index >= orderBooks.size()) {
                throw out_of_range("Invalid order book index");
            }
            return *orderBooks[index];
        }
        int getNumOrderBooks() { return orderBooks.size(); }
        Trader getTrader(int id) { return traders[id]; }

        vector<Trader> getTraders() {
            vector<Trader> traderList;
            for (auto& [id, trader] : traders) { traderList.push_back(trader); }
            return traderList;
        }

        // Place an order with multiple orderBooks active
        void placeOrder(const int contract, const string& orderType, const string& order_side, double price, int quantity, int traderID);

        void cancelOrder(int contract, OrderId orderId, int traderID) {
            if (contract < 0 || contract >= orderBooks.size()) {
                throw out_of_range("Invalid contract index");
            }
            getOrderBook(contract).CancelOrder(orderId);
            // getTrader(traderID).cancelOrder(orderId);
        }

        void calculateTradersPnl();
};;
