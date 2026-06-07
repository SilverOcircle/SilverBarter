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

	// BaseBuilding
	class Inventory_Base;
	class Bottle_Base;
	class Box_Base;
	class WoodenPlank: Inventory_Base {	varQuantityMax = 10; baseBuildingItem = 1; };
	class MetalPlate: Inventory_Base { varQuantityMax = 10; baseBuildingItem = 1; };
	class WoodenLog: Inventory_Base { varQuantityMax = 1; baseBuildingItem = 1; };
	class CombinationLock: Inventory_Base { baseBuildingItem = 1; };
	class CamoNet: Inventory_Base { baseBuildingItem = 1; };
	class Spotlight: Inventory_Base { baseBuildingItem = 1; };
	class XmasLights: Inventory_Base { baseBuildingItem = 1; };
	class PowerGenerator: Inventory_Base { baseBuildingItem = 1; };
	class CableReel: Inventory_Base { baseBuildingItem = 1; };
	class BatteryCharger: Inventory_Base { baseBuildingItem = 1; };
	class Fabric: Inventory_Base { baseBuildingItem = 1; };
	class Flag_Base: Inventory_Base { baseBuildingItem = 1; };
	class BarbedWire: Inventory_Base { baseBuildingItem = 1; };
	class Pipe: Inventory_Base { baseBuildingItem = 1; };
	class Rope: Inventory_Base { baseBuildingItem = 1; };
	class MetalWire: Inventory_Base { baseBuildingItem = 1; };
	class DuctTape: Inventory_Base { baseBuildingItem = 1; };
	class Nail: Inventory_Base { baseBuildingItem = 1; };
	class NailBox: Box_Base { baseBuildingItem = 1; };
	class ZenKitBase : Inventory_Base { baseBuildingItem = 1; };
	class LargeGasCanister: Inventory_Base { baseBuildingItem = 1; };
	class MediumGasCanister: Inventory_Base { baseBuildingItem = 1; };
	class SmallGasCanister: Inventory_Base { baseBuildingItem = 1; };
	class PortableGasStove: Inventory_Base { baseBuildingItem = 1; };
	class PortableGasLamp: Inventory_Base { baseBuildingItem = 1; };
	class Tripod: Inventory_Base { baseBuildingItem = 1; };
	
	//VehicleParts
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
