// SilverBarter Haupt-Konfiguration
class SilverBarterConfig
{
	// Config-Pfade
	private const static string MOD_FOLDER = "$profile:\\SilverBarter\\";
	private const static string CONFIG_NAME = "SilverBarterConfig.json";
	private const static string CURRENT_VERSION = "1";

	string CONFIG_VERSION;

	// Globale Einstellungen
	bool m_debugMode;

	// Trader-Konfiguration
	ref array<ref SilverTrader_ServerConfig> m_traders;

	void SilverBarterConfig()
	{
		m_traders = new array<ref SilverTrader_ServerConfig>;
	}

	void ~SilverBarterConfig()
	{
		if (m_traders)
		{
			m_traders.Clear();
			delete m_traders;
		}
	}

	void Load()
	{
		if (!g_Game.IsDedicatedServer())
			return;

		if (!FileExist(MOD_FOLDER))
		{
			MakeDirectory(MOD_FOLDER);
		}

		string path = MOD_FOLDER + CONFIG_NAME;
		if (FileExist(path))
		{
			JsonFileLoader<SilverBarterConfig>.JsonLoadFile(path, this);

			if (CONFIG_VERSION != CURRENT_VERSION)
			{
				JsonFileLoader<SilverBarterConfig>.JsonSaveFile(path + "_backup", this);
				Migrate();
				CONFIG_VERSION = CURRENT_VERSION;
				Save();
			}
		}
		else
		{
			SetDefaultValues();
			CONFIG_VERSION = CURRENT_VERSION;
			Save();
		}
	}

	private void Migrate()
	{
		// Keine Migrationen aktuell noetig
	}

	void Save()
	{
		if (!g_Game.IsDedicatedServer())
			return;

		if (!FileExist(MOD_FOLDER))
		{
			MakeDirectory(MOD_FOLDER);
		}

		JsonFileLoader<SilverBarterConfig>.JsonSaveFile(MOD_FOLDER + CONFIG_NAME, this);
	}

	void SetDefaultValues()
	{
		m_debugMode = false;
		m_traders = new array<ref SilverTrader_ServerConfig>;

		// Beispiel-Trader
		SilverTrader_ServerConfig exampleTrader = new SilverTrader_ServerConfig();
		exampleTrader.m_traderId = 0;
		exampleTrader.m_classname = "SurvivorM_Mirek";
		exampleTrader.m_position = "6618.94 41.62 7151.61";
		exampleTrader.m_orientation = -43.268715;
		exampleTrader.m_storageMaxSize = 5000;
		exampleTrader.m_storageCommission = 0.65;
		exampleTrader.m_dumpingByAmountAlgorithm = "linear";
		exampleTrader.m_dumpingByAmountModifier = 0.65;
		exampleTrader.m_dumpingByBadQuality = 0.5;
		exampleTrader.m_sellMaxQuantityPercent = 0.8;
		exampleTrader.m_buyMaxQuantityPercent = 0.9;

		exampleTrader.m_buyFilter = new array<string>;
		exampleTrader.m_buyFilter.Insert("Inventory_Base");
		exampleTrader.m_buyFilter.Insert("Weapon_Base");
		exampleTrader.m_buyFilter.Insert("Magazine_Base");
		exampleTrader.m_buyFilter.Insert("Ammunition_Base");
		exampleTrader.m_buyFilter.Insert("Box_Base");

		exampleTrader.m_sellFilter = new array<string>;
		exampleTrader.m_sellFilter.Insert("Inventory_Base");
		exampleTrader.m_sellFilter.Insert("Weapon_Base");
		exampleTrader.m_sellFilter.Insert("Magazine_Base");
		exampleTrader.m_sellFilter.Insert("Ammunition_Base");
		exampleTrader.m_sellFilter.Insert("!Container_Base");
		exampleTrader.m_sellFilter.Insert("Box_Base");
		exampleTrader.m_sellFilter.Insert("FirstAidKit");
		exampleTrader.m_sellFilter.Insert("SmallProtectorCase");
		exampleTrader.m_sellFilter.Insert("PlateCarrierPouches");
		exampleTrader.m_sellFilter.Insert("!SyringeFull");
		exampleTrader.m_sellFilter.Insert("!Paper");
		exampleTrader.m_sellFilter.Insert("!Zen_EmptyFood");
		exampleTrader.m_sellFilter.Insert("!FenceKit");
		exampleTrader.m_sellFilter.Insert("!WatchtowerKit");
		exampleTrader.m_sellFilter.Insert("!ShelterKit");
		exampleTrader.m_sellFilter.Insert("!TerritoryFlagKit");
		exampleTrader.m_sellFilter.Insert("!Flag_Base");
		exampleTrader.m_sellFilter.Insert("!Rag");
		exampleTrader.m_sellFilter.Insert("!BurlapStrip");
		exampleTrader.m_sellFilter.Insert("!Stone");
		exampleTrader.m_sellFilter.Insert("!SmallStone");
		exampleTrader.m_sellFilter.Insert("!Firewood");
		exampleTrader.m_sellFilter.Insert("!BoneHook");
		exampleTrader.m_sellFilter.Insert("!WoodenHook");
		exampleTrader.m_sellFilter.Insert("!Bark_ColorBase");
		exampleTrader.m_sellFilter.Insert("!Bone");
		exampleTrader.m_sellFilter.Insert("!Bait");
		exampleTrader.m_sellFilter.Insert("!BoneBait");
		exampleTrader.m_sellFilter.Insert("!Barrel_ColorBase");
		exampleTrader.m_sellFilter.Insert("!FireplaceBase");
		exampleTrader.m_sellFilter.Insert("!CookingStand");
		exampleTrader.m_sellFilter.Insert("!WoodenStick");
		exampleTrader.m_sellFilter.Insert("!Torch");
		exampleTrader.m_sellFilter.Insert("!LongWoodenStick");
		exampleTrader.m_sellFilter.Insert("!SharpWoodenStick");
		exampleTrader.m_sellFilter.Insert("!HandDrillKit");
		exampleTrader.m_sellFilter.Insert("!Spear");
		exampleTrader.m_sellFilter.Insert("!SmallGuts");
		exampleTrader.m_sellFilter.Insert("!Guts");
		exampleTrader.m_sellFilter.Insert("!Worm");
		exampleTrader.m_sellFilter.Insert("!ImprovisedFishingRod");
		exampleTrader.m_sellFilter.Insert("!TripwireTrap");
		exampleTrader.m_sellFilter.Insert("!RabbitSnareTrap");
		exampleTrader.m_sellFilter.Insert("!FishNetTrap");
		exampleTrader.m_sellFilter.Insert("!SmallFishTrap");
		exampleTrader.m_sellFilter.Insert("!Empty_ZenJameson");
		exampleTrader.m_sellFilter.Insert("!Zen_CamoShelterKit");
		exampleTrader.m_sellFilter.Insert("!Empty_Honey");
		exampleTrader.m_sellFilter.Insert("!Empty_Marmalade");
		exampleTrader.m_sellFilter.Insert("!Empty_Can_Opened");
		exampleTrader.m_sellFilter.Insert("!BF_DoorBarricadeKit");
		exampleTrader.m_sellFilter.Insert("!BF_WindowBarricadeKit");
		exampleTrader.m_sellFilter.Insert("!BF_WindowBarricadeMedKit");
		exampleTrader.m_sellFilter.Insert("!BF_DoubleDoorBarricadeKit");
		exampleTrader.m_sellFilter.Insert("!bl_improvised_whetstone");
		exampleTrader.m_sellFilter.Insert("!bl_improvised_sewing_kit");
		exampleTrader.m_sellFilter.Insert("!Single_Match");

		exampleTrader.m_attachments = new array<string>;
		exampleTrader.m_attachments.Insert("DownJacket_Orange");
		exampleTrader.m_attachments.Insert("Jeans_Black");
		exampleTrader.m_attachments.Insert("Slingbag_Black");
		exampleTrader.m_attachments.Insert("WoolGloves_Black");
		exampleTrader.m_attachments.Insert("BeanieHat_Black");
		exampleTrader.m_attachments.Insert("HikingBootsLow_Black");
		exampleTrader.m_attachments.Insert("Shemag_Green");

		// Item-spezifische Commission-Overrides (wertvolle/seltene Items)
		exampleTrader.m_commissionOverrides = new array<ref SilverCommissionOverride>;

		SilverCommissionOverride leatherOverride = new SilverCommissionOverride();
		leatherOverride.classname = "TannedLeather";
		leatherOverride.commission = 0.2;
		exampleTrader.m_commissionOverrides.Insert(leatherOverride);

		// Limitierte Items (werden bei jedem Restart auf maxQuantity zurueckgesetzt)
		exampleTrader.m_limitedItems = new array<ref SilverTrader_LimitedItem>;

		SilverTrader_LimitedItem limitedBook1 = new SilverTrader_LimitedItem();
		limitedBook1.classname = "ZenSkills_Book_Survival";
		limitedBook1.maxQuantity = 2;
		exampleTrader.m_limitedItems.Insert(limitedBook1);

		SilverTrader_LimitedItem limitedBook2 = new SilverTrader_LimitedItem();
		limitedBook2.classname = "ZenSkills_Book_Crafting";
		limitedBook2.maxQuantity = 2;
		exampleTrader.m_limitedItems.Insert(limitedBook2);

		SilverTrader_LimitedItem limitedBook3 = new SilverTrader_LimitedItem();
		limitedBook3.classname = "ZenSkills_Book_Hunting";
		limitedBook3.maxQuantity = 2;
		exampleTrader.m_limitedItems.Insert(limitedBook3);

		SilverTrader_LimitedItem limitedBook4 = new SilverTrader_LimitedItem();
		limitedBook4.classname = "ZenSkills_Book_Gathering";
		limitedBook4.maxQuantity = 2;
		exampleTrader.m_limitedItems.Insert(limitedBook4);

		// Standard-Items die der Trader von Anfang an hat
		exampleTrader.m_defaultItems = new array<ref SilverTrader_ItemEntry>;

		SilverTrader_ItemEntry item1 = new SilverTrader_ItemEntry();
		item1.classname = "AmmoBox_308Win_20Rnd";
		item1.quantity = 5;
		exampleTrader.m_defaultItems.Insert(item1);

		SilverTrader_ItemEntry item2 = new SilverTrader_ItemEntry();
		item2.classname = "AmmoBox_22_50Rnd";
		item2.quantity = 5;
		exampleTrader.m_defaultItems.Insert(item2);

		SilverTrader_ItemEntry item3 = new SilverTrader_ItemEntry();
		item3.classname = "AmmoBox_45ACP_25rnd";
		item3.quantity = 5;
		exampleTrader.m_defaultItems.Insert(item3);

		SilverTrader_ItemEntry item4 = new SilverTrader_ItemEntry();
		item4.classname = "ZenSkills_Injector_ExpBoost";
		item4.quantity = 8;
		exampleTrader.m_defaultItems.Insert(item4);

		SilverTrader_ItemEntry item5 = new SilverTrader_ItemEntry();
		item5.classname = "ZenSkills_Injector_PerkReset";
		item5.quantity = 2;
		exampleTrader.m_defaultItems.Insert(item5);

		SilverTrader_ItemEntry item6 = new SilverTrader_ItemEntry();
		item6.classname = "MeatTenderizer";
		item6.quantity = 10;
		exampleTrader.m_defaultItems.Insert(item6);

		SilverTrader_ItemEntry item11 = new SilverTrader_ItemEntry();
		item11.classname = "NailBox";
		item11.quantity = 10;
		exampleTrader.m_defaultItems.Insert(item11);

		SilverTrader_ItemEntry item12 = new SilverTrader_ItemEntry();
		item12.classname = "Battery9V";
		item12.quantity = 30;
		exampleTrader.m_defaultItems.Insert(item12);

		SilverTrader_ItemEntry item13 = new SilverTrader_ItemEntry();
		item13.classname = "Ammo_45ACP";
		item13.quantity = 10;
		exampleTrader.m_defaultItems.Insert(item13);

		SilverTrader_ItemEntry item14 = new SilverTrader_ItemEntry();
		item14.classname = "Ammo_380";
		item14.quantity = 10;
		exampleTrader.m_defaultItems.Insert(item14);

		SilverTrader_ItemEntry item15 = new SilverTrader_ItemEntry();
		item15.classname = "Ammo_22";
		item15.quantity = 10;
		exampleTrader.m_defaultItems.Insert(item15);

		SilverTrader_ItemEntry item16 = new SilverTrader_ItemEntry();
		item16.classname = "Ammo_12gaSlug";
		item16.quantity = 20;
		exampleTrader.m_defaultItems.Insert(item16);

		SilverTrader_ItemEntry item17 = new SilverTrader_ItemEntry();
		item17.classname = "Ammo_12gaPellets";
		item17.quantity = 10;
		exampleTrader.m_defaultItems.Insert(item17);

		SilverTrader_ItemEntry item18 = new SilverTrader_ItemEntry();
		item18.classname = "AmmoBox_12gaSlug_10Rnd";
		item18.quantity = 10;
		exampleTrader.m_defaultItems.Insert(item18);

		SilverTrader_ItemEntry item19 = new SilverTrader_ItemEntry();
		item19.classname = "AmmoBox_00buck_10rnd";
		item19.quantity = 10;
		exampleTrader.m_defaultItems.Insert(item19);

		SilverTrader_ItemEntry item20 = new SilverTrader_ItemEntry();
		item20.classname = "Hatchet";
		item20.quantity = 10;
		exampleTrader.m_defaultItems.Insert(item20);

		SilverTrader_ItemEntry item21 = new SilverTrader_ItemEntry();
		item21.classname = "B95";
		item21.quantity = 5;
		exampleTrader.m_defaultItems.Insert(item21);

		SilverTrader_ItemEntry item22 = new SilverTrader_ItemEntry();
		item22.classname = "MKII";
		item22.quantity = 5;
		exampleTrader.m_defaultItems.Insert(item22);

		SilverTrader_ItemEntry item23 = new SilverTrader_ItemEntry();
		item23.classname = "P1";
		item23.quantity = 5;
		exampleTrader.m_defaultItems.Insert(item23);

		SilverTrader_ItemEntry item24 = new SilverTrader_ItemEntry();
		item24.classname = "WaterBottle";
		item24.quantity = 10;
		exampleTrader.m_defaultItems.Insert(item24);

		SilverTrader_ItemEntry item25 = new SilverTrader_ItemEntry();
		item25.classname = "Crackers";
		item25.quantity = 10;
		exampleTrader.m_defaultItems.Insert(item25);

		SilverTrader_ItemEntry item26 = new SilverTrader_ItemEntry();
		item26.classname = "SaltySticks";
		item26.quantity = 10;
		exampleTrader.m_defaultItems.Insert(item26);

		SilverTrader_ItemEntry item27 = new SilverTrader_ItemEntry();
		item27.classname = "Zagorky";
		item27.quantity = 10;
		exampleTrader.m_defaultItems.Insert(item27);

		SilverTrader_ItemEntry item28 = new SilverTrader_ItemEntry();
		item28.classname = "Pate";
		item28.quantity = 10;
		exampleTrader.m_defaultItems.Insert(item28);

		SilverTrader_ItemEntry item29 = new SilverTrader_ItemEntry();
		item29.classname = "BandageDressing";
		item29.quantity = 35;
		exampleTrader.m_defaultItems.Insert(item29);

		SilverTrader_ItemEntry item30 = new SilverTrader_ItemEntry();
		item30.classname = "Whetstone";
		item30.quantity = 20;
		exampleTrader.m_defaultItems.Insert(item30);

		SilverTrader_ItemEntry item31 = new SilverTrader_ItemEntry();
		item31.classname = "Screwdriver";
		item31.quantity = 20;
		exampleTrader.m_defaultItems.Insert(item31);

		SilverTrader_ItemEntry item32 = new SilverTrader_ItemEntry();
		item32.classname = "WoolGloves_Black";
		item32.quantity = 8;
		exampleTrader.m_defaultItems.Insert(item32);

		SilverTrader_ItemEntry item33 = new SilverTrader_ItemEntry();
		item33.classname = "Shemag_Green";
		item33.quantity = 5;
		exampleTrader.m_defaultItems.Insert(item33);

		SilverTrader_ItemEntry item34 = new SilverTrader_ItemEntry();
		item34.classname = "HikingBootsLow_Black";
		item34.quantity = 5;
		exampleTrader.m_defaultItems.Insert(item34);

		m_traders.Insert(exampleTrader);
	}
};

// Trader-Daten Klasse (Inventar eines Traders)
class SilverTrader_Data
{
	ref map<string, float> m_items;

	void SilverTrader_Data()
	{
		m_items = new map<string, float>;
	}

	void ~SilverTrader_Data()
	{
		if (m_items)
		{
			delete m_items;
		}
	}

	void LoadFromJson(string path)
	{
		if (!m_items)
		{
			m_items = new map<string, float>;
		}
		else
		{
			m_items.Clear();
		}

		if (FileExist(path))
		{
			SilverTrader_DataJson jsonData = new SilverTrader_DataJson();
			JsonFileLoader<SilverTrader_DataJson>.JsonLoadFile(path, jsonData);

			if (jsonData && jsonData.m_itemList)
			{
				foreach (SilverTrader_ItemEntry entry : jsonData.m_itemList)
				{
					if (entry && entry.classname != "" && IsValidClassname(entry.classname))
					{
						m_items.Insert(entry.classname, entry.quantity);
					}
				}
			}
		}
	}

	void SaveToJson(string path)
	{
		SilverTrader_DataJson jsonData = new SilverTrader_DataJson();
		jsonData.m_itemList = new array<ref SilverTrader_ItemEntry>;

		if (m_items)
		{
			foreach (string classname, float quantity : m_items)
			{
				SilverTrader_ItemEntry entry = new SilverTrader_ItemEntry();
				entry.classname = classname;
				entry.quantity = quantity;
				jsonData.m_itemList.Insert(entry);
			}
		}

		JsonFileLoader<SilverTrader_DataJson>.JsonSaveFile(path, jsonData);
	}

	private bool IsValidClassname(string classname)
	{
		if (g_Game.ConfigIsExisting(CFG_VEHICLESPATH + " " + classname))
			return true;
		if (g_Game.ConfigIsExisting(CFG_MAGAZINESPATH + " " + classname))
			return true;
		if (g_Game.ConfigIsExisting(CFG_WEAPONSPATH + " " + classname))
			return true;
		return false;
	}
};

// JSON-Serialisierungs-Hilfsklassen
class SilverTrader_DataJson
{
	ref array<ref SilverTrader_ItemEntry> m_itemList;

	void SilverTrader_DataJson()
	{
		m_itemList = new array<ref SilverTrader_ItemEntry>;
	}
};

class SilverTrader_ItemEntry
{
	string classname;
	float quantity;
};

// Basis-Trader-Info Klasse (für Client/Server)
class SilverTrader_Info
{
	int m_traderId;
	vector m_position;
	ref array<string> m_buyFilter;
	ref array<string> m_sellFilter;
	ref array<ref SilverCommissionOverride> m_commissionOverrides; // Item-spezifische Commission
	int m_storageMaxSize;
	float m_storageCommission;
	string m_dumpingByAmountAlgorithm;
	float m_dumpingByAmountModifier;
	float m_dumpingByBadQuality;
	float m_sellMaxQuantityPercent;
	float m_buyMaxQuantityPercent;
	bool m_isRotatingTrader;

	void ~SilverTrader_Info()
	{
		if (m_buyFilter)
			delete m_buyFilter;
		if (m_sellFilter)
			delete m_sellFilter;
		if (m_commissionOverrides)
			delete m_commissionOverrides;
	}

	// Ermittelt Commission fuer ein Item (Override oder Fallback)
	float GetCommissionForItem(string classname)
	{
		if (m_commissionOverrides)
		{
			// Exakte Klasse pruefen (case-insensitive)
			string classnameLower = classname;
			classnameLower.ToLower();
			foreach (SilverCommissionOverride commOverride : m_commissionOverrides)
			{
				string overrideLower = commOverride.classname;
				overrideLower.ToLower();
				if (commOverride && overrideLower == classnameLower)
				{
					return commOverride.commission;
				}
			}

			// Parent-Klasse pruefen (fuer Vererbung wie "Goldnugget_Base")
			foreach (SilverCommissionOverride parentOverride : m_commissionOverrides)
			{
				if (parentOverride && g_Game.IsKindOf(classname, parentOverride.classname))
				{
					return parentOverride.commission;
				}
			}
		}

		// Fallback auf Standard-Commission
		return m_storageCommission;
	}
};

// Commission-Override Eintrag (fuer Item-spezifische Commission)
class SilverCommissionOverride
{
	string classname;
	float commission;
};

// Limited-Item Eintrag (wird bei jedem Restart auf maxQuantity zurueckgesetzt)
class SilverTrader_LimitedItem
{
	string classname;
	int maxQuantity;
};

// Pool-Item fuer rotierende Haendler (mit Gewichtung)
class SilverTrader_PoolItem
{
	string classname;
	int quantity;       // Menge pro Rotation
	float weight;       // Gewichtung (1.0 = normal, 0.1 = sehr selten)
};

// Rotierender Haendler Config (komplett isoliert vom normalen Trader)
class SilverRotatingTrader_Config : SilverTrader_Info
{
	string m_classname;
	ref array<string> m_attachments;
	ref array<string> m_spawnPositions;                        // Mehrere Spawn-Positionen (zufaellig bei Restart)
	float m_orientation;
	int m_rotationIntervalMinutes;                        // Rotationsintervall in Minuten
	int m_activeSlots;                                    // Wie viele Items pro Rotation aktiv
	ref array<ref SilverTrader_PoolItem> m_poolItems;     // Gesamtkatalog
	bool m_enableZenMapMarker;                            // ZenMap Marker auf Karte anzeigen
	string m_zenMapMarkerName;                            // Name des Markers auf der Karte
	string m_zenMapMarkerIcon;                            // Icon-Pfad (leer = Standard)

	void ~SilverRotatingTrader_Config()
	{
		if (m_attachments)
			delete m_attachments;
		if (m_spawnPositions)
			delete m_spawnPositions;
		if (m_poolItems)
			delete m_poolItems;
	}
};

// Server-spezifische Trader-Config (erweitert Info um Spawn-Daten)
class SilverTrader_ServerConfig : SilverTrader_Info
{
	string m_classname;
	ref array<string> m_attachments;
	ref array<ref SilverTrader_ItemEntry> m_defaultItems; // Start-Items fuer Trader
	ref array<ref SilverTrader_LimitedItem> m_limitedItems; // Bei Restart auf fixen Wert setzen
	float m_orientation;

	void ~SilverTrader_ServerConfig()
	{
		if (m_attachments)
			delete m_attachments;
		if (m_defaultItems)
			delete m_defaultItems;
		if (m_limitedItems)
			delete m_limitedItems;
	}
};

// Separate Config fuer rotierende Haendler
class SilverRotatingTradersConfig
{
	private const static string MOD_FOLDER = "$profile:\\SilverBarter\\";
	private const static string CONFIG_NAME = "SilverBarterRotatingTraders.json";

	ref array<ref SilverRotatingTrader_Config> m_rotatingTraders;

	void SilverRotatingTradersConfig()
	{
		m_rotatingTraders = new array<ref SilverRotatingTrader_Config>;
	}

	void ~SilverRotatingTradersConfig()
	{
		if (m_rotatingTraders)
		{
			m_rotatingTraders.Clear();
			delete m_rotatingTraders;
		}
	}

	void Load()
	{
		if (!g_Game.IsDedicatedServer())
			return;

		if (!FileExist(MOD_FOLDER))
		{
			MakeDirectory(MOD_FOLDER);
		}

		string path = MOD_FOLDER + CONFIG_NAME;
		if (FileExist(path))
		{
			JsonFileLoader<SilverRotatingTradersConfig>.JsonLoadFile(path, this);
		}
		else
		{
			SetDefaultValues();
			Save();
		}
	}

	void Save()
	{
		if (!g_Game.IsDedicatedServer())
			return;

		if (!FileExist(MOD_FOLDER))
		{
			MakeDirectory(MOD_FOLDER);
		}

		JsonFileLoader<SilverRotatingTradersConfig>.JsonSaveFile(MOD_FOLDER + CONFIG_NAME, this);
	}

	void SetDefaultValues()
	{
		m_rotatingTraders = new array<ref SilverRotatingTrader_Config>;

		SilverRotatingTrader_Config rotatingTrader = new SilverRotatingTrader_Config();
		rotatingTrader.m_traderId = 100;
		rotatingTrader.m_classname = "SurvivorM_Boris";
		rotatingTrader.m_spawnPositions = new array<string>;
		rotatingTrader.m_spawnPositions.Insert("6744.79 51.09 11380.01");
		rotatingTrader.m_spawnPositions.Insert("3634.65 99.05 7496.00");
		rotatingTrader.m_spawnPositions.Insert("4152.52 74.094 7759.84");
		rotatingTrader.m_spawnPositions.Insert("5226.59 35.481 8586.15");
		rotatingTrader.m_spawnPositions.Insert("6294.12 20.80 9524.32");
		rotatingTrader.m_spawnPositions.Insert("8172.09 31.48 10659.29");
		rotatingTrader.m_orientation = 0;
		rotatingTrader.m_rotationIntervalMinutes = 60;
		rotatingTrader.m_activeSlots = 5;
		rotatingTrader.m_storageMaxSize = 5000;
		rotatingTrader.m_storageCommission = 0.8;
		rotatingTrader.m_dumpingByAmountAlgorithm = "linear";
		rotatingTrader.m_dumpingByAmountModifier = 0.65;
		rotatingTrader.m_dumpingByBadQuality = 0.5;
		rotatingTrader.m_sellMaxQuantityPercent = 0.8;
		rotatingTrader.m_buyMaxQuantityPercent = 0.9;
		rotatingTrader.m_enableZenMapMarker = true;
		rotatingTrader.m_zenMapMarkerName = "Yuri";
		rotatingTrader.m_zenMapMarkerIcon = "";

		rotatingTrader.m_buyFilter = new array<string>;
		rotatingTrader.m_buyFilter.Insert("Inventory_Base");
		rotatingTrader.m_buyFilter.Insert("Weapon_Base");
		rotatingTrader.m_buyFilter.Insert("Magazine_Base");
		rotatingTrader.m_buyFilter.Insert("Ammunition_Base");

		rotatingTrader.m_sellFilter = new array<string>;
		rotatingTrader.m_sellFilter.Insert("Inventory_Base");
		rotatingTrader.m_sellFilter.Insert("Weapon_Base");
		rotatingTrader.m_sellFilter.Insert("Magazine_Base");
		rotatingTrader.m_sellFilter.Insert("Ammunition_Base");
		rotatingTrader.m_sellFilter.Insert("!SyringeFull");
		rotatingTrader.m_sellFilter.Insert("!Paper");
		rotatingTrader.m_sellFilter.Insert("!Zen_EmptyFood");
		rotatingTrader.m_sellFilter.Insert("!FenceKit");
		rotatingTrader.m_sellFilter.Insert("!WatchtowerKit");
		rotatingTrader.m_sellFilter.Insert("!ShelterKit");
		rotatingTrader.m_sellFilter.Insert("!TerritoryFlagKit");
		rotatingTrader.m_sellFilter.Insert("!Flag_Base");
		rotatingTrader.m_sellFilter.Insert("!Rag");
		rotatingTrader.m_sellFilter.Insert("!BurlapStrip");
		rotatingTrader.m_sellFilter.Insert("!Stone");
		rotatingTrader.m_sellFilter.Insert("!SmallStone");
		rotatingTrader.m_sellFilter.Insert("!Firewood");
		rotatingTrader.m_sellFilter.Insert("!BoneHook");
		rotatingTrader.m_sellFilter.Insert("!WoodenHook");
		rotatingTrader.m_sellFilter.Insert("!Bark_ColorBase");
		rotatingTrader.m_sellFilter.Insert("!Bone");
		rotatingTrader.m_sellFilter.Insert("!Bait");
		rotatingTrader.m_sellFilter.Insert("!BoneBait");
		rotatingTrader.m_sellFilter.Insert("!Barrel_ColorBase");
		rotatingTrader.m_sellFilter.Insert("!FireplaceBase");
		rotatingTrader.m_sellFilter.Insert("!CookingStand");
		rotatingTrader.m_sellFilter.Insert("!WoodenStick");
		rotatingTrader.m_sellFilter.Insert("!Torch");
		rotatingTrader.m_sellFilter.Insert("!LongWoodenStick");
		rotatingTrader.m_sellFilter.Insert("!SharpWoodenStick");
		rotatingTrader.m_sellFilter.Insert("!HandDrillKit");
		rotatingTrader.m_sellFilter.Insert("!Spear");
		rotatingTrader.m_sellFilter.Insert("!SmallGuts");
		rotatingTrader.m_sellFilter.Insert("!Guts");
		rotatingTrader.m_sellFilter.Insert("!Worm");
		rotatingTrader.m_sellFilter.Insert("!ImprovisedFishingRod");
		rotatingTrader.m_sellFilter.Insert("!TripwireTrap");
		rotatingTrader.m_sellFilter.Insert("!RabbitSnareTrap");
		rotatingTrader.m_sellFilter.Insert("!FishNetTrap");
		rotatingTrader.m_sellFilter.Insert("!SmallFishTrap");
		rotatingTrader.m_sellFilter.Insert("!Empty_ZenJameson");
		rotatingTrader.m_sellFilter.Insert("!Zen_CamoShelterKit");
		rotatingTrader.m_sellFilter.Insert("!Empty_Honey");
		rotatingTrader.m_sellFilter.Insert("!Empty_Marmalade");
		rotatingTrader.m_sellFilter.Insert("!Empty_Can_Opened");
		rotatingTrader.m_sellFilter.Insert("!BF_DoorBarricadeKit");
		rotatingTrader.m_sellFilter.Insert("!BF_WindowBarricadeKit");
		rotatingTrader.m_sellFilter.Insert("!BF_WindowBarricadeMedKit");
		rotatingTrader.m_sellFilter.Insert("!BF_DoubleDoorBarricadeKit");
		rotatingTrader.m_sellFilter.Insert("!bl_improvised_whetstone");
		rotatingTrader.m_sellFilter.Insert("!bl_improvised_sewing_kit");
		rotatingTrader.m_sellFilter.Insert("!Single_Match");

		rotatingTrader.m_commissionOverrides = new array<ref SilverCommissionOverride>;

		rotatingTrader.m_attachments = new array<string>;
		rotatingTrader.m_attachments.Insert("WoolCoat_Black");
		rotatingTrader.m_attachments.Insert("CargoPants_Black");
		rotatingTrader.m_attachments.Insert("MilitaryBoots_Black");
		rotatingTrader.m_attachments.Insert("Slingbag_Black");
		rotatingTrader.m_attachments.Insert("WoolGloves_Black");
		rotatingTrader.m_attachments.Insert("GP5GasMask");

		rotatingTrader.m_poolItems = new array<ref SilverTrader_PoolItem>;

		SilverTrader_PoolItem poolItem1 = new SilverTrader_PoolItem();
		poolItem1.classname = "AKM";
		poolItem1.quantity = 1;
		poolItem1.weight = 0.1;
		rotatingTrader.m_poolItems.Insert(poolItem1);

		SilverTrader_PoolItem poolItem2 = new SilverTrader_PoolItem();
		poolItem2.classname = "M4A1";
		poolItem2.quantity = 1;
		poolItem2.weight = 0.05;
		rotatingTrader.m_poolItems.Insert(poolItem2);

		SilverTrader_PoolItem poolItem3 = new SilverTrader_PoolItem();
		poolItem3.classname = "UMP45";
		poolItem3.quantity = 2;
		poolItem3.weight = 0.5;
		rotatingTrader.m_poolItems.Insert(poolItem3);

		SilverTrader_PoolItem poolItem4 = new SilverTrader_PoolItem();
		poolItem4.classname = "MP5K";
		poolItem4.quantity = 2;
		poolItem4.weight = 0.7;
		rotatingTrader.m_poolItems.Insert(poolItem4);

		SilverTrader_PoolItem poolItem5 = new SilverTrader_PoolItem();
		poolItem5.classname = "Ammo_762x39";
		poolItem5.quantity = 5;
		poolItem5.weight = 1.0;
		rotatingTrader.m_poolItems.Insert(poolItem5);

		SilverTrader_PoolItem poolItem6 = new SilverTrader_PoolItem();
		poolItem6.classname = "M68Optic";
		poolItem6.quantity = 1;
		poolItem6.weight = 0.3;
		rotatingTrader.m_poolItems.Insert(poolItem6);

		SilverTrader_PoolItem poolItem7 = new SilverTrader_PoolItem();
		poolItem7.classname = "FAMAS";
		poolItem7.quantity = 1;
		poolItem7.weight = 0.15;
		rotatingTrader.m_poolItems.Insert(poolItem7);

		SilverTrader_PoolItem poolItem8 = new SilverTrader_PoolItem();
		poolItem8.classname = "AugShort";
		poolItem8.quantity = 1;
		poolItem8.weight = 0.15;
		rotatingTrader.m_poolItems.Insert(poolItem8);

		SilverTrader_PoolItem poolItem9 = new SilverTrader_PoolItem();
		poolItem9.classname = "SVD";
		poolItem9.quantity = 1;
		poolItem9.weight = 0.05;
		rotatingTrader.m_poolItems.Insert(poolItem9);

		SilverTrader_PoolItem poolItem10 = new SilverTrader_PoolItem();
		poolItem10.classname = "VSS";
		poolItem10.quantity = 1;
		poolItem10.weight = 0.08;
		rotatingTrader.m_poolItems.Insert(poolItem10);

		SilverTrader_PoolItem poolItem11 = new SilverTrader_PoolItem();
		poolItem11.classname = "Saiga";
		poolItem11.quantity = 1;
		poolItem11.weight = 0.1;
		rotatingTrader.m_poolItems.Insert(poolItem11);

		SilverTrader_PoolItem poolItem12 = new SilverTrader_PoolItem();
		poolItem12.classname = "M16A2";
		poolItem12.quantity = 1;
		poolItem12.weight = 0.12;
		rotatingTrader.m_poolItems.Insert(poolItem12);

		SilverTrader_PoolItem poolItem13 = new SilverTrader_PoolItem();
		poolItem13.classname = "Deagle";
		poolItem13.quantity = 1;
		poolItem13.weight = 0.2;
		rotatingTrader.m_poolItems.Insert(poolItem13);

		SilverTrader_PoolItem poolItem14 = new SilverTrader_PoolItem();
		poolItem14.classname = "AmmoBox_556x45_20Rnd";
		poolItem14.quantity = 3;
		poolItem14.weight = 0.8;
		rotatingTrader.m_poolItems.Insert(poolItem14);

		SilverTrader_PoolItem poolItem15 = new SilverTrader_PoolItem();
		poolItem15.classname = "AmmoBox_308Win_20Rnd";
		poolItem15.quantity = 2;
		poolItem15.weight = 0.6;
		rotatingTrader.m_poolItems.Insert(poolItem15);

		SilverTrader_PoolItem poolItem16 = new SilverTrader_PoolItem();
		poolItem16.classname = "AmmoBox_357_20Rnd";
		poolItem16.quantity = 3;
		poolItem16.weight = 0.7;
		rotatingTrader.m_poolItems.Insert(poolItem16);

		SilverTrader_PoolItem poolItem17 = new SilverTrader_PoolItem();
		poolItem17.classname = "AmmoBox_9x39_20Rnd";
		poolItem17.quantity = 2;
		poolItem17.weight = 0.5;
		rotatingTrader.m_poolItems.Insert(poolItem17);

		SilverTrader_PoolItem poolItem18 = new SilverTrader_PoolItem();
		poolItem18.classname = "AmmoBox_762x39_20Rnd";
		poolItem18.quantity = 3;
		poolItem18.weight = 0.8;
		rotatingTrader.m_poolItems.Insert(poolItem18);

		SilverTrader_PoolItem poolItem19 = new SilverTrader_PoolItem();
		poolItem19.classname = "Mag_MP5_30Rnd";
		poolItem19.quantity = 2;
		poolItem19.weight = 0.6;
		rotatingTrader.m_poolItems.Insert(poolItem19);

		SilverTrader_PoolItem poolItem20 = new SilverTrader_PoolItem();
		poolItem20.classname = "Mag_AKM_Drum75Rnd";
		poolItem20.quantity = 1;
		poolItem20.weight = 0.05;
		rotatingTrader.m_poolItems.Insert(poolItem20);

		SilverTrader_PoolItem poolItem21 = new SilverTrader_PoolItem();
		poolItem21.classname = "Mag_STANAG_60Rnd";
		poolItem21.quantity = 1;
		poolItem21.weight = 0.08;
		rotatingTrader.m_poolItems.Insert(poolItem21);

		SilverTrader_PoolItem poolItem22 = new SilverTrader_PoolItem();
		poolItem22.classname = "Mag_STANAG_30Rnd";
		poolItem22.quantity = 2;
		poolItem22.weight = 0.5;
		rotatingTrader.m_poolItems.Insert(poolItem22);

		SilverTrader_PoolItem poolItem23 = new SilverTrader_PoolItem();
		poolItem23.classname = "Mag_Aug_30Rnd";
		poolItem23.quantity = 2;
		poolItem23.weight = 0.4;
		rotatingTrader.m_poolItems.Insert(poolItem23);

		SilverTrader_PoolItem poolItem24 = new SilverTrader_PoolItem();
		poolItem24.classname = "Mag_FAMAS_25Rnd";
		poolItem24.quantity = 2;
		poolItem24.weight = 0.4;
		rotatingTrader.m_poolItems.Insert(poolItem24);

		SilverTrader_PoolItem poolItem25 = new SilverTrader_PoolItem();
		poolItem25.classname = "Mag_SVD_10Rnd";
		poolItem25.quantity = 1;
		poolItem25.weight = 0.2;
		rotatingTrader.m_poolItems.Insert(poolItem25);

		SilverTrader_PoolItem poolItem26 = new SilverTrader_PoolItem();
		poolItem26.classname = "Mag_Saiga_Drum20Rnd";
		poolItem26.quantity = 1;
		poolItem26.weight = 0.1;
		rotatingTrader.m_poolItems.Insert(poolItem26);

		SilverTrader_PoolItem poolItem27 = new SilverTrader_PoolItem();
		poolItem27.classname = "Mag_Saiga_8Rnd";
		poolItem27.quantity = 2;
		poolItem27.weight = 0.4;
		rotatingTrader.m_poolItems.Insert(poolItem27);

		SilverTrader_PoolItem poolItem28 = new SilverTrader_PoolItem();
		poolItem28.classname = "M4_Suppressor";
		poolItem28.quantity = 1;
		poolItem28.weight = 0.15;
		rotatingTrader.m_poolItems.Insert(poolItem28);

		SilverTrader_PoolItem poolItem29 = new SilverTrader_PoolItem();
		poolItem29.classname = "AK_Suppressor";
		poolItem29.quantity = 1;
		poolItem29.weight = 0.15;
		rotatingTrader.m_poolItems.Insert(poolItem29);

		SilverTrader_PoolItem poolItem30 = new SilverTrader_PoolItem();
		poolItem30.classname = "PistolSuppressor";
		poolItem30.quantity = 1;
		poolItem30.weight = 0.25;
		rotatingTrader.m_poolItems.Insert(poolItem30);

		SilverTrader_PoolItem poolItem31 = new SilverTrader_PoolItem();
		poolItem31.classname = "SewingKit";
		poolItem31.quantity = 2;
		poolItem31.weight = 0.6;
		rotatingTrader.m_poolItems.Insert(poolItem31);

		SilverTrader_PoolItem poolItem32 = new SilverTrader_PoolItem();
		poolItem32.classname = "LeatherSewingKit";
		poolItem32.quantity = 2;
		poolItem32.weight = 0.5;
		rotatingTrader.m_poolItems.Insert(poolItem32);

		SilverTrader_PoolItem poolItem33 = new SilverTrader_PoolItem();
		poolItem33.classname = "PlateCarrierVest_Black";
		poolItem33.quantity = 1;
		poolItem33.weight = 0.2;
		rotatingTrader.m_poolItems.Insert(poolItem33);

		SilverTrader_PoolItem poolItem34 = new SilverTrader_PoolItem();
		poolItem34.classname = "PlateCarrierVest_Camo";
		poolItem34.quantity = 1;
		poolItem34.weight = 0.1;
		rotatingTrader.m_poolItems.Insert(poolItem34);

		SilverTrader_PoolItem poolItem35 = new SilverTrader_PoolItem();
		poolItem35.classname = "BallisticHelmet_Black";
		poolItem35.quantity = 1;
		poolItem35.weight = 0.2;
		rotatingTrader.m_poolItems.Insert(poolItem35);

		SilverTrader_PoolItem poolItem36 = new SilverTrader_PoolItem();
		poolItem36.classname = "dzn_module_card";
		poolItem36.quantity = 1;
		poolItem36.weight = 0.3;
		rotatingTrader.m_poolItems.Insert(poolItem36);

		SilverTrader_PoolItem poolItem37 = new SilverTrader_PoolItem();
		poolItem37.classname = "dzn_module_lantia";
		poolItem37.quantity = 1;
		poolItem37.weight = 0.2;
		rotatingTrader.m_poolItems.Insert(poolItem37);

		SilverTrader_PoolItem poolItem38 = new SilverTrader_PoolItem();
		poolItem38.classname = "dzn_module_surge";
		poolItem38.quantity = 1;
		poolItem38.weight = 0.2;
		rotatingTrader.m_poolItems.Insert(poolItem38);

		SilverTrader_PoolItem poolItem39 = new SilverTrader_PoolItem();
		poolItem39.classname = "dzn_module_ext";
		poolItem39.quantity = 1;
		poolItem39.weight = 0.15;
		rotatingTrader.m_poolItems.Insert(poolItem39);

		SilverTrader_PoolItem poolItem40 = new SilverTrader_PoolItem();
		poolItem40.classname = "dzn_module_ext2";
		poolItem40.quantity = 1;
		poolItem40.weight = 0.15;
		rotatingTrader.m_poolItems.Insert(poolItem40);

		SilverTrader_PoolItem poolItem41 = new SilverTrader_PoolItem();
		poolItem41.classname = "dzn_detector";
		poolItem41.quantity = 1;
		poolItem41.weight = 0.1;
		rotatingTrader.m_poolItems.Insert(poolItem41);

		SilverTrader_PoolItem poolItem42 = new SilverTrader_PoolItem();
		poolItem42.classname = "dzn_printer_filament_abs";
		poolItem42.quantity = 1;
		poolItem42.weight = 0.4;
		rotatingTrader.m_poolItems.Insert(poolItem42);

		SilverTrader_PoolItem poolItem43 = new SilverTrader_PoolItem();
		poolItem43.classname = "dzn_printer_filament_tpc";
		poolItem43.quantity = 1;
		poolItem43.weight = 0.4;
		rotatingTrader.m_poolItems.Insert(poolItem43);

		SilverTrader_PoolItem poolItem44 = new SilverTrader_PoolItem();
		poolItem44.classname = "dzn_printer_filament_nylon";
		poolItem44.quantity = 1;
		poolItem44.weight = 0.4;
		rotatingTrader.m_poolItems.Insert(poolItem44);

		m_rotatingTraders.Insert(rotatingTrader);
	}
};

// Globaler Config-Accessor
static ref SilverBarterConfig g_SilverBarterConfig;
static ref SilverRotatingTradersConfig g_SilverRotatingTradersConfig;

static SilverBarterConfig GetSilverBarterConfig()
{
	if (!g_SilverBarterConfig)
	{
		Print("[SilverBarter] Initializing config...");
		g_SilverBarterConfig = new SilverBarterConfig();
		g_SilverBarterConfig.Load();
	}
	return g_SilverBarterConfig;
}

static SilverRotatingTradersConfig GetSilverRotatingTradersConfig()
{
	if (!g_SilverRotatingTradersConfig)
	{
		Print("[SilverBarter] Initializing rotating traders config...");
		g_SilverRotatingTradersConfig = new SilverRotatingTradersConfig();
		g_SilverRotatingTradersConfig.Load();
	}
	return g_SilverRotatingTradersConfig;
}
