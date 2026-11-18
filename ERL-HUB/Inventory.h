// Inventory.h - namespace ReliefOps::Inventory
// Unit I : namespaces, inline utilities, call/return by reference, friend class
// Unit II: ADTs, static members, dynamic memory, deep copy, operator overloading
#ifndef RELIEFOPS_INVENTORY_H
#define RELIEFOPS_INVENTORY_H

#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "Exceptions.h"

namespace ReliefOps { class IncidentCommander; }   // forward declaration for friendship

namespace ReliefOps {
namespace Inventory {

// ---------- Unit I: inline utility functions ----------
inline double distanceKm(double x1, double y1, double x2, double y2) {
    return std::sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
}
inline double payloadTonnes(int quantity, double unitWeightKg) {
    return quantity * unitWeightKg / 1000.0;
}
// Call by reference: modifies the caller's counter directly, no copy.
inline void applyDelta(int& stock, int delta) {
    if (stock + delta < 0) throw InsufficientStockException("stock counter", -delta, stock);
    stock += delta;
}

enum class TriageLevel { Minor = 1, Delayed = 2, Immediate = 3 };

// ---------- Unit IV: user-defined I/O manipulators ----------
std::ostream& manifestRule(std::ostream& os);
std::ostream& manifestHeader(std::ostream& os);

// ---------- Unit II: ReliefCargo (abstract data type, owns a heap C-string) ----------
class ReliefCargo {
public:
    ReliefCargo();                                                       // default
    ReliefCargo(const std::string& name, int quantity, double unitWeightKg, int priority = 1); // parameterized + dynamic
    ReliefCargo(const ReliefCargo& other);                               // deep copy
    ReliefCargo& operator=(const ReliefCargo& other);                    // deep assignment
    ~ReliefCargo();                                                      // explicit destructor

    const char* name() const { return name_; }
    int quantity() const { return quantity_; }
    double unitWeightKg() const { return unitWeightKg_; }
    int priority() const { return priority_; }
    double totalWeightKg() const { return quantity_ * unitWeightKg_; }

    void addQuantity(int n);
    void removeQuantity(int n);

    bool operator==(const ReliefCargo& other) const;   // manifest identity / duplicate check
    bool operator!=(const ReliefCargo& other) const { return !(*this == other); }

    void printRow(std::ostream& os) const;
    void writeBinary(std::ostream& os) const;
    void readBinary(std::istream& is);

    friend std::ostream& operator<<(std::ostream& os, const ReliefCargo& c);
    friend std::istream& operator>>(std::istream& is, ReliefCargo& c);
    friend class ReliefOps::IncidentCommander;          // may bypass access during triage override

private:
    char* name_;
    int quantity_;
    double unitWeightKg_;
    int priority_;     // 1 (routine) .. 5 (life-critical)
};

// ---------- Unit II: DistributionDepot ----------
class DistributionDepot {
public:
    DistributionDepot(const std::string& name, double x, double y);
    DistributionDepot(const DistributionDepot& other);
    DistributionDepot& operator=(const DistributionDepot& other) = default;
    ~DistributionDepot();

    const std::string& name() const { return name_; }
    double x() const { return x_; }
    double y() const { return y_; }

    DistributionDepot& operator+=(const ReliefCargo& c);   // restock
    DistributionDepot& operator-=(const ReliefCargo& c);   // deduct on vehicle departure

    bool contains(const std::string& item) const;
    int quantityOf(const std::string& item) const;
    ReliefCargo& cargoRef(const std::string& item);               // return by reference
    const ReliefCargo& cargoRef(const std::string& item) const;
    std::vector<ReliefCargo> snapshot() const;

    static int depotCount() { return depotCount_; }

    friend std::ostream& operator<<(std::ostream& os, const DistributionDepot& d);
    friend class ReliefOps::IncidentCommander;

private:
    std::string name_;
    double x_, y_;
    std::unordered_map<std::string, ReliefCargo> stock_;
    static int depotCount_;
};

} // namespace Inventory
} // namespace ReliefOps
#endif
