# Day 22 - LLD: Parking Lot

## LLD Interview Approach - A Systematic Framework

Before diving into the parking lot design, let us establish a repeatable framework for tackling any Low-Level Design (LLD) problem in an interview. This 6-step method ensures you cover all dimensions interviewers evaluate.

### Step 1: Clarify Requirements and Constraints

Never start designing immediately. Ask questions to scope the problem:

- **Functional requirements**: What does the system DO? List 5-8 core use cases.
- **Non-functional requirements**: Scale (how many concurrent users?), latency expectations, data consistency needs.
- **Constraints**: Single machine or distributed? Real-time or batch? Read-heavy or write-heavy?
- **Assumptions**: State explicitly what you are assuming (e.g., "I'll assume a single building with multiple floors").

**Why this matters**: Jumping into classes without understanding constraints leads to over-engineering or under-designing. An interviewer who says "design a parking lot" might mean a 10-slot garage or a 10,000-slot airport facility. The design differs dramatically.

### Step 2: Identify Core Entities / Objects

Extract nouns from requirements. Each noun is a candidate class:

```
Requirements mention: vehicles, slots, floors, tickets, gates, payments
                      -> Vehicle, ParkingSlot, ParkingFloor, Ticket, Gate, Payment
```

**Heuristic**: If a noun has state that changes over time, it deserves a class. If it is just data with no behavior, it might be a value object or enum.

### Step 3: Define Relationships and Cardinality

Map how entities relate:

```
ParkingLot 1---* ParkingFloor (composition: floors cannot exist without lot)
ParkingFloor 1---* ParkingSlot (composition)
ParkingSlot 1---0..1 Vehicle (association: slot may or may not hold a vehicle)
Ticket *---1 Vehicle (association)
Ticket *---1 ParkingSlot (association)
```

Key decisions:
- **Composition vs Aggregation**: Does the child's lifecycle depend on the parent?
- **Cardinality**: One-to-one, one-to-many, many-to-many?
- **Direction**: Can a Vehicle navigate to its Slot, or only Slot to Vehicle, or both?

### Step 4: Apply Design Patterns

Do not force patterns. Identify where variability exists and apply patterns to encapsulate it:

| Variability | Pattern | Reasoning |
|-------------|---------|-----------|
| Pricing rules change independently of core logic | Strategy | New pricing without modifying ParkingLot |
| Slot allocation algorithms (nearest, random, type-based) | Strategy | Swap algorithms at runtime |
| Single coordination point | Singleton | One ParkingLot instance manages global state |
| Creating vehicles without knowing concrete type at compile time | Factory | Decouple creation from usage |
| Reacting to capacity events | Observer | Decoupled notification without polling |

### Step 5: Handle Concurrency

Most LLD problems have concurrent access. Address:

- **What is the shared mutable state?** (slots, counters, ticket sequences)
- **What is the granularity of locking?** (per-slot, per-floor, global - each has trade-offs)
- **Are there race conditions?** (two cars arriving at the same slot simultaneously)
- **Can we use lock-free structures?** (atomic counters for IDs, CAS for slot status)

### Step 6: Plan Extensibility

Demonstrate you think beyond the immediate requirements:

- Map each SOLID principle to your design
- Identify 2-3 future features and show how your design accommodates them without modification
- Show the Open-Closed Principle in action: "Adding EV charging requires a new SlotType enum value and a new PricingStrategy - no existing code changes"

---

## Problem Statement

Design a parking lot system supporting:
- Multiple floors, multiple slot sizes (small, medium, large)
- Different vehicle types (bike, car, truck) that must fit appropriate slots
- Park, unpark, find available slot
- Generate ticket on entry
- Compute fare on exit (time-based)
- Multiple entry/exit gates with concurrent operation

## Requirements Analysis

### Functional Requirements
1. Vehicle enters -> system finds nearest free slot of right size -> issues ticket
2. Vehicle exits -> compute fare from ticket -> free slot
3. Show availability (per floor / per slot type)
4. Support multiple entry and exit gates simultaneously
5. Generate unique ticket IDs

### Non-Functional Requirements
- Thread-safe slot allocation (no double-parking)
- Extensible: add new slot types, new pricing strategies without modifying core
- Recoverable: restart without losing state of parked vehicles
- Responsive: slot allocation in O(1) or O(log n) time

### Constraints
- Single building, multiple floors
- Each slot holds exactly one vehicle
- Vehicle type determines minimum slot size (bike -> small, car -> medium, truck -> large)
- A larger slot CAN accommodate a smaller vehicle (car in large slot) if no matching slots available

---

## Detailed Class Responsibility Analysis

### Why Each Class Exists

| Class | Single Responsibility | Why Not Merge? |
|-------|----------------------|----------------|
| `Vehicle` (abstract) | Encapsulates vehicle identity and type | Separates vehicle concerns from parking concerns |
| `ParkingSlot` | Manages single slot state (free/occupied) | Unit of allocation - needs independent locking |
| `ParkingFloor` | Aggregates slots, provides floor-level queries | Locality of reference for "nearest slot" |
| `ParkingLot` | Orchestrates allocation across floors | Single entry point, enforces business rules |
| `Ticket` | Immutable record of parking session | Audit trail, decoupled from slot lifecycle |
| `PricingStrategy` | Computes fare given duration | Changes independently of lot structure |
| `SlotAllocationStrategy` | Finds best available slot | Algorithm varies (nearest gate, distribute evenly) |

### Class Hierarchy

```
Vehicle (abstract)
  |-- Bike
  |-- Car
  |-- Truck
  |-- (future: ElectricCar, Handicapped)

ParkingSlot
  - id: string
  - floor: int
  - type: SlotType {SMALL, MEDIUM, LARGE}
  - status: SlotStatus {FREE, RESERVED, OCCUPIED, MAINTENANCE}
  - vehicle: Vehicle* (nullptr if free)
  - mutex: per-slot lock

ParkingFloor
  - floorNumber: int
  - slots: vector<ParkingSlot>
  - freeCountByType: map<SlotType, atomic<int>>

ParkingLot (Singleton)
  - floors: vector<ParkingFloor>
  - activeTickets: map<ticketId, Ticket>
  - entryGates: vector<Gate>
  - exitGates: vector<Gate>
  - pricingStrategy: PricingStrategy*
  - allocationStrategy: SlotAllocationStrategy*

Ticket
  - id: string (unique, monotonic)
  - vehicle: Vehicle*
  - slot: ParkingSlot*
  - entryTime: time_point
  - exitTime: time_point (set on unpark)

PricingStrategy (interface)
  +computeFare(entryTime, exitTime, vehicleType) -> double

HourlyPricing : PricingStrategy
FlatRatePricing : PricingStrategy
TieredPricing : PricingStrategy
SurgePricing : PricingStrategy (decorator over base strategy)
```

---

## Design Pattern Justification

### Strategy Pattern for Pricing

**Why Strategy and not simple if-else?**

Consider the alternative:
```cpp
double computeFare(Ticket t) {
    if (pricingType == HOURLY) return hours * rate;
    else if (pricingType == FLAT) return flatRate;
    else if (pricingType == TIERED) { /* complex logic */ }
    // Every new pricing = modify this function (violates OCP)
}
```

With Strategy:
```cpp
class PricingStrategy {
public:
    virtual double computeFare(TimePoint entry, TimePoint exit, VehicleType vt) = 0;
};

// Adding weekend pricing: create new class, inject it. Zero changes to ParkingLot.
```

**Trade-off**: Strategy adds a virtual dispatch overhead (~1 nanosecond) but gains unlimited extensibility. For a parking lot where fare computation happens once per exit (low frequency), this overhead is negligible.

### Singleton for ParkingLot

**Why Singleton here?**

- Physical constraint: there IS only one parking lot
- Shared state: all gates must see the same slot availability
- Resource coordination: ticket ID generation must be globally unique

**Why not just a global variable?**
- Controlled initialization (lazy, thread-safe with Meyers' Singleton)
- Testability (can reset in tests, or use dependency injection as alternative)
- Explicit lifetime management

**When to avoid Singleton in LLD**: If the system could have multiple instances (multiple parking lots in a chain), use a Registry pattern instead.

### Factory for Vehicle Creation

**Why Factory?**
- At entry gates, we know the vehicle type (string from sensor/input) but need a Vehicle object
- Decouples gate logic from concrete vehicle classes
- Adding a new vehicle type (Electric, Handicapped) requires only factory modification

```cpp
class VehicleFactory {
public:
    static unique_ptr<Vehicle> create(VehicleType type, string licensePlate) {
        switch(type) {
            case BIKE: return make_unique<Bike>(licensePlate);
            case CAR: return make_unique<Car>(licensePlate);
            case TRUCK: return make_unique<Truck>(licensePlate);
        }
    }
};
```

### Observer for Capacity Alerts

**Why Observer?**
- Multiple stakeholders care about capacity events (display boards, operator alerts, mobile app)
- ParkingLot should not know about all subscribers
- Adding a new notification channel (SMS, webhook) requires zero changes to core

---

## Thread-Safety Analysis

### The Core Problem

Multiple entry gates operate simultaneously. Two cars arrive at the same instant. Both query "nearest free slot of type MEDIUM." Without synchronization, both get the same slot - one car has nowhere to park.

### Mutex Granularity Options

```
+-------------------------------------------------------------------+
| Granularity     | Contention | Throughput | Complexity | Deadlock  |
+-----------------+------------+------------+------------+-----------+
| Global mutex    | HIGH       | LOW        | LOW        | None      |
| Per-floor mutex | MEDIUM     | MEDIUM     | MEDIUM     | Possible  |
| Per-slot mutex  | LOW        | HIGH       | HIGH       | Possible  |
| Lock-free (CAS) | NONE       | HIGHEST    | HIGHEST    | None      |
+-----------------+------------+------------+------------+-----------+
```

### Analysis of Each Approach

**Global Mutex (simplest)**
```cpp
class ParkingLot {
    mutex globalLock;
    ParkingSlot* allocate(VehicleType vt) {
        lock_guard<mutex> lk(globalLock);
        // find and allocate - completely serial
    }
};
```
- Good for: small lots (< 100 slots), low concurrency (< 10 gates)
- Bad for: large airport lot with 50 entry gates - all serialize on one lock

**Per-Floor Mutex (balanced)**
```cpp
class ParkingFloor {
    mutex floorLock;
    ParkingSlot* allocateOnFloor(VehicleType vt) {
        lock_guard<mutex> lk(floorLock);
        // Only blocks other allocations on SAME floor
    }
};
```
- Good for: multi-floor lots where gates are floor-specific
- Deadlock risk: if allocation tries floor 1, fails, tries floor 2 - need consistent ordering

**Per-Slot CAS (highest throughput)**
```cpp
class ParkingSlot {
    atomic<bool> isFree{true};
    bool tryOccupy(Vehicle* v) {
        bool expected = true;
        if (isFree.compare_exchange_strong(expected, false)) {
            vehicle = v;
            return true;
        }
        return false; // Someone else got it
    }
};
```
- Good for: highest concurrency, no blocking
- Bad for: "nearest slot" query becomes complex (scan and CAS loop)

### Recommended Approach for Interview

Use **per-floor mutex** as the default answer. It balances:
- Throughput: floors are independent, so N floors = N concurrent allocations
- Simplicity: single lock per floor, no deadlock if you allocate top-down
- Reasoning: "Most parking lots have floor-level entry, so per-floor locking matches physical access patterns"

---

## Slot Lifecycle State Machine

```
                    +------------------+
                    |                  |
                    v                  |
    +-------+   reserve()   +----------+   timeout/cancel
    | FREE  | -----------> | RESERVED  | ----------------+
    +-------+              +----------+                  |
        |                       |                        |
        | occupy()              | arrive()               |
        |                       v                        |
        |               +----------+                     |
        +-------------> | OCCUPIED |                     |
                        +----------+                     |
                              |                          |
                              | vacate()                 |
                              v                          |
                        +----------+                     |
                        |   FREE   | <-------------------+
                        +----------+
                              |
                              | markMaintenance()
                              v
                        +-------------+
                        | MAINTENANCE |
                        +-------------+
                              |
                              | clearMaintenance()
                              v
                        +----------+
                        |   FREE   |
                        +----------+
```

### State Transitions and Guards

| From | To | Trigger | Guard Condition |
|------|----|---------|-----------------|
| FREE | RESERVED | reserve(vehicleType, time) | slot.type >= vehicleType |
| FREE | OCCUPIED | occupy(vehicle) | slot.type >= vehicle.type |
| RESERVED | OCCUPIED | arrive(vehicle) | vehicle matches reservation |
| RESERVED | FREE | timeout / cancel | reservation expired |
| OCCUPIED | FREE | vacate() | ticket paid |
| FREE | MAINTENANCE | markMaintenance() | admin action |
| MAINTENANCE | FREE | clearMaintenance() | admin action |

---

## Pricing Strategy Deep Dive

### Strategy Comparison

| Strategy | Formula | Best For | Trade-off |
|----------|---------|----------|-----------|
| Hourly | `ceil(hours) * rate_per_hour` | Short-term parking (malls) | Simple but penalizes 1h01m same as 2h |
| Flat Rate | `fixed_amount` | Events, daily max | Predictable revenue, unfair to short stays |
| Tiered | `first_hour * r1 + next_3 * r2 + beyond * r3` | Airports, mixed use | Complex but fair |
| Surge | `base_rate * demand_multiplier` | Peak hours (stadiums) | Maximizes revenue, may anger customers |
| Subscription | `monthly_fee, unlimited access` | Office workers | Guaranteed revenue, capacity planning harder |

### Tiered Pricing Example

```cpp
class TieredPricing : public PricingStrategy {
    // First hour: $5
    // Hours 2-4: $3/hr
    // Beyond 4: $2/hr
    // Daily cap: $25
public:
    double computeFare(TimePoint entry, TimePoint exit, VehicleType vt) override {
        double hours = duration_hours(entry, exit);
        double fare = 0;
        
        if (hours <= 1) fare = 5.0;
        else if (hours <= 4) fare = 5.0 + (hours - 1) * 3.0;
        else fare = 5.0 + 9.0 + (hours - 4) * 2.0;
        
        fare = min(fare, 25.0); // Daily cap
        fare *= vehicleMultiplier(vt); // Truck = 2x, Bike = 0.5x
        return fare;
    }
};
```

### Combining Strategies (Decorator Pattern)

```cpp
class SurgeDecorator : public PricingStrategy {
    PricingStrategy* base;
    double multiplier;
public:
    double computeFare(...) override {
        return base->computeFare(...) * multiplier;
    }
};

// Usage: new SurgeDecorator(new TieredPricing(), 1.5) during peak hours
```

**Why Decorator over modifying TieredPricing?** Because surge is orthogonal - it can apply to ANY base strategy. Decorator composition gives us `hourly + surge`, `flat + surge`, `tiered + weekend` without combinatorial explosion.

---

## Extensibility Analysis with SOLID Mapping

### Adding EV Charging Slots

| SOLID Principle | How Design Supports It |
|-----------------|------------------------|
| **S** (SRP) | New `EVChargingSlot` subclass handles charging logic; `ParkingSlot` unchanged |
| **O** (OCP) | Add `ELECTRIC` to SlotType enum; existing allocation logic works via switch extension |
| **L** (LSP) | `EVChargingSlot` IS-A `ParkingSlot` - can be used anywhere a slot is expected |
| **I** (ISP) | Add `IChargeable` interface only for EV slots; non-EV slots don't implement it |
| **D** (DIP) | `ParkingLot` depends on `ParkingSlot` abstraction, not concrete types |

### Adding Valet Service

```
New classes needed:
  - ValetRequest (entity)
  - ValetService (manages request queue)
  - ValetPricing (extends PricingStrategy - adds valet fee)

Existing code changes: NONE
  - ValetService uses ParkingLot.allocate() internally
  - It is a layer ON TOP of existing system
```

### Adding Advance Reservations

```
New classes needed:
  - Reservation (entity: vehicle, slotType, startTime, endTime)
  - ReservationManager (handles CRUD, conflict detection)

Existing code changes: MINIMAL
  - ParkingSlot gains RESERVED state (already in our state machine)
  - AllocationStrategy must skip reserved slots
```

---

## Persistence Layer Design

### Why Persistence?

If the system crashes, all parked vehicles lose their tickets. On restart, we need to know:
- Which slots are occupied
- Which vehicles are parked where
- Active tickets and their entry times

### Approach 1: Periodic Snapshot

```
Every 30 seconds:
  - Serialize ParkingLot state to JSON/binary file
  - On restart: load snapshot, reconstruct in-memory state

Pros: Simple, fast recovery
Cons: Up to 30 seconds of data loss, snapshot might be inconsistent
```

### Approach 2: Write-Ahead Log (WAL)

```
Every state change (park, unpark, reserve):
  - Append operation to log file BEFORE updating in-memory state
  - On restart: replay log from beginning (or last checkpoint)

Pros: No data loss, every operation recorded
Cons: Log grows forever (need compaction), replay can be slow
```

### Approach 3: Database-Backed (Production)

```
Every state change:
  - Write to database (SQLite for single-machine, PostgreSQL for distributed)
  - In-memory state serves as cache

Schema:
  CREATE TABLE slots (id, floor, type, status, vehicle_plate);
  CREATE TABLE tickets (id, vehicle_plate, slot_id, entry_time, exit_time, fare);
  CREATE TABLE vehicles (plate PK, type, owner_name);
```

### Recommended for Interview

Mention WAL for correctness, then say "in production, we would use a database for persistence and treat in-memory as a cache with write-through policy." This shows you understand both the mechanism and the practical choice.

---

## Capacity and Performance Estimates

### Slot Lookup Performance

| Data Structure | Lookup | Insert/Remove | Space | Best For |
|----------------|--------|---------------|-------|----------|
| Free list per type | O(1) pop | O(1) push | O(n) | Fast allocation |
| Priority queue (by distance to gate) | O(log n) | O(log n) | O(n) | "Nearest" semantics |
| Bitmap per floor | O(1) with ffs() | O(1) bit flip | O(n/8) | Compact, cache-friendly |

### Scale Estimation

For a large airport parking lot:
- 10 floors x 1000 slots = 10,000 total slots
- Peak entries: 500 vehicles/hour = ~8/second
- Lock contention at per-floor granularity: 8/sec / 10 floors = 0.8 allocations/floor/sec
- Each allocation takes ~1 microsecond of critical section time
- Contention probability: negligible (< 0.0001% collision rate)

**Conclusion**: Per-floor mutex is more than sufficient for any realistic parking lot.

---

## Complete Design - Putting It All Together

```
+-------------------------------------------------------+
|                    ParkingLot (Singleton)              |
|  - floors: vector<ParkingFloor>                       |
|  - pricingStrategy: PricingStrategy*                  |
|  - allocationStrategy: AllocationStrategy*            |
|  - ticketCounter: atomic<uint64_t>                    |
+-------------------------------------------------------+
        |                    |                    |
        v                    v                    v
+---------------+   +---------------+   +---------------+
| Floor 1       |   | Floor 2       |   | Floor N       |
| - mutex       |   | - mutex       |   | - mutex       |
| - slots[]     |   | - slots[]     |   | - slots[]     |
| - freeLists[] |   | - freeLists[] |   | - freeLists[] |
+---------------+   +---------------+   +---------------+

Entry Flow:
  Gate -> VehicleFactory.create(type, plate)
       -> ParkingLot.park(vehicle)
       -> AllocationStrategy.findSlot(floors, vehicleType)
       -> slot.occupy(vehicle)
       -> Ticket(id++, vehicle, slot, now())
       -> return ticket

Exit Flow:
  Gate -> ParkingLot.unpark(ticket)
       -> PricingStrategy.computeFare(ticket.entry, now(), vehicle.type)
       -> Payment.process(fare)
       -> slot.vacate()
       -> notify(observers, SlotFreedEvent)
```

---

## Patterns Used Summary

| Pattern | Where | Why (Trade-off Reasoning) |
|---------|-------|---------------------------|
| Strategy | Pricing, Slot Allocation | Rules change independently; avoid God-class with all logic |
| Factory | Vehicle creation at gates | Decouple gate from concrete vehicle types |
| Singleton | ParkingLot manager | Physical constraint: one lot, shared state |
| Observer | Capacity alerts, display boards | Decouple notification from core logic |
| State | Slot lifecycle (Free/Reserved/Occupied) | Enforce valid transitions, prevent illegal states |
| Decorator | Surge pricing over base strategy | Compose behaviors without combinatorial subclasses |

## Files
- [parking_lot.cpp](parking_lot.cpp) - full single-file implementation

## Interview Questions
1. How do you handle multiple entry gates concurrently? (Per-floor mutex with consistent ordering)
2. How would you scale to 1000 floors / millions of slots? (Sharded free-lists per type, bitmap index)
3. What if slots are reserved in advance? (Add RESERVED state, ReservationManager, time-based expiry)
4. EV charging slots - how to extend? (New SlotType + IChargeable interface, new PricingStrategy)
5. What if pricing varies by day/time? (Strategy + Decorator composition)
6. Persistence - how to recover state on restart? (WAL + periodic snapshots, or write-through to DB)
7. How do you prevent starvation of large slots? (Policy: only allow car in large slot if no medium available)
8. How do you handle the "nearest slot" problem efficiently? (Min-heap per slot type, keyed by distance to gate)
9. What is the time complexity of your park/unpark operations? (O(1) with free-list, O(log n) with heap)
10. How would you add a mobile app showing real-time availability? (WebSocket + Observer pattern for push updates)

## Daily Assignment
1. Extend pricing: weekend/weekday differential using Decorator pattern.
2. Add reserved slots for monthly subscribers with time-based expiry.
3. Add capacity alerts when floor > 90% full (Observer pattern with multiple subscribers).
4. Add multi-gate concurrent simulation with N threads - verify no double-allocation.
5. Implement the WAL-based persistence layer and verify crash recovery.
