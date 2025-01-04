#include <vector>
#include <string>
#include <unordered_map>
#include "../include/exchange.h" 
#include "../include/orderBook.h"
#include "../inja/inja.hpp" 

using namespace std;
using json = nlohmann::json;
// NEED TO FIX THIS TO TRACK ORDER OWNERS AND ENSURE YOU DONT TRADE WITH YOURSELF

static int OrderID = 1; // Static counter for unique order IDs

void Exchange::placeOrder(const int contract, const string& orderType, const string& order_side, double price, int quantity, int traderID) {
    Side side = (order_side == "buy") ? Side::Buy : Side::Sell;
    OrderType type = (orderType == "limit") ? OrderType::GoodTillCancel : OrderType::Market;

    OrderPointer order = make_shared<Order>(type, OrderID++, side, price, quantity, contract, traderID);
    traders[traderID].addOrderToTrader(order);

    // Add the order to the Orderbook
    Trades trades = orderBooks[contract]->AddOrder(order);

    for (const auto& trade : trades) {
        // Get the buyer and seller from our map:
        int buyerId = trade.GetBuyerID();
        int sellerId = trade.GetSellerID();

        json tradeDetails;
        tradeDetails["buyerTraderID"] = buyerId;
        tradeDetails["sellerTraderID"] = sellerId;
        tradeDetails["contract"] = contract; // If trade occured has to have been on contract submitted on
        tradeDetails["buy"] = {
            {"price", trade.GetBidTrade().price_},
            {"quantity", trade.GetBidTrade().quantity_}
        };
        tradeDetails["sell"] = {
            {"price", trade.GetAskTrade().price_},
            {"quantity", trade.GetAskTrade().quantity_}
        };

        json completedJson;
        std::ifstream inFile("completedTrades.json");
        if (inFile.is_open()) {
            try {
                inFile >> completedJson;
            } catch (...) {
                completedJson = json::array();
            }
            inFile.close();
        } else { completedJson = json::array(); }
        completedJson.push_back(tradeDetails);
        std::ofstream outFile("completedTrades.json");
        if (!outFile.is_open()) {
            // Handle error if needed
            return;
        }
        // Pretty-print with indentation of 4 spaces
        outFile << completedJson.dump(4) << std::endl;
        outFile.close();
    }

    size_t updatedSize = orderBooks[contract]->Size();
    cout << "Orderbook size updated to: " << updatedSize << endl;
}

void Exchange::calculateTradersPnl() {
        ifstream inFile("completedTrades.json");
        if (!inFile.is_open()) {
            cerr << "Unable to open completed.json file" << endl;
            return;
        }

        json completedTrades;
        inFile >> completedTrades;
        inFile.close();

        vector<double> contractPrices(this->getContractSettlements());

        for (const auto& tradeInfo : completedTrades) {
            int buyerId = tradeInfo["buyerTraderID"];
            int sellerId = tradeInfo["sellerTraderID"];
            int contract = tradeInfo["contract"];

            double buyPrice = tradeInfo["buy"]["price"];
            double sellPrice = tradeInfo["sell"]["price"];

            int buyQuantity = tradeInfo["buy"]["quantity"];
            int sellQuantity = tradeInfo["sell"]["quantity"];

            double buyerPnl = (contractPrices[contract] - buyPrice) * buyQuantity;
            double sellerPnl = (sellPrice - contractPrices[contract]) * sellQuantity;

            traders[buyerId].addProfit(buyerPnl);
            traders[sellerId].addProfit(sellerPnl);
        }

        for (Trader t : this->getTraders()) {
            cout << "Trader: " << t.getName() << " PnL: " << t.getPnl() << endl;
        }
}