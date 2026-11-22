// SPDX-FileCopyrightText: 2026 COCONAT10MEN
// SPDX-License-Identifier: MIT

#include "button.h"
#include "decoration.h"

#include <KPluginFactory>

K_PLUGIN_FACTORY_WITH_JSON(CaelestiaWindowFramesFactory,
                           "caelestiawindowframes.json",
                           registerPlugin<Caelestia::Decoration>();
                           registerPlugin<Caelestia::Button>();)

#include "plugin.moc"
