// Exceptions.h - Unit IV: custom exception framework derived from std::exception
#ifndef RELIEFOPS_EXCEPTIONS_H
#define RELIEFOPS_EXCEPTIONS_H

#include <exception>
#include <iomanip>
#include <sstream>
#include <string>

namespace ReliefOps {

inline std::string fmt(double v, int prec = 1) {
    std::ostringstream os;
    os << std::fixed << std::setprecision(prec) << v;
    return os.str();
}

// Common root so callers can catch every relief-related failure at once.
class ReliefException : public std::exception {
protected:
    std::string message_;
public:
    explicit ReliefException(const std::string& m) : message_(m) {}
    const char* what() const noexcept override { return message_.c_str(); }
};

class CargoOverweightException : public ReliefException {
public:
    CargoOverweightException(const std::string& vehicleId, double attemptedKg, double capacityKg)
        : ReliefException("[CargoOverweight] Vehicle " + vehicleId + ": load of " + fmt(attemptedKg) +
                          " kg exceeds capacity of " + fmt(capacityKg) + " kg") {}
};

class FuelDepletionException : public ReliefException {
public:
    FuelDepletionException(const std::string& vehicleId, double requiredL, double availableL)
        : ReliefException("[FuelDepletion] Vehicle " + vehicleId + ": route needs " + fmt(requiredL) +
                          " L but only " + fmt(availableL) + " L available") {}
};

class DamagedCorridorException : public ReliefException {
public:
    DamagedCorridorException(const std::string& vehicleId, const std::string& corridor)
        : ReliefException("[DamagedCorridor] Vehicle " + vehicleId + ": " + corridor + " route is impassable") {}
};

class InsufficientStockException : public ReliefException {
public:
    InsufficientStockException(const std::string& item, int requested, int available)
        : ReliefException("[InsufficientStock] " + item + ": requested " + std::to_string(requested) +
                          ", available " + std::to_string(available)) {}
};

} // namespace ReliefOps
#endif
