// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_DISPLAY_CHROMEOS_DISPLAY_UTIL_H_
#define UI_DISPLAY_CHROMEOS_DISPLAY_UTIL_H_

#include <string>
#include <vector>

#include "ui/display/chromeos/display_configurator.h"
#include "ui/display/types/display_constants.h"

namespace ui {

// Returns a string describing |state|.
std::string DisplayPowerStateToString(chromeos::DisplayPowerState state);

// Returns a string describing |state|.
std::string MultipleDisplayStateToString(MultipleDisplayState state);

// Returns the number of displays in |displays| that should be turned on, per
// |state|.  If |display_power| is non-NULL, it is updated to contain the
// on/off state of each corresponding entry in |displays|.
int DISPLAY_EXPORT GetDisplayPower(
    const std::vector<DisplayConfigurator::DisplayState>& display_states,
    chromeos::DisplayPowerState state,
    std::vector<bool>* display_power);

}  // namespace ui

#endif  // UI_DISPLAY_CHROMEOS_DISPLAY_UTIL_H_
