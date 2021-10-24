/**
 * @file brokerClient.hpp
 *
 * File containing the implementation of the BrokerClient interface.
 */

#include "BrokerClient.hpp"
#include <iostream>

BrokerClient::BrokerClient(double cashBalance) : cashBalance_(cashBalance) {}

uint32_t BrokerClient::SubmitOrder(Order order) {
  // Return value will be stored in here.
  uint32_t quantityTransacted = 0;

  // If we end up making a transaction, it will be stored in here.
  Order completedOrder = {};

  switch (order.kind) {
  case Buy: {
    // Don't overdraw our cash balance on a buy order.
    quantityTransacted = std::min((double)order.position.quantity,
                                  GetCashBalance() / order.position.price);
    if (quantityTransacted == 0) {
      break;
    }

    completedOrder.kind = order.kind;
    SecurityPosition position = {.name = order.position.name,
                                 .quantity = quantityTransacted,
                                 .price = order.position.price};
    completedOrder.position = position;

    // Update internal state from the newly processed order.
    HandleBuy(completedOrder);
    break;
  }
  case Sell: {
    // Don't sell shared we don't have.
    if (portfolio_.find(order.position.name) == portfolio_.end()) {
      quantityTransacted = 0;
      break;
    } else {
      quantityTransacted = std::min(order.position.quantity,
                                    portfolio_[order.position.name].quantity);
    }

    completedOrder.kind = order.kind;
    SecurityPosition position = {.name = order.position.name,
                                 .quantity = quantityTransacted,
                                 .price = order.position.price};
    completedOrder.position = position;

    // Update internal state from the newly processed order.
    HandleSell(completedOrder);
    break;
  }
  }
  return quantityTransacted;
}

void BrokerClient::HandleBuy(Order order) {
  assert(order.kind == Buy);

  /*
   * Update portfolio, either adding a new entry or re-computing the existing
   * entry.
   */
  if (portfolio_.find(order.position.name) != portfolio_.end()) {
    SecurityPosition &position = portfolio_[order.position.name];
    /*
     * If we already own some of this security, we calculate the portfolio
     * price as the average buy price across all buy orders (a weighted
     * average on a per-share basis)
     */
    double weightedPrice =
        ((double)((order.position.quantity * order.position.price) +
                  (position.quantity * position.price)) /
         (double)(order.position.quantity + position.quantity));
    position.quantity += order.position.quantity;
    position.price = weightedPrice;
  } else {
    SecurityPosition position = order.position;
    portfolio_.insert(std::make_pair(order.position.name, position));
  }

  /*
   * Insert the order into the specific outstanding buy order queue for the
   * given security.
   */
  if (outstandingBuyOrderMap_.find(order.position.name) ==
      outstandingBuyOrderMap_.end()) {
    std::queue<Order> queue;
    outstandingBuyOrderMap_.insert(std::make_pair(order.position.name, queue));
  }
  outstandingBuyOrderMap_[order.position.name].push(order);

  // Decrease cash by the amount we purchased.
  cashBalance_ -= order.position.price * order.position.quantity;
  transactions_.push_back(order);
}

void BrokerClient::HandleSell(Order order) {
  assert(order.kind == Sell);

  /*
   * Update the buy orders from which we calculate the current weighted
   * average cost basis (price) for the given security. This is done by
   * removing buy orders from a FIFO queue, until we've removed as many
   * shares worth of buy orders as we are selling in this transaction.
   */
  double buyValueRemoved = 0;
  uint32_t buyQuantityRemoved = 0;
  std::queue<Order> &buyOrderQueue =
      outstandingBuyOrderMap_[order.position.name];
  while (buyQuantityRemoved < order.position.quantity) {
    Order &buyOrder = buyOrderQueue.front();

    // Either consume a complete or partial buy order.
    if ((order.position.quantity - buyQuantityRemoved) >=
        buyOrder.position.quantity) {
      buyQuantityRemoved += buyOrder.position.quantity;
      buyValueRemoved += buyOrder.position.quantity * buyOrder.position.price;
      buyOrderQueue.pop();
    } else {
      buyOrder.position.quantity -=
          (order.position.quantity - buyQuantityRemoved);
      buyValueRemoved += ((order.position.quantity - buyQuantityRemoved) *
                          buyOrder.position.price);
      buyQuantityRemoved = order.position.quantity;
    }
  }

  /*
   * Recompute the portfolio weighted average price for this security. This
   * is done by computing (newValueTotal / newQuantityTotal), which we can
   * because we know the old value and quantity, and how much value we just
   * removed from the current buy order queue above.
   */
  SecurityPosition &position = portfolio_[order.position.name];
  position.price =
      ((double)((position.price * position.quantity) - buyValueRemoved) /
       (double)(position.quantity - order.position.quantity));
  position.quantity -= order.position.quantity;

  // If we've sold everything, remove the security from the map.
  if (position.quantity == 0) {
    auto it = portfolio_.find(order.position.name);
    portfolio_.erase(it);
  }

  // Increase cash by the amount we sold.
  cashBalance_ += order.position.price * order.position.quantity;
  transactions_.push_back(order);
}

std::vector<SecurityPosition> BrokerClient::GetPositions() {
  std::vector<SecurityPosition> positions;
  for (auto it = portfolio_.begin(); it != portfolio_.end(); it++) {
    positions.push_back(it->second);
  }
  return positions;
}
