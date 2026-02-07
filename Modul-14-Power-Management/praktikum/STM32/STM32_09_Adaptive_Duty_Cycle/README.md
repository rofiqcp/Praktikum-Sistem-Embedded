# STM32_09: Adaptive Duty Cycle (Integrasi)

## Deskripsi
Program integrasi lengkap: state machine dengan adaptive sleep mode selection berdasarkan level baterai. Gabungan Sleep, Stop, dan Standby.

## Hardware
- STM32F103C8T6 (Blue Pill)
- Button PA0, LED PC13
- ADC PA1 (battery via voltage divider)
- UART1 (PA9/PA10)

## State Machine
```
INIT → SENSE → PROCESS → SLEEP → (wake) → SENSE → ...
```

## Adaptive Strategy
| Battery | Mode    | Duration | Current |
|---------|---------|----------|---------|
| >60%    | Sleep   | 3s       | ~2mA    |
| 30-60%  | Stop    | 10s      | ~20µA   |
| <30%    | Standby | 30s      | ~2µA    |

## Cara Build & Upload
```bash
pio run -t upload
pio device monitor
```
