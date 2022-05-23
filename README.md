# Triangular Arbitrage Example

### Reading the Code
The core algorithm is contained within `triangular_arbitrage.cpp`. 

See `triangular_arbitrage::intialise(const strategy_initialiser&)` for the initialisation stage of the strategy. This is where all the possible triangular sequences are pre-computed and cached.

See `triangular_arbitrage::run_iteration()` for the active stage of the strategy. This is where any order book updates are read and the associated sequences are checked for potential arbitrage opportunities.

All marketblocks types and functions are located within the `mb` namespace.

### Running the Example Build

An example build has been provided in the `build` folder. Simply run `triangular_arbitrage_example.exe` to begin checking for triangular arbitrage opportunities with the default configurations.

Various parameters may be adjusted through the config files located inside the `configs` folder. All config files are JSON encoded.

<details><summary>runner.json</summary>

- `exchangeIds` - Specifies which exchanges to run the strategy on. Specifying an empty array will use all supported exchanges.
- `httpTimeout` - Specifies the timeout for HTTP requests in ms. A value of 0 disables the timeout.
- `runMode` - Sets the run mode. Valid options are `"live"`, `"live_test"` or `"back_test"`
- `websocketTimeout` - Specifies the timeout for the websocket connection handshake in ms. A value of 0 disables the timeout.
  
</details>
<details><summary>paper_trading.json</summary>
  
Contains parameters used by the trading simulator when the Live-Test run mode is enabled
  
  - `balances` - Initial virtual balances
  - `fee` - Simulated fee to use when executing paper trades
  
</details>

<details><summary>kraken.json</summary>
  
Contains parameters required to access authenticated endpoints when using the Kraken exchange
  
</details>

<details><summary>coinbase.json</summary>
  
Contains parameters required to access authenticated endpoints when using the Coinbase exchange
  
</details>

