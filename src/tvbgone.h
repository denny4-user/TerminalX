#pragma once

enum TvRegion { TV_NA = 0, TV_EU = 1, TV_BOTH = 2 };

// Blasts the power-off database. Shows progress; OK or long-NAV cancels.
// Returns true if it finished, false if the user cancelled.
bool tvbgone_run(TvRegion region);
