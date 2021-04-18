Test check list
===
Assuming testing has all subsystems connected
---
1. Heartbeat light toggling periodically
	- canbus is working
	- normal pins are working
	- ecus not overwhelmed
2. Check for any state being displayed, not quickly changing between states
    - no faults or missed loops
    - canbus interrupts not overwhelming or crashed ecus
3. Check CanPins can update
    - communication between ecus is probably okay