#pragma once

#include "TradeInfo.h"

class Trade
{
public:
    Trade(const TradeInfo& bidTrade, const TradeInfo& askTrade)
        : bidTrade_{ bidTrade }
        , askTrade_{ askTrade }
    { }

    Trade(const TradeInfo& bidTrade, const TradeInfo& askTrade, const Price transactionPrice)
        : bidTrade_{ bidTrade }
        , askTrade_{ askTrade }
        , transactionPrice_{ transactionPrice }
    { }

    const TradeInfo& GetBidTrade() const { return bidTrade_; }
    const TradeInfo& GetAskTrade() const { return askTrade_; }
    const int GetBuyerID() const { return bidTrade_.traderID_; }
    const int GetSellerID() const { return askTrade_.traderID_;}
    const int GetTransactionPrice() const { return transactionPrice_; }

private:
    TradeInfo bidTrade_;
    TradeInfo askTrade_;
    Price transactionPrice_{};
};

using Trades = std::vector<Trade>;
