/*
  ESP32-S3 USB HID Keyboard Controller
  --------------------------------------
  Tools -> USB Mode        -> "USB-OTG (TinyUSB)"
  Tools -> USB CDC On Boot -> "Enabled"

  Vollstndiges DE-Tastaturlayout Mapping.
  Alle Sonderzeichen werden als explizite
  Tastenkombinationen gesendet.
*/

#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <USB.h>
#include <USBHIDKeyboard.h>
#include <Update.h>

// -- Version ---------------------------------------------------
#define FW_VERSION "1.0.7"
#define FW_DATE    "2026-08-14"

USBHIDKeyboard Keyboard;
WebServer server(80);
Preferences prefs;

String savedSSID, savedPass;
bool   wifiConnected = false;

bool   g_running      = false;
int    g_runCount     = 0;
bool   g_looping      = false;
int    g_loopInterval = 1000;
int    g_typeDelay    = 20;
int    g_burstSize    = 3;    // Zeichen pro USB-Paket (Burst-Modus)
bool   g_clipMode     = false; // Clipboard-Modus via PowerShell
bool   g_capsState    = false; // Unser eigener CAPS-Zustand tracker

// -- Autostart --
bool   g_autoEnabled  = false;
bool   g_autoStarted  = false;
unsigned long g_autoStartTime = 0;
int    g_autoDelay    = 5000;  // ms nach Boot warten
int    g_autoRepeat   = 1;     // 0 = endlos
int    g_autoSpeed    = 20;
int    g_autoBurst    = 3;
String g_autoTmplId   = "";    // Template ID als String

#define MAX_STEPS 32
struct Step { String type; String value; int stepSpeed=-1; }; // stepSpeed: -1=global
Step g_steps[MAX_STEPS];
int  g_stepCount = 0;
unsigned long g_nextRun = 0;

// -- HID Rohe Keycodes (US-Layout Basis) ----------------------
// Diese Werte sind HID Usage IDs, unabhngig vom OS-Layout
#define KEY_A       0x04
#define KEY_B       0x05
#define KEY_C       0x06
#define KEY_D       0x07
#define KEY_E       0x08
#define KEY_F       0x09
#define KEY_G       0x0A
#define KEY_H       0x0B
#define KEY_I       0x0C
#define KEY_J       0x0D
#define KEY_K       0x0E
#define KEY_L       0x0F
#define KEY_M       0x10
#define KEY_N       0x11
#define KEY_O       0x12
#define KEY_P       0x13
#define KEY_Q       0x14
#define KEY_R       0x15
#define KEY_S       0x16
#define KEY_T       0x17
#define KEY_U       0x18
#define KEY_V       0x19
#define KEY_W       0x1A
#define KEY_X       0x1B
#define KEY_Y       0x1C
#define KEY_Z       0x1D
#define KEY_1       0x1E
#define KEY_2       0x1F
#define KEY_3       0x20
#define KEY_4       0x21
#define KEY_5       0x22
#define KEY_6       0x23
#define KEY_7       0x24
#define KEY_8       0x25
#define KEY_9       0x26
#define KEY_0       0x27
#define KEY_ENTER_R 0x28
#define KEY_ESC_R   0x29
#define KEY_BSPACE  0x2A
#define KEY_TAB_R   0x2B
#define KEY_SPACE_R 0x2C
#define KEY_MINUS   0x2D  // DE: 
#define KEY_EQUAL   0x2E  // DE: 
#define KEY_LBRACE  0x2F  // DE: 
#define KEY_RBRACE  0x30  // DE: +
#define KEY_BSLASH  0x31  // DE: #
#define KEY_HASH    0x32  // DE: #
#define KEY_SCOLON  0x33  // DE: 
#define KEY_QUOTE   0x34  // DE: 
#define KEY_GRAVE   0x35  // DE: ^
#define KEY_COMMA   0x36
#define KEY_DOT     0x37
#define KEY_SLASH   0x38  // DE: -
#define KEY_NONEUSB 0x64  // DE: < > |

#define MOD_LSHIFT  0x02
#define MOD_RALT    0x40  // AltGr

// Sende rohen HID Keycode mit Modifier
void sendRaw(uint8_t modifier, uint8_t keycode) {
  uint8_t report[8] = {modifier, 0, keycode, 0, 0, 0, 0, 0};
  Keyboard.sendReport((KeyReport*)report);
  // Delays skalieren mit Geschwindigkeit aber nie unter USB-Minimum
  uint8_t d1 = (g_typeDelay >= 20) ? 30 : (g_typeDelay >= 5) ? 20 : 12;
  uint8_t d2 = (g_typeDelay >= 20) ? 10 : 5;
  delay(d1);
  memset(report, 0, 8);
  Keyboard.sendReport((KeyReport*)report);
  delay(d2);
}

// -- Vollstndiges DE-Layout Zeichen-Mapping -------------------
void typeCharDE(char c) {
  switch(c) {
    // -- Buchstaben ------------------------------------------
    case 'a': sendRaw(0, KEY_A); break;
    case 'b': sendRaw(0, KEY_B); break;
    case 'c': sendRaw(0, KEY_C); break;
    case 'd': sendRaw(0, KEY_D); break;
    case 'e': sendRaw(0, KEY_E); break;
    case 'f': sendRaw(0, KEY_F); break;
    case 'g': sendRaw(0, KEY_G); break;
    case 'h': sendRaw(0, KEY_H); break;
    case 'i': sendRaw(0, KEY_I); break;
    case 'j': sendRaw(0, KEY_J); break;
    case 'k': sendRaw(0, KEY_K); break;
    case 'l': sendRaw(0, KEY_L); break;
    case 'm': sendRaw(0, KEY_M); break;
    case 'n': sendRaw(0, KEY_N); break;
    case 'o': sendRaw(0, KEY_O); break;
    case 'p': sendRaw(0, KEY_P); break;
    case 'q': sendRaw(0, KEY_Q); break;
    case 'r': sendRaw(0, KEY_R); break;
    case 's': sendRaw(0, KEY_S); break;
    case 't': sendRaw(0, KEY_T); break;
    case 'u': sendRaw(0, KEY_U); break;
    case 'v': sendRaw(0, KEY_V); break;
    case 'w': sendRaw(0, KEY_W); break;
    case 'x': sendRaw(0, KEY_X); break;
    case 'y': sendRaw(0, KEY_Z); break;  // DE: y liegt auf Z-Taste
    case 'z': sendRaw(0, KEY_Y); break;  // DE: z liegt auf Y-Taste

    case 'A': sendRaw(MOD_LSHIFT, KEY_A); break;
    case 'B': sendRaw(MOD_LSHIFT, KEY_B); break;
    case 'C': sendRaw(MOD_LSHIFT, KEY_C); break;
    case 'D': sendRaw(MOD_LSHIFT, KEY_D); break;
    case 'E': sendRaw(MOD_LSHIFT, KEY_E); break;
    case 'F': sendRaw(MOD_LSHIFT, KEY_F); break;
    case 'G': sendRaw(MOD_LSHIFT, KEY_G); break;
    case 'H': sendRaw(MOD_LSHIFT, KEY_H); break;
    case 'I': sendRaw(MOD_LSHIFT, KEY_I); break;
    case 'J': sendRaw(MOD_LSHIFT, KEY_J); break;
    case 'K': sendRaw(MOD_LSHIFT, KEY_K); break;
    case 'L': sendRaw(MOD_LSHIFT, KEY_L); break;
    case 'M': sendRaw(MOD_LSHIFT, KEY_M); break;
    case 'N': sendRaw(MOD_LSHIFT, KEY_N); break;
    case 'O': sendRaw(MOD_LSHIFT, KEY_O); break;
    case 'P': sendRaw(MOD_LSHIFT, KEY_P); break;
    case 'Q': sendRaw(MOD_LSHIFT, KEY_Q); break;
    case 'R': sendRaw(MOD_LSHIFT, KEY_R); break;
    case 'S': sendRaw(MOD_LSHIFT, KEY_S); break;
    case 'T': sendRaw(MOD_LSHIFT, KEY_T); break;
    case 'U': sendRaw(MOD_LSHIFT, KEY_U); break;
    case 'V': sendRaw(MOD_LSHIFT, KEY_V); break;
    case 'W': sendRaw(MOD_LSHIFT, KEY_W); break;
    case 'X': sendRaw(MOD_LSHIFT, KEY_X); break;
    case 'Y': sendRaw(MOD_LSHIFT, KEY_Z); break;  // DE: Y liegt auf Z-Taste
    case 'Z': sendRaw(MOD_LSHIFT, KEY_Y); break;  // DE: Z liegt auf Y-Taste

    // -- Zahlen ----------------------------------------------
    case '0': sendRaw(0, KEY_0); break;
    case '1': sendRaw(0, KEY_1); break;
    case '2': sendRaw(0, KEY_2); break;
    case '3': sendRaw(0, KEY_3); break;
    case '4': sendRaw(0, KEY_4); break;
    case '5': sendRaw(0, KEY_5); break;
    case '6': sendRaw(0, KEY_6); break;
    case '7': sendRaw(0, KEY_7); break;
    case '8': sendRaw(0, KEY_8); break;
    case '9': sendRaw(0, KEY_9); break;

    // -- Sonderzeichen mit SHIFT ------------------------------
    // DE: SHIFT+1=!  2="  3=  4=$  5=%  6=&  7=/  8=(  9=)  0==
    case '!': sendRaw(MOD_LSHIFT, KEY_1); break;
    case '"': sendRaw(MOD_LSHIFT, KEY_2); break;
    case '$': sendRaw(MOD_LSHIFT, KEY_4); break;
    case '%': sendRaw(MOD_LSHIFT, KEY_5); break;
    case '&': sendRaw(MOD_LSHIFT, KEY_6); break;
    case '/': sendRaw(MOD_LSHIFT, KEY_7); break;
    case '(': sendRaw(MOD_LSHIFT, KEY_8); break;
    case ')': sendRaw(MOD_LSHIFT, KEY_9); break;
    case '=': sendRaw(MOD_LSHIFT, KEY_0); break;
    case '?': sendRaw(MOD_LSHIFT, KEY_MINUS); break;  // DE: SHIFT+=?
    case '`': sendRaw(MOD_LSHIFT, KEY_EQUAL); break;  // DE: SHIFT+=`

    // DE: Shift+Buchstabe-Tasten
    case '*': sendRaw(MOD_LSHIFT, KEY_RBRACE); break; // DE: SHIFT++=*
    case '\'':sendRaw(MOD_LSHIFT, KEY_HASH);   break; // DE: SHIFT+#='
    case ';': sendRaw(MOD_LSHIFT, KEY_COMMA);  break; // DE: SHIFT+,=;
    case ':': sendRaw(MOD_LSHIFT, KEY_DOT);    break; // DE: SHIFT+.=:
    case '_': sendRaw(MOD_LSHIFT, KEY_SLASH);  break; // DE: SHIFT+-=_
    case '>': sendRaw(MOD_LSHIFT, KEY_NONEUSB);break; // DE: SHIFT+<=>>

    // -- Sonderzeichen ohne Modifier -------------------------
    case ' ': sendRaw(0, KEY_SPACE_R); break;
    case '-': sendRaw(0, KEY_SLASH);   break; // DE: -
    case '.': sendRaw(0, KEY_DOT);     break;
    case ',': sendRaw(0, KEY_COMMA);   break;
    case '+': sendRaw(0, KEY_RBRACE);  break; // DE: +
    case '#': sendRaw(0, KEY_HASH);    break; // DE: #
    case '<': sendRaw(0, KEY_NONEUSB); break; // DE: <

    // -- AltGr Zeichen ----------------------------------------
    case '@': sendRaw(MOD_RALT, KEY_Q);       break; // DE: AltGr+q=@
    case '{': sendRaw(MOD_RALT, KEY_7);       break; // DE: AltGr+7={
    case '[': sendRaw(MOD_RALT, KEY_8);       break; // DE: AltGr+8=[
    case ']': sendRaw(MOD_RALT, KEY_9);       break; // DE: AltGr+9=]
    case '}': sendRaw(MOD_RALT, KEY_0);       break; // DE: AltGr+0=}
    case '\\':sendRaw(MOD_RALT, KEY_MINUS);   break; // DE: AltGr+=backslash
    case '~': sendRaw(MOD_RALT, KEY_RBRACE);  break; // DE: AltGr++=~
    case '|': sendRaw(MOD_RALT, KEY_NONEUSB); break; // DE: AltGr+<=|
    case '\u00b5': sendRaw(MOD_RALT, KEY_M);  break; // DE: AltGr+m=

    // -- Deutsche Umlaute -------------------------------------
    case '\xC3': break; // UTF-8 Multibyte  wird im typeText() behandelt

    // -- Steuerzeichen ----------------------------------------
    case '\n': sendRaw(0, KEY_ENTER_R); break;
    case '\t': sendRaw(0, KEY_TAB_R);   break;

    default:
      // Fallback fr unbekannte Zeichen
      Serial.printf("Unbekannt: 0x%02X\n", (uint8_t)c);
      break;
  }
  if (g_typeDelay > 0) delay(g_typeDelay);
}

// UTF-8 Umlaut Behandlung
void typeUmlaut(uint8_t b1, uint8_t b2) {
  // Hufige deutsche Umlaute (UTF-8 2-Byte Sequenz)
  if (b1 == 0xC3) {
    switch(b2) {
      case 0xA4: sendRaw(0, KEY_QUOTE);           break; // 
      case 0x84: sendRaw(MOD_LSHIFT, KEY_QUOTE);  break; // 
      case 0xB6: sendRaw(0, KEY_SCOLON);          break; // 
      case 0x96: sendRaw(MOD_LSHIFT, KEY_SCOLON); break; // 
      case 0xBC: sendRaw(0, KEY_LBRACE);          break; // 
      case 0x9C: sendRaw(MOD_LSHIFT, KEY_LBRACE); break; // 
      case 0x9F: sendRaw(0, KEY_MINUS);           break; // 
      default: Serial.printf("Unbekannt UTF8: C3 %02X\n", b2); break;
    }
  }
}

// Escape Text fr PowerShell (single quotes)
String escapePSText(String text) {
  String out = "";
  for (int i = 0; i < (int)text.length(); i++) {
    if (text[i] == '\'') out += "\'\'"; // PS single-quote escape
    else out += text[i];
  }
  return out;
}

// Clipboard-Modus: Text via PowerShell in Zwischenablage, dann Ctrl+V
void typeTextClipboard(String text) {
  Serial.println("Clipboard-Modus: starte...");

  // CAPS LOCK sicherstellen: 2x druecken = garantiert aus
  sendRaw(0, KEY_CAPS_LOCK); delay(80);
  sendRaw(0, KEY_CAPS_LOCK); delay(80);

  // 1. Win+R als ein HID-Report (beide Tasten gleichzeitig)
  uint8_t rel[8] = {0,0,0,0,0,0,0,0};
  uint8_t winR[8] = {0x08, 0, 0x15, 0, 0, 0, 0, 0}; // 0x08=LGUI, 0x15=R
  Keyboard.sendReport((KeyReport*)winR);
  delay(150);
  Keyboard.sendReport((KeyReport*)rel);
  delay(1200); // Warten bis Run-Dialog offen ist

  // 2. PS-Befehl tippen (clipMode kurz aus damit typeText normal laeuft)
  g_clipMode = false;
  int savedDelay = g_typeDelay;
  g_typeDelay = 20;

  typeText("powershell -command Set-Clipboard -Value '");
  typeText(escapePSText(text));
  typeText("'");

  g_typeDelay = savedDelay;
  g_clipMode = true;

  // 3. Enter
  delay(150);
  sendRaw(0, KEY_ENTER_R);
  delay(1800); // PS ausfuehren lassen

  // 4. Ctrl+V als ein HID-Report
  uint8_t ctrlV[8] = {0x01, 0, 0x19, 0, 0, 0, 0, 0}; // 0x01=LCTRL, 0x19=V
  Keyboard.sendReport((KeyReport*)ctrlV);
  delay(120);
  Keyboard.sendReport((KeyReport*)rel);
  Serial.println("Clipboard-Modus: fertig!");
}

void typeText(String text) {
  text.replace("\\n", "\n");
  text.replace("\\t", "\t");

  // Clipboard-Modus: blitzschnell via PowerShell+Ctrl+V
  if (g_clipMode) {
    typeTextClipboard(text);
    return;
  }

  // CAPS LOCK deaktivieren:
  // g_capsState trackt unseren eigenen Zustand seit Sketch-Start.
  // Beim Setup haben wir CAPS 2x gedrckt = synchronisiert auf AUS.
  // Seitdem tracken wir jeden CAPS-Tastendruck selbst.
  if (g_capsState) {
    sendRaw(0, KEY_CAPS_LOCK);
    delay(60);
    g_capsState = false;
  }


  // Burst / Normal Modus
  int i = 0;
  while (i < (int)text.length() && g_running) { // g_running prfen!
    // Webserver kurz bedienen damit Stop-Befehl ankommen kann
    server.handleClient();

    // Burst: g_burstSize Zeichen senden
    for (int b = 0; b < g_burstSize && i < (int)text.length() && g_running; b++) {
      uint8_t ch = (uint8_t)text[i];
      if (ch == 0xC3 && i+1 < (int)text.length()) {
        typeUmlaut(ch, (uint8_t)text[i+1]);
        i += 2;
      } else {
        typeCharDE((char)ch);
        i++;
      }
    }
    // Pause nach Burst
    if (g_typeDelay > 0) delay(g_typeDelay);
    if (g_typeDelay == 0 && i % 30 == 0) delay(1);
  }
}

// -- Sondertasten Lookup ---------------------------------------
struct SpecialKey { const char* name; uint8_t code; };
SpecialKey specialKeys[] = {
  {"ESC",KEY_ESC},
  {"F1",KEY_F1},{"F2",KEY_F2},{"F3",KEY_F3},{"F4",KEY_F4},
  {"F5",KEY_F5},{"F6",KEY_F6},{"F7",KEY_F7},{"F8",KEY_F8},
  {"F9",KEY_F9},{"F10",KEY_F10},{"F11",KEY_F11},{"F12",KEY_F12},
  {"TAB",KEY_TAB},{"CAPS",KEY_CAPS_LOCK},{"ENTER",KEY_RETURN},
  {"BACKSPACE",KEY_BACKSPACE},{"DELETE",KEY_DELETE},
  {"INSERT",KEY_INSERT},{"HOME",KEY_HOME},{"END",KEY_END},
  {"PAGEUP",KEY_PAGE_UP},{"PAGEDOWN",KEY_PAGE_DOWN},
  {"UP",KEY_UP_ARROW},{"DOWN",KEY_DOWN_ARROW},
  {"LEFT",KEY_LEFT_ARROW},{"RIGHT",KEY_RIGHT_ARROW},
  {"LSHIFT",KEY_LEFT_SHIFT},{"RSHIFT",KEY_RIGHT_SHIFT},
  {"LCTRL",KEY_LEFT_CTRL},{"RCTRL",KEY_RIGHT_CTRL},
  {"LALT",KEY_LEFT_ALT},{"RALT",KEY_RIGHT_ALT},
  {"LGUI",KEY_LEFT_GUI},{"RGUI",KEY_RIGHT_GUI},
  {"SPACE",' '},{"PRINT",KEY_PRINT_SCREEN},
  {"SCROLL",KEY_SCROLL_LOCK},{"PAUSE",KEY_PAUSE},
  {"NUMLOCK",KEY_NUM_LOCK},
  {nullptr,0}
};

uint8_t getSpecialKey(String name) {
  name.trim(); name.toUpperCase();
  for (int i = 0; specialKeys[i].name; i++)
    if (name == specialKeys[i].name) return specialKeys[i].code;
  return 0;
}

void sendCombo(String combo) {
  combo.trim();
  String parts[8]; int count=0, start=0;
  for (int i=0; i<=(int)combo.length()&&count<8; i++) {
    if (i==(int)combo.length()||combo[i]=='+') {
      parts[count++]=combo.substring(start,i); start=i+1;
    }
  }
  if (count==1) {
    uint8_t k=getSpecialKey(parts[0]);
    if (k) {
      if (k == KEY_CAPS_LOCK) g_capsState = !g_capsState;
      Keyboard.press(k); delay(60); Keyboard.release(k);
    }
    else if (parts[0].length()==1) typeCharDE(parts[0][0]);
  } else {
    for (int i=0; i<count; i++) {
      uint8_t k=getSpecialKey(parts[i]);
      if (k) Keyboard.press(k);
      else if (parts[i].length()==1) Keyboard.press((uint8_t)parts[i][0]);
    }
    delay(80);
    Keyboard.releaseAll();
  }
}

void executeSequence() {
  for (int i=0; i<g_stepCount&&g_running; i++) {
    server.handleClient(); // Stop-Befehl empfangen
    if (!g_running) break; // sofort abbrechen
    if (g_steps[i].type=="text") {
      // Per-Schritt Geschwindigkeit
      // -1 = global, -2 = Blitz+ (0ms + max burst), >= 0 = expliziter ms-Wert
      int savedDelay = g_typeDelay;
      int savedBurst = g_burstSize;
      if (g_steps[i].stepSpeed == -2) {
        g_typeDelay = 0;    // Blitz+: kein Delay
        g_burstSize = 20;   // Blitz+: maximaler Burst
      } else if (g_steps[i].stepSpeed >= 0) {
        g_typeDelay = g_steps[i].stepSpeed;
      }
      typeText(g_steps[i].value);
      g_typeDelay = savedDelay;
      g_burstSize = savedBurst;
    }
    else if (g_steps[i].type=="key")   sendCombo(g_steps[i].value);
    else if (g_steps[i].type=="delay") delay(g_steps[i].value.toInt());
  }
  g_runCount++;
}

// -- JSON Helfer -----------------------------------------------
String jsonStr(String body, String key) {
  String search="\""+key+"\":\"";
  int i=body.indexOf(search); if(i<0) return "";
  i+=search.length();
  String result="";
  for (int j=i; j<(int)body.length(); j++) {
    if (body[j]=='\\'&&j+1<(int)body.length()) {
      char nx=body[j+1];
      if(nx=='"'){result+='"';j++;}
      else if(nx=='n'){result+='\n';j++;}
      else if(nx=='t'){result+='\t';j++;}
      else if(nx=='\\'){result+='\\';j++;}
      else result+=body[j];
    } else if(body[j]=='"') break;
    else result+=body[j];
  }
  return result;
}

// -- HTML ------------------------------------------------------
const char INDEX_HTML[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html lang="de">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1">
<title>ESP32 Keyboard</title>
<style>
@import url('https://fonts.googleapis.com/css2?family=JetBrains+Mono:wght@400;600&family=Inter:wght@400;500;600&display=swap');
:root{--bg:#0a0a0f;--surface:#13131a;--border:#1e1e2e;--accent:#7c6af7;--accent2:#a78bfa;--green:#4ade80;--red:#f87171;--text:#e2e8f0;--muted:#4a5568;}
*{margin:0;padding:0;box-sizing:border-box;-webkit-tap-highlight-color:transparent;}
body{background:var(--bg);color:var(--text);font-family:'Inter',sans-serif;min-height:100vh;padding:1rem;display:flex;flex-direction:column;align-items:center;}
.wrap{width:100%;max-width:560px;padding-top:1.2rem;}
header{text-align:center;margin-bottom:1.2rem;}
.badge{display:inline-flex;align-items:center;gap:.35rem;background:#1a1a2e;border:1px solid var(--border);border-radius:100px;padding:.28rem .8rem;font-size:.65rem;letter-spacing:.12em;text-transform:uppercase;color:var(--accent2);margin-bottom:.6rem;}
.dot{width:5px;height:5px;border-radius:50%;background:var(--accent);animation:blink 2s infinite;}
@keyframes blink{0%,100%{opacity:1}50%{opacity:.2}}
h1{font-family:'JetBrains Mono',monospace;font-size:1.35rem;font-weight:600;background:linear-gradient(135deg,#fff,var(--accent2));-webkit-background-clip:text;-webkit-text-fill-color:transparent;background-clip:text;}
.sub{color:var(--muted);font-size:.75rem;margin-top:.25rem;}
.tabs{display:flex;gap:.35rem;margin-bottom:.9rem;}
.tab{flex:1;padding:.65rem .2rem;background:var(--surface);border:1px solid var(--border);border-radius:10px;color:var(--muted);font-size:.72rem;font-weight:500;cursor:pointer;text-align:center;transition:all .15s;touch-action:manipulation;}
.tab.active{background:var(--accent);border-color:var(--accent);color:#fff;}
.panel{display:none;}.panel.active{display:block;}
.card{background:var(--surface);border:1px solid var(--border);border-radius:14px;padding:1.2rem;margin-bottom:.7rem;}
lbl{display:block;font-size:.65rem;letter-spacing:.1em;text-transform:uppercase;color:var(--muted);margin-bottom:.4rem;}
textarea,input[type=number],input[type=text],input[type=password]{width:100%;background:var(--bg);border:1px solid var(--border);border-radius:9px;color:var(--text);font-family:'JetBrains Mono',monospace;font-size:.9rem;padding:.75rem .9rem;outline:none;transition:border-color .2s;-webkit-appearance:none;}
textarea{resize:vertical;min-height:80px;line-height:1.6;}
textarea:focus,input:focus{border-color:var(--accent);}
textarea::placeholder,input::placeholder{color:var(--muted);}
.sequence{display:flex;flex-direction:column;gap:.35rem;margin-bottom:.7rem;}
.seq-step{display:flex;align-items:center;gap:.4rem;background:var(--bg);border:1px solid var(--border);border-radius:9px;padding:.55rem .75rem;}
.seq-num{font-family:'JetBrains Mono',monospace;font-size:.65rem;color:var(--muted);min-width:16px;}
.seq-label{flex:1;font-size:.8rem;color:var(--text);word-break:break-all;}
.seq-type{font-size:.6rem;padding:.12rem .4rem;border-radius:4px;font-family:'JetBrains Mono',monospace;flex-shrink:0;}
.seq-type.text{background:#1a1a3a;color:var(--accent2);}
.seq-type.key{background:#1a2a1a;color:var(--green);}
.seq-type.delay{background:#2a2a1a;color:#fbbf24;}
.seq-del{background:none;border:none;color:var(--muted);cursor:pointer;font-size:1rem;padding:.15rem .35rem;border-radius:4px;touch-action:manipulation;}
.seq-spd{font-size:.6rem;padding:.12rem .4rem;border-radius:4px;background:#1a2a1a;color:#4ade80;font-family:monospace;flex-shrink:0;}
/* Vorlagen */
.tmpl-list{display:flex;flex-direction:column;gap:.5rem;}
.tmpl-item{background:var(--bg);border:1px solid var(--border);border-radius:10px;padding:.8rem 1rem;display:flex;align-items:center;gap:.6rem;transition:border-color .15s;}
.tmpl-item:hover{border-color:var(--accent);}
.tmpl-icon{font-size:1.1rem;flex-shrink:0;}
.tmpl-info{flex:1;min-width:0;}
.tmpl-name{font-size:.85rem;font-weight:500;color:var(--text);white-space:nowrap;overflow:hidden;text-overflow:ellipsis;}
.tmpl-meta{font-size:.68rem;color:var(--muted);margin-top:.15rem;}
.tmpl-actions{display:flex;gap:.3rem;flex-shrink:0;}
.tmpl-btn{background:none;border:1px solid var(--border);border-radius:7px;color:var(--muted);font-size:.75rem;padding:.3rem .6rem;cursor:pointer;touch-action:manipulation;white-space:nowrap;}
.tmpl-btn.primary{background:var(--accent);border-color:var(--accent);color:#fff;}
.tmpl-btn.primary:active{background:#6b5ce7;}
.tmpl-btn:active{border-color:var(--accent);color:var(--accent);}
.tmpl-empty{text-align:center;color:var(--muted);font-size:.8rem;padding:2rem 0;}
.auto-item{display:flex;align-items:center;gap:.75rem;background:var(--bg);border:1px solid var(--border);border-radius:10px;padding:.8rem 1rem;margin-bottom:.5rem;cursor:pointer;transition:border-color .15s;}
.auto-item.selected{border-color:var(--accent);background:#1a1a2e;}
.auto-check{width:20px;height:20px;border-radius:50%;border:2px solid var(--border);display:flex;align-items:center;justify-content:center;flex-shrink:0;transition:all .15s;}
.auto-item.selected .auto-check{background:var(--accent);border-color:var(--accent);}
.auto-check-inner{width:8px;height:8px;border-radius:50%;background:#fff;display:none;}
.auto-item.selected .auto-check-inner{display:block;}
.auto-info{flex:1;min-width:0;}
.auto-name{font-size:.85rem;font-weight:500;color:var(--text);}
.auto-meta{font-size:.7rem;color:var(--muted);margin-top:.1rem;}
.seq-del:active{color:var(--red);}
.seq-actions{display:flex;gap:.2rem;flex-shrink:0;}
.seq-btn{background:none;border:none;color:var(--muted);cursor:pointer;font-size:.85rem;padding:.2rem .35rem;border-radius:4px;touch-action:manipulation;}
.seq-btn:active{color:var(--accent);}
/* Edit Modal */
.modal-bg{position:fixed;inset:0;background:#000a;display:flex;align-items:center;justify-content:center;z-index:200;padding:1rem;}
.modal{background:var(--surface);border:1px solid var(--border);border-radius:14px;padding:1.4rem;width:100%;max-width:480px;}
.modal h3{font-size:.95rem;font-weight:600;margin-bottom:1rem;color:var(--text);}
.modal-btns{display:flex;gap:.5rem;margin-top:1rem;}
.add-row{display:flex;gap:.35rem;flex-wrap:wrap;align-items:stretch;}
.add-row select{background:var(--bg);border:1px solid var(--border);border-radius:9px;color:var(--text);font-size:.82rem;padding:.65rem .75rem;outline:none;cursor:pointer;-webkit-appearance:none;}
.add-row .grow{flex:1;min-width:100px;}
.btn{padding:.8rem 1rem;border:none;border-radius:10px;font-size:.88rem;font-weight:600;cursor:pointer;transition:all .15s;font-family:'Inter',sans-serif;display:inline-flex;align-items:center;justify-content:center;gap:.4rem;touch-action:manipulation;-webkit-appearance:none;}
.btn:disabled{opacity:.3;cursor:not-allowed;}
.btn-accent{background:var(--accent);color:#fff;}.btn-accent:not(:disabled):active{background:#6b5ce7;}
.btn-stop{background:var(--red);color:#fff;}.btn-stop:not(:disabled):active{background:#dc2626;}
.btn-add{background:#1a1a2e;border:1px solid var(--border);color:var(--accent2);}.btn-add:active{border-color:var(--accent);}
.btn-sm{padding:.55rem .85rem;font-size:.78rem;}
.btn-full{width:100%;}
.run-row{display:flex;gap:.5rem;margin-top:.9rem;}.run-row .btn{flex:1;padding:.9rem;}
.speed-row{display:flex;gap:.3rem;}
.speed-btn{flex:1;padding:.65rem .2rem;background:var(--bg);border:1px solid var(--border);border-radius:8px;color:var(--muted);font-size:.68rem;font-weight:500;cursor:pointer;text-align:center;transition:all .15s;touch-action:manipulation;line-height:1.3;}
.speed-btn .icon{font-size:1rem;display:block;margin-bottom:.15rem;}
.speed-btn.active{background:var(--accent);border-color:var(--accent);color:#fff;}
.toggle-row{display:flex;gap:.35rem;}
.toggle-btn{flex:1;padding:.7rem .3rem;background:var(--bg);border:1px solid var(--border);border-radius:9px;color:var(--muted);font-size:.8rem;font-weight:500;cursor:pointer;text-align:center;transition:all .15s;touch-action:manipulation;}
.toggle-btn.active{background:var(--accent);border-color:var(--accent);color:#fff;}
.stepper{display:flex;align-items:center;gap:.5rem;background:var(--bg);border:1px solid var(--border);border-radius:9px;padding:.4rem .6rem;}
.stepper-val{flex:1;text-align:center;font-family:'JetBrains Mono',monospace;font-size:.9rem;color:var(--accent2);}
.stepper-btn{background:var(--surface);border:1px solid var(--border);border-radius:7px;color:var(--text);font-size:1.1rem;width:36px;height:36px;display:flex;align-items:center;justify-content:center;cursor:pointer;touch-action:manipulation;flex-shrink:0;border:none;}
.stepper-btn:active{background:var(--accent);}
.settings-grid{display:flex;flex-direction:column;gap:.85rem;}
.setting-block lbl{margin-bottom:.45rem;display:block;}
.status-bar{background:var(--surface);border:1px solid var(--border);border-radius:12px;padding:.8rem 1.1rem;display:flex;align-items:center;justify-content:space-between;font-size:.8rem;margin-bottom:.7rem;}
.s-left{display:flex;align-items:center;gap:.5rem;}
.sdot{width:8px;height:8px;border-radius:50%;background:var(--muted);flex-shrink:0;}
.sdot.running{background:var(--green);animation:blink .8s infinite;}
.sdot.done{background:var(--accent);}.sdot.stopped{background:var(--red);}
#s-text{font-weight:500;}
.s-right{font-family:'JetBrains Mono',monospace;font-size:.72rem;color:var(--accent2);}
.key-section-title{font-size:.65rem;letter-spacing:.1em;text-transform:uppercase;color:var(--muted);margin-bottom:.4rem;margin-top:.85rem;}
.key-section-title:first-child{margin-top:0;}
.key-grid{display:flex;flex-wrap:wrap;gap:.3rem;}
.key{padding:.5rem .7rem;background:var(--bg);border:1px solid var(--border);border-radius:7px;font-family:'JetBrains Mono',monospace;font-size:.72rem;color:var(--text);cursor:pointer;user-select:none;touch-action:manipulation;}
.key:active{background:var(--accent);border-color:var(--accent);color:#fff;transform:scale(.95);}
.key.mod{border-color:#252540;color:var(--accent2);}
.wifi-card lbl{margin-top:.7rem;display:block;}.wifi-card lbl:first-child{margin-top:0;}
.toast{position:fixed;bottom:1.2rem;left:50%;transform:translateX(-50%) translateY(80px);background:#1e1e2e;border:1px solid var(--accent);border-radius:10px;padding:.55rem 1.1rem;font-size:.8rem;color:var(--accent2);transition:transform .22s;z-index:99;white-space:nowrap;pointer-events:none;}
.toast.show{transform:translateX(-50%) translateY(0);}
</style>
</head>
<body>
<div class="wrap">
  <header>
    <div class="badge"><div class="dot"></div>ESP32-S3 - USB HID - DE-Layout</div>
    <div class="badge" id="fw-badge" style="margin-left:.3rem;cursor:pointer;" onclick="checkVersion()" title="Klicken zum Pruefen">v?.?.?</div>
    <h1>Keyboard Controller</h1>
    <p class="sub">Alle DE-Sonderzeichen unterstuetzt</p>
  </header>
  <div class="tabs">
    <div class="tab active" onclick="switchTab('seq')">&#9889; Sequenz</div>
    <div class="tab" onclick="switchTab('quick')">&#9000; Tasten</div>
    <div class="tab" onclick="switchTab('tmpl')">&#128193; Vorlagen</div>
    <div class="tab" onclick="switchTab('auto')">&#9654;&#9654; Autostart</div>
    <div class="tab" onclick="switchTab('wifi')">&#128225; WLAN</div>
  </div>

  <div class="panel active" id="tab-seq">
    <div class="card">
      <lbl>Schritte</lbl>
      <div class="sequence" id="seq-list">
        <div style="color:var(--muted);font-size:.78rem;text-align:center;padding:.6rem 0;">Noch keine Schritte.</div>
      </div>
      <div class="add-row">
        <select id="step-type" onchange="onTypeChange()">
          <option value="text"> Text</option>
          <option value="key"> Taste</option>
          <option value="delay"> Pause</option>
        </select>
        <input type="text" id="step-text" class="grow" placeholder="Text eingeben... auch &amp; { [ @ etc.">
        <!-- Text-Geschwindigkeit (nur bei Text sichtbar) -->
        <select id="step-speed" title="Geschwindigkeit fuer diesen Schritt" onchange="onStepSpeedChange()" style="background:var(--bg);border:1px solid var(--border);border-radius:9px;color:var(--accent2);font-size:.75rem;padding:.6rem .5rem;outline:none;cursor:pointer;display:none;">
          <option value="-1"> Global</option>
          <option value="120">&#128002; Sehr langsam</option>
          <option value="60">&#128694; Langsam</option>
          <option value="20">&#128100; Normal</option>
          <option value="5">&#128640; Schnell</option>
          <option value="0">&#9889; Blitz</option>
          <option value="-2">&#127756; Custom</option>
        </select>
        <input type="number" id="step-custom-ms" value="0" min="0" max="500"
          style="display:none;width:80px;background:var(--bg);border:1px solid var(--accent);border-radius:9px;color:var(--accent2);font-family:monospace;font-size:.85rem;padding:.6rem .5rem;outline:none;"
          placeholder="ms">
        <select id="step-key" class="grow" style="display:none;max-width:none;">
          <optgroup label="Modifier"><option>LCTRL</option><option>LSHIFT</option><option>LALT</option><option>LGUI</option><option>RCTRL</option><option>RSHIFT</option><option>RALT</option><option>RGUI</option></optgroup>
          <optgroup label="Funktionstasten"><option>ESC</option><option>F1</option><option>F2</option><option>F3</option><option>F4</option><option>F5</option><option>F6</option><option>F7</option><option>F8</option><option>F9</option><option>F10</option><option>F11</option><option>F12</option></optgroup>
          <optgroup label="Navigation"><option>TAB</option><option>ENTER</option><option>BACKSPACE</option><option>DELETE</option><option>INSERT</option><option>HOME</option><option>END</option><option>PAGEUP</option><option>PAGEDOWN</option><option>UP</option><option>DOWN</option><option>LEFT</option><option>RIGHT</option></optgroup>
          <optgroup label="System"><option>CAPS</option><option>SPACE</option><option>PRINT</option><option>NUMLOCK</option></optgroup>
          <optgroup label="H&auml;ufige Combos">
            <option value="LCTRL+c">Ctrl+C</option><option value="LCTRL+v">Ctrl+V</option>
            <option value="LCTRL+x">Ctrl+X</option><option value="LCTRL+z">Ctrl+Z</option>
            <option value="LCTRL+a">Ctrl+A</option><option value="LCTRL+s">Ctrl+S</option>
            <option value="LALT+F4">Alt+F4</option><option value="LCTRL+LALT+DELETE">Ctrl+Alt+Del</option>
            <option value="LGUI+d">Win+D</option><option value="LGUI+l">Win+L</option>
            <option value="LGUI+r">Win+R</option><option value="LALT+TAB">Alt+Tab</option>
            <option value="LCTRL+LSHIFT+ESC">Task-Manager</option>
            <option value="__custom__"> Eigene...</option>
          </optgroup>
        </select>
        <input type="text" id="step-custom" class="grow" placeholder="z.B. LCTRL+n" style="display:none;">
        <input type="number" id="step-delay" placeholder="ms" value="500" min="50" style="display:none;width:80px;">
        <button class="btn btn-add btn-sm" onclick="addStep()">&#43; Add</button>
      </div>
    </div>

    <div class="card">
      <div class="settings-grid">
        <div class="setting-block">
          <lbl>Wiederholungen</lbl>
          <div class="toggle-row">
            <div class="toggle-btn active" id="rep-once" onclick="setRepeat('once')">1 Einmalig</div>
            <div class="toggle-btn" id="rep-loop" onclick="setRepeat('loop')"> Dauerhaft</div>
          </div>
        </div>
        <div class="setting-block" id="interval-block" style="display:none;">
          <lbl>Pause zwischen L&auml;ufen</lbl>
          <div class="stepper">
            <div class="stepper-btn" onclick="changeInterval(-500)"></div>
            <div class="stepper-val" id="interval-val">1.0 s</div>
            <div class="stepper-btn" onclick="changeInterval(500)"></div>
          </div>
        </div>
        <div class="setting-block">
          <lbl>Tippgeschwindigkeit</lbl>
          <div class="speed-row">
            <div class="speed-btn" onclick="setSpeed(0)"><span class="icon">&#128002;</span>Sehr<br>langsam</div>
            <div class="speed-btn" onclick="setSpeed(1)"><span class="icon">&#128694;</span>Langsam</div>
            <div class="speed-btn active" onclick="setSpeed(2)"><span class="icon">&#128100;</span>Normal</div>
            <div class="speed-btn" onclick="setSpeed(3)"><span class="icon">&#128640;</span>Schnell</div>
            <div class="speed-btn" onclick="setSpeed(4)"><span class="icon">&#9889;</span>Blitz</div>
            <div class="speed-btn" onclick="setSpeed(5)"><span class="icon">&#127756;</span>Custom</div>
          </div>
          <div id="speed-info" style="text-align:center;font-size:.72rem;color:var(--accent2);margin-top:.4rem;font-family:monospace;">20 ms / Zeichen (Normal)</div>
          <div id="custom-speed-block" style="display:none;margin-top:.6rem;">
            <div class="stepper">
              <div class="stepper-btn" onclick="changeCustomSpeed(-1)">&#8722;</div>
              <input type="number" id="custom-speed-val" value="0" min="0" max="500"
                style="flex:1;text-align:center;background:var(--bg);border:1px solid var(--border);border-radius:7px;color:var(--accent2);font-family:monospace;font-size:.9rem;padding:.4rem;outline:none;width:0;"
                oninput="setCustomSpeed(parseInt(this.value)||0)"
                onfocus="this.select()">
              <div class="stepper-btn" onclick="changeCustomSpeed(1)">&#43;</div>
            </div>
            <div style="font-size:.65rem;color:var(--muted);margin-top:.35rem;text-align:center;">ms / Zeichen &middot; 0 = Maximum</div>
          </div>
        </div>

        <!-- Burst Modus -->
        <div class="setting-block">
          <div style="display:flex;align-items:center;gap:.5rem;cursor:pointer;" onclick="toggleBurst()">
            <div id="burst-check" style="width:20px;height:20px;border-radius:5px;border:2px solid var(--border);display:flex;align-items:center;justify-content:center;flex-shrink:0;transition:all .15s;">
              <span id="burst-check-icon" style="display:none;color:#fff;font-size:.8rem;">&#10003;</span>
            </div>
            <lbl style="margin:0;cursor:pointer;">Burst-Modus <span style="color:var(--accent2);font-size:.6rem;font-weight:400;">(Zeichen pro USB-Paket)</span></lbl>
          </div>
          <div id="burst-block" style="display:none;margin-top:.5rem;">
            <div class="stepper">
              <div class="stepper-btn" onclick="changeBurst(-1)">&#8722;</div>
              <div class="stepper-val" id="burst-val">3 Zeichen</div>
              <div class="stepper-btn" onclick="changeBurst(1)">&#43;</div>
            </div>
            <div style="font-size:.65rem;color:var(--muted);margin-top:.35rem;text-align:center;">Mehr = schneller &middot; bei zu hohem Wert koennen Zeichen fehlen</div>
          </div>
        </div>

        <!-- Clipboard Modus -->
        <div class="setting-block">
          <lbl>Eingabe-Modus</lbl>
          <div class="toggle-row">
            <div class="toggle-btn active" id="clip-off" onclick="setClipMode(false)">&#9000; Normal tippen</div>
            <div class="toggle-btn" id="clip-on" onclick="setClipMode(true)">&#128203; Copy+Paste</div>
          </div>
          <div id="clip-desc" style="font-size:.68rem;color:var(--muted);margin-top:.5rem;padding:.5rem .75rem;background:var(--bg);border-radius:8px;border:1px solid var(--border);line-height:1.6;"></div>
        </div>
      </div>
      <div class="run-row">
        <button class="btn btn-accent" id="btn-start" onclick="runSeq()"> Starten</button>
        <button class="btn btn-stop" id="btn-stop" onclick="stopSeq()" disabled> Stoppen</button>
      </div>
      <button onclick="clearAll()" style="width:100%;margin-top:.5rem;padding:.6rem;background:transparent;border:1px solid #1e1e2e;border-radius:9px;color:#4a5568;font-size:.75rem;cursor:pointer;font-family:Inter,sans-serif;" onmouseover="this.style.borderColor='#f87171';this.style.color='#f87171'" onmouseout="this.style.borderColor='#1e1e2e';this.style.color='#4a5568'"> Alles loeschen</button>
    </div>

    <div class="status-bar">
      <div class="s-left"><div class="sdot" id="sdot"></div><span id="s-text">Bereit</span></div>
      <div class="s-right">L&auml;ufe: <span id="s-count">0</span></div>
    </div>
  </div>

  <div class="panel" id="tab-quick">
    <div class="card">
      <div class="key-section-title">Modifier</div>
      <div class="key-grid">
        <div class="key mod" onclick="quickKey('LCTRL')">L Ctrl</div><div class="key mod" onclick="quickKey('LSHIFT')">L Shift</div>
        <div class="key mod" onclick="quickKey('LALT')">L Alt</div><div class="key mod" onclick="quickKey('LGUI')"> Win</div>
        <div class="key mod" onclick="quickKey('RCTRL')">R Ctrl</div><div class="key mod" onclick="quickKey('RSHIFT')">R Shift</div>
        <div class="key mod" onclick="quickKey('RALT')">AltGr</div><div class="key mod" onclick="quickKey('RGUI')"> Win R</div>
      </div>
      <div class="key-section-title">Funktionstasten</div>
      <div class="key-grid">
        <div class="key" onclick="quickKey('ESC')">ESC</div>
        <div class="key" onclick="quickKey('F1')">F1</div><div class="key" onclick="quickKey('F2')">F2</div>
        <div class="key" onclick="quickKey('F3')">F3</div><div class="key" onclick="quickKey('F4')">F4</div>
        <div class="key" onclick="quickKey('F5')">F5</div><div class="key" onclick="quickKey('F6')">F6</div>
        <div class="key" onclick="quickKey('F7')">F7</div><div class="key" onclick="quickKey('F8')">F8</div>
        <div class="key" onclick="quickKey('F9')">F9</div><div class="key" onclick="quickKey('F10')">F10</div>
        <div class="key" onclick="quickKey('F11')">F11</div><div class="key" onclick="quickKey('F12')">F12</div>
      </div>
      <div class="key-section-title">Navigation &amp; System</div>
      <div class="key-grid">
        <div class="key" onclick="quickKey('TAB')">Tab</div><div class="key" onclick="quickKey('CAPS')">Caps</div>
        <div class="key" onclick="quickKey('ENTER')">Enter</div><div class="key" onclick="quickKey('BACKSPACE')"></div>
        <div class="key" onclick="quickKey('DELETE')">Del</div><div class="key" onclick="quickKey('INSERT')">Ins</div>
        <div class="key" onclick="quickKey('HOME')">Home</div><div class="key" onclick="quickKey('END')">End</div>
        <div class="key" onclick="quickKey('PAGEUP')">PgUp</div><div class="key" onclick="quickKey('PAGEDOWN')">PgDn</div>
        <div class="key" onclick="quickKey('UP')"></div><div class="key" onclick="quickKey('DOWN')"></div>
        <div class="key" onclick="quickKey('LEFT')"></div><div class="key" onclick="quickKey('RIGHT')"></div>
        <div class="key" onclick="quickKey('SPACE')">Space</div><div class="key" onclick="quickKey('PRINT')">PrtSc</div>
        <div class="key" onclick="quickKey('NUMLOCK')">NumLk</div>
      </div>
      <div class="key-section-title">H&auml;ufige Kombinationen</div>
      <div class="key-grid">
        <div class="key" onclick="quickKey('LCTRL+c')">Ctrl+C</div><div class="key" onclick="quickKey('LCTRL+v')">Ctrl+V</div>
        <div class="key" onclick="quickKey('LCTRL+x')">Ctrl+X</div><div class="key" onclick="quickKey('LCTRL+z')">Ctrl+Z</div>
        <div class="key" onclick="quickKey('LCTRL+a')">Ctrl+A</div><div class="key" onclick="quickKey('LCTRL+s')">Ctrl+S</div>
        <div class="key" onclick="quickKey('LALT+F4')">Alt+F4</div><div class="key" onclick="quickKey('LCTRL+LALT+DELETE')">Ctrl+Alt+Del</div>
        <div class="key" onclick="quickKey('LGUI+d')">Win+D</div><div class="key" onclick="quickKey('LGUI+l')">Win+L</div>
        <div class="key" onclick="quickKey('LGUI+r')">Win+R</div><div class="key" onclick="quickKey('LALT+TAB')">Alt+Tab</div>
        <div class="key" onclick="quickKey('LCTRL+LSHIFT+ESC')">Task-Mgr</div>
      </div>
    </div>
  </div>

  <!-- -- Vorlagen -- -->
  <div class="panel" id="tab-tmpl">
    <div class="card" id="tmpl-editor-card">
      <div style="display:flex;align-items:center;justify-content:space-between;margin-bottom:.75rem;">
        <lbl style="margin:0;">Vorlage bearbeiten</lbl>
        <button class="tmpl-btn" onclick="cancelTmplEdit()" id="tmpl-cancel-btn" style="display:none;">&#10005; Abbrechen</button>
      </div>
      <input type="text" id="tmpl-name-input" placeholder="Vorlagenname z.B. Login-Sequenz" style="margin-bottom:.6rem;">
      <div class="sequence" id="tmpl-step-list">
        <div class="tmpl-empty" id="tmpl-step-empty">Noch keine Schritte.</div>
      </div>
      <div class="add-row">
        <select id="tmpl-step-type" onchange="tmplTypeChange()">
          <option value="text">Text</option>
          <option value="key">Taste</option>
          <option value="delay">Pause</option>
        </select>
        <input type="text" id="tmpl-step-text" class="grow" placeholder="Text eingeben...">
        <select id="tmpl-step-speed" title="Geschwindigkeit" onchange="onTmplSpeedChange()" style="background:var(--bg);border:1px solid var(--border);border-radius:9px;color:var(--accent2);font-size:.75rem;padding:.6rem .5rem;outline:none;cursor:pointer;display:none;">
          <option value="-1">&#127760; Global</option>
          <option value="120">&#128002; Sehr langsam</option>
          <option value="60">&#128694; Langsam</option>
          <option value="20">&#128100; Normal</option>
          <option value="5">&#128640; Schnell</option>
          <option value="0">&#9889; Blitz</option>
          <option value="-2">&#127756; Custom</option>
        </select>
        <input type="number" id="tmpl-step-custom-ms" value="0" min="0" max="500"
          style="display:none;width:90px;background:var(--bg);border:1px solid var(--accent);border-radius:9px;color:var(--accent2);font-family:monospace;font-size:.85rem;padding:.6rem .5rem;outline:none;"
          placeholder="ms">
        <select id="tmpl-step-key" class="grow" style="display:none;max-width:none;">
          <optgroup label="Modifier"><option>LCTRL</option><option>LSHIFT</option><option>LALT</option><option>LGUI</option><option>RALT</option></optgroup>
          <optgroup label="Funktionstasten"><option>ESC</option><option>F1</option><option>F2</option><option>F3</option><option>F4</option><option>F5</option><option>F6</option><option>F7</option><option>F8</option><option>F9</option><option>F10</option><option>F11</option><option>F12</option></optgroup>
          <optgroup label="Navigation"><option>TAB</option><option>ENTER</option><option>BACKSPACE</option><option>DELETE</option><option>HOME</option><option>END</option><option>UP</option><option>DOWN</option><option>LEFT</option><option>RIGHT</option></optgroup>
          <optgroup label="Combos"><option value="LCTRL+c">Ctrl+C</option><option value="LCTRL+v">Ctrl+V</option><option value="LCTRL+a">Ctrl+A</option><option value="LCTRL+s">Ctrl+S</option><option value="LALT+F4">Alt+F4</option><option value="LGUI+r">Win+R</option><option value="LGUI+l">Win+L</option><option value="LCTRL+LALT+DELETE">Ctrl+Alt+Del</option><option value="__custom__">Eigene...</option></optgroup>
        </select>
        <input type="text" id="tmpl-step-custom" class="grow" placeholder="z.B. LCTRL+n" style="display:none;">
        <input type="number" id="tmpl-step-delay" placeholder="ms" value="500" min="50" style="display:none;width:80px;">
        <button class="btn btn-add btn-sm" onclick="tmplAddStep()">&#43; Add</button>
      </div>
      <div style="display:flex;gap:.5rem;margin-top:.9rem;">
        <button class="btn btn-accent" style="flex:2;justify-content:center;" onclick="saveTmpl()">&#128190; Speichern</button>
        <button class="btn btn-ghost" style="flex:1;justify-content:center;" onclick="tmplFromSeq()">&#8645; Aus Sequenz</button>
      </div>
    </div>
    <div class="card">
      <div style="display:flex;align-items:center;justify-content:space-between;margin-bottom:.6rem;">
        <lbl style="margin:0;">Gespeicherte Vorlagen</lbl>
        
      </div>
      <div class="tmpl-list" id="tmpl-list">
        <div class="tmpl-empty">Noch keine Vorlagen.</div>
      </div>
    </div>
  </div>

  <!-- Autostart Panel -->
  <div class="panel" id="tab-auto">
    <div class="card">
      <div style="display:flex;align-items:center;justify-content:space-between;margin-bottom:.75rem;">
        <lbl style="margin:0;">&#9654;&#9654; Autostart</lbl>
        <div style="display:flex;align-items:center;gap:.5rem;">
          <span style="font-size:.75rem;color:var(--muted);">Aktiv</span>
          <div id="auto-toggle" onclick="toggleAutostart()"
            style="width:44px;height:24px;border-radius:12px;background:var(--border);cursor:pointer;position:relative;transition:background .2s;">
            <div id="auto-toggle-thumb"
              style="width:20px;height:20px;border-radius:50%;background:#fff;position:absolute;top:2px;left:2px;transition:left .2s;"></div>
          </div>
        </div>
      </div>

      <!-- Vorlage auswhlen -->
      <lbl>Vorlage auswhlen</lbl>
      <div id="auto-tmpl-list" style="margin-bottom:.9rem;">
        <div class="tmpl-empty">Keine Vorlagen vorhanden. Erst unter Vorlagen anlegen.</div>
      </div>

      <!-- Einstellungen -->
      <div id="auto-settings">
        <lbl>Verzoegerung nach Boot</lbl>
        <div class="stepper" style="margin-bottom:.75rem;">
          <div class="stepper-btn" onclick="changeAutoDelay(-1000)">&#8722;</div>
          <div class="stepper-val" id="auto-delay-val">5 s</div>
          <div class="stepper-btn" onclick="changeAutoDelay(1000)">&#43;</div>
        </div>

        <lbl>Wiederholungen</lbl>
        <div class="stepper" style="margin-bottom:.75rem;">
          <div class="stepper-btn" onclick="changeAutoRepeat(-1)">&#8722;</div>
          <div class="stepper-val" id="auto-repeat-val">1x</div>
          <div class="stepper-btn" onclick="changeAutoRepeat(1)">&#43;</div>
        </div>


      </div>

      <button class="btn btn-accent btn-full" onclick="saveAutostart()" style="margin-top:.5rem;justify-content:center;">
        &#128190; Autostart speichern
      </button>
      <div id="auto-status" style="text-align:center;font-size:.75rem;color:var(--muted);margin-top:.5rem;min-height:1rem;"></div>
    </div>

    <div style="font-size:.72rem;color:var(--muted);text-align:center;line-height:1.8;padding:.5rem;">
      Die gewaehlte Vorlage wird automatisch nach dem Boot<br>
      des ESP32 ausgefuehrt. Sicherstellen dass der PC<br>
      bereit ist bevor der ESP32 startet!
    </div>
  </div>

  <div class="panel" id="tab-wifi">
    <div class="card wifi-card">
      <!-- Aktuelle Verbindung -->
      <div id="wifi-current" style="background:var(--bg);border:1px solid var(--border);border-radius:9px;padding:.75rem 1rem;margin-bottom:.9rem;display:flex;align-items:center;gap:.75rem;">
        <div id="wifi-dot" style="width:8px;height:8px;border-radius:50%;background:var(--muted);flex-shrink:0;"></div>
        <div>
          <div id="wifi-ssid-cur" style="font-size:.82rem;font-weight:500;color:var(--text);">Lade...</div>
          <div id="wifi-ip-cur" style="font-size:.7rem;color:var(--muted);margin-top:.15rem;"></div>
        </div>
      </div>
      <lbl>Neues WLAN einrichten</lbl>
      <input type="text" id="w-ssid" placeholder="WLAN-Name (SSID)" autocomplete="off">
      <lbl>Passwort</lbl>
      <input type="password" id="w-pass" placeholder="WLAN-Passwort">
      <div style="margin-top:.9rem;"><button class="btn btn-accent btn-full" onclick="saveWifi()">&#128190; Speichern &amp; Neustart</button></div>
      <div id="wifi-msg" style="margin-top:.65rem;font-size:.75rem;color:var(--muted);text-align:center;min-height:1rem;"></div>
    </div>
    <div style="font-size:.72rem;color:var(--muted);text-align:center;line-height:1.8;padding:.5rem;">
      Kein WLAN? Hotspot: <span style="color:var(--accent2);font-family:'JetBrains Mono',monospace;">ESP32-Keyboard</span> - PW: <span style="color:var(--accent2);font-family:'JetBrains Mono',monospace;">12345678</span><br>
      &rarr; <span style="color:var(--accent2);">http://192.168.4.1/</span>
    </div>
    <div style="margin-top:.75rem;text-align:center;">
      <a href="/ota" style="display:inline-flex;align-items:center;gap:.4rem;background:#1a1a2e;border:1px solid #1e1e2e;border-radius:9px;padding:.6rem 1.1rem;color:#a78bfa;font-size:.78rem;text-decoration:none;">&#128295; Firmware Update (OTA)</a>
    </div>
  </div>
</div>
<div class="toast" id="toast"></div>

<!-- Edit Modal -->
<div class="modal-bg" id="edit-modal" style="display:none;" onclick="if(event.target===this)closeEdit()">
  <div class="modal">
    <h3>Schritt bearbeiten  <span id="edit-type"></span></h3>
    <lbl>Wert</lbl>
    <input type="text" id="edit-val" placeholder="Neuer Wert..." style="margin-bottom:.5rem;">
    <div id="edit-speed-row" style="display:none;margin-bottom:.5rem;">
      <lbl>Geschwindigkeit dieses Schritts</lbl>
      <select id="edit-speed" onchange="onEditSpeedChange()" style="width:100%;background:var(--bg);border:1px solid var(--border);border-radius:9px;color:var(--text);font-size:.85rem;padding:.6rem .8rem;outline:none;margin-bottom:.4rem;">
        <option value="-1">&#127760; Global (wie oben)</option>
        <option value="120">&#128002; Sehr langsam (120ms)</option>
        <option value="60">&#128694; Langsam (60ms)</option>
        <option value="20">&#128100; Normal (20ms)</option>
        <option value="5">&#128640; Schnell (5ms)</option>
        <option value="0">&#9889; Blitz (0ms)</option>
        <option value="-2">&#127756; Custom</option>
      </select>
      <div id="edit-custom-block" style="display:none;">
        <lbl>Eigener Wert (ms / Zeichen)</lbl>
        <input type="number" id="edit-custom-ms" value="0" min="0" max="500"
          style="width:100%;background:var(--bg);border:1px solid var(--accent);border-radius:9px;color:var(--accent2);font-family:monospace;font-size:.9rem;padding:.6rem .9rem;outline:none;"
          placeholder="z.B. 10">
        <div style="font-size:.65rem;color:var(--muted);margin-top:.25rem;">0 ms = Maximum (schnellst moeglich)</div>
      </div>
    </div>
    <div style="font-size:.68rem;color:var(--muted);margin-bottom:.5rem;">Fuer Text: beliebiger Text - Fuer Taste: z.B. LCTRL+c - Fuer Pause: ms Zahl</div>
    <div class="modal-btns">
      <button class="btn btn-accent" style="flex:2" onclick="saveEdit()"> Speichern</button>
      <button class="btn btn-ghost" style="flex:1" onclick="closeEdit()">Abbrechen</button>
    </div>
  </div>
</div>

<script>
let steps=[],repeatMode='once',running=false,pollTimer=null,loopInterval=1000,speedIndex=2,burstSize=3,clipMode=false,customSpeedMs=0;

//  Persistenz: alles in localStorage speichern 
function saveState(){
  try {
    localStorage.setItem('esp32kb', JSON.stringify({
      steps, repeatMode, loopInterval, speedIndex, burstSize, burstEnabled, clipMode, customSpeedMs
    }));
  } catch(e){}
}

function loadState(){
  try {
    const raw = localStorage.getItem('esp32kb');
    if (!raw) return;
    const s = JSON.parse(raw);
    if (s.steps)        { steps = s.steps; renderSteps(); }
    if (s.repeatMode)   setRepeat(s.repeatMode);
    if (s.loopInterval) { loopInterval = s.loopInterval; document.getElementById('interval-val').textContent=(loopInterval/1000).toFixed(1)+' s'; }
    if (s.speedIndex !== undefined) setSpeed(s.speedIndex); // also updates speed-info
    if (s.burstSize)    {
      burstSize = s.burstSize;
      document.getElementById('burst-val').textContent=burstSize+' Zeichen';
    }
    if (s.burstEnabled) {
      burstEnabled=true;
      const block=document.getElementById('burst-block');
      const check=document.getElementById('burst-check');
      const icon=document.getElementById('burst-check-icon');
      if(block) block.style.display='';
      if(check){ check.style.background='var(--accent)'; check.style.borderColor='var(--accent)'; }
      if(icon) icon.style.display='';
    }
    setClipMode(s.clipMode===true);
    if (s.customSpeedMs !== undefined) { customSpeedMs = s.customSpeedMs; const csv=document.getElementById('custom-speed-val'); if(csv) csv.value=customSpeedMs; }
  } catch(e){}
}
function switchTab(n){document.querySelectorAll('.tab').forEach(t=>t.classList.remove('active'));document.querySelectorAll('.panel').forEach(p=>p.classList.remove('active'));['seq','quick','tmpl','auto','wifi'].forEach((t,i)=>{if(t===n){document.querySelectorAll('.tab')[i].classList.add('active');document.getElementById('tab-'+t).classList.add('active');}});if(n==='wifi')loadWifiInfo();if(n==='tmpl')loadTemplates();if(n==='auto'){loadTemplates();setTimeout(loadAutostart,300);}}
function onTypeChange(){const t=document.getElementById('step-type').value;document.getElementById('step-text').style.display=t==='text'?'':'none';document.getElementById('step-speed').style.display=t==='text'?'':'none';document.getElementById('step-key').style.display=t==='key'?'':'none';document.getElementById('step-custom').style.display='none';document.getElementById('step-delay').style.display=t==='delay'?'':'none';if(t==='key')document.getElementById('step-key').onchange=function(){document.getElementById('step-custom').style.display=this.value==='__custom__'?'':'none';};}
function addStep(){const t=document.getElementById('step-type').value;let step={type:t},v;if(t==='text'){v=document.getElementById('step-text').value;if(!v){showToast('Bitte Text!');return;}const spd=parseInt(document.getElementById('step-speed').value);step.value=v;step.speed=spd;step.label='"'+(v.length>32?v.slice(0,32)+'...':v)+'"';document.getElementById('step-text').value='';}else if(t==='key'){const sel=document.getElementById('step-key').value;if(sel==='__custom__'){
    v=document.getElementById('step-custom').value.trim();
    if(!v){showToast('Taste waehlen!');return;}
    step.value=v; step.label=v;
  } else {
    v=sel;
    // Zeige den lesbaren Text aus der Option statt des internen Werts
    const keyEl=document.getElementById('step-key');
    const opt=keyEl?keyEl.options[keyEl.selectedIndex]:null;
    step.value=v;
    step.label=opt&&opt.text&&opt.text!==v ? opt.text : v;
  }
  if(!step.value){showToast('Taste waehlen!');return;}}else{v=parseInt(document.getElementById('step-delay').value)||500;step.value=String(v);step.label=v+'ms warten';}steps.push(step);renderSteps();saveState();}
function renderSteps(){
  const el=document.getElementById('seq-list');
  if(!steps.length){el.innerHTML='<div style="color:var(--muted);font-size:.78rem;text-align:center;padding:.6rem 0;">Keine Schritte.</div>';return;}
  const speedIcons={120:'slow',60:'slow',20:'ok',5:'fast',0:'fast'};
  el.innerHTML=steps.map(function(s,i){
    const typeLabel=s.type==='text'?'TEXT':s.type==='key'?'TASTE':'PAUSE';
    let spdBadge='';
    if(s.type==='text'&&s.speed!==undefined){
      if(s.speed===-2) spdBadge='<span class="seq-spd">0ms&#127756;</span>';
      else if(s.speed>=0) spdBadge='<span class="seq-spd">'+s.speed+'ms</span>';
    }
    const upBtn='<button class="seq-btn" onclick="moveStep('+i+',-1)"'+(i===0?' style="opacity:.2;pointer-events:none"':'')+'>up</button>';
    const dnBtn='<button class="seq-btn" onclick="moveStep('+i+',1)"'+(i===steps.length-1?' style="opacity:.2;pointer-events:none"':'')+'>dn</button>';
    return '<div class="seq-step">'
      +'<span class="seq-num">'+(i+1)+'</span>'
      +'<span class="seq-label">'+s.label+'</span>'
      +spdBadge
      +'<span class="seq-type '+s.type+'">'+typeLabel+'</span>'
      +'<div class="seq-actions">'
      +'<button class="seq-btn" onclick="moveStep('+i+',-1)"'+(i===0?' style="opacity:.2;pointer-events:none"':'')+'>&#8593;</button>'
      +'<button class="seq-btn" onclick="moveStep('+i+',1)"'+(i===steps.length-1?' style="opacity:.2;pointer-events:none"':'')+'>&#8595;</button>'
      +'<button class="seq-btn" onclick="editStep('+i+')">&#9998;</button>'
      +'<button class="seq-btn" onclick="removeStep('+i+')" style="color:var(--red)">&#10005;</button>'
      +'</div></div>';
  }).join('');
}
function removeStep(i){steps.splice(i,1);renderSteps();saveState();}
function moveStep(i,dir){
  const j=i+dir;
  if(j<0||j>=steps.length)return;
  [steps[i],steps[j]]=[steps[j],steps[i]];
  renderSteps();
  saveState();
}
// Edit Modal
let editIdx=-1;
function onEditSpeedChange(){
  const val = document.getElementById('edit-speed').value;
  const block = document.getElementById('edit-custom-block');
  if(block) block.style.display = val==='-2' ? '' : 'none';
}

function editStep(i){
  editIdx=i;
  const s=steps[i];
  document.getElementById('edit-type').textContent=s.type==='text'?' Text':s.type==='key'?' Taste':' Pause';
  document.getElementById('edit-val').value=s.value;
  const speedRow=document.getElementById('edit-speed-row');
  const speedSel=document.getElementById('edit-speed');
  const customBlock=document.getElementById('edit-custom-block');
  const customMs=document.getElementById('edit-custom-ms');
  if(s.type==='text'){
    speedRow.style.display='block';
    const spd = s.speed!==undefined ? s.speed : -1;
    // Custom: wenn Wert nicht in den Standard-Optionen ist
    const stdVals = [-1,120,60,20,5,0,-2];
    if(spd >= 1 && !stdVals.includes(spd)){
      // Eigener Wert
      speedSel.value = '-2';
      if(customBlock) customBlock.style.display = '';
      if(customMs) customMs.value = spd;
    } else {
      speedSel.value = String(spd);
      if(customBlock) customBlock.style.display = spd===-2 ? '' : 'none';
      if(customMs) customMs.value = spd >= 0 ? spd : 0;
    }
  } else {
    speedRow.style.display='none';
    if(customBlock) customBlock.style.display='none';
  }
  document.getElementById('edit-modal').style.display='flex';
}
function saveEdit(){
  if(editIdx<0)return;
  const val=document.getElementById('edit-val').value;
  if(!val){showToast('Bitte Wert eingeben!');return;}
  steps[editIdx].value=val;
  steps[editIdx].label=steps[editIdx].type==='text'?'"'+(val.length>32?val.slice(0,32)+'...':val)+'"':steps[editIdx].type==='delay'?val+'ms warten':val;
  if(steps[editIdx].type==='text'){
    const spd=document.getElementById('edit-speed');
    if(spd){
      let speedVal = parseInt(spd.value);
      if(speedVal===-2){
        // Custom: eigenen ms-Wert lesen
        const cms=document.getElementById('edit-custom-ms');
        speedVal = cms ? Math.max(0, parseInt(cms.value)||0) : 0;
      }
      steps[editIdx].speed=speedVal;
    }
  }
  closeEdit();
  renderSteps();
  saveState();
}
function closeEdit(){document.getElementById('edit-modal').style.display='none';editIdx=-1;}
function setRepeat(m){repeatMode=m;document.getElementById('rep-once').classList.toggle('active',m==='once');document.getElementById('rep-loop').classList.toggle('active',m==='loop');document.getElementById('interval-block').style.display=m==='loop'?'':'none';saveState();}
function changeInterval(d){loopInterval=Math.max(500,Math.min(30000,loopInterval+d));document.getElementById('interval-val').textContent=(loopInterval/1000).toFixed(1)+' s';saveState();}
function setSpeed(i){
  speedIndex=i;
  document.querySelectorAll('.speed-btn').forEach((b,j)=>b.classList.toggle('active',j===i));
  document.getElementById('custom-speed-block').style.display=i===5?'':'none';
  const info=document.getElementById('speed-info');
  if(info){
    const labels=['120 ms / Zeichen (Sehr langsam)','60 ms / Zeichen (Langsam)','20 ms / Zeichen (Normal)','5 ms / Zeichen (Schnell)','0 ms / Zeichen (Blitz)','Eigene Eingabe unten'];
    info.textContent=labels[i]||'';
    info.style.display=i===5?'none':'block';
  }
  saveState();
}
function changeCustomSpeed(d){
  customSpeedMs=Math.max(0,customSpeedMs+d);
  const el=document.getElementById('custom-speed-val');
  if(el) el.value=customSpeedMs;
  saveState();
}
function setCustomSpeed(val){
  customSpeedMs=Math.max(0,val||0);
  saveState();
}
function toggleBurst(){
  const block = document.getElementById('burst-block');
  const check = document.getElementById('burst-check');
  const icon  = document.getElementById('burst-check-icon');
  const open  = block.style.display !== 'none';
  block.style.display = open ? 'none' : '';
  check.style.background = open ? '' : 'var(--accent)';
  check.style.borderColor = open ? 'var(--border)' : 'var(--accent)';
  icon.style.display = open ? 'none' : '';
  if(!open) burstEnabled=true; else burstEnabled=false;
  saveState();
}
let burstEnabled=false;

function changeBurst(d){
  burstSize=Math.max(1,Math.min(20,burstSize+d));
  document.getElementById('burst-val').textContent=burstSize+' Zeichen';
  updateBurstPreview();
  saveState();
}
function setClipMode(on){
  clipMode=on;
  document.getElementById('clip-off').classList.toggle('active',!on);
  document.getElementById('clip-on').classList.toggle('active',on);
  const desc=document.getElementById('clip-desc');
  if(desc){
    if(on){
      desc.innerHTML='<b>&#128203; Copy+Paste:</b> Text wird per PowerShell in die Zwischenablage kopiert und mit Ctrl+V eingefuegt. Sehr schnell, unabhaengig von Textlaenge. <b>Nur Windows.</b>';
    } else {
      desc.innerHTML='<b>&#9000; Normal tippen:</b> Jedes Zeichen wird einzeln als Tastendruck gesendet. Funktioniert ueberall. Geschwindigkeit haengt von den Einstellungen oben ab.';
    }
  }
  saveState();
}

let burstOpen = false;
function updateBurstPreview(){
  const p = document.getElementById('burst-preview');
  if(p) p.textContent = burstOpen ? burstSize+' Zeichen' : (burstSize!==3?burstSize+' Zeichen':'');
}
function runSeq(){if(!steps.length){showToast('Keine Schritte!');return;}const payload={steps:steps.map(s=>({type:s.type,value:s.value,speed:s.speed!==undefined?s.speed:-1})),speed:speedIndex,customMs:customSpeedMs,burst:burstEnabled?burstSize:1,clipMode:clipMode,repeat:repeatMode==='loop',interval:loopInterval};fetch('/run',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(payload)}).then(r=>{if(r.ok){running=true;setUIRunning(true);startPoll();}});}
function stopSeq(){fetch('/stop',{method:'POST'}).then(()=>{running=false;stopPoll();setUIRunning(false);setStatus('stopped','Gestoppt');});}
function setUIRunning(on){document.getElementById('btn-start').disabled=on;document.getElementById('btn-stop').disabled=!on;if(on)setStatus('running','L&auml;uft...');}
function setStatus(state,text){document.getElementById('sdot').className='sdot '+state;document.getElementById('s-text').textContent=text;}
function startPoll(){pollTimer=setInterval(()=>{fetch('/status').then(r=>r.json()).then(d=>{document.getElementById('s-count').textContent=d.count;if(!d.running&&running){running=false;stopPoll();setUIRunning(false);setStatus('done','OK Fertig');}}).catch(()=>{});},700);}
function stopPoll(){clearInterval(pollTimer);pollTimer=null;}
function quickKey(k){fetch('/sendkey',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({key:k})}).then(()=>showToast(' '+k));}
function saveWifi(){const ssid=document.getElementById('w-ssid').value.trim();const pass=document.getElementById('w-pass').value;if(!ssid){showToast('SSID fehlt!');return;}document.getElementById('wifi-msg').textContent='Speichere...';fetch('/config',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({ssid,password:pass})}).then(r=>{document.getElementById('wifi-msg').textContent=r.ok?'OK Gespeichert! Neustart...':'X Fehler';}).catch(()=>{document.getElementById('wifi-msg').textContent='X Verbindungsfehler';});}
//  Vorlagen 
// -- Vorlagen --
let templates = [];
let tmplSteps = [];
let editTmplId = null;

function loadTemplates(){
  // Lade von ESP32 (fuer alle Geraete gleich)
  fetch('/templates')
    .then(function(r){ return r.json(); })
    .then(function(data){
      templates = Array.isArray(data) ? data : [];
      renderTemplates();
    })
    .catch(function(){
      // Fallback: localStorage (offline)
      try { templates = JSON.parse(localStorage.getItem('esp32kb_templates') || '[]'); }
      catch(e){ templates = []; }
      renderTemplates();
    });
}

function saveTemplates(){
  const json = JSON.stringify(templates);
  // Auf ESP32 speichern
  fetch('/templates/save', {
    method: 'POST',
    headers: {'Content-Type': 'application/json'},
    body: json
  }).then(function(r){
    if(!r.ok) console.error('Vorlage speichern fehlgeschlagen');
    else {
      // Auch lokal cachen als Backup
      try { localStorage.setItem('esp32kb_templates', json); } catch(e){}
    }
  }).catch(function(){
    // Fallback: nur localStorage
    try { localStorage.setItem('esp32kb_templates', json); } catch(e){}
  });
}

// Init neues Template
function newTmpl(){
  editTmplId = null;
  tmplSteps = [];
  document.getElementById('tmpl-name-input').value = '';
  document.getElementById('tmpl-cancel-btn').style.display = 'none';
  renderTmplSteps();
}

// Template aus aktueller Sequenz laden
function tmplFromSeq(){
  if(!steps.length){ showToast('Keine Schritte in der Sequenz!'); return; }
  tmplSteps = JSON.parse(JSON.stringify(steps));
  renderTmplSteps();
  showToast('Schritte aus Sequenz geladen!');
}

// Schritt zur Vorlage hinzufuegen
function onTmplSpeedChange(){
  const val = document.getElementById('tmpl-step-speed').value;
  const cms = document.getElementById('tmpl-step-custom-ms');
  if(cms) cms.style.display = val==='-2' ? '' : 'none';
}

function tmplTypeChange(){
  const t = document.getElementById('tmpl-step-type').value;
  document.getElementById('tmpl-step-text').style.display   = t==='text'  ? '' : 'none';
  document.getElementById('tmpl-step-key').style.display    = t==='key'   ? '' : 'none';
  document.getElementById('tmpl-step-custom').style.display = 'none';
  document.getElementById('tmpl-step-delay').style.display  = t==='delay' ? '' : 'none';
  const spd = document.getElementById('tmpl-step-speed');
  if(spd) spd.style.display = t==='text' ? '' : 'none';
  const cms = document.getElementById('tmpl-step-custom-ms');
  if(cms){ cms.style.display='none'; } // reset
  if(t==='key') document.getElementById('tmpl-step-key').onchange = function(){
    document.getElementById('tmpl-step-custom').style.display = this.value==='__custom__' ? '' : 'none';
  };
}

function tmplAddStep(){
  const t = document.getElementById('tmpl-step-type').value;
  let step = {type:t}, v;
  if(t==='text'){
    v = document.getElementById('tmpl-step-text').value;
    if(!v){ showToast('Text eingeben!'); return; }
    const spd = document.getElementById('tmpl-step-speed');
    step.value = v;
    step.speed = spd ? parseInt(spd.value) : -1;
    step.label = '"'+(v.length>32?v.slice(0,32)+'...':v)+'"';
    document.getElementById('tmpl-step-text').value = '';
  } else if(t==='key'){
    const sel = document.getElementById('tmpl-step-key').value;
    if(sel==='__custom__'){
      v = document.getElementById('tmpl-step-custom').value.trim();
      if(!v){ showToast('Taste waehlen!'); return; }
      step.value=v; step.label=v;
    } else {
      v=sel;
      const keyEl=document.getElementById('tmpl-step-key');
      const opt=keyEl?keyEl.options[keyEl.selectedIndex]:null;
      step.value=v;
      step.label=opt&&opt.text&&opt.text!==v ? opt.text : v;
    }
    if(!step.value){ showToast('Taste waehlen!'); return; }
  } else {
    v = parseInt(document.getElementById('tmpl-step-delay').value)||500;
    step.value = String(v); step.label = v+'ms warten';
  }
  tmplSteps.push(step);
  renderTmplSteps();
}

function tmplRemoveStep(i){ tmplSteps.splice(i,1); renderTmplSteps(); }
function tmplMoveStep(i,dir){
  const j=i+dir;
  if(j<0||j>=tmplSteps.length)return;
  [tmplSteps[i],tmplSteps[j]]=[tmplSteps[j],tmplSteps[i]];
  renderTmplSteps();
}

// Inline-Edit eines Vorlage-Schritts
let tmplEditIdx = -1;

function tmplEditStep(i){
  tmplEditIdx = i;
  const s = tmplSteps[i];
  // Zeige Inline-Editor
  renderTmplSteps();
}

function tmplSaveInlineEdit(i){
  const valEl = document.getElementById('tmpl-inline-val-'+i);
  const spdEl = document.getElementById('tmpl-inline-spd-'+i);
  if(!valEl) return;
  const val = valEl.value.trim();
  if(!val){ showToast('Bitte Wert eingeben!'); return; }
  tmplSteps[i].value = val;
  if(tmplSteps[i].type==='text'){
    tmplSteps[i].label = '"'+(val.length>32?val.slice(0,32)+'...':val)+'"';
    if(spdEl) tmplSteps[i].speed = parseInt(spdEl.value);
  } else if(tmplSteps[i].type==='delay'){
    tmplSteps[i].label = val+'ms warten';
  } else {
    tmplSteps[i].label = val;
  }
  tmplEditIdx = -1;
  renderTmplSteps();
}

function tmplCancelEdit(){
  tmplEditIdx = -1;
  renderTmplSteps();
}

function renderTmplSteps(){
  const el = document.getElementById('tmpl-step-list');
  if(!tmplSteps.length){
    el.innerHTML = '<div class="tmpl-empty" id="tmpl-step-empty">Noch keine Schritte.</div>';
    return;
  }
  el.innerHTML = tmplSteps.map(function(s,i){
    const typeLabel = s.type==='text'?'TEXT':s.type==='key'?'TASTE':'PAUSE';
    const spdIcon = s.type==='text' ? spdBadgeText(s.speed) : '';

    // Inline-Editor aktiv?
    if(tmplEditIdx === i){
      const stdVals = [-1,120,60,20,5,0,-2];
      const isCustom = s.type==='text' && s.speed!==undefined && s.speed>=1 && !stdVals.includes(s.speed);
      const curSpd = isCustom ? -2 : (s.speed!==undefined ? s.speed : -1);
      const curMs  = isCustom ? s.speed : 0;

      const spdOpts = [
        ['&#127760; Global','-1'],
        ['&#128002; Sehr langsam','120'],
        ['&#128694; Langsam','60'],
        ['&#128100; Normal','20'],
        ['&#128640; Schnell','5'],
        ['&#9889; Blitz','0'],
        ['&#127756; Custom','-2']
      ].map(function(o){
        const sel = String(curSpd)===o[1]?' selected':'';
        return '<option value="'+o[1]+'"'+sel+'>'+o[0]+'</option>';
      }).join('');

      return '<div class="seq-step" style="flex-direction:column;align-items:stretch;gap:.4rem;">'
        +'<div style="display:flex;align-items:center;gap:.4rem;">'
          +'<span class="seq-num">'+(i+1)+'</span>'
          +'<span class="seq-type '+s.type+'">'+typeLabel+'</span>'
          +'<button class="seq-btn" onclick="tmplCancelEdit()" style="margin-left:auto;color:var(--muted);">&#10005; Abbrechen</button>'
        +'</div>'
        +'<input type="text" id="tmpl-inline-val-'+i+'" value="'+s.value+'" style="width:100%;background:var(--bg);border:1px solid var(--accent);border-radius:8px;color:var(--text);font-family:monospace;font-size:.85rem;padding:.5rem .75rem;outline:none;">'
        +(s.type==='text'?
          '<select id="tmpl-inline-spd-'+i+'" onchange="onTmplInlineSpdChange('+i+')" style="width:100%;background:var(--bg);border:1px solid var(--border);border-radius:8px;color:var(--accent2);font-size:.78rem;padding:.45rem .7rem;outline:none;">'+spdOpts+'</select>'
          +'<input type="number" id="tmpl-inline-cms-'+i+'" value="'+curMs+'" min="0" max="500" placeholder="ms" style="display:'+(curSpd===-2?'':'none')+';width:100%;background:var(--bg);border:1px solid var(--accent);border-radius:8px;color:var(--accent2);font-family:monospace;font-size:.82rem;padding:.4rem .7rem;outline:none;">'
          :'')
        +'<button class="btn btn-accent btn-sm" onclick="tmplSaveInlineEdit('+i+')" style="width:100%;justify-content:center;">&#128190; Speichern</button>'
        +'</div>';
    }

    // Normal-Ansicht
    return '<div class="seq-step">'
      +'<span class="seq-num">'+(i+1)+'</span>'
      +'<span class="seq-label">'+s.label+'</span>'
      +(spdIcon?'<span class="seq-spd">'+spdIcon+'</span>':'')
      +'<span class="seq-type '+s.type+'">'+typeLabel+'</span>'
      +'<div class="seq-actions">'
      +'<button class="seq-btn" onclick="tmplMoveStep('+i+',-1)"'+(i===0?' style="opacity:.2;pointer-events:none"':'')+'>&#8593;</button>'
      +'<button class="seq-btn" onclick="tmplMoveStep('+i+',1)"'+(i===tmplSteps.length-1?' style="opacity:.2;pointer-events:none"':'')+'>&#8595;</button>'
      +'<button class="seq-btn" onclick="tmplEditStep('+i+')">&#9998;</button>'
      +'<button class="seq-btn" onclick="tmplRemoveStep('+i+')" style="color:var(--red)">&#10005;</button>'
      +'</div></div>';
  }).join('');
}

// Vorlage speichern
function saveTmpl(){
  const name = document.getElementById('tmpl-name-input').value.trim();
  if(!name){ showToast('Bitte Namen eingeben!'); return; }
  if(!tmplSteps.length){ showToast('Keine Schritte!'); return; }

  if(editTmplId !== null){
    // Bearbeiten
    const idx = templates.findIndex(t=>t.id===editTmplId);
    if(idx>=0){
      templates[idx].name = name;
      templates[idx].steps = JSON.parse(JSON.stringify(tmplSteps));
    }
  } else {
    // Neu anlegen
    templates.unshift({
      id: Date.now(),
      name: name,
      steps: JSON.parse(JSON.stringify(tmplSteps)),
      created: new Date().toLocaleDateString('de-DE')
    });
  }
  saveTemplates();
  renderTemplates();
  newTmpl(); // Editor zuruecksetzen
  showToast('Vorlage gespeichert!');
}

function cancelTmplEdit(){
  editTmplId = null;
  newTmpl();
}

// Vorlage editieren
function editTemplate(id){
  const tmpl = templates.find(t=>t.id===id);
  if(!tmpl) return;
  editTmplId = id;
  tmplSteps = JSON.parse(JSON.stringify(tmpl.steps));
  document.getElementById('tmpl-name-input').value = tmpl.name;
  document.getElementById('tmpl-cancel-btn').style.display = '';
  renderTmplSteps();
  // Scroll zum Editor
  document.getElementById('tmpl-editor-card').scrollIntoView({behavior:'smooth'});
}

// Vorlage in Sequenz laden (ersetzen)
function importTemplate(id){
  const tmpl = templates.find(t=>t.id===id);
  if(!tmpl) return;
  if(steps.length>0 && !confirm('Aktuelle Schritte ersetzen?')) return;
  steps = JSON.parse(JSON.stringify(tmpl.steps));
  renderSteps();
  saveState();
  switchTab('seq');
  showToast('Vorlage geladen: '+tmpl.name);
}

// Vorlage an Sequenz anhaengen
function appendTemplate(id){
  const tmpl = templates.find(t=>t.id===id);
  if(!tmpl) return;
  const newSteps = JSON.parse(JSON.stringify(tmpl.steps));
  steps = steps.concat(newSteps);
  renderSteps();
  saveState();
  switchTab('seq');
  showToast('Vorlage angehaengt: '+tmpl.name);
}

// Vorlage loeschen
function deleteTemplate(id){
  if(!confirm('Vorlage loeschen?')) return;
  templates = templates.filter(t=>t.id!==id);
  saveTemplates();
  renderTemplates();

  // Autostart deaktivieren falls diese Vorlage aktiv war
  if(String(autoCfg.tmplId)===String(id) && autoCfg.enabled){
    autoCfg.enabled=false;
    autoCfg.tmplId=null;
    fetch('/autostart/save',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(autoCfg)});
    showToast('Vorlage geloescht & Autostart deaktiviert!');
    // Autostart Tab aktualisieren falls offen
    renderAutostart();
  } else {
    showToast('Vorlage geloescht.');
  }
}

// Vorlagenliste rendern
// Speed label helper
function speedLabel(spd){
  if(spd===undefined||spd===-1) return '';
  if(spd===-2) return '&#127756;';
  if(spd===0)  return '&#9889;';
  if(spd<=5)   return '&#128640;';
  if(spd<=20)  return '&#128100;';
  if(spd<=60)  return '&#128694;';
  return '&#128002;';
}
// ms-Badge fuer Schritte-Anzeige
function spdBadgeText(spd){
  if(spd===undefined||spd===-1) return '';
  if(spd===-2) return '0ms&#127756;';
  return spd+'ms';
}

function toggleTmplPreview(id){
  const el = document.getElementById('tmpl-preview-'+id);
  if(!el) return;
  const isOpen = el.style.display !== 'none';
  document.querySelectorAll('.tmpl-preview').forEach(function(p){p.style.display='none';});
  document.querySelectorAll('.tmpl-arrow').forEach(function(a){a.innerHTML='&#9660;';});
  if(!isOpen){
    el.style.display='block';
    const arrow=document.getElementById('tmpl-arrow-'+id);
    if(arrow) arrow.innerHTML='&#9650;';
  }
}

function renderTemplates(){
  const el = document.getElementById('tmpl-list');
  if(!el) return;
  if(!templates.length){
    el.innerHTML = '<div class="tmpl-empty">Noch keine Vorlagen.</div>';
    return;
  }
  el.innerHTML = templates.map(function(t){
    const meta = t.steps.length+' Schritte &middot; '+t.created;
    const preview = t.steps.map(function(s,i){
      const typeLabel = s.type==='text'?'TEXT':s.type==='key'?'TASTE':'PAUSE';
      let spdIcon='';
      if(s.type==='text'&&s.speed!==undefined){
        if(s.speed===-2) spdIcon='0ms&#127756;';
        else if(s.speed>=0) spdIcon=s.speed+'ms';
      }
      return '<div class="seq-step" style="pointer-events:none;opacity:.85;">'
        +'<span class="seq-num">'+(i+1)+'</span>'
        +'<span class="seq-label">'+s.label+'</span>'
        +(spdIcon?'<span class="seq-spd">'+spdIcon+'</span>':'')
        +'<span class="seq-type '+s.type+'">'+typeLabel+'</span>'
        +'</div>';
    }).join('');
    return '<div class="tmpl-item" style="flex-direction:column;align-items:stretch;gap:.5rem;">'
      +'<div style="display:flex;align-items:center;gap:.6rem;">'
        +'<div class="tmpl-icon" style="cursor:pointer;" onclick="toggleTmplPreview('+t.id+')">&#128196;</div>'
        +'<div class="tmpl-info" style="cursor:pointer;flex:1;min-width:0;" onclick="toggleTmplPreview('+t.id+')">'
          +'<div class="tmpl-name">'+t.name+' <span class="tmpl-arrow" id="tmpl-arrow-'+t.id+'" style="font-size:.65rem;color:var(--muted);">&#9660;</span></div>'
          +'<div class="tmpl-meta">'+meta+'</div>'
        +'</div>'
        +'<div class="tmpl-actions">'
          +'<button class="tmpl-btn" onclick="appendTemplate('+t.id+')" style="background:#1a2a3a;border-color:#7c6af7;color:#a78bfa;">&#43; Anh.</button>'
          +'<button class="tmpl-btn primary" onclick="importTemplate('+t.id+')">&#9654; Laden</button>'
          +'<button class="tmpl-btn" onclick="editTemplate('+t.id+')">&#9998;</button>'
          +'<button class="tmpl-btn" onclick="deleteTemplate('+t.id+')" style="color:var(--red);border-color:var(--red);">&#10005;</button>'
        +'</div>'
      +'</div>'
      +'<div class="tmpl-preview sequence" id="tmpl-preview-'+t.id+'" style="display:none;margin:0;gap:.3rem;">'
        +preview
      +'</div>'
      +'</div>';
  }).join('');
}
// -- Autostart JS --
let autoCfg = { enabled:false, tmplId:null, delay:5000, repeat:1, speed:20, burst:3 };
const autoSpeedDelays = [120,60,20,5,0];

function loadAutostart(){
  fetch('/autostart')
    .then(function(r){ return r.json(); })
    .then(function(d){
      if(d && typeof d === 'object'){
        autoCfg = Object.assign(autoCfg, d);
      }
      renderAutostart();
    })
    .catch(function(){ renderAutostart(); });
}

function renderAutostart(){
  // Toggle
  const tog = document.getElementById('auto-toggle');
  const thumb = document.getElementById('auto-toggle-thumb');
  if(tog) tog.style.background = autoCfg.enabled ? 'var(--accent)' : 'var(--border)';
  if(thumb) thumb.style.left = autoCfg.enabled ? '22px' : '2px';

  // Delay
  const dv = document.getElementById('auto-delay-val');
  if(dv) dv.textContent = (autoCfg.delay/1000).toFixed(0)+' s';

  // Repeat
  const rv = document.getElementById('auto-repeat-val');
  if(rv) rv.textContent = autoCfg.repeat===0 ? 'Endlos' : autoCfg.repeat+'x';

  // Template liste
  const el = document.getElementById('auto-tmpl-list');
  if(!el) return;
  if(!templates.length){
    el.innerHTML='<div class="tmpl-empty">Keine Vorlagen. Erst unter Vorlagen anlegen.</div>';
    return;
  }
  el.innerHTML = templates.map(function(t){
    const sel = String(autoCfg.tmplId) === String(t.id);
    // Schritte Vorschau
    const preview = t.steps.map(function(s,i){
      const typeLabel = s.type==='text'?'TEXT':s.type==='key'?'TASTE':'PAUSE';
      let spdIcon='';
      if(s.type==='text'&&s.speed!==undefined){
        if(s.speed===-2) spdIcon='&#127756;ms';
        else if(s.speed>=0) spdIcon=s.speed+'ms';
      }
      return '<div class="seq-step" style="pointer-events:none;opacity:.85;">'
        +'<span class="seq-num">'+(i+1)+'</span>'
        +'<span class="seq-label">'+s.label+'</span>'
        +(spdIcon?'<span class="seq-spd">'+spdIcon+'</span>':'')
        +'<span class="seq-type '+s.type+'">'+typeLabel+'</span>'
        +'</div>';
    }).join('');
    return '<div style="margin-bottom:.5rem;">'
      // Auswahl-Zeile
      +'<div class="auto-item'+(sel?' selected':'')+'" onclick="selectAutoTmpl('+t.id+')" style="margin-bottom:0;border-radius:'+(t.steps.length?'10px 10px 0 0':'10px')+';">'
        +'<div class="auto-check"><div class="auto-check-inner"></div></div>'
        +'<div class="auto-info">'
          +'<div class="auto-name">'+t.name+'</div>'
          +'<div class="auto-meta">'+t.steps.length+' Schritte &middot; '+t.created+'</div>'
        +'</div>'
        +'<button class="seq-btn" onclick="event.stopPropagation();toggleAutoPreview('+t.id+')" style="flex-shrink:0;">&#9660;</button>'
      +'</div>'
      // Vorschau
      +'<div id="auto-preview-'+t.id+'" class="sequence" style="display:none;margin:0;gap:.3rem;background:var(--bg);border:1px solid var(--border);border-top:none;border-radius:0 0 10px 10px;padding:.5rem .75rem;">'
        +preview
      +'</div>'
      +'</div>';
  }).join('');
}

function onStepSpeedChange(){
  const val = document.getElementById('step-speed').value;
  const cms = document.getElementById('step-custom-ms');
  if(cms) cms.style.display = val==='-2' ? '' : 'none';
}

function onTmplInlineSpdChange(i){
  const sel = document.getElementById('tmpl-inline-spd-'+i);
  const cms = document.getElementById('tmpl-inline-cms-'+i);
  if(sel && cms) cms.style.display = sel.value==='-2' ? '' : 'none';
}

function toggleAutoPreview(id){
  const el = document.getElementById('auto-preview-'+id);
  if(!el) return;
  const open = el.style.display !== 'none';
  document.querySelectorAll('[id^="auto-preview-"]').forEach(function(p){p.style.display='none';});
  if(!open) el.style.display = 'block';
}

function toggleAutostart(){
  autoCfg.enabled = !autoCfg.enabled;
  renderAutostart();
}

function selectAutoTmpl(id){
  autoCfg.tmplId = id;
  renderAutostart();
}

function changeAutoDelay(d){
  autoCfg.delay = Math.max(1000, Math.min(60000, autoCfg.delay+d));
  document.getElementById('auto-delay-val').textContent=(autoCfg.delay/1000).toFixed(0)+' s';
}

function changeAutoRepeat(d){
  autoCfg.repeat = Math.max(0, Math.min(99, autoCfg.repeat+d));
  document.getElementById('auto-repeat-val').textContent=autoCfg.repeat===0?'Endlos':autoCfg.repeat+'x';
}

function saveAutostart(){
  if(autoCfg.enabled && !autoCfg.tmplId){
    showToast('Bitte eine Vorlage auswaehlen!');
    return;
  }
  const status = document.getElementById('auto-status');
  if(status) status.textContent = 'Speichern...';
  fetch('/autostart/save',{
    method:'POST',
    headers:{'Content-Type':'application/json'},
    body: JSON.stringify(autoCfg)
  }).then(function(r){
    if(r.ok){
      if(status) status.textContent = '&#10003; Gespeichert!';
      setTimeout(function(){ if(status) status.textContent=''; }, 3000);
      showToast('Autostart gespeichert! Neustart noetig.');
      if(status){
        status.innerHTML = '&#128161; Tritt erst nach einem Neustart des ESP32 in Kraft!';
        status.style.color = 'var(--accent2)';
      }
    } else {
      if(status) status.textContent = 'Fehler beim Speichern';
    }
  }).catch(function(){
    if(status) status.textContent = 'Verbindungsfehler';
  });
}

let toastT;function showToast(msg){const t=document.getElementById('toast');t.textContent=msg;t.classList.add('show');clearTimeout(toastT);toastT=setTimeout(()=>t.classList.remove('show'),2000);}
function clearAll(){if(!confirm('Alle Schritte und Einstellungen loeschen?'))return;steps=[];repeatMode='once';loopInterval=1000;speedIndex=2;burstSize=3;clipMode=false;customSpeedMs=0;localStorage.removeItem('esp32kb');renderSteps();setRepeat('once');setSpeed(2);setClipMode(false);document.getElementById('interval-val').textContent='1.0 s';document.getElementById('burst-val').textContent='3 Zeichen (Standard)';const csv2=document.getElementById('custom-speed-val'); if(csv2) csv2.value=0; burstOpen=false; const bb=document.getElementById('burst-block'); if(bb) bb.style.display='none'; updateBurstPreview();showToast('Alles geloescht!');}
// Beim Laden State wiederherstellen
window.addEventListener('load', function(){ loadState(); setTimeout(function(){ if(!clipMode) setClipMode(false); }, 100); });
window.addEventListener('load', function(){ loadTemplates(); }); // renderTemplates wird in loadTemplates aufgerufen
window.addEventListener('load', checkVersion);
window.addEventListener('load', loadWifiInfo);

function checkVersion(){
  fetch('/version').then(r=>r.json()).then(d=>{
    const b=document.getElementById('fw-badge');
    if(b) b.textContent='v'+d.version+' ('+d.date+')';
  }).catch(()=>{
    const b=document.getElementById('fw-badge');
    if(b) b.textContent='v?';
  });
}
function loadWifiInfo(){
  fetch('/wifiinfo').then(r=>r.json()).then(d=>{
    const dot=document.getElementById('wifi-dot');
    const ssid=document.getElementById('wifi-ssid-cur');
    const ip=document.getElementById('wifi-ip-cur');
    if(!dot||!ssid||!ip) return;
    if(d.connected){
      dot.style.background='#4ade80';
      dot.style.animation='blink 2s infinite';
      ssid.textContent=d.ssid+' ('+d.rssi+')';
      ip.textContent='IP: '+d.ip;
    } else {
      dot.style.background='#fbbf24';
      ssid.textContent='Hotspot aktiv';
      ip.textContent=d.ip+' - Kein Heimnetz';
    }
  }).catch(()=>{});
}
</script>
</body>
</html>
)rawhtml";

// -- HTTP Handler ----------------------------------------------
void handleVersion(){
  String json = "{\"version\":\"" + String(FW_VERSION) + "\",\"date\":\"" + String(FW_DATE) + "\"}";
  server.send(200,"application/json",json);
}

// -- Vorlagen auf ESP32 speichern/laden --
// -- Autostart Config --
void handleGetAutostart(){
  prefs.begin("autostart", true);
  String data = prefs.getString("cfg", "{}");
  prefs.end();
  server.send(200, "application/json", data);
}

void handleSaveAutostart(){
  if(server.method()!=HTTP_POST){server.send(405,"text/plain","Method Not Allowed");return;}
  String body = server.arg("plain");
  prefs.begin("autostart", false);
  prefs.putString("cfg", body);
  prefs.end();
  // Parse und in globale Variablen laden
  g_autoEnabled = body.indexOf("\"enabled\":true") >= 0;
  int di = body.indexOf("\"delay\":"); if(di>=0){di+=8;g_autoDelay=body.substring(di,body.indexOf(",",di)<0?body.indexOf("}",di):body.indexOf(",",di)).toInt();}
  int ri = body.indexOf("\"repeat\":"); if(ri>=0){ri+=9;g_autoRepeat=body.substring(ri,body.indexOf(",",ri)<0?body.indexOf("}",ri):body.indexOf(",",ri)).toInt();}
  int si = body.indexOf("\"speed\":"); if(si>=0){si+=8;g_autoSpeed=body.substring(si,body.indexOf(",",si)<0?body.indexOf("}",si):body.indexOf(",",si)).toInt();}
  int bi = body.indexOf("\"burst\":"); if(bi>=0){bi+=8;g_autoBurst=body.substring(bi,body.indexOf(",",bi)<0?body.indexOf("}",bi):body.indexOf(",",bi)).toInt();}
  // tmplId kann String oder Number sein
  int ti = body.indexOf("\"tmplId\":\"");
  if(ti>=0){ ti+=10; g_autoTmplId=body.substring(ti,body.indexOf("\"",ti)); }
  else {
    ti = body.indexOf("\"tmplId\":"); 
    if(ti>=0){ ti+=9; int te=body.indexOf(",",ti); if(te<0)te=body.indexOf("}",ti); g_autoTmplId=body.substring(ti,te); g_autoTmplId.trim(); }
  }
  Serial.printf("Autostart: enabled=%d delay=%d repeat=%d\n",g_autoEnabled,g_autoDelay,g_autoRepeat);
  // WICHTIG: g_autoStarted und g_autoStartTime werden NICHT veraendert!
  // g_autoStartTime wird nur beim Boot in setup() gesetzt.
  // Autostart laeuft erst beim naechsten Neustart des ESP32.
  // g_autoStartTime bleibt 0 wenn nach dem Boot gespeichert wird.
  server.send(200,"text/plain","OK");
}

void handleGetTemplates(){
  prefs.begin("templates", true);
  String data = prefs.getString("data", "[]");
  prefs.end();
  server.send(200, "application/json", data);
}

void handleSaveTemplates(){
  if(server.method() != HTTP_POST){ server.send(405,"text/plain","Method Not Allowed"); return; }
  String body = server.arg("plain");
  if(body.length() == 0){ server.send(400,"text/plain","Empty"); return; }
  // Max 10KB fuer Vorlagen
  if(body.length() > 10240){ server.send(413,"text/plain","Too large"); return; }
  prefs.begin("templates", false);
  prefs.putString("data", body);
  prefs.end();
  Serial.printf("Vorlagen gespeichert: %d bytes\n", body.length());
  server.send(200,"text/plain","OK");
}

void handleWifiInfo(){
  String ssid = wifiConnected ? WiFi.SSID() : "Hotspot (ESP32-Keyboard)";
  String ip   = wifiConnected ? WiFi.localIP().toString() : WiFi.softAPIP().toString();
  String rssi = wifiConnected ? String(WiFi.RSSI())+"dBm" : "N/A";
  String json = "{\"ssid\":\""+ssid+"\",\"ip\":\""+ip+"\",\"rssi\":\""+rssi+"\",\"connected\":"+String(wifiConnected?"true":"false")+"}";
  server.send(200,"application/json",json);
}

void handleRoot(){server.send_P(200,"text/html",INDEX_HTML);}

void handleRun(){
  if(server.method()!=HTTP_POST){server.send(405,"text/plain","nope");return;}
  String body=server.arg("plain");

  // Tippgeschwindigkeit
  int si=body.indexOf("\"speed\":");
  if(si>=0){
    si+=8; int e=body.indexOf(",",si); if(e<0) e=body.indexOf("}",si);
    int sv=body.substring(si,e).toInt();
    if(sv==5){
      int ci=body.indexOf("\"customMs\":");
      if(ci>=0){ci+=11;int ce=body.indexOf(",",ci);if(ce<0)ce=body.indexOf("}",ci);g_typeDelay=body.substring(ci,ce).toInt();}
      else g_typeDelay=0;
    } else {
      const int d[]={120,60,20,5,0}; g_typeDelay=d[constrain(sv,0,4)];
    }
  }

  // Burst-Groesse
  int bi=body.indexOf("\"burst\":");
  if(bi>=0){bi+=8;int e=body.indexOf(",",bi);if(e<0)e=body.indexOf("}",bi);g_burstSize=max(1,(int)body.substring(bi,e).toInt());}
  else g_burstSize=1;

  // Clipboard-Modus
  g_clipMode = body.indexOf("\"clipMode\":true") >= 0;
  Serial.printf("Run: delay=%d burst=%d clip=%d\n", g_typeDelay, g_burstSize, g_clipMode);

  // Loop
  g_looping=body.indexOf("\"repeat\":true")>=0;
  int li=body.indexOf("\"interval\":");
  if(li>=0){li+=11;int e=body.indexOf(",",li);if(e<0)e=body.indexOf("}",li);g_loopInterval=body.substring(li,e).toInt();if(g_loopInterval<200)g_loopInterval=200;}

  // Schritte parsen
  g_stepCount=0;
  int ss=body.indexOf("\"steps\":[");
  if(ss>=0){
    ss+=9; int pos=ss;
    while(pos<(int)body.length()&&g_stepCount<MAX_STEPS){
      int os=body.indexOf("{",pos); if(os<0) break;
      int oe=body.indexOf("}",os); if(oe<0) break;
      String obj=body.substring(os,oe+1);
      String type=jsonStr(obj,"type");
      String value=jsonStr(obj,"value");
      if(type.length()>0){
        g_steps[g_stepCount].type=type;
        g_steps[g_stepCount].value=value;
        // Per-Schritt Geschwindigkeit
        int spIdx = obj.indexOf("\"speed\":");
        if(spIdx>=0){spIdx+=8;int se=obj.indexOf(",",spIdx);if(se<0)se=obj.indexOf("}",spIdx);g_steps[g_stepCount].stepSpeed=obj.substring(spIdx,se).toInt();}
        else g_steps[g_stepCount].stepSpeed=-1;
        g_stepCount++;
      }
      pos=oe+1;
    }
  }

  g_running=true; g_runCount=0; g_nextRun=0;
  server.send(200,"text/plain","OK");
}

void handleStop(){
  g_running=false;
  Keyboard.releaseAll();
  Serial.println("Stop empfangen!");
  server.send(200,"text/plain","OK");
}

void handleStatus(){server.send(200,"application/json","{\"running\":"+String(g_running?"true":"false")+",\"count\":"+String(g_runCount)+"}");}

void handleSendKey(){
  if(server.method()!=HTTP_POST){server.send(405,"text/plain","nope");return;}
  String key=jsonStr(server.arg("plain"),"key");
  if(key.length()>0)sendCombo(key);
  server.send(200,"text/plain","OK");
}

// -- OTA Update Handler ---------------------------------------
void handleOTAPage() {
  String html = R"HTML(
<!DOCTYPE html><html lang="de"><head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>ESP32 Firmware Update</title>
<style>
*{margin:0;padding:0;box-sizing:border-box;}
body{background:#0a0a0f;color:#e2e8f0;font-family:Inter,sans-serif;min-height:100vh;display:flex;align-items:center;justify-content:center;padding:1.5rem;}
.card{background:#13131a;border:1px solid #1e1e2e;border-radius:16px;padding:2rem;width:100%;max-width:440px;}
h2{font-size:1.1rem;font-weight:600;margin-bottom:.4rem;}
p{color:#4a5568;font-size:.8rem;margin-bottom:1.5rem;line-height:1.6;}
.drop{border:2px dashed #1e1e2e;border-radius:12px;padding:2rem;text-align:center;cursor:pointer;transition:border-color .2s;margin-bottom:1rem;}
.drop:hover,.drop.over{border-color:#7c6af7;}
.drop input{display:none;}
.drop-icon{font-size:2rem;margin-bottom:.5rem;}
.drop-text{font-size:.82rem;color:#4a5568;}
.drop-name{font-size:.85rem;color:#a78bfa;margin-top:.4rem;font-weight:500;}
.btn{width:100%;padding:.85rem;background:#7c6af7;color:#fff;border:none;border-radius:10px;font-size:.9rem;font-weight:600;cursor:pointer;}
.btn:disabled{opacity:.4;cursor:not-allowed;}
.btn:not(:disabled):hover{background:#6b5ce7;}
.progress{margin-top:1rem;display:none;}
.bar-bg{background:#1e1e2e;border-radius:100px;height:8px;overflow:hidden;}
.bar{background:#7c6af7;height:100%;width:0%;transition:width .3s;border-radius:100px;}
.status{text-align:center;font-size:.8rem;margin-top:.75rem;color:#4a5568;min-height:1.2rem;}
.status.ok{color:#4ade80;}.status.err{color:#f87171;}
.back{display:block;text-align:center;margin-top:1.2rem;font-size:.75rem;color:#4a5568;text-decoration:none;}
.back:hover{color:#a78bfa;}
</style></head><body>
<div class="card">
  <h2> Firmware Update</h2>
  <div id="cur-ver" style="font-size:.75rem;color:#4a5568;margin-bottom:.5rem;">Aktuelle Version wird geladen...</div>
  <p>Kompiliere in Arduino IDE: <b>Sketch  Export Compiled Binary</b><br>
  Dann die <b>.bin</b> Datei hier hochladen. ESP32 flasht sich selbst neu.</p>
  <div class="drop" id="drop" onclick="document.getElementById('file').click()"
    ondragover="event.preventDefault();this.classList.add('over')"
    ondragleave="this.classList.remove('over')"
    ondrop="event.preventDefault();this.classList.remove('over');handleFile(event.dataTransfer.files[0])">
    <input type="file" id="file" accept=".bin" onchange="handleFile(this.files[0])">
    <div class="drop-icon"></div>
    <div class="drop-text">Datei hierher ziehen oder tippen zum Auswhlen</div>
    <div class="drop-name" id="fname"></div>
  </div>
  <button class="btn" id="btn" onclick="upload()" disabled>Firmware hochladen</button>
  <div class="progress" id="prog">
    <div class="bar-bg"><div class="bar" id="bar"></div></div>
    <div class="status" id="status">Uploading...</div>
  </div>
  <a href="/" class="back">&larr; Zurueck zum Controller</a>
</div>
<script>
let f=null;
fetch('/version').then(r=>r.json()).then(d=>{
  document.getElementById('cur-ver').textContent='Aktuell: v'+d.version+' vom '+d.date;
  document.getElementById('cur-ver').style.color='#a78bfa';
}).catch(()=>{});
function handleFile(file){
  if(!file||!file.name.endsWith('.bin')){alert('Bitte eine .bin Datei whlen!');return;}
  f=file;
  document.getElementById('fname').textContent=file.name+' ('+Math.round(file.size/1024)+' KB)';
  document.getElementById('btn').disabled=false;
}
function upload(){
  if(!f)return;
  const fd=new FormData();
  fd.append('firmware',f,f.name);
  const xhr=new XMLHttpRequest();
  xhr.open('POST','/update');
  document.getElementById('prog').style.display='block';
  document.getElementById('btn').disabled=true;
  xhr.upload.onprogress=e=>{
    const p=Math.round(e.loaded/e.total*100);
    document.getElementById('bar').style.width=p+'%';
    document.getElementById('status').textContent='Hochladen '+p+'%';
  };
  xhr.onload=()=>{
    if(xhr.status===200){
      document.getElementById('status').textContent=' Fertig! ESP32 startet neu';
      document.getElementById('status').className='status ok';
    } else {
      document.getElementById('status').textContent=' Fehler: '+xhr.responseText;
      document.getElementById('status').className='status err';
      document.getElementById('btn').disabled=false;
    }
  };
  xhr.onerror=()=>{
    const pct=document.getElementById('bar').style.width;
    if(pct==='100%'){
      document.getElementById('status').textContent='Fertig! ESP32 startet neu...';
      document.getElementById('status').className='status ok';
      setTimeout(()=>{
        document.getElementById('status').textContent='Verbinde neu...';
        setTimeout(()=>{ window.location.href='/'; }, 4000);
      }, 1500);
    } else {
      document.getElementById('status').textContent='X Verbindungsfehler';
      document.getElementById('status').className='status err';
      document.getElementById('btn').disabled=false;
    }
  };
  xhr.send(fd);
}
</script>
</body></html>)HTML";
  server.send(200, "text/html", html);
}

void handleOTAUpdate() {
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) {
      Update.abort();
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    Update.write(upload.buf, upload.currentSize);
  } else if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true)) {
      server.send(200, "text/plain", "OK");
      delay(1000);
      ESP.restart();
    } else {
      server.send(500, "text/plain", Update.errorString());
    }
  } else if (upload.status == UPLOAD_FILE_ABORTED) {
    Update.abort();
    server.send(500, "text/plain", "Aborted");
  }
}

void handleConfig(){
  String body=server.arg("plain");
  String ssid=jsonStr(body,"ssid"),pass=jsonStr(body,"password");
  if(ssid.length()==0){server.send(400,"text/plain","SSID fehlt");return;}
  prefs.begin("wifi",false);prefs.putString("ssid",ssid);prefs.putString("pass",pass);prefs.end();
  server.send(200,"text/plain","OK");delay(500);ESP.restart();
}

void setup(){
  Serial.begin(115200);
  Keyboard.begin();
  USB.begin();
  delay(2000); // USB verbinden lassen
  // CAPS LOCK in bekannten Zustand bringen:
  // 2x drcken = net zero nderung, danach tracken wir selbst
  // Nutzer sollte CAPS vor dem Einstecken ausschalten
  // Wir setzen unseren internen State auf false = AUS
  g_capsState = false;
  Serial.println("\n=== ESP32 Keyboard Controller (DE-Layout) ===");
  prefs.begin("wifi",true);savedSSID=prefs.getString("ssid","");savedPass=prefs.getString("pass","");prefs.end();
  if(savedSSID.length()>0){
    Serial.printf("Verbinde: %s\n",savedSSID.c_str());
    WiFi.mode(WIFI_STA);WiFi.begin(savedSSID.c_str(),savedPass.c_str());
    int t=0;while(WiFi.status()!=WL_CONNECTED&&t<30){delay(500);Serial.print(".");t++;}Serial.println();
    if(WiFi.status()==WL_CONNECTED){wifiConnected=true;Serial.println("OK IP: "+WiFi.localIP().toString());MDNS.begin("esp32-keyboard");Serial.println("OK http://esp32-keyboard.local/");}
    else Serial.println("ERR -> Hotspot");
  }
  if(!wifiConnected){WiFi.mode(WIFI_AP);WiFi.softAP("ESP32-Keyboard","12345678");Serial.println("Hotspot: ESP32-Keyboard / 12345678 -> http://192.168.4.1/");}
  server.on("/",handleRoot);
  server.on("/version",handleVersion);
  server.on("/templates",         handleGetTemplates);
  server.on("/autostart",         handleGetAutostart);
  server.on("/autostart/save", HTTP_POST, handleSaveAutostart);
  server.on("/templates/save", HTTP_POST, handleSaveTemplates);
  server.on("/wifiinfo",handleWifiInfo);
  server.on("/run",HTTP_POST,handleRun);
  server.on("/stop",HTTP_POST,handleStop);
  server.on("/status",handleStatus);
  server.on("/sendkey",HTTP_POST,handleSendKey);
  server.on("/config",HTTP_POST,handleConfig);
  server.on("/ota", handleOTAPage);
  server.on("/update", HTTP_POST,
    [](){}, // onComplete leer - wird in handleOTAUpdate behandelt
    handleOTAUpdate
  );
  server.begin();Serial.println("Bereit!");

  // 1. Autostart Config laden
  prefs.begin("autostart", true);
  String autoCfg = prefs.getString("cfg", "{}");
  prefs.end();
  g_autoEnabled = autoCfg.indexOf("\"enabled\":true") >= 0;
  if(g_autoEnabled){
    int di=autoCfg.indexOf("\"delay\":"); if(di>=0){di+=8;g_autoDelay=autoCfg.substring(di,autoCfg.indexOf(",",di)<0?autoCfg.indexOf("}",di):autoCfg.indexOf(",",di)).toInt();}
    int ri=autoCfg.indexOf("\"repeat\":"); if(ri>=0){ri+=9;g_autoRepeat=autoCfg.substring(ri,autoCfg.indexOf(",",ri)<0?autoCfg.indexOf("}",ri):autoCfg.indexOf(",",ri)).toInt();}
    int si=autoCfg.indexOf("\"speed\":"); if(si>=0){si+=8;g_autoSpeed=autoCfg.substring(si,autoCfg.indexOf(",",si)<0?autoCfg.indexOf("}",si):autoCfg.indexOf(",",si)).toInt();}
    int bi=autoCfg.indexOf("\"burst\":"); if(bi>=0){bi+=8;g_autoBurst=autoCfg.substring(bi,autoCfg.indexOf(",",bi)<0?autoCfg.indexOf("}",bi):autoCfg.indexOf(",",bi)).toInt();}
    int ti=autoCfg.indexOf("\"tmplId\":\"");
    if(ti>=0){ ti+=10; g_autoTmplId=autoCfg.substring(ti,autoCfg.indexOf("\"",ti)); }
    else { ti=autoCfg.indexOf("\"tmplId\":"); if(ti>=0){ ti+=9; int te=autoCfg.indexOf(",",ti); if(te<0)te=autoCfg.indexOf("}",ti); g_autoTmplId=autoCfg.substring(ti,te); g_autoTmplId.trim(); } }
    Serial.printf("Autostart aktiv: delay=%dms repeat=%d tmpl=%s\n",g_autoDelay,g_autoRepeat,g_autoTmplId.c_str());
  }

  // 2. Autostart-Timer setzen (NACH dem Laden der Config!)
  if(g_autoEnabled && g_autoTmplId.length()>0){
    g_autoStartTime = millis();
    Serial.printf("Autostart Timer gestartet: %dms Verzoegerung\n", g_autoDelay);
  }
}

// Autostart: Vorlage aus Preferences laden und ausfhren
void loadAndRunAutoTemplate(){
  prefs.begin("templates", true);
  String data = prefs.getString("data", "[]");
  prefs.end();
  if(data.length()<3) return;

  // Template mit passender ID suchen
  String searchId = "\"id\":"+g_autoTmplId;
  int tStart = data.indexOf(searchId);
  if(tStart<0){ Serial.println("Autostart: Template nicht gefunden!"); return; }

  // Steps aus Template extrahieren
  int stepsStart = data.indexOf("\"steps\":[", tStart);
  if(stepsStart<0) return;
  stepsStart += 9;

  g_stepCount=0;
  g_typeDelay=g_autoSpeed;
  g_burstSize=g_autoBurst;

  int pos=stepsStart;
  while(pos<(int)data.length()&&g_stepCount<MAX_STEPS){
    int os=data.indexOf("{",pos); if(os<0) break;
    // Finde zugehriges schlieendes }
    int oe=data.indexOf("}",os); if(oe<0) break;
    // Check ob wir noch im steps array sind
    if(data.indexOf("]",pos) < oe && data.indexOf("]",pos) < data.indexOf("{",pos+1)) break;
    String obj=data.substring(os,oe+1);
    String type=jsonStr(obj,"type");
    String value=jsonStr(obj,"value");
    if(type.length()==0) break;
    g_steps[g_stepCount].type=type;
    g_steps[g_stepCount].value=value;
    int spIdx=obj.indexOf("\"speed\":"); if(spIdx>=0){spIdx+=8;int se=obj.indexOf(",",spIdx);if(se<0)se=obj.indexOf("}",spIdx);g_steps[g_stepCount].stepSpeed=obj.substring(spIdx,se).toInt();}
    else g_steps[g_stepCount].stepSpeed=-1;
    g_stepCount++;
    pos=oe+1;
  }

  if(g_stepCount==0){ Serial.println("Autostart: Keine Schritte!"); return; }

  g_running=true;
  g_runCount=0;
  g_looping=g_autoRepeat==0;
  g_loopInterval=1000;
  g_nextRun=0;
  Serial.printf("Autostart: %d Schritte werden ausgefuehrt\n", g_stepCount);
}

void loop(){
  server.handleClient();

  // Autostart: wird genau 1x pro Boot ausgefuehrt
  // g_autoStartTime wird beim Boot gesetzt und nie wieder veraendert
  // g_autoStarted bleibt true bis zum naechsten Neustart
  if(g_autoEnabled && !g_autoStarted && !g_running && g_autoStartTime > 0){
    if(millis()-g_autoStartTime >= (unsigned long)g_autoDelay){
      g_autoStarted=true;
      Serial.println("Autostart wird ausgefuehrt...");
      loadAndRunAutoTemplate();
      if(g_autoRepeat>0) g_looping=false;
    }
  }

  if(g_running){
    unsigned long now=millis();
    if(now>=g_nextRun){
      executeSequence();
      if(g_looping&&g_running){
        g_nextRun=millis()+g_loopInterval;
      } else if(!g_looping && g_autoRepeat>1 && g_runCount<g_autoRepeat && g_autoStarted){
        // Mehrfach wiederholen
        g_nextRun=millis()+500;
      } else {
        g_running=false;
        Serial.println("Fertig.");
      }
    }
  }
}
