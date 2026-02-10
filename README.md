# SilverBarter - DayZ Barter Trading System

A currency-free barter trading system for DayZ. Players trade items directly with NPC traders who maintain their own dynamic inventories.

---

## Background & Credits

This project is based on the work of **Terje Bruoygard** ([GitHub](https://github.com/TerjeBruoygard)) and his **Syberia Project**. The original barter trader was part of his vision for a more immersive DayZ experience.

Unfortunately, the Syberia Project was discontinued some time ago, and due to recent disagreements within the DayZ modding community, Terje's planned trader mod for his newer projects will no longer be released.

Many players - myself included - missed the barter trader dearly. It was one of those features that just *felt right* in DayZ: no magic currency, no economy exploits, just pure item-for-item trading with dynamic supply and demand.

That's why I took it upon myself to bring it back, making it as stable and robust as possible. I loved that trader, and I hope you will too.

**A heartfelt thank you to Terje for the original inspiration and codebase.**

---

## Table of Contents

- [Background & Credits](#background--credits)
- [System Overview](#system-overview)
- [How It Works](#how-it-works)
- [Price Calculation](#price-calculation)
- [Configuration Reference](#configuration-reference)
- [File Structure](#file-structure)
- [Installation](#installation)
- [Security Notes](#security-notes)
- [Rotating Pool Traders](#rotating-pool-traders)
- [ZenMap Integration](#zenmap-integration)
- [Troubleshooting](#troubleshooting)
- [Support the Project](#support-the-project)

---

## System Overview

SilverBarter implements a realistic barter economy where:

- **No currency** - Items are exchanged directly for other items
- **Dynamic stock** - Each trader maintains their own inventory that changes through player trades
- **Supply/demand pricing** - Prices fluctuate based on trader stock levels
- **Server authority** - All validation and price calculations happen server-side (anti-cheat)

### Core Concepts

| Concept | Description |
|---------|-------------|
| **Trader Stock** | Each trader has a virtual inventory. When players sell, stock increases. When players buy, stock decreases. |
| **Dumping** | Price modifier based on stock level. High stock = lower prices, low stock = higher prices. |
| **Commission** | Fee deducted when selling items to the trader. Buy price is always higher than sell price. |
| **Barter Rule** | Players must both give AND receive items. One-sided trades are blocked. |

---

## How It Works

### Trading Flow

```
1. Player interacts with Trader NPC
2. Server sends trader info + current stock to client
3. Client displays UI with available items
4. Player selects items to sell (from inventory) and buy (from trader stock)
5. Client sends trade request to server
6. Server validates:
   - Player owns the items
   - Items pass filters
   - Price balance is non-negative (sell value >= buy value)
   - Player is buying something (not just dumping items)
7. Server executes trade:
   - Deletes sold items from player
   - Updates trader stock
   - Spawns purchased items to player (inventory or ground)
8. Server persists updated stock to JSON
```

### Stock Persistence

Trader inventories are saved to `$profile:\SilverBarter\TraderData\trader_X.json` where X is the trader ID.

- Auto-save every 5 minutes
- Save on server shutdown
- Limited items reset to `maxQuantity` on every server restart

---

## Price Calculation

### Base Formula

All prices are calculated in abstract "value units" (not displayed to players).

```
BuyPrice = DumpingMultiplier * Quantity * 1000
SellPrice = BuyPrice * (1 - Commission) * ItemQuantity01 * QualityMultiplier
```

### Dumping (Supply/Demand)

The dumping system adjusts prices based on how full the trader's stock is:

```
DumpingMultiplier = Lerp(1.0, DumpingModifier, StockFillRatio)

Where:
- StockFillRatio = CurrentStock / MaxStock
- DumpingModifier = Configured value (e.g., 0.65)
```

**Example with DumpingModifier = 0.65:**

| Stock Level | Multiplier | Effect |
|-------------|------------|--------|
| 0% (empty) | 1.00 | Full price |
| 50% | 0.825 | 17.5% cheaper |
| 100% (full) | 0.65 | 35% cheaper |

### Commission

Commission is deducted when players sell items:

```
SellPrice = BuyPrice * (1 - Commission)
```

**Example with 65% commission:**
- Buy price: 1000 units
- Sell price: 1000 * (1 - 0.65) = 350 units

This creates a natural spread between buy/sell prices.

### Quality Modifier

Items with "Worn" condition receive an additional penalty:

```
If HealthLevel == WORN:
    SellPrice = SellPrice * DumpingByBadQuality
```

Items worse than "Worn" (Damaged, Badly Damaged, Ruined) cannot be sold.

### Item Quantity

Stackable items and magazines factor in their fill level:

```
ItemQuantity01 = CurrentQuantity / MaxQuantity
```

A half-full ammo box sells for half the price of a full one.

### Max Stock Per Item

Each item type has a maximum stock limit based on its inventory size:

```
MaxStock = StorageMaxSize / (ItemWidth * ItemHeight)
```

A trader with 5000 storage can hold more small items than large ones.

---

## Configuration Reference

Configuration is stored at `$profile:\SilverBarter\SilverBarterConfig.json`

### Global Settings

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `m_debugMode` | bool | false | Enable verbose logging to RPT file. Useful for troubleshooting. Disable in production to reduce log spam. |

### Trader Configuration

Each trader in the `m_traders` array supports these options:

#### Basic Settings

| Field | Type | Description |
|-------|------|-------------|
| `m_traderId` | int | Unique identifier for this trader. Must be unique across all traders. |
| `m_classname` | string | DayZ class name for the NPC model (e.g., `"SurvivorM_Mirek"`). |
| `m_position` | string | Spawn position as `"x y z"` (e.g., `"6618.94 41.62 7151.61"`). |
| `m_rotation` | float | Y-axis rotation in degrees. |

#### Economy Settings

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `m_storageMaxSize` | int | 5000 | Total virtual storage capacity in inventory slots. Determines how much stock the trader can hold. Higher = more items, slower price changes. |
| `m_storageCommission` | float | 0.65 | Default commission rate (0.0 - 1.0). Deducted from sell prices. 0.65 = 65% fee, player gets 35% of buy price when selling. |
| `m_dumpingByAmountAlgorithm` | string | "linear" | Price algorithm. Currently only `"linear"` is supported. |
| `m_dumpingByAmountModifier` | float | 0.65 | Minimum price multiplier at full stock. 0.65 = prices drop to 65% when stock is full. Lower value = more aggressive price drops. |
| `m_dumpingByBadQuality` | float | 0.5 | Price multiplier for "Worn" condition items. 0.5 = 50% of normal sell price. |
| `m_sellMaxQuantityPercent` | float | 0.8 | Maximum % of max stock a player can sell in one trade. 0.8 = can sell up to 80% of max stock per item type. |
| `m_buyMaxQuantityPercent` | float | 0.9 | Maximum % of max stock a player can buy in one trade. 0.9 = can buy up to 90% of available stock per item type. |

#### Filters

Filters control which items can be traded. They use DayZ class names and support inheritance.

| Field | Type | Description |
|-------|------|-------------|
| `m_buyFilter` | array | Items players can **buy** from this trader. |
| `m_sellFilter` | array | Items players can **sell** to this trader. |

**Filter Syntax:**

- `"ClassName"` - Allow this class and all children (inheritance)
- `"!ClassName"` - Explicitly exclude this class and children

Filters are processed in order. Later entries override earlier ones.

**Example:**
```json
"m_sellFilter": [
    "Inventory_Base",      // Allow all inventory items
    "!Container_Base",     // But exclude containers
    "FirstAidKit",         // Except FirstAidKit (re-allow)
    "!Rag"                 // Exclude rags specifically
]
```

#### Commission Overrides

Override the default commission for specific item types:

```json
"m_commissionOverrides": [
    {
        "classname": "TannedLeather",
        "commission": 0.2
    },
    {
        "classname": "Goldnugget_Base",
        "commission": 0.1
    }
]
```

Lower commission = player keeps more value when selling. Use this for rare/valuable items.

#### Limited Items

Items that reset to a fixed quantity on every server restart:

```json
"m_limitedItems": [
    {
        "classname": "ZenSkills_Book_Survival",
        "maxQuantity": 2
    }
]
```

Use this for rare items that should have controlled availability.

#### Default Items

Starting inventory when the trader is first created:

```json
"m_defaultItems": [
    {
        "classname": "Hatchet",
        "quantity": 10
    },
    {
        "classname": "BandageDressing",
        "quantity": 35
    }
]
```

Only applied if no saved trader data exists (first start).

#### NPC Appearance

| Field | Type | Description |
|-------|------|-------------|
| `m_attachments` | array | Clothing/gear class names to equip on the NPC. |

```json
"m_attachments": [
    "DownJacket_Orange",
    "Jeans_Black",
    "HikingBootsLow_Black"
]
```

---

## Rotating Pool Traders

In addition to the standard barter traders, there is a second trader type: the **Rotating Pool Trader**. This trader type has a completely separate system with the following features:

- **Rotating inventory** — A small selection of items is randomly chosen from a large pool using weighted random sampling
- **Automatic rotation** — The inventory refreshes automatically at a configurable interval (e.g. every hour)
- **Multiple spawn positions** — The trader can appear at a random position on each server restart
- **No buy-back into inventory** — Items sold to the trader are destroyed and do not appear in the trader's stock
- **No persistence** — The inventory is not saved; each rotation generates a fresh selection

### Separate Configuration File

Rotating traders are configured in their own file:

```
$profile:\SilverBarter\SilverBarterRotatingTraders.json
```

This file is automatically generated with an example trader on first server start.

### Rotating Trader Configuration

Each entry in `m_rotatingTraders` supports the following fields:

**Basic Settings:**

| Field | Type | Description |
|-------|------|-------------|
| `m_classname` | string | DayZ class name for the NPC model (e.g. `"SurvivorM_Mirek"`) |
| `m_attachments` | array | Clothing/gear class names to equip on the NPC |
| `m_spawnPositions` | array | List of possible spawn positions as `"x y z"`. A random one is chosen on each restart. |
| `m_orientation` | float | Y-axis rotation in degrees |

**Rotation Settings:**

| Field | Type | Description |
|-------|------|-------------|
| `m_rotationIntervalMinutes` | int | Interval in minutes after which the inventory is regenerated (e.g. `60`) |
| `m_activeSlots` | int | Number of items selected from the pool per rotation |

**Economy (inherited from SilverTrader_Info):**

The fields `m_storageMaxSize`, `m_storageCommission`, `m_dumpingByAmountModifier`, etc. are inherited from the standard trader and work identically.

### Pool Items

The `m_poolItems` array defines the full catalog from which items are selected via weighted random sampling on each rotation:

```json
"m_poolItems": [
    {
        "classname": "AKM",
        "quantity": 2,
        "weight": 0.3
    },
    {
        "classname": "Hatchet",
        "quantity": 10,
        "weight": 1.0
    },
    {
        "classname": "NVGoggles",
        "quantity": 1,
        "weight": 0.1
    }
]
```

| Field | Type | Description |
|-------|------|-------------|
| `classname` | string | DayZ class name of the item |
| `quantity` | int | Available quantity per rotation |
| `weight` | float | Weight for random selection. `1.0` = normal, `0.1` = very rare |

**Example:** With `m_activeSlots: 5` and 20 pool items, 5 items are randomly selected on each rotation. Items with a higher `weight` value appear more frequently.

### Example Configuration

```json
{
    "CONFIG_VERSION": "1",
    "m_rotatingTraders": [
        {
            "m_classname": "SurvivorM_Mirek",
            "m_attachments": ["GorkaHelmet", "TTSKOJacket_Camo", "TTSKOPants_Camo"],
            "m_spawnPositions": ["6618.94 41.62 7151.61", "7200.00 300.00 3100.00"],
            "m_orientation": 180.0,
            "m_rotationIntervalMinutes": 60,
            "m_activeSlots": 5,
            "m_storageMaxSize": 3000,
            "m_storageCommission": 0.65,
            "m_poolItems": [
                { "classname": "AKM", "quantity": 2, "weight": 0.3 },
                { "classname": "Hatchet", "quantity": 10, "weight": 1.0 },
                { "classname": "BandageDressing", "quantity": 20, "weight": 1.0 },
                { "classname": "NVGoggles", "quantity": 1, "weight": 0.1 }
            ],
            "m_buyFilter": ["Weapon_Base", "Clothing_Base"],
            "m_sellFilter": ["Inventory_Base"]
        }
    ]
}
```

---

## ZenMap Integration

SilverBarter optionally supports displaying trader markers on the ZenMap map. This feature requires the **ZenMap** mod to be installed.

### Activation (Rotating Traders only)

The following fields are available in the rotating trader config:

| Field | Type | Description |
|-------|------|-------------|
| `m_enableZenMapMarker` | bool | `true` = show marker on the ZenMap map |
| `m_zenMapMarkerName` | string | Display name of the marker (e.g. `"Wandering Trader"`) |
| `m_zenMapMarkerIcon` | string | Icon path for the marker (empty = default icon) |

```json
{
    "m_enableZenMapMarker": true,
    "m_zenMapMarkerName": "Wandering Trader",
    "m_zenMapMarkerIcon": ""
}
```

If ZenMap is not installed, these fields are simply ignored.

---

## File Structure

```
SilverBarter/
├── scripts/
│   ├── 3_Game/
│   │   ├── Constants.c              # RPC channel constants
│   │   └── SilverBarterConfig.c     # Config classes and JSON handling
│   ├── 4_World/
│   │   ├── Classes/
│   │   │   └── UserActionsComponent/
│   │   │       ├── Actions/Interact/
│   │   │       │   └── ActionTraderInteract.c  # Player interaction action
│   │   │       └── SilverActionConstructor.c   # Action registration
│   │   ├── Entities/
│   │   │   ├── ManBase/
│   │   │   │   └── PlayerBase.c     # Player class extension
│   │   │   └── Trading/
│   │   │       └── TraderPoint.c    # Invisible interaction trigger
│   │   ├── GUI/
│   │   │   └── SilverTraderMenu.c   # Trading UI
│   │   ├── Plugins/
│   │   │   ├── PluginManager.c      # Plugin registration
│   │   │   └── PluginBase/
│   │   │       └── PluginSilverTrader.c  # Main plugin (RPC, validation, trading)
│   │   └── SilverRPC.c              # RPC handler
│   └── 5_Mission/
│       └── mission/
│           ├── SilverMissionServer.c    # Server mission hooks
│           └── SilverMissionGameplay.c  # Client mission hooks
└── README.md
```

### Runtime Data (Server)

```
$profile:\SilverBarter\
├── SilverBarterConfig.json              # Main configuration (standard traders)
├── SilverBarterRotatingTraders.json     # Configuration (rotating traders)
└── TraderData/
    ├── trader_0.json                    # Stock data for trader ID 0
    ├── trader_1.json                    # Stock data for trader ID 1
    └── ...                              # (Rotating traders have no persistence)
```

---

## Installation

1. Copy the `SilverBarter` folder to your server's mod directory
2. Add `SilverBarter` to your server's `-mod=` parameter
3. Start the server - default configs will be generated:
   - `$profile:\SilverBarter\SilverBarterConfig.json` (standard traders)
   - `$profile:\SilverBarter\SilverBarterRotatingTraders.json` (rotating traders)
4. Edit the configs to set up your traders (positions, filters, stock, etc.)
5. Restart the server

### Requirements

- DayZ Server (compatible with current stable version)
- No additional dependencies

---

## Security Notes

The system implements several anti-exploit measures:

- **Server authority**: All price calculations and validations happen server-side
- **Ownership checks**: Players can only sell items they actually own
- **Distance checks**: Players must be within 5m of the trader
- **Barter rule**: Players cannot dump items without receiving something
- **Quantity caps**: Maximum 10 different item types and 50 items per type per transaction
- **Filter validation**: Items must pass both buy and sell filters
- **Stock validation**: Cannot buy more than trader has in stock

---

## Troubleshooting

### Logs

All operations are logged with `[SilverBarter]` prefix. Check your server logs for:

- `Trade abgelehnt: Negativer Preis` - Player tried to buy more than they can afford
- `Trade abgelehnt: Verkauf ohne Gegenkauf` - Player tried to sell without buying anything
- `ERROR: Config nicht geladen` - Configuration file missing or invalid

### Common Issues

| Issue | Solution |
|-------|----------|
| Trader doesn't spawn | Check `m_position` coordinates and `m_classname` validity |
| Can't sell item | Check `m_sellFilter` - item may be excluded |
| Can't buy item | Check `m_buyFilter` and verify trader has stock |
| Prices seem wrong | Adjust `m_dumpingByAmountModifier` and `m_storageCommission` |
| Stock resets unexpectedly | Check `m_limitedItems` - may be configured to reset |

---

## Support the Project

If you enjoy SilverBarter and want to support its continued development, consider buying me a coffee or making a donation. Every contribution helps keep this project alive and motivates further improvements.

- [Buy Me a Coffee](https://buymeacoffee.com/jcdmh)

Thank you for your support!
