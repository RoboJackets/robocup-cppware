#include "kicker.hpp"

String triggermode_to_str(TriggerMode trigger) {
    switch (trigger)
    {
    case Immediate:
        return "Immediate";
    case Breakbeam:
        return "Breakbeam";
    default:
        return "Disabled";
    }
}

String shootmode_to_str(ShootMode type) {
    switch (type)
    {
    case Chip:
        return "Chip";
    default:
        return "Kick";
    }
}