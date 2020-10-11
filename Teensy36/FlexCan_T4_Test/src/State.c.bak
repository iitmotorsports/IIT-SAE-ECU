/**
 * State controls what can be done and when
 * 
 * CAN BUS faults
 *  Are we in a state that ignores these faults?
 *  Maybe we need to check for specific faults regardless of what fault(hard/soft)?
 *      We must pass data(the fault bytes) to State incase we do
 *      In the case of a unrecoverable fault, disable interrupts and do what is needed
 *      Each state can have their own way of managing faults
 * 
 * Notifying tablet
 *  Are we on a teensy that can notify the tablet?
 *      If so, just push the address with the appropriate status code if somthing of interest happens
 *      If the tablet receives a status code of a diffrent state then that means we have changed states
 * 
 * Sequences
 *  Specific sequences that must be carried out in a state can be implemented here
 *  Interrupts can be disabled and any faults can be checked for manually
 * 
 */