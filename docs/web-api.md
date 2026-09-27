# Web API

The HTTP server is not running. Ethernet is not in the Appli `.ioc` IP list (`UART7`, LTDC, TouchGFX, and the rest; no ETH, no LwIP). `psu_web_request` is the handler a future non-blocking server should call. It shares `psu_app` with the LCD.

`psu_http_next` copies the body in caller-sized TCP windows. The page is a single static string in flash, not a RAM duplicate of a site.

## `GET /api/state`

JSON integers, units in the names:

- `requested_mv`, `requested_ma`
- `applied_mv`, `applied_ma`, `applied_valid` (0 until G0 readback matches)
- `output_requested`, `output_confirmed`
- `vout_mv`, `vin_mv`, `iout_ua` (signed), `power_mw`
- `g0_online`, `g4_link` (0 offline, 1 online, 2 stale), `pps_allowed`
- `seq_status`, `seq_mv`, `seq_ma`
- `net`, `ip` (0.0.0.0 until a real DHCP bind)
- `cmd`, `cmd_state`

## `POST /api/command`

Body fields, same clamps as the LCD (0–27000 mV, 0–5000 mA):

```json
{"voltage_mv":12000,"current_ma":2000}
{"output":0}
```

The first client name takes the lease. A second name receives `{"result":"rejected","reason":"lease"}`. Risky output-on is still rejected while a fault is latched.

`GET /` returns the small lab console page. It polls `/api/state` and does not keep its own setpoint as the source of truth.
