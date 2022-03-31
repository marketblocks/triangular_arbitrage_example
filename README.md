# Triangular Arbitrage Example
An automated triangular arbitrage bot built using the marketblocks library.

## What is marketblocks?
marketblocks is a high performance C++ library that provides an abstract interface for interacting with cryptocurrency exchanges. It also provides an extensive toolset and a robust framework for the rapid development and testing of automated cryptocurrency trading bots. 

### Key Features
marketblocks boasts the following key features:
  - A consistent and predictable API for interacting with any exchange
  - All common REST API endpoints
  - Order Book websocket stream subscriptions
  - Live and Live-Test run modes allowing the strategy to be tested in real time with zero risk
  - Built-in logging framework
  - Extensive unit and integration test coverage

### Supported Exchanges
The list of supported exchanges is growing every day and marketblocks already supports:
  - Kraken
  - Coinbase Pro

## Why use marketblocks?
Here are the ***top 5*** reasons to use marketblocks:

  #### 1. Exchange Independence
The exchange API abstraction layer in marketblocks means that your trading bots can be developed without coupling to a specific exchange. This means that the exact same code required to execute your strategy on one exchange can be used to execute your strategy on any other exchange supported by marketblocks. 

  #### 2. No Heavy Lifting
The marketblocks framework takes care of everything that goes on behind the scenes so the developer only has to worry about implementing the logic for your trading strategy. Loading config files, HTTP and websocket protocol requests, error handling and recovery, logging, even maintaining a local order book from a websocket stream is all handled internally.

  #### 3. Extensive Functional Library
Not only does the developer have access to the backend framework that runs your algorithm, they will also have an arsenal of common mathematical, financial and general utility functions at their disposible, further reducing development time that would otherwise need to be spent on unimportant details.
  
  #### 4. Live Strategy Testing
Easily switch between 'Live' and 'Live-Test' modes from a config file so that you can test your strategy in real time with zero financial risk. This takes advantage of marketblocks' built-in paper trading system which simulates all your orders and maintains your virtual balances.
  
  #### 5. Test Coverage
All of marketblocks' features are extensively unit tested both through "happy path" scenarios and complex edge cases. All of the exchange REST API endpoints are also covered by automated integration tests that ensures a valid response is received when called with real account parameters.  

## How to use this example
