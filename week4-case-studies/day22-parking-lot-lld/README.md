# Day 22 - LLD: Parking Lot

## Problem
Design a parking lot system supporting:
- Multiple floors, multiple slot sizes (small, medium, large)
- Different vehicle types (bike, car, truck) - must fit slot
- Park, unpark, find available slot
- Generate ticket on entry
- Compute fare on exit (time-based)
- Multiple entry/exit gates

## Requirements (Functional)
1. Vehicle enters -> system finds nearest free slot of right size -> issues ticket
2. Vehicle exits -> compute fare from ticket -> free slot
3. Show availability (per floor / per slot type)

## Non-Functional
- Thread-safe slot allocation
- Extensible: add new slot types, new pricing strategies

## Patterns Used
| Pattern | Where |
|---------|-------|
| Strategy | Pricing, slot allocation |
| Factory | Vehicle creation |
| Singleton | ParkingLot manager |
| Observer (optional) | Notify operator when full |

## Files
- [parking_lot.cpp](parking_lot.cpp) - full single-file implementation

## Key Classes
```
Vehicle (abstract) - Bike, Car, Truck
SlotType - SMALL, MEDIUM, LARGE
ParkingSlot - id, type, isFree, vehicle*
ParkingFloor - vector<slots>
ParkingLot - vector<floors>, allocate(), free()
Ticket - id, vehicle, slot, entryTime
PricingStrategy (interface) - HourlyPricing, FlatPricing
```

## Interview Questions
1. How do you handle multiple entry gates concurrently? (lock per floor or atomic counter)
2. How would you scale to 1000 floors / millions of slots? (in-memory map per type, lazy free-list)
3. What if slots reserved in advance? (add Reservation entity)
4. EV charging slots - how to extend?
5. What if pricing varies by day/time? (Strategy + Decorator)
6. Persistence - how to recover state on restart?

## Daily Assignment
1. Extend pricing: weekend/weekday differential.
2. Add reserved slots for monthly subscribers.
3. Add capacity alerts when floor > 90% full (Observer).
4. Add multi-gate concurrent simulation with N threads.
