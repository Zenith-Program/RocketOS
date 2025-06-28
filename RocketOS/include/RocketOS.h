#pragma once

#include "RocketOSGeneral.h"


#ifdef RocketOS_CFG_HasShell
#include "shell\RocketOS_Shell.h"
#endif
#ifdef RocketOS_CFG_HasPersistent
#include "persistent\RocketOS_Persistent.h"
#endif
#ifdef RocketOS_CFG_HasTelemetry
#include "telemetry\RocketOS_Telemetry.h"
#endif
#ifdef RocketOS_CFG_HasSimulation
#include "simulation\RocketOS_Simulation.h"
#endif
#ifdef RocketOS_CFG_HasProcessing
#include "processing\RocketOS_Processing.h"
#endif
#ifdef RocketOS_CFG_HasUtilities
#include "utilities\RocketOS_Utilities.h"
#endif