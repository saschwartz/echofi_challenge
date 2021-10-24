# echofi_challenge
EchoFi Coding Challenge

This repo is an implementation of the EchoFi coding challenge, which asks for an interface that can be used to place buy and sell orders for stock tickers, as well as retrieve a transaction history and a current portfolio state.

## Overview

### Interface

This implementation provides an interface `BrokerClient`, which is initialized with some initial cash balance. The main interface methods are:
- `SubmitOrder`, through which a user submits a buy or sell order for a stock for some price and returns the amount of stock actually bought or sold
- `GetTransactions`, which returns a list of processed orders (not necessarily the same orders as submitted, if they exceed the amount of cash available or stock owned.
- `GetPositions`, which returns the net current portfolio positions for each ticker.
- `GetCashBalance`, which returns the user's remaining cash balance.

### Design

This implementation makes the decision to track a weighted average cost basis for each security, which informs the data structures chosen for the rest of the implementation. We maintain:

- A map of stock name (a unique identifier) into current position for that stock (this represents the portfolio).
- A map of stock name into a FIFO queue of "current" buy orders for that stock (i.e. buy orders used for calculating the current cost basis).
- A vector of completed buy transactions.

These structures allow the following algorithmic complexity for each method:

- `GetTransactions` is `O(1)`, simply returning the transaction vector. As long as the user just takes a reference this is zero-copy so truly `O(1)`. 
- `GetPositions`, which must loop over the existing map of positions to create a vector from the portfolio map, so is `O(n)` in terms of `n` stocks held. This could be made `O(1)` if the portfolio map is just returned directly, but it is assumed the user will only hold maximally a few hundred stocks.
- `GetCashBalance` is `O(1)`, just returning an instance variable.


`SubmitOrder` warrants some additional discussion.

In the case of a `Buy` order, we simply recompute weighted average cost basis using the new order, which is a single double-size floating point operation. We can then lookup and update the necessary portfolio position and update it in `O(1)`, and push the newly processed transaction to the current buy order queue and transaction queue in `O(1)` each. So buy orders are `O(1)`.

In the case of a `Sell` order, the weighted average cost basis calculation can theoretically be more expensive than `Buy`, given how it is implemented. The way that we implement recomputing weighted average, is to continually `pop` orders from the current buy order queue for the security (to the total order size) while calculating some `totalValueRemoved` parameter, so that we may then compute `(totalValue - totalValueRemoved) / (totalQuantity - QuantitySold)` to get the new weighted average. In the worst case, this could pop all the buy orders in the queue. So this has a theoretical worst case of `O(n)` where `n` is the number of current buy orders. This could end up being quite an expensive computation, and so a future design improvement would be to consider running the cost basis calculation in a separate thread while returning from the order processing immediately, with the consequence that `GetPositions` might return a stale cost basis until the computation is complete.





