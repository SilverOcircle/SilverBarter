class CfgPatches {
	class SilverBarter {
		units[] = {"TraderPoint"};
		requiredAddons[] = {"DZ_Data", "DZ_Scripts"};
	};
};

class CfgMods {
	class SilverBarter {
		type = "mod";
		author = "SilverOcircle";

		class defs {
			class gameScriptModule {
				value = "";
				files[] = {"SilverBarter/scripts/3_Game"};
			};

			class worldScriptModule {
				value = "";
				files[] = {"SilverBarter/scripts/4_World"};
			};

			class missionScriptModule {
				value = "";
				files[] = {"SilverBarter/scripts/5_Mission"};
			};
		};
	};
};

class CfgVehicles
{
	// TraderPoint Entity Definition (unsichtbar - nur fuer Interaktionslogik)
	class HouseNoDestruct;
	class TraderPoint: HouseNoDestruct
	{
		scope = 2;
		model = "\dz\data\lightpoint.p3d";
	};

	// Fahrzeugteile: vehiclePartItem = 1 damit Kategorie-Filter greift
	class Inventory_Base;
	class Bottle_Base;
	class Box_Base;

	class TruckBattery: Inventory_Base { vehiclePartItem = 1; };
	class CarBattery: Inventory_Base { vehiclePartItem = 1; };
	class BrakeFluid: Inventory_Base { vehiclePartItem = 1; };
	class EngineOil: Inventory_Base { vehiclePartItem = 1; };
	class CarRadiator: Inventory_Base { vehiclePartItem = 1; };
	class HeadlightH7: Inventory_Base { vehiclePartItem = 1; };
	class HeadlightH7_Box: Box_Base { vehiclePartItem = 1; };
	class SparkPlug: Inventory_Base { vehiclePartItem = 1; };
	class GlowPlug: Inventory_Base { vehiclePartItem = 1; };
	class TireRepairKit: Inventory_Base { vehiclePartItem = 1; };
	class CanisterGasoline: Bottle_Base { vehiclePartItem = 1; };
	class CarWheel: Inventory_Base { vehiclePartItem = 1; };
	class CarDoor: Inventory_Base { vehiclePartItem = 1; };
};
