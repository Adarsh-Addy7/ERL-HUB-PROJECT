// Logistics.h - namespace ReliefOps::Logistics
// Unit I  : overloaded allocation routines, friend class IncidentCommander
// Unit II : TransportVehicle ADT (dynamic manifest, static analytics, operators)
// Unit III: virtual-base diamond, containership, runtime polymorphism
// Unit IV : binary persistence with atomic update, functors, manipulators
#ifndef RELIEFOPS_LOGISTICS_H
#define RELIEFOPS_LOGISTICS_H

#include <functional>
#include <iostream>
#include <string>
#include <vector>

#include "Inventory.h"

namespace ReliefOps {
namespace Logistics {

using Inventory::DistributionDepot;
using Inventory::ReliefCargo;
using Inventory::TriageLevel;

// ---------- Unit III: contained (composed) subsystems ----------
class GPSBeacon {
public:
    GPSBeacon(const std::string& id, double x, double y) : id_(id), x_(x), y_(y) {}
    void moveTo(double x, double y) { x_ = x; y_ = y; }
    double x() const { return x_; }
    double y() const { return y_; }
    const std::string& id() const { return id_; }
private:
    std::string id_;
    double x_, y_;
};

class EmergencyLifeSupportUnit {
public:
    EmergencyLifeSupportUnit(int oxygenMinutes, int medKits) : oxygenMinutes_(oxygenMinutes), medKits_(medKits) {}
    int oxygenMinutes() const { return oxygenMinutes_; }
    int medKits() const { return medKits_; }
    bool operational() const { return oxygenMinutes_ > 0; }
private:
    int oxygenMinutes_;
    int medKits_;
};

// ---------- Unit II: TransportVehicle ----------
class TransportVehicle {
public:
    TransportVehicle(const std::string& id, double capacityKg, double fuelLitres, int initialSlots = 4); // dynamic ctor
    TransportVehicle(const TransportVehicle& other);               // deep copy of manifest
    TransportVehicle& operator=(const TransportVehicle& other);
    virtual ~TransportVehicle();

    const std::string& id() const { return id_; }
    double capacityKg() const { return capacityKg_; }
    double fuel() const { return fuelLitres_; }
    double& fuelRef() { return fuelLitres_; }                      // return by reference
    double loadKg() const;
    int manifestSize() const { return manifestSize_; }
    const ReliefCargo& manifestItem(int i) const { return manifest_[i]; }

    void checkCapacity(const ReliefCargo& c) const;                // throws CargoOverweightException
    TransportVehicle& operator+=(const ReliefCargo& c);            // load cargo
    ReliefCargo unloadLast();
    bool operator==(const TransportVehicle& other) const;          // manifest identity
    bool hasDuplicateOf(const ReliefCargo& c) const;

    void saveManifest(std::ostream& os) const;
    void loadManifest(std::istream& is);

    // static analytics shared across all vehicles
    static int operationalVehicles() { return operationalVehicles_; }
    static double totalDispatchedTonnes() { return dispatchedTonnes_; }
    static void recordDispatch(double tonnes) { dispatchedTonnes_ += tonnes; }

    friend std::ostream& operator<<(std::ostream& os, const TransportVehicle& v);
    friend std::istream& operator>>(std::istream& is, TransportVehicle& v);

private:
    void growManifest();

    std::string id_;
    double capacityKg_;
    double fuelLitres_;
    ReliefCargo* manifest_;
    int manifestSize_;
    int manifestCap_;
    static int operationalVehicles_;
    static double dispatchedTonnes_;
};

// ---------- Unit III: abstract ReliefVehicle + diamond hierarchy ----------
class ReliefVehicle : public TransportVehicle {
public:
    ReliefVehicle(const std::string& id, double capacityKg, double fuelLitres, double x, double y);
    virtual ~ReliefVehicle();

    // Evaluates the route: throws on damaged corridor / insufficient fuel, otherwise
    // reserves the fuel, moves the GPS beacon and returns the ETA in hours.
    virtual double evaluateRoute(double destX, double destY, bool corridorDamaged) = 0;
    virtual std::string vehicleType() const = 0;

    const GPSBeacon& gps() const { return gps_; }
    void returnToBase(double x, double y, double refuelTo) { gps_.moveTo(x, y); fuelRef() = refuelTo; }
    const EmergencyLifeSupportUnit& lifeSupport() const { return lifeSupport_; }

protected:
    double commitRoute(double destX, double destY, double litresPerKm, double speedKmh);

    GPSBeacon gps_;                         // containership: HAS-A, not IS-A
    EmergencyLifeSupportUnit lifeSupport_;
};

class AerialTransport : public virtual ReliefVehicle {
public:
    AerialTransport(const std::string& id, double cap, double fuel, double x, double y);
    ~AerialTransport();
    double evaluateRoute(double dx, double dy, bool corridorDamaged) override;
    std::string vehicleType() const override { return "Aerial"; }
};

class GroundConvoy : public virtual ReliefVehicle {
public:
    GroundConvoy(const std::string& id, double cap, double fuel, double x, double y);
    ~GroundConvoy();
    double evaluateRoute(double dx, double dy, bool corridorDamaged) override;
    std::string vehicleType() const override { return "Ground"; }
};

class AmphibiousCraft : public virtual ReliefVehicle {
public:
    AmphibiousCraft(const std::string& id, double cap, double fuel, double x, double y);
    ~AmphibiousCraft();
    double evaluateRoute(double dx, double dy, bool corridorDamaged) override;
    std::string vehicleType() const override { return "Amphibious"; }
};

// Diamond: ReliefVehicle <- GroundConvoy / AmphibiousCraft <- AmphibiousMedicalUnit
// (virtual inheritance => exactly ONE ReliefVehicle / TransportVehicle sub-object)
class AmphibiousMedicalUnit : public GroundConvoy, public AmphibiousCraft {
public:
    AmphibiousMedicalUnit(const std::string& id, double cap, double fuel, double x, double y);
    ~AmphibiousMedicalUnit();
    double evaluateRoute(double dx, double dy, bool corridorDamaged) override;
    std::string vehicleType() const override { return "Amphibious Medical Unit"; }
};

// ---------- Unit I: overloaded supply allocation ----------
ReliefCargo allocateSupplies(const DistributionDepot& depot, const std::string& item, int volumeUnits);
ReliefCargo allocateSupplies(const DistributionDepot& depot, const std::string& item, int casualties, TriageLevel level);

// ---------- Unit IV: persistence ----------
struct TransitRecord {              // POD so it can be written to a binary stream
    char vehicleId[16];
    char cargo[24];
    int quantity;
    double tonnes;
    double distanceKm;
    double etaHours;
};

std::ostream& transitHeader(std::ostream& os);                       // custom manipulator
std::ostream& operator<<(std::ostream& os, const TransitRecord& r);

// Writes to "<path>.tmp" then renames over <path>, so readers never see a half-written file.
void atomicWrite(const std::string& path, const std::function<void(std::ostream&)>& writer);
void saveTransitLog(const std::string& path, const std::vector<TransitRecord>& log);
std::vector<TransitRecord> loadTransitLog(const std::string& path);

// Transactional dispatch: on any failure, depot and vehicle are rolled back, then the exception rethrown.
double dispatchMission(DistributionDepot& depot, ReliefVehicle& vehicle, const ReliefCargo& load,
                       double destX, double destY, bool corridorDamaged, std::vector<TransitRecord>& log);

// ---------- Unit IV: functors for urgent medical needs ----------
struct MoreUrgent {   // for std::sort: highest priority first
    bool operator()(const ReliefCargo& a, const ReliefCargo& b) const {
        if (a.priority() != b.priority()) return a.priority() > b.priority();
        return a.quantity() > b.quantity();
    }
};
struct LessUrgent {   // for PriorityDispatchQueue (max-heap): urgent = "largest"
    bool operator()(const ReliefCargo& a, const ReliefCargo& b) const { return MoreUrgent()(b, a); }
};

} // namespace Logistics

// Friend of ReliefCargo and DistributionDepot: overrides access barriers for critical triage.
class IncidentCommander {
public:
    void triageOverride(Inventory::DistributionDepot& depot, const std::string& item, int newPriority) const;
    void auditDepot(const Inventory::DistributionDepot& depot) const;
};

} // namespace ReliefOps
#endif
