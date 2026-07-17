// ============================================================================
// led_status.h — feedback della sincronizzazione orario sul LED RGB onboard,
//   per le schede senza display (vedi HAS_STATUS_LED in board_config.h).
// ============================================================================
#pragma once

void ledStatusInit();
void ledStatusTick();   // da chiamare ad ogni loop(), anche a sync conclusa
