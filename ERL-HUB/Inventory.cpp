#include "Inventory.h"

#include <algorithm>
#include <cstdint>
#include <cstring>

namespace ReliefOps {
namespace Inventory {

static char* dupString(const char* s) {
    char* p = new char[std::strlen(s) + 1];
    std::strcpy(p, s);
    return p;
}

// ---------- manipulators ----------
std::ostream& manifestRule(std::ostream& os) { return os << std::string(54, '-') << '\n'; }

std::ostream& manifestHeader(std::ostream& os) {
    std::ios::fmtflags f = os.flags();
    os << manifestRule << std::left << std::setw(18) << "Item" << std::right << std::setw(8) << "Qty"
       << std::setw(10) << "Kg/unit" << std::setw(12) << "Total kg" << std::setw(6) << "Pri" << '\n'
       << manifestRule;
    os.flags(f);
    return os;
}

// ---------- ReliefCargo ----------
ReliefCargo::ReliefCargo() : name_(dupString("unnamed")), quantity_(0), unitWeightKg_(0.0), priority_(1) {}

ReliefCargo::ReliefCargo(const std::string& name, int quantity, double unitWeightKg, int priority)
    : name_(dupString(name.c_str())), quantity_(quantity), unitWeightKg_(unitWeightKg), priority_(priority) {}

ReliefCargo::ReliefCargo(const ReliefCargo& o)
    : name_(dupString(o.name_)), quantity_(o.quantity_), unitWeightKg_(o.unitWeightKg_), priority_(o.priority_) {}

ReliefCargo& ReliefCargo::operator=(const ReliefCargo& o) {
    if (this != &o) {
        char* fresh = dupString(o.name_);   // allocate first: safe if it throws
        delete[] name_;
        name_ = fresh;
        quantity_ = o.quantity_;
        unitWeightKg_ = o.unitWeightKg_;
        priority_ = o.priority_;
    }
    return *this;
}

ReliefCargo::~ReliefCargo() { delete[] name_; }

void ReliefCargo::addQuantity(int n) { applyDelta(quantity_, n); }

void ReliefCargo::removeQuantity(int n) {
    if (n > quantity_) throw InsufficientStockException(name_, n, quantity_);
    applyDelta(quantity_, -n);
}

bool ReliefCargo::operator==(const ReliefCargo& o) const {
    return std::strcmp(name_, o.name_) == 0 && quantity_ == o.quantity_ && unitWeightKg_ == o.unitWeightKg_;
}

void ReliefCargo::printRow(std::ostream& os) const {
    std::ios::fmtflags f = os.flags();
    std::streamsize p = os.precision();
    os << std::left << std::setw(18) << name_ << std::right << std::setw(8) << quantity_ << std::fixed
       << std::setprecision(1) << std::setw(10) << unitWeightKg_ << std::setw(12) << totalWeightKg()
       << std::setw(6) << priority_ << '\n';
    os.flags(f);
    os.precision(p);
}

void ReliefCargo::writeBinary(std::ostream& os) const {
    std::uint32_t len = static_cast<std::uint32_t>(std::strlen(name_));
    os.write(reinterpret_cast<const char*>(&len), sizeof len);
    os.write(name_, len);
    os.write(reinterpret_cast<const char*>(&quantity_), sizeof quantity_);
    os.write(reinterpret_cast<const char*>(&unitWeightKg_), sizeof unitWeightKg_);
    os.write(reinterpret_cast<const char*>(&priority_), sizeof priority_);
}

void ReliefCargo::readBinary(std::istream& is) {
    std::uint32_t len = 0;
    is.read(reinterpret_cast<char*>(&len), sizeof len);
    if (!is || len > 1024) throw ReliefException("[Corrupt] invalid cargo record");
    std::string n(len, '\0');
    is.read(&n[0], len);
    int q = 0, p = 0;
    double w = 0;
    is.read(reinterpret_cast<char*>(&q), sizeof q);
    is.read(reinterpret_cast<char*>(&w), sizeof w);
    is.read(reinterpret_cast<char*>(&p), sizeof p);
    if (!is) throw ReliefException("[Corrupt] truncated cargo record");
    *this = ReliefCargo(n, q, w, p);
}

std::ostream& operator<<(std::ostream& os, const ReliefCargo& c) {
    return os << c.name_ << " x" << c.quantity_ << " @ " << fmt(c.unitWeightKg_) << " kg/unit [P" << c.priority_ << "]";
}

// Expected text format: Name quantity unitWeightKg priority   (use no spaces in Name)
std::istream& operator>>(std::istream& is, ReliefCargo& c) {
    std::string n;
    int q, p;
    double w;
    if (is >> n >> q >> w >> p) c = ReliefCargo(n, q, w, p);
    return is;
}

// ---------- DistributionDepot ----------
int DistributionDepot::depotCount_ = 0;

DistributionDepot::DistributionDepot(const std::string& name, double x, double y) : name_(name), x_(x), y_(y) {
    ++depotCount_;
}
DistributionDepot::DistributionDepot(const DistributionDepot& o) : name_(o.name_), x_(o.x_), y_(o.y_), stock_(o.stock_) {
    ++depotCount_;
}
DistributionDepot::~DistributionDepot() { --depotCount_; }

DistributionDepot& DistributionDepot::operator+=(const ReliefCargo& c) {
    auto it = stock_.find(c.name());
    if (it == stock_.end()) stock_[std::string(c.name())] = c;
    else it->second.addQuantity(c.quantity());
    return *this;
}

DistributionDepot& DistributionDepot::operator-=(const ReliefCargo& c) {
    auto it = stock_.find(c.name());
    if (it == stock_.end()) throw InsufficientStockException(c.name(), c.quantity(), 0);
    it->second.removeQuantity(c.quantity());   // throws before mutating if insufficient
    return *this;
}

bool DistributionDepot::contains(const std::string& item) const { return stock_.find(item) != stock_.end(); }

int DistributionDepot::quantityOf(const std::string& item) const {
    auto it = stock_.find(item);
    return it == stock_.end() ? 0 : it->second.quantity();
}

ReliefCargo& DistributionDepot::cargoRef(const std::string& item) {
    auto it = stock_.find(item);
    if (it == stock_.end()) throw InsufficientStockException(item, 1, 0);
    return it->second;
}
const ReliefCargo& DistributionDepot::cargoRef(const std::string& item) const {
    auto it = stock_.find(item);
    if (it == stock_.end()) throw InsufficientStockException(item, 1, 0);
    return it->second;
}

std::vector<ReliefCargo> DistributionDepot::snapshot() const {
    std::vector<ReliefCargo> v;
    for (const auto& kv : stock_) v.push_back(kv.second);
    return v;
}

std::ostream& operator<<(std::ostream& os, const DistributionDepot& d) {
    os << "Depot " << d.name_ << " @(" << d.x_ << "," << d.y_ << ")\n" << manifestHeader;
    std::vector<const ReliefCargo*> rows;
    for (const auto& kv : d.stock_) rows.push_back(&kv.second);
    std::sort(rows.begin(), rows.end(),
              [](const ReliefCargo* a, const ReliefCargo* b) { return std::strcmp(a->name(), b->name()) < 0; });
    for (const ReliefCargo* r : rows) r->printRow(os);
    return os << manifestRule;
}

} // namespace Inventory
} // namespace ReliefOps
