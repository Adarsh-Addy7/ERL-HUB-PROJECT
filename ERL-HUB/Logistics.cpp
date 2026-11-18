#include "Logistics.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>

namespace ReliefOps {
namespace Logistics {

// ================= TransportVehicle =================
int TransportVehicle::operationalVehicles_ = 0;
double TransportVehicle::dispatchedTonnes_ = 0.0;

TransportVehicle::TransportVehicle(const std::string& id, double capacityKg, double fuelLitres, int initialSlots)
    : id_(id), capacityKg_(capacityKg), fuelLitres_(fuelLitres),
      manifest_(new ReliefCargo[initialSlots > 0 ? initialSlots : 1]), manifestSize_(0),
      manifestCap_(initialSlots > 0 ? initialSlots : 1) {
    ++operationalVehicles_;
}

TransportVehicle::TransportVehicle(const TransportVehicle& o)
    : id_(o.id_), capacityKg_(o.capacityKg_), fuelLitres_(o.fuelLitres_), manifest_(new ReliefCargo[o.manifestCap_]),
      manifestSize_(o.manifestSize_), manifestCap_(o.manifestCap_) {
    for (int i = 0; i < manifestSize_; ++i) manifest_[i] = o.manifest_[i];
    ++operationalVehicles_;
}

TransportVehicle& TransportVehicle::operator=(const TransportVehicle& o) {
    if (this != &o) {
        ReliefCargo* fresh = new ReliefCargo[o.manifestCap_];
        for (int i = 0; i < o.manifestSize_; ++i) fresh[i] = o.manifest_[i];
        delete[] manifest_;
        manifest_ = fresh;
        manifestSize_ = o.manifestSize_;
        manifestCap_ = o.manifestCap_;
        id_ = o.id_;
        capacityKg_ = o.capacityKg_;
        fuelLitres_ = o.fuelLitres_;
    }
    return *this;
}

TransportVehicle::~TransportVehicle() {
    delete[] manifest_;
    --operationalVehicles_;
}

double TransportVehicle::loadKg() const {
    double sum = 0;
    for (int i = 0; i < manifestSize_; ++i) sum += manifest_[i].totalWeightKg();
    return sum;
}

void TransportVehicle::checkCapacity(const ReliefCargo& c) const {
    double after = loadKg() + c.totalWeightKg();
    if (after > capacityKg_) throw CargoOverweightException(id_, after, capacityKg_);
}

void TransportVehicle::growManifest() {
    int newCap = manifestCap_ * 2;
    ReliefCargo* fresh = new ReliefCargo[newCap];
    for (int i = 0; i < manifestSize_; ++i) fresh[i] = manifest_[i];
    delete[] manifest_;
    manifest_ = fresh;
    manifestCap_ = newCap;
}

TransportVehicle& TransportVehicle::operator+=(const ReliefCargo& c) {
    checkCapacity(c);
    if (manifestSize_ == manifestCap_) growManifest();
    manifest_[manifestSize_++] = c;
    return *this;
}

ReliefCargo TransportVehicle::unloadLast() {
    if (manifestSize_ == 0) throw ReliefException("[Manifest] vehicle " + id_ + " has nothing to unload");
    return manifest_[--manifestSize_];
}

bool TransportVehicle::operator==(const TransportVehicle& o) const {
    if (manifestSize_ != o.manifestSize_) return false;
    for (int i = 0; i < manifestSize_; ++i)
        if (manifest_[i] != o.manifest_[i]) return false;
    return true;
}

bool TransportVehicle::hasDuplicateOf(const ReliefCargo& c) const {
    for (int i = 0; i < manifestSize_; ++i)
        if (manifest_[i] == c) return true;
    return false;
}

void TransportVehicle::saveManifest(std::ostream& os) const {
    std::uint32_t n = static_cast<std::uint32_t>(manifestSize_);
    os.write(reinterpret_cast<const char*>(&n), sizeof n);
    for (int i = 0; i < manifestSize_; ++i) manifest_[i].writeBinary(os);
}

void TransportVehicle::loadManifest(std::istream& is) {
    std::uint32_t n = 0;
    is.read(reinterpret_cast<char*>(&n), sizeof n);
    if (!is || n > 100000) throw ReliefException("[Corrupt] invalid manifest header");
    std::vector<ReliefCargo> temp(n);          // read everything first, commit only if all good
    for (std::uint32_t i = 0; i < n; ++i) temp[i].readBinary(is);
    int cap = n > 0 ? static_cast<int>(n) : 1;
    ReliefCargo* fresh = new ReliefCargo[cap];
    for (std::uint32_t i = 0; i < n; ++i) fresh[i] = temp[i];
    delete[] manifest_;
    manifest_ = fresh;
    manifestSize_ = static_cast<int>(n);
    manifestCap_ = cap;
}

std::ostream& operator<<(std::ostream& os, const TransportVehicle& v) {
    os << "Vehicle " << v.id_ << " | capacity " << fmt(v.capacityKg_) << " kg | fuel " << fmt(v.fuelLitres_)
       << " L | load " << fmt(v.loadKg()) << " kg\n" << Inventory::manifestHeader;
    for (int i = 0; i < v.manifestSize_; ++i) v.manifest_[i].printRow(os);
    return os << Inventory::manifestRule;
}

// Format: <count> followed by <count> cargo records ("Name qty kg priority")
std::istream& operator>>(std::istream& is, TransportVehicle& v) {
    int n;
    if (is >> n) {
        TransportVehicle tmp(v.id_, v.capacityKg_, v.fuelLitres_);
        for (int i = 0; i < n; ++i) {
            ReliefCargo c;
            if (!(is >> c)) return is;
            tmp += c;
        }
        v = tmp;
    }
    return is;
}

// ================= ReliefVehicle hierarchy =================
ReliefVehicle::ReliefVehicle(const std::string& id, double cap, double fuel, double x, double y)
    : TransportVehicle(id, cap, fuel), gps_(id + "-GPS", x, y), lifeSupport_(120, 4) {}

ReliefVehicle::~ReliefVehicle() { std::cout << "    [dtor] ReliefVehicle base released (" << id() << ")\n"; }

double ReliefVehicle::commitRoute(double dx, double dy, double litresPerKm, double speedKmh) {
    double km = Inventory::distanceKm(gps_.x(), gps_.y(), dx, dy);
    double need = km * litresPerKm;
    if (need > fuel()) throw FuelDepletionException(id(), need, fuel());
    fuelRef() -= need;
    gps_.moveTo(dx, dy);
    return km / speedKmh;
}

AerialTransport::AerialTransport(const std::string& id, double cap, double fuel, double x, double y)
    : ReliefVehicle(id, cap, fuel, x, y) {}
AerialTransport::~AerialTransport() { std::cout << "    [dtor] AerialTransport(" << id() << ")\n"; }
double AerialTransport::evaluateRoute(double dx, double dy, bool) {
    return commitRoute(dx, dy, 2.5, 220.0);                      // flies over damaged corridors
}

GroundConvoy::GroundConvoy(const std::string& id, double cap, double fuel, double x, double y)
    : ReliefVehicle(id, cap, fuel, x, y) {}
GroundConvoy::~GroundConvoy() { std::cout << "    [dtor] GroundConvoy(" << id() << ")\n"; }
double GroundConvoy::evaluateRoute(double dx, double dy, bool corridorDamaged) {
    if (corridorDamaged) throw DamagedCorridorException(id(), "road");
    return commitRoute(dx, dy, 0.6, 45.0);
}

AmphibiousCraft::AmphibiousCraft(const std::string& id, double cap, double fuel, double x, double y)
    : ReliefVehicle(id, cap, fuel, x, y) {}
AmphibiousCraft::~AmphibiousCraft() { std::cout << "    [dtor] AmphibiousCraft(" << id() << ")\n"; }
double AmphibiousCraft::evaluateRoute(double dx, double dy, bool) {
    return commitRoute(dx, dy, 1.2, 30.0);                       // switches to waterways
}

// The most-derived class initialises the single shared virtual base itself.
AmphibiousMedicalUnit::AmphibiousMedicalUnit(const std::string& id, double cap, double fuel, double x, double y)
    : ReliefVehicle(id, cap, fuel, x, y), GroundConvoy(id, cap, fuel, x, y), AmphibiousCraft(id, cap, fuel, x, y) {}
AmphibiousMedicalUnit::~AmphibiousMedicalUnit() { std::cout << "    [dtor] AmphibiousMedicalUnit(" << id() << ")\n"; }
double AmphibiousMedicalUnit::evaluateRoute(double dx, double dy, bool corridorDamaged) {
    // Final overrider resolves the ambiguity between the two parents.
    return corridorDamaged ? commitRoute(dx, dy, 0.9, 30.0)      // water mode
                           : commitRoute(dx, dy, 0.9, 45.0);     // road mode
}

// ================= Overloaded allocation (Unit I) =================
ReliefCargo allocateSupplies(const DistributionDepot& depot, const std::string& item, int volumeUnits) {
    const ReliefCargo& s = depot.cargoRef(item);
    int qty = std::min(volumeUnits, s.quantity());
    if (qty <= 0) throw InsufficientStockException(item, volumeUnits, s.quantity());
    return ReliefCargo(item, qty, s.unitWeightKg(), 1);
}

ReliefCargo allocateSupplies(const DistributionDepot& depot, const std::string& item, int casualties, TriageLevel level) {
    const ReliefCargo& s = depot.cargoRef(item);
    int perPatient = (level == TriageLevel::Immediate) ? 3 : (level == TriageLevel::Delayed) ? 2 : 1;
    int qty = std::min(casualties * perPatient, s.quantity());
    if (qty <= 0) throw InsufficientStockException(item, casualties * perPatient, s.quantity());
    return ReliefCargo(item, qty, s.unitWeightKg(), static_cast<int>(level) + 2);   // priority 3..5
}

// ================= Persistence (Unit IV) =================
std::ostream& transitHeader(std::ostream& os) {
    std::ios::fmtflags f = os.flags();
    os << std::string(70, '-') << '\n' << std::left << std::setw(10) << "Vehicle" << std::setw(18) << "Cargo"
       << std::right << std::setw(6) << "Qty" << std::setw(10) << "Tonnes" << std::setw(12) << "Dist(km)"
       << std::setw(10) << "ETA(h)" << '\n' << std::string(70, '-') << '\n';
    os.flags(f);
    return os;
}

std::ostream& operator<<(std::ostream& os, const TransitRecord& r) {
    std::ios::fmtflags f = os.flags();
    std::streamsize p = os.precision();
    os << std::left << std::setw(10) << r.vehicleId << std::setw(18) << r.cargo << std::right << std::setw(6)
       << r.quantity << std::fixed << std::setprecision(2) << std::setw(10) << r.tonnes << std::setw(12)
       << r.distanceKm << std::setw(10) << r.etaHours;
    os.flags(f);
    os.precision(p);
    return os;
}

void atomicWrite(const std::string& path, const std::function<void(std::ostream&)>& writer) {
    const std::string tmp = path + ".tmp";
    {
        std::fstream out(tmp.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
        if (!out) throw ReliefException("[IO] cannot open " + tmp);
        writer(out);
        out.flush();
        if (!out) {
            out.close();
            std::remove(tmp.c_str());
            throw ReliefException("[IO] write failed, original file left untouched: " + path);
        }
    }
    if (std::rename(tmp.c_str(), path.c_str()) != 0) {      // Windows cannot rename over an existing file
        std::remove(path.c_str());
        if (std::rename(tmp.c_str(), path.c_str()) != 0) {
            std::remove(tmp.c_str());
            throw ReliefException("[IO] cannot replace " + path);
        }
    }
}

static const char kMagic[4] = {'R', 'L', 'H', '1'};

void saveTransitLog(const std::string& path, const std::vector<TransitRecord>& log) {
    atomicWrite(path, [&](std::ostream& os) {
        os.write(kMagic, 4);
        std::uint32_t placeholder = 0;
        std::streampos countPos = os.tellp();
        os.write(reinterpret_cast<const char*>(&placeholder), sizeof placeholder);
        for (const TransitRecord& r : log) os.write(reinterpret_cast<const char*>(&r), sizeof r);
        std::uint32_t n = static_cast<std::uint32_t>(log.size());
        std::streampos end = os.tellp();
        os.seekp(countPos);                                   // patch the record count in the header
        os.write(reinterpret_cast<const char*>(&n), sizeof n);
        os.seekp(end);
    });
}

std::vector<TransitRecord> loadTransitLog(const std::string& path) {
    std::fstream in(path.c_str(), std::ios::in | std::ios::binary);
    if (!in) throw ReliefException("[IO] cannot open " + path);
    char magic[4];
    std::uint32_t n = 0;
    in.read(magic, 4);
    in.read(reinterpret_cast<char*>(&n), sizeof n);
    if (!in || std::memcmp(magic, kMagic, 4) != 0 || n > 1000000)
        throw ReliefException("[Corrupt] " + path + " is not a valid transit log");
    std::vector<TransitRecord> log(n);
    for (std::uint32_t i = 0; i < n; ++i) in.read(reinterpret_cast<char*>(&log[i]), sizeof(TransitRecord));
    if (!in) throw ReliefException("[Corrupt] " + path + " is truncated");
    return log;
}

double dispatchMission(DistributionDepot& depot, ReliefVehicle& v, const ReliefCargo& load, double dx, double dy,
                       bool corridorDamaged, std::vector<TransitRecord>& log) {
    depot -= load;                       // throws before any change if stock is insufficient
    bool loaded = false;
    try {
        v += load;                       // may throw CargoOverweightException
        loaded = true;
        double km = Inventory::distanceKm(v.gps().x(), v.gps().y(), dx, dy);
        double eta = v.evaluateRoute(dx, dy, corridorDamaged);   // may throw route/fuel exceptions
        double tonnes = Inventory::payloadTonnes(load.quantity(), load.unitWeightKg());
        TransportVehicle::recordDispatch(tonnes);
        TransitRecord r;
        std::memset(&r, 0, sizeof r);
        std::strncpy(r.vehicleId, v.id().c_str(), sizeof r.vehicleId - 1);
        std::strncpy(r.cargo, load.name(), sizeof r.cargo - 1);
        r.quantity = load.quantity();
        r.tonnes = tonnes;
        r.distanceKm = km;
        r.etaHours = eta;
        log.push_back(r);
        return eta;
    } catch (...) {
        if (loaded) v.unloadLast();      // roll back the vehicle
        depot += load;                   // roll back the depot
        throw;
    }
}

} // namespace Logistics

// ================= IncidentCommander (friend) =================
void IncidentCommander::triageOverride(Inventory::DistributionDepot& depot, const std::string& item, int newPriority) const {
    Inventory::ReliefCargo& c = depot.stock_.at(item);   // direct access to private data
    c.priority_ = newPriority;
}

void IncidentCommander::auditDepot(const Inventory::DistributionDepot& depot) const {
    std::cout << "  [IncidentCommander audit] depot '" << depot.name_ << "' tracks " << depot.stock_.size()
              << " cargo lines (raw private map)\n";
}

} // namespace ReliefOps
