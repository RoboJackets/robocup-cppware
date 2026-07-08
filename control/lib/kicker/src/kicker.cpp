#include "kicker.hpp"

String kicktrigger_to_str(KickTrigger trigger) {
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

String kicktype_to_str(KickType type) {
    switch (type)
    {
    case Chip:
        return "Chip";
    default:
        return "Kick";
    }
}