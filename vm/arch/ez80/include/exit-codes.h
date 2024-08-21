#pragma once

#include <cstddef>

enum RustleExitStatus {
    ExitSuccess = 0,
    ExitError = -1,
    ExitTypeError = -2,
    ExitMemoryError = -3,
    ExitTerminated = -4,
    ExitDevAssertFailed = -5
};