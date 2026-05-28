# Day 24 - LLD: BookMyShow

## LLD Interview Approach (Quick Reference)

Refer to [Day 22](../day22-parking-lot-lld/README.md) for the full 6-step LLD framework. Applied here:

1. **Clarify**: Online movie ticket booking with concurrent seat selection, payment integration, and time-limited holds
2. **Entities**: City, Theater, Screen, Show, Seat, Booking, Payment, User
3. **Relationships**: City has Theaters, Theater has Screens, Screen has Shows (time-bound), Show has Seats, Booking links User to Seats
4. **Patterns**: State (booking lifecycle), Strategy (pricing), Singleton (BookingManager), Observer (notifications)
5. **Concurrency**: Multiple users selecting same seat simultaneously - the critical design challenge
6. **Extensibility**: Waitlists, group bookings, offers/coupons, food ordering

---

## Problem Statement

Design a movie ticket booking system where users browse cities, theaters, and shows, then select and book seats. The critical challenge is preventing double-booking when thousands of users try to book popular show seats simultaneously.

---

## Requirements Analysis

### Functional Requirements
1. Search shows by city + movie + date
2. View seat layout (available/booked/held)
3. Lock seats during checkout (5-minute hold)
4. Confirm booking on payment success
5. Release seats on payment failure or timeout
6. Cancel booking with refund policy
7. Show booking history

### Non-Functional Requirements
- No double-booking under ANY circumstance (correctness > availability)
- Seat hold expires automatically (no manual cleanup)
- Support 1M+ concurrent users for popular movie releases
- Booking confirmation latency < 2 seconds
- 99.99% availability for browsing, 99.9% for booking

---

## Entity Model Deep Dive

```
City
  |-- 1:N -- Theater
                |-- 1:N -- Screen (capacity, layout)
                              |-- 1:N -- Show (movie, startTime, endTime)
                                           |-- 1:N -- ShowSeat (derived from Screen layout)
                                                        |-- status: AVAILABLE|HELD|BOOKED
                                                        |-- heldBy: userId (if HELD)
                                                        |-- heldUntil: timestamp (TTL)
                                                        |-- booking: Booking* (if BOOKED)

Booking
  - id, uniqueRef
  - userId
  - showId
  - seats: vector<ShowSeat*>
  - status: PENDING|CONFIRMED|CANCELLED|REFUNDED
  - payment: Payment*
  - createdAt, confirmedAt

Payment
  - id, bookingId
  - amount, currency
  - status: INITIATED|SUCCESS|FAILED|REFUNDED
  - gatewayRef, gatewayResponse
```

### Why ShowSeat (not just Seat)?

A `Seat` is a physical entity (Row A, Number 5) that exists permanently. A `ShowSeat` is the availability of that seat for a specific show. This separation is critical:

- Same physical seat is AVAILABLE for the 3pm show but BOOKED for the 6pm show
- ShowSeat is created when a Show is scheduled (ShowSeats = Screen.seats x Show)
- ShowSeat carries per-show state (held, booked, price tier for that show)

---

## Seat Selection Concurrency - The Core Challenge

### The Problem

10,000 users open the same show page. 500 click "Book" on seat A5 within the same second. Only ONE must succeed. The other 499 must see "seat unavailable" immediately.

### Approach 1: Pessimistic Locking (SELECT FOR UPDATE)

```sql
BEGIN TRANSACTION;
-- Lock the row - other transactions BLOCK here
SELECT * FROM show_seats 
WHERE show_id = ? AND seat_id = ? AND status = 'AVAILABLE'
FOR UPDATE;

-- If found, mark as held
UPDATE show_seats SET status = 'HELD', held_by = ?, held_until = NOW() + INTERVAL 5 MINUTE
WHERE show_id = ? AND seat_id = ?;

COMMIT;
```

**How it works**: The database acquires a row-level exclusive lock. Other transactions attempting to read the same row with `FOR UPDATE` will BLOCK until the lock is released.

**Characteristics**:
- Guarantees correctness (no double-booking possible)
- Simple to reason about
- Blocks other users (increases latency under high concurrency)
- Risk of deadlock if multiple seats locked in different order

### Approach 2: Optimistic Locking (Version Column)

```sql
-- Read current state (no lock)
SELECT status, version FROM show_seats WHERE show_id = ? AND seat_id = ?;
-- Application checks: status == 'AVAILABLE'

-- Attempt update with version check
UPDATE show_seats 
SET status = 'HELD', held_by = ?, held_until = NOW() + INTERVAL 5 MINUTE, version = version + 1
WHERE show_id = ? AND seat_id = ? AND version = ? AND status = 'AVAILABLE';

-- Check affected rows: if 0, someone else got it -> retry or fail
```

**How it works**: No lock acquired during read. On write, the version check acts as a compare-and-swap. If version changed (someone else modified), the update affects 0 rows and we know we lost the race.

**Characteristics**:
- No blocking (higher throughput under moderate contention)
- May require retries (wasted work under high contention)
- More complex application logic
- Better for read-heavy workloads (browsing >> booking)

### Comparison Table

```
+------------------------------------------------------------------+
| Dimension        | Pessimistic Lock    | Optimistic Lock          |
+------------------+---------------------+--------------------------+
| Correctness      | Guaranteed          | Guaranteed (with retry)  |
| Read latency     | May block           | Never blocks             |
| Write latency    | Predictable         | Variable (retry storms)  |
| Throughput (low) | Good                | Good                     |
| Throughput (high)| Degrades (blocking) | Degrades (retries)       |
| Deadlock risk    | Yes (multi-seat)    | No                       |
| Code complexity  | Low                 | Medium (retry logic)     |
| Best for         | High write conflict | Low-moderate conflict    |
+------------------+---------------------+--------------------------+
```

### Approach 3: Redis-Based Distributed Lock (Recommended for Scale)

```
SETNX seat:{showId}:{seatId} userId EX 300
-- Returns 1 if acquired (seat was free)
-- Returns 0 if already held (someone else has it)
-- Auto-expires in 300 seconds (5 min TTL)
```

**Why Redis?**
- O(1) atomic operation
- Built-in TTL (no background cleanup needed)
- Handles thousands of concurrent requests per second
- No deadlock (single key, no multi-key transaction needed per seat)
- Horizontal scaling with Redis Cluster (shard by showId)

**Trade-off**: Requires separate persistent store for confirmed bookings. Redis is the "lock layer", database is the "truth layer."

---

## Temporary Reservation Pattern

### Hold Timer with TTL

When a user selects seats and proceeds to payment:

```
1. Acquire hold (Redis SETNX with 5-min TTL)
2. Show payment page with countdown timer (4:59, 4:58...)
3. User enters payment details
4. On payment success: convert hold to confirmed booking in DB
5. On payment failure/timeout: hold auto-expires, seat becomes available
```

### Why 5 Minutes?

- Too short (1 min): User cannot enter card details + OTP in time -> frustration
- Too long (15 min): Seats blocked unnecessarily -> lower conversion, lost revenue
- 5 minutes: Sufficient for payment flow, short enough to cycle back to market
- Industry standard: BookMyShow uses 8 min, IRCTC uses 10 min

### Reservation State Machine

```
                         select_seats()
    +----------+  ----------------------->  +--------+
    | AVAILABLE |                            | HELD   |
    +----------+  <-----------------------  +--------+
         ^            timeout (TTL)              |
         |            OR cancel()                |
         |                                       | payment_success()
         |                                       v
         |                                  +--------+
         |                                  | BOOKED |
         |                                  +--------+
         |                                       |
         |            cancel_booking()           |
         |  <------------------------------------+
         |         (within cancellation window)
         |
         |         refund_processed()       +----------+
         +  <----------------------------  | CANCELLED |
                                           +----------+
```

### State Transition Rules

| From | To | Trigger | Side Effects |
|------|----|---------|--------------|
| AVAILABLE | HELD | User selects seat | Set TTL timer, notify UI |
| HELD | AVAILABLE | TTL expires | Clear hold, update seat map |
| HELD | AVAILABLE | User cancels selection | Clear hold immediately |
| HELD | BOOKED | Payment confirmed | Persist booking, send confirmation |
| HELD | AVAILABLE | Payment failed | Clear hold, show error |
| BOOKED | CANCELLED | User cancels (within policy) | Initiate refund, release seat |
| CANCELLED | AVAILABLE | Refund processed | Seat back in pool |

---

## Payment Integration - Two-Phase Approach

### Why Two Phases?

You cannot charge the user and book the seat atomically across two different systems (payment gateway + booking database). This is a distributed transaction problem.

### The Pattern: Reserve -> Pay -> Confirm

```
Phase 1: RESERVE (idempotent)
  - Lock seat in our system (HELD state)
  - Create booking in PENDING state
  - Generate payment order with gateway

Phase 2: CONFIRM or COMPENSATE
  - Payment gateway calls webhook: SUCCESS or FAILURE
  - On SUCCESS: update booking to CONFIRMED, release hold (seat is now BOOKED)
  - On FAILURE: update booking to FAILED, release hold (seat returns to AVAILABLE)
```

### Handling Edge Cases

**Payment succeeds but webhook fails to reach us:**
```
- Polling: Background job queries payment gateway for pending payments
- Idempotency: Webhook handler is idempotent (safe to process twice)
- Timeout: If no response in 10 minutes, query gateway explicitly
```

**Payment succeeds but our DB write fails:**
```
- Retry with exponential backoff
- If still fails: refund the payment (compensating transaction)
- Log for manual reconciliation
- NEVER leave money charged without a booking
```

**User closes browser during payment:**
```
- Hold TTL still active (5 min)
- If payment completes via gateway, webhook still fires -> we confirm
- If payment never completes, TTL expires -> seat released
- User can return to "My Bookings" and see pending booking
```

### Compensating Transaction Pattern

```
Normal flow:    Reserve Seat -> Charge Payment -> Confirm Booking
Compensation:   Release Seat <- Refund Payment <- Cancel Booking

If step N fails, execute compensations for steps N-1, N-2, ... 1
```

This is the Saga pattern applied to booking systems. Each step has an explicit undo operation.

---

## Flash Sale / High-Demand Handling

### The Problem

A blockbuster movie releases 100,000 seats across a city. 5 million users try to book in the first minute. This is essentially a flash sale problem.

### Solution 1: Virtual Waiting Room

```
                    [5M Users]
                        |
                        v
              +-------------------+
              | Waiting Room      |
              | (Queue Service)   |
              | Position: 1/50000 |
              +-------------------+
                        |
                        | (admit 1000/minute)
                        v
              +-------------------+
              | Booking System    |
              | (normal capacity) |
              +-------------------+
```

**How it works**:
1. When load exceeds threshold, new users enter a queue
2. Users see their position and estimated wait time
3. System admits N users per minute (matching booking capacity)
4. Admitted users have a session token valid for 10 minutes to complete booking
5. If time expires without booking, next user is admitted

**Why this works**: Converts a spike of 5M simultaneous requests into a steady stream of 1000 requests/minute that the booking system can handle reliably.

### Solution 2: Queue-Based Seat Selection

```
Instead of direct DB writes:

User selects seat -> Message enqueued -> Worker processes sequentially
                                              |
                                    +-------- v --------+
                                    | Worker:           |
                                    | 1. Check available|
                                    | 2. Reserve        |
                                    | 3. Notify user    |
                                    +-------------------+

User sees: "Processing your request..." (async)
Notification: "Seats confirmed!" or "Sorry, already taken"
```

**Trade-off**: Higher latency (seconds vs milliseconds) but guaranteed no contention (single-threaded worker per show). Acceptable for high-demand shows where users expect to wait.

### Solution 3: Inventory Pre-Sharding

```
For a show with 300 seats:
  - Shard into 10 groups of 30 seats each
  - Each shard has its own lock
  - 10 concurrent bookings can proceed simultaneously
  - User is assigned to a shard based on session hash

Reduces contention by 10x without changing the booking logic.
```

### Bot Prevention

| Technique | How | Effectiveness |
|-----------|-----|---------------|
| CAPTCHA on seat selection | Human verification | Medium (CAPTCHA farms exist) |
| Rate limiting per IP/user | Max 5 attempts per minute | Medium (distributed bots) |
| Browser fingerprinting | Detect headless browsers | High (but privacy concerns) |
| Proof-of-work puzzle | Computational cost per request | High (expensive for bots at scale) |
| Device attestation | Verify genuine device (SafetyNet/DeviceCheck) | Highest |

---

## Pricing Model

### Pricing Tiers by Seat Type

```
Screen Layout (facing screen):
+--------------------------------------------------+
|                   SCREEN                          |
+--------------------------------------------------+
|  [P] [P] [P] [P] [P] [P] [P] [P] [P] [P]     |  <- Premium ($20)
|  [P] [P] [P] [P] [P] [P] [P] [P] [P] [P]     |
|                                                  |
|  [S] [S] [S] [S] [S] [S] [S] [S] [S] [S]     |  <- Standard ($12)
|  [S] [S] [S] [S] [S] [S] [S] [S] [S] [S]     |
|  [S] [S] [S] [S] [S] [S] [S] [S] [S] [S]     |
|  [S] [S] [S] [S] [S] [S] [S] [S] [S] [S]     |
|                                                  |
|  [R] [R] [R] [R] [R] [R] [R] [R] [R] [R]     |  <- Recliner ($25)
|  [R] [R] [R] [R] [R] [R] [R] [R] [R] [R]     |
+--------------------------------------------------+
```

### Dynamic Pricing Strategy

```cpp
class DynamicPricingStrategy : public PricingStrategy {
public:
    double computePrice(Seat seat, Show show) override {
        double base = seat.tierPrice;
        
        // Time-based: weekend/evening premium
        if (isWeekend(show.date)) base *= 1.2;
        if (isEveningShow(show.startTime)) base *= 1.1;
        
        // Demand-based: high occupancy premium
        double occupancy = show.bookedSeats / show.totalSeats;
        if (occupancy > 0.8) base *= 1.15;
        
        // Supply-based: last few seats premium
        if (show.availableSeats < 10) base *= 1.3;
        
        return base;
    }
};
```

**Why Strategy?** Different theaters may have different pricing models. A luxury theater uses dynamic pricing while a budget theater uses flat pricing. Strategy allows per-theater configuration without code changes.

---

## Scalability Architecture

### Read Path (Browsing) - Scale Independently

```
[Users] -> [CDN] -> [Read Replicas]
                         |
              +----------+----------+
              |          |          |
         [Show Cache] [Seat Map] [Movie Catalog]
         (Redis)      (Redis)    (Elasticsearch)
```

- Seat map cached in Redis, invalidated on booking/release
- Movie catalog in Elasticsearch for search
- Show listings CDN-cached with 30-second TTL
- Read replicas handle 99% of traffic (browsing)

### Write Path (Booking) - Serialize for Correctness

```
[User] -> [API Gateway] -> [Booking Service]
                                |
                           [Redis Lock] (seat hold)
                                |
                           [Primary DB] (booking record)
                                |
                           [Payment Gateway] (charge)
                                |
                           [Notification] (confirmation)
```

- Booking writes go to primary DB only
- Redis provides the fast lock layer
- Payment is async (webhook-based confirmation)
- Notifications are fire-and-forget (queue-based)

### Database Choice Justification

| Data | Store | Why |
|------|-------|-----|
| Movie catalog | PostgreSQL + Elasticsearch | Structured + full-text search |
| Show schedule | PostgreSQL | Relational (theater -> screen -> show) |
| Seat availability | Redis | O(1) reads/writes, TTL for holds |
| Bookings | PostgreSQL | ACID for financial records |
| User sessions | Redis | Fast, ephemeral |
| Analytics | ClickHouse/BigQuery | Column-store for aggregations |

---

## Cancellation and Refund Design

### Refund Policy State Machine

```
Booking CONFIRMED
    |
    | cancel_request (> 4 hours before show)
    v
CANCELLATION_REQUESTED
    |
    | process_refund (full refund - 5% fee)
    v
REFUND_INITIATED
    |
    | gateway_confirms
    v
REFUNDED (seat back to AVAILABLE)

---

Booking CONFIRMED
    |
    | cancel_request (< 4 hours before show)
    v
CANCELLATION_REQUESTED
    |
    | process_refund (50% refund)
    v
PARTIAL_REFUND_INITIATED
    |
    | gateway_confirms
    v
PARTIALLY_REFUNDED (seat back to AVAILABLE)

---

Booking CONFIRMED
    |
    | cancel_request (< 1 hour or show started)
    v
CANCELLATION_DENIED (no refund, seat remains BOOKED)
```

---

## Patterns Used Summary

| Pattern | Where | Why (Trade-off Reasoning) |
|---------|-------|---------------------------|
| State | Booking lifecycle, Seat status | Enforce valid transitions, clear error handling |
| Strategy | Pricing (tier, dynamic, promotional) | Different pricing per theater/event without code change |
| Singleton | BookingManager | Centralized coordination for seat allocation |
| Observer | Payment status, booking confirmation | Decouple notification from core booking logic |
| Factory | Show/Theater/City creation | Decouple creation logic from business logic |
| Saga | Payment + Booking (compensating transactions) | Handle distributed transaction without 2PC |

## Files
- [bookmyshow.cpp](bookmyshow.cpp)

## Interview Questions
1. How do you prevent two users booking the same seat? (Redis SETNX with TTL, or DB-level FOR UPDATE)
2. What if payment is slow? (Lock with TTL, auto-release on expiry, webhook for async confirmation)
3. How to handle 1M concurrent users for a popular show? (Virtual waiting room, queue-based processing, inventory sharding)
4. Optimistic vs pessimistic locking - when to use each? (Pessimistic for high contention, optimistic for read-heavy)
5. Refund policy - state transitions? (Time-based tiers: full > 4hr, partial > 1hr, none after)
6. How to scale: read replicas for browsing, primary for booking, Redis for lock layer
7. How to surface "x people are looking at this" - real-time presence? (WebSocket + Redis pub/sub per show)
8. How to prevent bots from hoarding seats? (CAPTCHA, rate limiting, device attestation, proof-of-work)
9. Why not use a distributed transaction (2PC) for payment + booking? (Availability sacrifice, latency, complexity - Saga is preferred)
10. How would you implement a waitlist? (Queue per show, notify on cancellation, time-limited offer to waitlisted user)

## Daily Assignment
1. Add seat-level pricing (premium/recliner/standard) with Strategy pattern.
2. Add waitlist when show is full - notify first in queue on cancellation.
3. Add discount/coupon strategy that composes with base pricing.
4. Add a background job to expire locked seats after 5 min (TTL-based cleanup).
5. Implement the two-phase booking flow with compensating transaction on payment failure.
