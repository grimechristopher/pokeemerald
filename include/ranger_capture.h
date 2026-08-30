#ifndef GUARD_RANGER_CAPTURE_H
#define GUARD_RANGER_CAPTURE_H

#include "main.h"

// Minigame result states (stored in gRangerCaptureState)
#define RANGER_CAPTURE_IDLE     0
#define RANGER_CAPTURE_RUNNING  1
#define RANGER_CAPTURE_SUCCESS  2
#define RANGER_CAPTURE_FAIL     3

extern u8 gRangerCaptureState;
extern MainCallback gRangerCapture_ReturnCallback;

void RangerCapture_Init(void);

#endif // GUARD_RANGER_CAPTURE_H
