#pragma once

/* Stub Tracy profiler - disabled on non-Windows builds */
#define TracyCZone(name, active) do { } while(0)
#define TracyCZoneEnd(name) do { } while(0)
#define TracyCZoneC(name, color, active) do { } while(0)
#define TracyCZoneEndC(name) do { } while(0)

