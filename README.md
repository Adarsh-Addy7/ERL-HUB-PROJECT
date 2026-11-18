# ERL-HUB: Post-Earthquake Multi-Agency Emergency Relief Logistics Hub
**Course:** Object-Oriented Programming Using C++ (CIC-211) | **Option 2**
Students Name - 1. Adarsh Singh, 14914802725, Sem - 3, Group - C789(C8),
                2. Ishaan Rai, 17414802725, Sem - 3, Group - C789(C9).
Standard C++ (builds warning-free on C++11, C++14 and C++17). Verified leak-free with AddressSanitizer/UBSan.

## Build & run
```
make        # builds ./erlhub
make run    # builds and runs the full demo
make clean
```
Or directly: `g++ -std=c++11 -Wall -Wextra main.cpp Inventory.cpp Logistics.cpp -o erlhub`

## Modes
On start the program asks which mode to use:
1. **Interactive console**: you enter the depot, stock, vehicles and missions yourself through a menu (restock, register vehicle, allocate supplies, dispatch mission, triage override, urgent-needs queue, save/load transit log, refuel/return vehicle). Invalid input is rejected and re-asked; failed missions roll back automatically.
2. **Automatic demo**: a scripted run that exercises every syllabus requirement (also available as `./erlhub demo`).

## Files
| File | Purpose |
|---|---|
| `Exceptions.h` | Custom exception framework (Unit IV) |
| `Inventory.h/.cpp` | `ReliefOps::Inventory`: `ReliefCargo`, `DistributionDepot`, inline helpers, manipulators |
| `Logistics.h/.cpp` | `ReliefOps::Logistics`: `TransportVehicle`, vehicle hierarchy, allocation, persistence, `IncidentCommander` |
| `PriorityDispatchQueue.h` | Generic template class |
| `main.cpp` | Demo that exercises every requirement and prints results |

## Requirement checklist
### Unit I
- Namespaces `ReliefOps::Logistics`, `ReliefOps::Inventory`: `Inventory.h`, `Logistics.h`
- Function overloading for supply allocation (volume-based vs. triage-based): `allocateSupplies()` in `Logistics.cpp`
- Call by reference: `applyDelta(int&, int)`; return by reference: `DistributionDepot::cargoRef()`, `TransportVehicle::fuelRef()`
- Inline utilities: `distanceKm()`, `payloadTonnes()`
- Friend class `IncidentCommander` (friend of `ReliefCargo` and `DistributionDepot`): `triageOverride()` edits private priority directly

### Unit II
- ADTs with information hiding: `ReliefCargo`, `DistributionDepot`, `TransportVehicle` (private data, const getters)
- Static members/functions: `TransportVehicle::operationalVehicles()`, `totalDispatchedTonnes()`, `DistributionDepot::depotCount()`
- Dynamic allocation: `ReliefCargo` heap name buffer; `TransportVehicle` dynamic manifest array (dynamic constructor, deep copy constructor, copy assignment, destructors, growth)
- Operators: `+=` (restock), `-=` (deduct on departure), `==` (manifest identity / duplicate check), `<<` and `>>` (console + serialization)

### Unit III
- `ReliefVehicle` (abstract, pure virtual `evaluateRoute()`, `vehicleType()`) derived into `AerialTransport`, `GroundConvoy`, `AmphibiousCraft`
- Diamond solved with virtual inheritance: `AmphibiousMedicalUnit : GroundConvoy, AmphibiousCraft` (single shared `ReliefVehicle`, visible in the destructor trace)
- Containership: `GPSBeacon` and `EmergencyLifeSupportUnit` are members of `ReliefVehicle` (HAS-A, not IS-A)
- Runtime polymorphism through `std::vector<ReliefVehicle*>` with virtual destructors

### Unit IV
- Exceptions derived from `std::exception`: `CargoOverweightException`, `FuelDepletionException`, `DamagedCorridorException`, `InsufficientStockException`
- Binary `std::fstream` persistence with atomic update (write `.tmp`, then rename): `atomicWrite()`, `saveTransitLog()`, `TransportVehicle::saveManifest()`; header count patched with `seekp()`
- Transactional rollback: `dispatchMission()` restores depot and vehicle on failure
- `PriorityDispatchQueue<T, Compare>` template, `std::vector`, `std::unordered_map`, `std::sort`, `std::for_each`, functors `MoreUrgent` / `LessUrgent`
- User-defined manipulators: `manifestHeader`, `manifestRule`, `transitHeader`
