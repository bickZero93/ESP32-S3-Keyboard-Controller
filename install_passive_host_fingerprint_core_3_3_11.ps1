# Passive USB host fingerprint installer for Arduino-ESP32 Core 3.3.11
# Installs Option A (MS OS 1.0/2.0) plus descriptor-order/vendor observation hooks.
# It patches ONLY cores/esp32/esp32-hal-tinyusb.c.
$ErrorActionPreference = 'Stop'

$core = Join-Path $env:LOCALAPPDATA 'Arduino15\packages\esp32\hardware\esp32\3.3.11\cores\esp32\esp32-hal-tinyusb.c'
if (-not (Test-Path $core)) {
  throw "Arduino-ESP32 3.3.11 core file not found: $core"
}

$current = [IO.File]::ReadAllText($core)
if ($current.Contains('tinyusb_host_fp_core_present')) {
  Write-Host 'Passive host fingerprint core hook is already installed.' -ForegroundColor Green
  exit 0
}

# Preserve the currently installed state, including the earlier Option-A patch.
$preUpgrade = "$core.hostfp_preupgrade_backup"
if (-not (Test-Path $preUpgrade)) {
  Copy-Item $core $preUpgrade -Force
  Write-Host "Pre-upgrade backup created: $preUpgrade"
}

# The previous Option-A installer created a pristine original backup.
# Prefer that as a clean base so the new patch does not stack duplicate callbacks.
$optionABackup = "$core.optionA_backup"
if (Test-Path $optionABackup) {
  $text = [IO.File]::ReadAllText($optionABackup)
  Write-Host 'Using pristine .optionA_backup as patch base.'
} else {
  if ($current.Contains('tinyusb_os_probe_core_present')) {
    throw "Old Option-A patch detected but .optionA_backup is missing. Restore the original 3.3.11 core first."
  }
  $text = $current
  Copy-Item $core $optionABackup -Force
  Write-Host "Original backup created: $optionABackup"
}

$nl = if ($text.Contains("`r`n")) { "`r`n" } else { "`n" }

$anchor1 = 'static uint8_t *tinyusb_config_descriptor = NULL;'
if (-not $text.Contains($anchor1)) { throw 'Anchor 1 not found; core source does not match Arduino-ESP32 3.3.11.' }

$block1 = @'

/* ---- keyboard-controller passive host fingerprint hooks --------------------
 * Adds observation only; normal HID/MSC/CDC control paths stay in TinyUSB.
 * Option A:
 *   1 = MS OS 1.0 string 0xEE requested
 *   2 = MS OS 1.0 Extended Compat ID (wIndex 0x0004)
 *   3 = MS OS 1.0 Extended Properties (wIndex 0x0005)
 *   4 = MS OS 2.0 descriptor-set request (wIndex 0x0007)
 * Descriptor events:
 *   1 = Device, 2 = Configuration, 3 = String, 4 = BOS
 */
#define TINYUSB_OS_PROBE_VENDOR_CODE 0x21

__attribute__((weak)) bool tinyusb_os_probe_enabled_cb(void) {
  return false;
}
__attribute__((weak)) void tinyusb_os_probe_event_cb(uint8_t event, uint16_t wIndex) {
  (void)event; (void)wIndex;
}
bool tinyusb_os_probe_core_present(void) {
  return true;
}

__attribute__((weak)) void tinyusb_host_fp_descriptor_event_cb(uint8_t type, uint8_t index, uint16_t langid) {
  (void)type; (void)index; (void)langid;
}
__attribute__((weak)) void tinyusb_host_fp_vendor_event_cb(uint8_t bmRequestType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, uint16_t wLength) {
  (void)bmRequestType; (void)bRequest; (void)wValue; (void)wIndex; (void)wLength;
}
bool tinyusb_host_fp_core_present(void) {
  return true;
}

/* Microsoft OS 1.0 string descriptor: "MSFT100" + vendor code 0x21 + pad. */
static uint16_t const tinyusb_ms_os_10_string_descriptor[] = {
  0x0312, 0x004D, 0x0053, 0x0046, 0x0054, 0x0031, 0x0030, 0x0030, 0x0021
};

/* Neutral feature descriptor headers. They observe the Microsoft path without
 * requesting WINUSB/RNDIS binding for the existing HID/MSC interfaces. */
static uint8_t const tinyusb_ms_os_10_compat_empty[] = {
  0x10,0x00,0x00,0x00, 0x00,0x01, 0x04,0x00, 0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00
};
static uint8_t const tinyusb_ms_os_10_properties_empty[] = {
  0x0A,0x00,0x00,0x00, 0x00,0x01, 0x05,0x00, 0x00,0x00
};

/* Neutral MS OS 2.0 set header only. */
#define TINYUSB_OS_PROBE_MS20_SET_LEN 10
static uint8_t const tinyusb_ms_os_20_probe_descriptor[] = {
  0x0A,0x00, 0x00,0x00, 0x00,0x00,0x03,0x06, 0x0A,0x00
};
#define TINYUSB_OS_PROBE_BOS_TOTAL_LEN (TUD_BOS_DESC_LEN + TUD_BOS_MICROSOFT_OS_DESC_LEN)
static uint8_t const tinyusb_os_probe_bos_descriptor[] = {
  TUD_BOS_DESCRIPTOR(TINYUSB_OS_PROBE_BOS_TOTAL_LEN, 1),
  TUD_BOS_MS_OS_20_DESCRIPTOR(TINYUSB_OS_PROBE_MS20_SET_LEN, VENDOR_REQUEST_MICROSOFT)
};
/* ---- end keyboard-controller hooks ---------------------------------------- */
'@
$text = $text.Replace($anchor1, $anchor1 + $block1.Replace("`n", $nl))

# Observe Device descriptor requests.
$deviceAnchor = '__attribute__((weak)) uint8_t const *tud_descriptor_device_cb(void) {'
if (-not $text.Contains($deviceAnchor)) { throw 'Device descriptor callback anchor not found.' }
$text = $text.Replace($deviceAnchor, $deviceAnchor + ($nl + '  tinyusb_host_fp_descriptor_event_cb(1, 0, 0);'))

# Observe Configuration descriptor requests.
$configAnchor = '__attribute__((weak)) uint8_t const *tud_descriptor_configuration_cb(uint8_t index) {'
if (-not $text.Contains($configAnchor)) { throw 'Configuration descriptor callback anchor not found.' }
$text = $text.Replace($configAnchor, $configAnchor + ($nl + '  tinyusb_host_fp_descriptor_event_cb(2, index, 0);'))

# Observe String descriptors and provide MS OS 1.0 string 0xEE.
$stringAnchor = '  uint8_t chr_count;'
if (-not $text.Contains($stringAnchor)) { throw 'String descriptor callback anchor not found.' }
$stringBlock = @'

  tinyusb_host_fp_descriptor_event_cb(3, index, langid);

  if (index == 0xEE && tinyusb_os_probe_enabled_cb()) {
    tinyusb_os_probe_event_cb(1, 0x00EE);
    return tinyusb_ms_os_10_string_descriptor;
  }
'@
$text = $text.Replace($stringAnchor, $stringAnchor + $stringBlock.Replace("`n", $nl))

# BOS observation + neutral MS OS 2.0 platform capability while A is active.
$oldBos = @'
uint8_t const *tud_descriptor_bos_cb(void) {
  //log_v("");
  return tinyusb_bos_descriptor;
}
'@.Replace("`n", $nl)
$newBos = @'
uint8_t const *tud_descriptor_bos_cb(void) {
  //log_v("");
  tinyusb_host_fp_descriptor_event_cb(4, 0, 0);
  if (tinyusb_os_probe_enabled_cb()) {
    return tinyusb_os_probe_bos_descriptor;
  }
  return tinyusb_bos_descriptor;
}
'@.Replace("`n", $nl)
if (-not $text.Contains($oldBos)) { throw 'BOS callback anchor not found.' }
$text = $text.Replace($oldBos, $newBos)

# Observe all vendor SETUP packets, then handle the neutral Microsoft probes.
$vendorAnchor = 'bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request) {'
if (-not $text.Contains($vendorAnchor)) { throw 'Vendor callback anchor not found.' }
$vendorBlock = @'

  if (stage == CONTROL_STAGE_SETUP && request != NULL) {
    uint8_t const fp_bmRequestType = (uint8_t)(
      ((uint8_t)request->bmRequestType_bit.direction << 7) |
      ((uint8_t)request->bmRequestType_bit.type << 5) |
      (uint8_t)request->bmRequestType_bit.recipient
    );
    tinyusb_host_fp_vendor_event_cb(
      fp_bmRequestType,
      request->bRequest,
      request->wValue,
      request->wIndex,
      request->wLength
    );
  }

  if (tinyusb_os_probe_enabled_cb() &&
      request->bmRequestType_bit.type == TUSB_REQ_TYPE_VENDOR &&
      request->bmRequestType_bit.direction == TUSB_DIR_IN) {

    if (request->bRequest == TINYUSB_OS_PROBE_VENDOR_CODE &&
        (request->wIndex == 0x0004 || request->wIndex == 0x0005)) {
      if (stage == CONTROL_STAGE_SETUP) {
        if (request->wIndex == 0x0004) {
          tinyusb_os_probe_event_cb(2, request->wIndex);
          uint16_t n = request->wLength;
          if (n > sizeof(tinyusb_ms_os_10_compat_empty)) n = sizeof(tinyusb_ms_os_10_compat_empty);
          return tud_control_xfer(rhport, request, (void *)tinyusb_ms_os_10_compat_empty, n);
        }
        tinyusb_os_probe_event_cb(3, request->wIndex);
        uint16_t n = request->wLength;
        if (n > sizeof(tinyusb_ms_os_10_properties_empty)) n = sizeof(tinyusb_ms_os_10_properties_empty);
        return tud_control_xfer(rhport, request, (void *)tinyusb_ms_os_10_properties_empty, n);
      }
      return true;
    }

    if (request->bRequest == VENDOR_REQUEST_MICROSOFT && request->wIndex == 0x0007) {
      if (stage == CONTROL_STAGE_SETUP) {
        tinyusb_os_probe_event_cb(4, request->wIndex);
        uint16_t n = request->wLength;
        if (n > sizeof(tinyusb_ms_os_20_probe_descriptor)) n = sizeof(tinyusb_ms_os_20_probe_descriptor);
        return tud_control_xfer(rhport, request, (void *)tinyusb_ms_os_20_probe_descriptor, n);
      }
      return true;
    }
  }
'@
$text = $text.Replace($vendorAnchor, $vendorAnchor + $vendorBlock.Replace("`n", $nl))

$required = @(
  'tinyusb_os_probe_core_present',
  'tinyusb_host_fp_core_present',
  'tinyusb_host_fp_descriptor_event_cb(1',
  'tinyusb_host_fp_descriptor_event_cb(2',
  'tinyusb_host_fp_descriptor_event_cb(3',
  'tinyusb_host_fp_descriptor_event_cb(4',
  'tinyusb_host_fp_vendor_event_cb(',
  'index == 0xEE',
  'tinyusb_os_probe_event_cb(4'
)
foreach ($r in $required) {
  if (-not $text.Contains($r)) {
    throw "Patch validation failed at '$r'. Nothing was written."
  }
}

[IO.File]::WriteAllText($core, $text, [Text.UTF8Encoding]::new($false))
Write-Host 'Option A + passive host fingerprint hooks installed.' -ForegroundColor Green
Write-Host 'Close and reopen Arduino IDE before compiling the current firmware.' -ForegroundColor Yellow
Write-Host "Pristine backup: $optionABackup"
Write-Host "Pre-upgrade backup: $preUpgrade"
