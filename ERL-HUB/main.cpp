// main.cpp - ERL-HUB: interactive console + automatic demo covering Units I-IV of CIC-211
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "Logistics.h"
#include "PriorityDispatchQueue.h"

using namespace ReliefOps;
using namespace ReliefOps::Inventory;
using namespace ReliefOps::Logistics;

static void section(const std::string& title) { std::cout << "\n===== " << title << " =====\n"; }

static int runDemo() {
    std::vector<TransitRecord> transitLog;

    // ---------------------------------------------------------------- Unit I / II
    section("UNIT II: ReliefCargo, DistributionDepot, operators, deep copy");
    DistributionDepot depot("Srinagar-Hub", 0, 0);
    depot += ReliefCargo("RationKits", 500, 2.5, 2);
    depot += ReliefCargo("TraumaKits", 120, 4.0, 5);
    depot += ReliefCargo("WaterTanks", 40, 100.0, 3);
    depot += ReliefCargo("ThermalBlankets", 800, 0.6, 1);
    depot += ReliefCargo("RationKits", 100, 2.5, 2);          // += merges with existing stock
    std::cout << depot;

    ReliefCargo& trauma = depot.cargoRef("TraumaKits");         // return by reference
    trauma.addQuantity(30);                                     // edits the depot's own stock
    std::cout << "After editing through returned reference: " << depot.cargoRef("TraumaKits") << "\n";

    ReliefCargo original("RationKits", 10, 2.5, 2);
    ReliefCargo copy(original);                                 // deep copy (own heap buffer)
    copy.addQuantity(5);
    std::cout << "original: " << original << "\ncopy    : " << copy
              << "\noriginal == copy ? " << std::boolalpha << (original == copy) << "\n";

    std::istringstream in("ThermalBlankets 50 0.6 1");
    ReliefCargo parsed;
    in >> parsed;                                               // operator>>
    std::cout << "Parsed via operator>>: " << parsed << "\n";
    std::cout << "Active depots (static): " << DistributionDepot::depotCount() << "\n";

    {
        TransportVehicle t1("TRK-X", 1000, 100);
        t1 += ReliefCargo("RationKits", 20, 2.5, 2);
        TransportVehicle t2(t1);                                // deep copy of manifest
        t2 += ReliefCargo("WaterTanks", 2, 100.0, 3);
        std::cout << "t1 == t2 ? " << (t1 == t2) << "   | duplicate cargo in t2? "
                  << t2.hasDuplicateOf(ReliefCargo("RationKits", 20, 2.5, 2)) << "\n"
                  << t2 << "Static vehicle count inside block: " << TransportVehicle::operationalVehicles() << "\n";
    }
    std::cout << "Static vehicle count after block: " << TransportVehicle::operationalVehicles() << "\n";

    // ---------------------------------------------------------------- Unit I
    section("UNIT I: overloaded allocation + friend IncidentCommander");
    ReliefCargo byVolume = allocateSupplies(depot, "WaterTanks", 10);
    ReliefCargo byTriage = allocateSupplies(depot, "TraumaKits", 25, TriageLevel::Immediate);
    std::cout << "Volume-based : " << byVolume << "\nTriage-based : " << byTriage << "\n";
    IncidentCommander commander;
    commander.triageOverride(depot, "ThermalBlankets", 4);
    commander.auditDepot(depot);
    std::cout << "ThermalBlankets priority after override: " << depot.cargoRef("ThermalBlankets").priority() << "\n";
    std::cout << "inline helpers: distance(0,0 -> 30,40) = " << distanceKm(0, 0, 30, 40) << " km, payload of "
              << byTriage.quantity() << " trauma kits = " << payloadTonnes(byTriage.quantity(), 4.0) << " t\n";

    // ---------------------------------------------------------------- Unit III
    section("UNIT III: polymorphic fleet via ReliefVehicle* (virtual base / diamond)");
    std::vector<ReliefVehicle*> fleet;
    fleet.push_back(new AerialTransport("AIR-01", 1500, 800, 0, 0));
    fleet.push_back(new GroundConvoy("GND-01", 12000, 300, 0, 0));
    fleet.push_back(new AmphibiousCraft("AMB-01", 6000, 400, 0, 0));
    fleet.push_back(new AmphibiousMedicalUnit("AMU-01", 4000, 500, 0, 0));
    fleet.push_back(new AerialTransport("AIR-02", 1500, 50, 0, 0));       // deliberately low on fuel
    std::cout << "Operational vehicles (static): " << TransportVehicle::operationalVehicles() << "\n";
    std::cout << "Route sweep to (60,80) = 100 km, corridor DAMAGED:\n";
    for (ReliefVehicle* v : fleet) {
        try {
            double eta = v->evaluateRoute(60, 80, true);                  // dynamic dispatch via vtable
            std::cout << "  " << v->id() << " (" << v->vehicleType() << ") ETA " << fmt(eta, 2) << " h, fuel left "
                      << fmt(v->fuel()) << " L, GPS at (" << v->gps().x() << "," << v->gps().y() << ")\n";
        } catch (const DamagedCorridorException& e) {
            std::cout << "  " << e.what() << "\n";
        } catch (const FuelDepletionException& e) {
            std::cout << "  " << e.what() << "\n";
        }
    }
    // return the fleet to the depot and refuel before the real missions
    for (ReliefVehicle* v : fleet) v->returnToBase(depot.x(), depot.y(), 400.0);

    // ---------------------------------------------------------------- Unit IV exceptions + transactions
    section("UNIT IV: transactional dispatch, custom exceptions, rollback");
    auto stockLine = [&](const std::string& what) {
        std::cout << "  " << what << " | WaterTanks=" << depot.quantityOf("WaterTanks")
                  << " RationKits=" << depot.quantityOf("RationKits") << " TraumaKits=" << depot.quantityOf("TraumaKits") << "\n";
    };
    stockLine("before");
    try {
        double eta = dispatchMission(depot, *fleet[0], byTriage, 30, 40, false, transitLog);
        std::cout << "  OK: " << fleet[0]->id() << " delivered triage kits, ETA " << fmt(eta, 2) << " h\n";
    } catch (const std::exception& e) { std::cout << "  FAILED: " << e.what() << "\n"; }

    try {
        ReliefCargo tooHeavy("WaterTanks", 30, 100.0, 3);                // 3000 kg on a 1500 kg helicopter
        dispatchMission(depot, *fleet[0], tooHeavy, 30, 40, false, transitLog);
    } catch (const CargoOverweightException& e) { std::cout << "  CAUGHT: " << e.what() << "\n"; }
    stockLine("after overweight attempt (rolled back)");

    try {
        ReliefCargo food("RationKits", 200, 2.5, 2);
        dispatchMission(depot, *fleet[1], food, 60, 80, true, transitLog);   // road is damaged
    } catch (const DamagedCorridorException& e) { std::cout << "  CAUGHT: " << e.what() << "\n"; }
    stockLine("after damaged-corridor attempt (rolled back)");

    try {
        ReliefCargo ghost("TraumaKits", 100000, 4.0, 5);
        dispatchMission(depot, *fleet[0], ghost, 30, 40, false, transitLog);
    } catch (const InsufficientStockException& e) { std::cout << "  CAUGHT: " << e.what() << "\n"; }

    try {
        ReliefCargo food("RationKits", 200, 2.5, 2);
        double eta = dispatchMission(depot, *fleet[3], food, 60, 80, true, transitLog);
        std::cout << "  OK: " << fleet[3]->id() << " (amphibious medical unit) reached destination via water, ETA "
                  << fmt(eta, 2) << " h\n";
        ReliefCargo water("WaterTanks", 20, 100.0, 3);
        dispatchMission(depot, *fleet[2], water, 10, 10, false, transitLog);
        std::cout << "  OK: " << fleet[2]->id() << " delivered water tanks\n";
    } catch (const std::exception& e) { std::cout << "  FAILED: " << e.what() << "\n"; }
    stockLine("after successful missions");
    std::cout << "Total dispatched (static): " << fmt(TransportVehicle::totalDispatchedTonnes(), 2) << " tonnes\n";

    // ---------------------------------------------------------------- Unit IV STL
    section("UNIT IV: STL containers, algorithms, functors, manipulators, templates");
    std::vector<ReliefCargo> needs = depot.snapshot();                    // from std::unordered_map
    std::sort(needs.begin(), needs.end(), MoreUrgent());                  // functor-driven sort
    std::cout << "Stock sorted by medical urgency:\n" << manifestHeader;
    std::for_each(needs.begin(), needs.end(), [](const ReliefCargo& c) { c.printRow(std::cout); });
    std::cout << manifestRule;

    PriorityDispatchQueue<ReliefCargo, LessUrgent> queue;                 // generic template
    for (const ReliefCargo& c : needs) queue.push(c);
    std::cout << "Dispatch order from PriorityDispatchQueue<ReliefCargo>:";
    while (!queue.empty()) std::cout << " " << queue.pop().name();
    std::cout << "\n";
    PriorityDispatchQueue<int> ints;                                      // same template, different T
    for (int x : {3, 9, 1, 7}) ints.push(x);
    std::cout << "PriorityDispatchQueue<int> order:";
    while (!ints.empty()) std::cout << " " << ints.pop();
    std::cout << "\n";

    // ---------------------------------------------------------------- Unit IV persistence
    section("UNIT IV: binary persistence with atomic update");
    try {
        saveTransitLog("transit_log.bin", transitLog);
        std::vector<TransitRecord> restored = loadTransitLog("transit_log.bin");
        std::cout << "Restored " << restored.size() << " transit records from disk:\n" << transitHeader;
        for (const TransitRecord& r : restored) std::cout << r << "\n";

        atomicWrite("manifest_AIR-01.bin", [&](std::ostream& os) { fleet[0]->saveManifest(os); });
        TransportVehicle audit("AUDIT", 1500, 0);
        std::fstream mf("manifest_AIR-01.bin", std::ios::in | std::ios::binary);
        audit.loadManifest(mf);
        std::cout << "Manifest restored from disk:\n" << audit
                  << "Restored manifest identical to live AIR-01 manifest? " << (audit == *fleet[0]) << "\n";

        { std::ofstream bad("corrupt.bin", std::ios::binary); bad << "not a log"; }
        loadTransitLog("corrupt.bin");
    } catch (const ReliefException& e) {
        std::cout << "Corrupt file detected safely: " << e.what() << "\n";
    }

    // ---------------------------------------------------------------- cleanup
    section("Fleet shutdown via base pointers (virtual destructors)");
    for (ReliefVehicle* v : fleet) {
        std::cout << "  deleting " << v->id() << ":\n";
        delete v;
    }
    fleet.clear();
    std::cout << "Operational vehicles after shutdown (static): " << TransportVehicle::operationalVehicles() << "\n";
    return 0;
}


// ===================================================================
//                  INTERACTIVE (menu-driven) MODE
// ===================================================================
namespace {

struct InputClosed {};   // thrown when stdin ends (Ctrl+Z / Ctrl+D)

std::string readLine(const std::string& prompt) {
    std::cout << prompt << std::flush;
    std::string s;
    if (!std::getline(std::cin, s)) throw InputClosed();
    std::string::size_type a = s.find_first_not_of(" \t\r\n"), b = s.find_last_not_of(" \t\r\n");
    return a == std::string::npos ? std::string() : s.substr(a, b - a + 1);
}

int readInt(const std::string& prompt, int lo, int hi) {
    for (;;) {
        std::istringstream ss(readLine(prompt));
        int v;
        std::string extra;
        if ((ss >> v) && !(ss >> extra) && v >= lo && v <= hi) return v;
        std::cout << "  Please enter a whole number between " << lo << " and " << hi << ".\n";
    }
}

double readDouble(const std::string& prompt, double lo, double hi) {
    for (;;) {
        std::istringstream ss(readLine(prompt));
        double v;
        std::string extra;
        if ((ss >> v) && !(ss >> extra) && v >= lo && v <= hi) return v;
        std::cout << "  Please enter a number between " << lo << " and " << hi << ".\n";
    }
}

std::string readWord(const std::string& prompt) {
    for (;;) {
        std::string s = readLine(prompt);
        if (!s.empty()) { std::replace(s.begin(), s.end(), ' ', '_'); return s; }
        std::cout << "  This cannot be empty.\n";
    }
}

bool readYesNo(const std::string& prompt) {
    for (;;) {
        std::string s = readLine(prompt + " (y/n): ");
        if (s == "y" || s == "Y") return true;
        if (s == "n" || s == "N") return false;
    }
}

// Uses the overloaded operator>> of ReliefCargo.
bool readCargo(ReliefCargo& out) {
    std::istringstream ss(readLine("  Cargo as: Name Qty KgPerUnit Priority(1-5)  e.g. RationKits 200 2.5 2\n  > "));
    ReliefCargo c;
    std::string extra;
    if (!(ss >> c) || (ss >> extra) || c.quantity() <= 0 || c.unitWeightKg() <= 0 || c.priority() < 1 || c.priority() > 5) {
        std::cout << "  Invalid cargo. Use: Name Quantity KgPerUnit Priority(1-5), numbers must be positive.\n";
        return false;
    }
    out = c;
    return true;
}

struct Fleet {
    std::vector<ReliefVehicle*> v;
    void shutdown() {
        for (std::size_t i = 0; i < v.size(); ++i) {
            std::cout << "  releasing " << v[i]->id() << ":\n";
            delete v[i];                       // virtual destructor chain via base pointer
        }
        v.clear();
    }
    ~Fleet() { shutdown(); }
};

ReliefVehicle* makeVehicle(int type, const std::string& id, double cap, double fuel, double x, double y) {
    switch (type) {
        case 1: return new AerialTransport(id, cap, fuel, x, y);
        case 2: return new GroundConvoy(id, cap, fuel, x, y);
        case 3: return new AmphibiousCraft(id, cap, fuel, x, y);
        default: return new AmphibiousMedicalUnit(id, cap, fuel, x, y);
    }
}

void showFleet(const Fleet& f) {
    if (f.v.empty()) { std::cout << "  (no vehicles registered yet - use option 3)\n"; return; }
    std::cout << std::string(86, '-') << '\n';
    for (std::size_t i = 0; i < f.v.size(); ++i) {
        const ReliefVehicle* v = f.v[i];
        std::cout << "  " << i + 1 << ". " << std::left << std::setw(8) << v->id() << std::setw(26) << v->vehicleType()
                  << "fuel " << std::setw(8) << fmt(v->fuel()) << "load " << fmt(v->loadKg()) << "/" << fmt(v->capacityKg())
                  << " kg   at (" << v->gps().x() << "," << v->gps().y() << ")\n";
    }
    std::cout << std::string(86, '-') << '\n';
}

ReliefVehicle* pickVehicle(Fleet& f) {
    if (f.v.empty()) { std::cout << "  No vehicles yet - register one with option 3.\n"; return nullptr; }
    showFleet(f);
    int n = readInt("  Choose vehicle number: ", 1, static_cast<int>(f.v.size()));
    return f.v[n - 1];
}

void printMenu() {
    std::cout << "\n---------------- ERL-HUB MENU ----------------\n"
                 "  1  Restock depot (+=)\n"
                 "  2  View depot stock\n"
                 "  3  Register a vehicle\n"
                 "  4  View fleet and statistics\n"
                 "  5  Allocate supplies (volume / triage preview)\n"
                 "  6  Dispatch a mission\n"
                 "  7  Triage override (IncidentCommander)\n"
                 "  8  Urgent-needs priority queue\n"
                 "  9  Transit log (view / save / load)\n"
                 " 10  Unload, refuel and return a vehicle to depot\n"
                 " 11  Run the automatic demo\n"
                 "  0  Exit\n";
}

int runInteractive() {
    std::cout << "\n=== ERL-HUB interactive console ===\n";
    std::string depotName = readWord("Depot name (e.g. Srinagar-Hub): ");
    double dx = readDouble("Depot X coordinate in km: ", -100000, 100000);
    double dy = readDouble("Depot Y coordinate in km: ", -100000, 100000);
    DistributionDepot depot(depotName, dx, dy);
    Fleet fleet;
    std::vector<TransitRecord> log;
    IncidentCommander commander;

    if (readYesNo("Load sample stock and a sample fleet to start quickly?")) {
        depot += ReliefCargo("RationKits", 500, 2.5, 2);
        depot += ReliefCargo("TraumaKits", 120, 4.0, 5);
        depot += ReliefCargo("WaterTanks", 40, 100.0, 3);
        depot += ReliefCargo("ThermalBlankets", 800, 0.6, 1);
        fleet.v.push_back(new AerialTransport("AIR-01", 1500, 800, dx, dy));
        fleet.v.push_back(new GroundConvoy("GND-01", 12000, 300, dx, dy));
        fleet.v.push_back(new AmphibiousCraft("AMB-01", 6000, 400, dx, dy));
        fleet.v.push_back(new AmphibiousMedicalUnit("AMU-01", 4000, 500, dx, dy));
        std::cout << "  Sample data loaded.\n";
    }

    for (;;) {
        printMenu();
        int choice = readInt("Choose an option: ", 0, 11);
        if (choice == 0) break;
        try {
            switch (choice) {
            case 1: {
                ReliefCargo c;
                if (readCargo(c)) { depot += c; std::cout << "  Restocked: " << c << "\n"; }
                break;
            }
            case 2:
                std::cout << depot;
                break;
            case 3: {
                int type = readInt("  Type: 1) Aerial  2) Ground  3) Amphibious  4) Amphibious Medical Unit: ", 1, 4);
                std::string id = readWord("  Vehicle ID (e.g. AIR-07): ");
                bool dup = false;
                for (std::size_t i = 0; i < fleet.v.size(); ++i) if (fleet.v[i]->id() == id) dup = true;
                if (dup) { std::cout << "  A vehicle with that ID already exists.\n"; break; }
                double cap = readDouble("  Capacity in kg: ", 1, 1e7);
                double fuel = readDouble("  Fuel in litres: ", 0, 1e7);
                fleet.v.push_back(makeVehicle(type, id, cap, fuel, depot.x(), depot.y()));
                std::cout << "  Registered " << id << " (" << fleet.v.back()->vehicleType() << ") at the depot.\n";
                break;
            }
            case 4:
                showFleet(fleet);
                std::cout << "  Operational vehicles (static): " << TransportVehicle::operationalVehicles()
                          << " | Depots (static): " << DistributionDepot::depotCount()
                          << " | Total dispatched: " << fmt(TransportVehicle::totalDispatchedTonnes(), 2) << " t\n";
                break;
            case 5: {
                int mode = readInt("  1) volume-based   2) triage-based: ", 1, 2);
                std::string item = readWord("  Item name: ");
                ReliefCargo r;
                if (mode == 1) {
                    int units = readInt("  Units requested: ", 1, 1000000);
                    r = allocateSupplies(depot, item, units);
                } else {
                    int cas = readInt("  Number of casualties: ", 1, 1000000);
                    int lvl = readInt("  Triage level 1) Minor  2) Delayed  3) Immediate: ", 1, 3);
                    r = allocateSupplies(depot, item, cas, static_cast<TriageLevel>(lvl));
                }
                std::cout << "  Suggested allocation: " << r << "\n";
                break;
            }
            case 6: {
                ReliefVehicle* v = pickVehicle(fleet);
                if (!v) break;
                std::cout << depot;
                std::string item = readWord("  Cargo item to send: ");
                if (!depot.contains(item)) { std::cout << "  No such item in the depot.\n"; break; }
                int qty = readInt("  Quantity: ", 1, 1000000);
                double x = readDouble("  Destination X in km: ", -100000, 100000);
                double y = readDouble("  Destination Y in km: ", -100000, 100000);
                bool damaged = readYesNo("  Is the road corridor damaged?");
                const ReliefCargo& stock = depot.cargoRef(item);
                ReliefCargo load(item, qty, stock.unitWeightKg(), stock.priority());
                try {
                    double eta = dispatchMission(depot, *v, load, x, y, damaged, log);
                    std::cout << "  SUCCESS: " << v->id() << " carries " << load << ", ETA " << fmt(eta, 2) << " h\n";
                } catch (const ReliefException& e) {
                    std::cout << "  MISSION FAILED: " << e.what()
                              << "\n  (rolled back - depot stock and vehicle load are unchanged)\n";
                }
                break;
            }
            case 7: {
                std::string item = readWord("  Item to re-prioritise: ");
                if (!depot.contains(item)) { std::cout << "  No such item in the depot.\n"; break; }
                int p = readInt("  New priority (1-5): ", 1, 5);
                commander.triageOverride(depot, item, p);
                commander.auditDepot(depot);
                std::cout << "  " << item << " is now priority " << depot.cargoRef(item).priority() << ".\n";
                break;
            }
            case 8: {
                std::vector<ReliefCargo> needs = depot.snapshot();
                if (needs.empty()) { std::cout << "  Depot is empty.\n"; break; }
                std::sort(needs.begin(), needs.end(), MoreUrgent());
                std::cout << manifestHeader;
                std::for_each(needs.begin(), needs.end(), [](const ReliefCargo& c) { c.printRow(std::cout); });
                std::cout << manifestRule;
                PriorityDispatchQueue<ReliefCargo, LessUrgent> q;
                for (std::size_t i = 0; i < needs.size(); ++i) q.push(needs[i]);
                std::cout << "  Dispatch order:";
                while (!q.empty()) std::cout << " " << q.pop().name();
                std::cout << "\n";
                break;
            }
            case 9: {
                int sub = readInt("  1) view current log  2) save to file  3) load from file and view: ", 1, 3);
                std::vector<TransitRecord> shown;
                if (sub == 1) shown = log;
                else {
                    std::string path = readLine("  File name (Enter for transit_log.bin): ");
                    if (path.empty()) path = "transit_log.bin";
                    if (sub == 2) { saveTransitLog(path, log); std::cout << "  Saved " << log.size() << " records to " << path << ".\n"; break; }
                    shown = loadTransitLog(path);
                    std::cout << "  Loaded " << shown.size() << " records from " << path << ".\n";
                }
                if (shown.empty()) { std::cout << "  (no records)\n"; break; }
                std::cout << transitHeader;
                for (std::size_t i = 0; i < shown.size(); ++i) std::cout << shown[i] << "\n";
                break;
            }
            case 10: {
                ReliefVehicle* v = pickVehicle(fleet);
                if (!v) break;
                double fuel = readDouble("  Refuel to how many litres: ", 0, 1e7);
                while (v->manifestSize() > 0) v->unloadLast();
                v->returnToBase(depot.x(), depot.y(), fuel);
                std::cout << "  " << v->id() << " unloaded, refuelled to " << fmt(fuel) << " L and back at the depot.\n";
                break;
            }
            case 11:
                runDemo();
                break;
            }
        } catch (const InputClosed&) {
            throw;
        } catch (const std::exception& e) {
            std::cout << "  ERROR: " << e.what() << "\n";
        }
    }
    std::cout << "\nShutting down the fleet through base pointers (virtual destructors):\n";
    fleet.shutdown();
    std::cout << "Goodbye.\n";
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    if (argc > 1 && std::string(argv[1]) == "demo") return runDemo();   // erlhub demo
    try {
        std::cout << "==== ERL-HUB: Emergency Relief Logistics Hub ====\n"
                     "  1) Interactive console (you enter the data)\n"
                     "  2) Automatic demo of all syllabus features\n"
                     "  0) Exit\n";
        int mode = readInt("Choose mode: ", 0, 2);
        if (mode == 1) return runInteractive();
        if (mode == 2) return runDemo();
    } catch (const InputClosed&) {
        std::cout << "\n[input closed] exiting.\n";
    }
    return 0;
}
