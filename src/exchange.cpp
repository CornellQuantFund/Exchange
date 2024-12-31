#include <vector>
#include <string>
#include "../include/exchange.h" 
#include "../include/orderBook.h"

using namespace std;

static int OrderID = 1; // Static counter for unique order IDs

void Exchange::placeOrder(const int contract, const string& orderType, const string& order_side, double price, int quantity, int traderID) {
    Side side = (order_side == "buy") ? Side::Buy : Side::Sell;
    OrderType type = (orderType == "limit") ? OrderType::GoodTillCancel : OrderType::Market;

    OrderPointer order = make_shared<Order>(type, OrderID++, side, price, quantity, contract);
    traders[traderID].addOrderToTrader(order);

    // Add the order to the Orderbook
    Trades trades = orderBooks[contract]->AddOrder(order);

    for (const auto& trade : trades) {
        cout << "Trader " << traderID << " bought contract " << contract 
             << " at a price of " << trade.GetBidTrade().price_ 
             << " with a size of " << trade.GetBidTrade().quantity_ << endl;

        cout << "Trader " << traderID << " sold contract " << contract 
             << " at a price of " << trade.GetAskTrade().price_ 
             << " with a size of " << trade.GetAskTrade().quantity_ << endl;
    }

    size_t updatedSize = orderBooks[contract]->Size();
    cout << "Orderbook size updated to: " << updatedSize << endl;
}