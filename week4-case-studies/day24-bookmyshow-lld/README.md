# Day 24 - LLD: BookMyShow

## Problem
Movie ticket booking. Users browse cities -> theaters -> shows -> seats -> book + pay. Concurrent users may try to book the same seat - prevent double booking.

## Requirements
1. Search shows by city + movie + date
2. View seat layout (available/booked)
3. Lock seats during checkout (5-min hold)
4. Confirm booking on payment success
5. Release seats on payment failure / timeout

## Patterns Used
| Pattern | Where |
|---------|-------|
| Singleton | BookingManager |
| Strategy | Pricing, Seat type |
| State | Booking lifecycle (LOCKED, CONFIRMED, CANCELLED, EXPIRED) |
| Observer | Notify on payment status |
| Factory | Show/Theater/City creation |

## Files
- [bookmyshow.cpp](bookmyshow.cpp)

## Concurrency
- Per-show lock (mutex) for seat allocation
- TTL on seat lock; background thread releases expired holds

## Interview Questions
1. How do you prevent two users booking same seat?
2. What if payment is slow? (lock with TTL, auto-release)
3. How to handle 1M concurrent users for a popular show? (queue, optimistic locking, regional sharding)
4. Refund policy - state transitions?
5. How to scale: read replicas for browsing, primary for booking.
6. How to surface "x people are looking at this" - real-time presence.
7. Recommendation engine integration?

## Daily Assignment
1. Add seat-level pricing (premium/recliner/standard).
2. Add waitlist when show is full.
3. Add discount/coupon strategy.
4. Add a background job to expire locked seats after 5 min.
