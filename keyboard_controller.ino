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
#include <LittleFS.h>
#include <USB.h>
#include <USBHIDKeyboard.h>
#include <Update.h>

// -- Version ---------------------------------------------------
#define FW_VERSION "1.4.1"
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
int    g_burstSize    = 1;    // Standard: jedes Zeichen einzeln; Burst nur explizit
bool   g_clipMode     = false; // Clipboard-Modus via PowerShell
int    g_clipCustomDelay = 20; // Default: sicherer Wert; 0 = maximal schnell
bool   g_capsState    = false; // Unser eigener CAPS-Zustand tracker

// -- Autostart --
bool   g_autoEnabled  = false;
bool   g_autoStarted  = false;
unsigned long g_autoStartTime = 0;
int    g_autoDelay    = 5000;  // ms nach Boot warten
int    g_autoRepeat   = 1;     // 0 = endlos
int    g_autoSpeed    = 20;
int    g_autoBurst    = 1;
String g_autoTmplId   = "";    // Template ID als String

#define MAX_STEPS 128
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

// Sende rohen HID-Keycode. Die Inter-Character-Verzögerung wird zentral in typeText() gesteuert.
// Dadurch bedeutet "20 ms" tatsächlich ungefähr 20 ms zwischen Zeichen und nicht zusätzlich 40+ ms.
void sendRaw(uint8_t modifier, uint8_t keycode) {
  uint8_t report[8] = {modifier, 0, keycode, 0, 0, 0, 0, 0};
  Keyboard.sendReport((KeyReport*)report);
  // Kurze, feste Haltezeit nur für USB/HID-Stabilität; nicht als Tippgeschwindigkeit zählen.
  delay(2);
  memset(report, 0, 8);
  Keyboard.sendReport((KeyReport*)report);
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

// Clipboard-Modus: Text via PowerShell in Zwischenablage, dann Ctrl+V.
// Optimiert: nur die notwendigen Windows-Wartezeiten; die Eingabegeschwindigkeit
// des PowerShell-Befehls ist separat einstellbar. 0 ms = so schnell wie USB/HID erlaubt.
void typeTextClipboard(String text) {
  Serial.println("Clipboard-Modus: starte...");

  // Wir halten den bisherigen sicheren Caps-Ansatz bei, damit der Clipboard-Befehl
  // nicht durch einen vom Sketch bekannten Caps-Lock-Zustand verfälscht wird.
  if (g_capsState) {
    sendRaw(0, KEY_CAPS_LOCK);
    g_capsState = false;
    delay(20);
  }

  uint8_t rel[8] = {0,0,0,0,0,0,0,0};
  uint8_t winR[8] = {0x08, 0, 0x15, 0, 0, 0, 0, 0};
  Keyboard.sendReport((KeyReport*)winR);
  delay(20);
  Keyboard.sendReport((KeyReport*)rel);
  delay(250); // Run-Dialog: deutlich kürzer als vorher, aber ausreichend für Windows

  bool savedClip = g_clipMode;
  int savedDelay = g_typeDelay;
  g_clipMode = false;
  int clipboardDelay = (g_clipCustomDelay >= 0) ? constrain(g_clipCustomDelay, 0, 500) : 20;
  g_typeDelay = clipboardDelay;

  typeText("powershell -NoProfile -Command Set-Clipboard -Value '");
  typeText(escapePSText(text));
  typeText("'");

  g_typeDelay = savedDelay;
  g_clipMode = savedClip;

  // Kleine Sicherheitsreserve vor Enter; bei 0 ms trotzdem kurz warten.
  delay(max(20, min(180, clipboardDelay + 20)));
  sendRaw(0, KEY_ENTER_R);
  delay(250); // PowerShell/Clipboard setzen

  // Ctrl+V als korrektes deutsches HID-Report
  uint8_t ctrlV[8] = {0x01, 0, KEY_V, 0, 0, 0, 0, 0};
  Keyboard.sendReport((KeyReport*)ctrlV);
  delay(2);
  Keyboard.sendReport((KeyReport*)rel);
  Serial.println("Clipboard-Modus: fertig!");
}

void typeText(String text) {
  text.replace("\\n", "\n");
  text.replace("\\t", "\t");

  if (g_clipMode) {
    typeTextClipboard(text);
    return;
  }

  // Unser eigener Caps-Lock-Tracker darf niemals einen Textlauf verfälschen.
  if (g_capsState) {
    sendRaw(0, KEY_CAPS_LOCK);
    g_capsState = false;
    delay(20);
  }

  int i = 0;
  int charsSinceYield = 0;
  while (i < (int)text.length() && g_running) {
    server.handleClient();
    if (!g_running) break;

    // Burst zählt Zeichen, nicht UTF-8-Bytes.
    int sent = 0;
    while (sent < max(1, g_burstSize) && i < (int)text.length() && g_running) {
      uint8_t b = (uint8_t)text[i];
      if (b == 0xC3 && i + 1 < (int)text.length()) {
        typeUmlaut(b, (uint8_t)text[i+1]);
        i += 2;
      } else {
        typeCharDE((char)b);
        i++;
      }
      sent++;
      charsSinceYield++;

    }

    // Zwischen einzelnen Bursts ebenfalls die eingestellte Verzögerung.
    if (g_typeDelay > 0 && i < (int)text.length()) delay(g_typeDelay);

    // Auch bei 0 ms regelmäßig dem Webserver Zeit geben, damit Stop sofort reagiert.
    if (charsSinceYield >= 24) {
      server.handleClient();
      charsSinceYield = 0;
      if (g_typeDelay == 0) delay(1);
    }
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

// Liefert HID-Keycode fuer deutsche UTF-8-Umlaute. shift wird bei A/O/U gesetzt.
uint8_t getDEUtf8Key(String s, bool &shift) {
  shift = false;
  if (s.length() != 2 || (uint8_t)s[0] != 0xC3) return 0;
  switch ((uint8_t)s[1]) {
    case 0xA4: return KEY_QUOTE;                    // ä
    case 0x84: shift = true; return KEY_QUOTE;      // Ä
    case 0xB6: return KEY_SCOLON;                   // ö
    case 0x96: shift = true; return KEY_SCOLON;     // Ö
    case 0xBC: return KEY_LBRACE;                   // ü
    case 0x9C: shift = true; return KEY_LBRACE;     // Ü
    case 0x9F: return KEY_MINUS;                    // ß
    default: return 0;
  }
}

// Liefert ein deutsches HID-Keycode/Modifier-Paar für ein einzelnes sichtbares Zeichen.
bool resolveDEChar(String token, uint8_t &modifier, uint8_t &keycode) {
  modifier = 0; keycode = 0;
  if (token.length() == 1) {
    char c = token[0];
    const char *lo = "abcdefghijklmnopqrstuvwxyz";
    const uint8_t codes[] = {KEY_A,KEY_B,KEY_C,KEY_D,KEY_E,KEY_F,KEY_G,KEY_H,KEY_I,KEY_J,KEY_K,KEY_L,KEY_M,KEY_N,KEY_O,KEY_P,KEY_Q,KEY_R,KEY_S,KEY_T,KEY_U,KEY_V,KEY_W,KEY_X,KEY_Z,KEY_Y};
    if (c >= 'a' && c <= 'z') { keycode = codes[c-'a']; return true; }
    if (c >= 'A' && c <= 'Z') { keycode = codes[c-'A']; modifier=MOD_LSHIFT; return true; }
    switch(c) {
      case '0':keycode=KEY_0;return true; case '1':keycode=KEY_1;return true; case '2':keycode=KEY_2;return true; case '3':keycode=KEY_3;return true; case '4':keycode=KEY_4;return true;
      case '5':keycode=KEY_5;return true; case '6':keycode=KEY_6;return true; case '7':keycode=KEY_7;return true; case '8':keycode=KEY_8;return true; case '9':keycode=KEY_9;return true;
      case ' ':keycode=KEY_SPACE_R;return true; case '-':keycode=KEY_SLASH;return true; case '.':keycode=KEY_DOT;return true; case ',':keycode=KEY_COMMA;return true;
      case '+':keycode=KEY_RBRACE;return true; case '#':keycode=KEY_HASH;return true; case '<':keycode=KEY_NONEUSB;return true;
      case '!':modifier=MOD_LSHIFT;keycode=KEY_1;return true; case '"':modifier=MOD_LSHIFT;keycode=KEY_2;return true; case '$':modifier=MOD_LSHIFT;keycode=KEY_4;return true; case '%':modifier=MOD_LSHIFT;keycode=KEY_5;return true;
      case '&':modifier=MOD_LSHIFT;keycode=KEY_6;return true; case '/':modifier=MOD_LSHIFT;keycode=KEY_7;return true; case '(':modifier=MOD_LSHIFT;keycode=KEY_8;return true; case ')':modifier=MOD_LSHIFT;keycode=KEY_9;return true; case '=':modifier=MOD_LSHIFT;keycode=KEY_0;return true;
      case '?':modifier=MOD_LSHIFT;keycode=KEY_MINUS;return true; case '*':modifier=MOD_LSHIFT;keycode=KEY_RBRACE;return true; case '\'':modifier=MOD_LSHIFT;keycode=KEY_HASH;return true;
      case ';':modifier=MOD_LSHIFT;keycode=KEY_COMMA;return true; case ':':modifier=MOD_LSHIFT;keycode=KEY_DOT;return true; case '_':modifier=MOD_LSHIFT;keycode=KEY_SLASH;return true; case '>':modifier=MOD_LSHIFT;keycode=KEY_NONEUSB;return true;
      case '@':modifier=MOD_RALT;keycode=KEY_Q;return true; case '{':modifier=MOD_RALT;keycode=KEY_7;return true; case '[':modifier=MOD_RALT;keycode=KEY_8;return true; case ']':modifier=MOD_RALT;keycode=KEY_9;return true; case '}':modifier=MOD_RALT;keycode=KEY_0;return true; case '\\':modifier=MOD_RALT;keycode=KEY_MINUS;return true; case '~':modifier=MOD_RALT;keycode=KEY_RBRACE;return true; case '|':modifier=MOD_RALT;keycode=KEY_NONEUSB;return true;
    }
  }
  bool shift=false; uint8_t uk=getDEUtf8Key(token,shift);
  if(uk){ keycode=uk; modifier=shift?MOD_LSHIFT:0; return true; }
  return false;
}

bool resolveSpecial(String token, uint8_t &keycode) {
  keycode = getSpecialKey(token);
  return keycode != 0;
}

// Rohe HID Usage IDs fuer Sondertasten in Kombinationen.
// Wichtig: Arduino KEY_* Konstanten sind fuer Keyboard.press() gedacht und
// duerfen nicht ungeprueft als Usage ID in einen Raw-HID-Report geschrieben werden.
bool resolveSpecialRaw(String token, uint8_t &keycode) {
  String u=token; u.trim(); u.toUpperCase();
  keycode=0;
  if(u=="ESC") keycode=0x29;
  else if(u=="F1") keycode=0x3A; else if(u=="F2") keycode=0x3B; else if(u=="F3") keycode=0x3C; else if(u=="F4") keycode=0x3D;
  else if(u=="F5") keycode=0x3E; else if(u=="F6") keycode=0x3F; else if(u=="F7") keycode=0x40; else if(u=="F8") keycode=0x41;
  else if(u=="F9") keycode=0x42; else if(u=="F10") keycode=0x43; else if(u=="F11") keycode=0x44; else if(u=="F12") keycode=0x45;
  else if(u=="PRINT") keycode=0x46; else if(u=="SCROLL") keycode=0x47; else if(u=="PAUSE") keycode=0x48;
  else if(u=="INSERT") keycode=0x49; else if(u=="HOME") keycode=0x4A; else if(u=="PAGEUP") keycode=0x4B;
  else if(u=="DELETE") keycode=0x4C; else if(u=="END") keycode=0x4D; else if(u=="PAGEDOWN") keycode=0x4E;
  else if(u=="RIGHT") keycode=0x4F; else if(u=="LEFT") keycode=0x50; else if(u=="DOWN") keycode=0x51; else if(u=="UP") keycode=0x52;
  else if(u=="NUMLOCK") keycode=0x53; else if(u=="CAPS") keycode=0x39;
  else if(u=="ENTER") keycode=0x28; else if(u=="BACKSPACE") keycode=0x2A; else if(u=="TAB") keycode=0x2B; else if(u=="SPACE") keycode=0x2C;
  return keycode!=0;
}

void sendCombo(String combo) {
  combo.trim();
  if (!combo.length()) return;

  // Ein einzelnes "+" ist eine echte Taste und kein Trenner.
  if (combo == "+") {
    uint8_t m=0,k=0;
    if (resolveDEChar(combo,m,k)) sendRaw(m,k);
    return;
  }

  String parts[8]; int count=0, start=0;
  for (int i=0; i<=(int)combo.length() && count<8; i++) {
    if (i==(int)combo.length() || combo[i]=='+') {
      String part=combo.substring(start,i); part.trim();
      // Doppeltes bzw. abschliessendes ++ steht fuer die Plus-Taste,
      // z.B. LCTRL++ => Ctrl + Plus.
      if (!part.length() && i==(int)combo.length() && i>0 && combo[i-1]=='+') part="+";
      if (part.length()) parts[count++]=part;
      start=i+1;
    }
  }
  if (!count) return;

  uint8_t modifier=0, keycode=0;
  if (count==1) {
    if (resolveSpecial(parts[0], keycode)) {
      if (keycode==KEY_CAPS_LOCK) g_capsState=!g_capsState;
      Keyboard.press(keycode); delay(2); Keyboard.release(keycode);
      return;
    }
    if (resolveDEChar(parts[0], modifier, keycode)) { sendRaw(modifier,keycode); return; }
    return;
  }

  // Kombination: Modifier sammeln und die eigentliche Taste als echte HID Usage ID aufloesen.
  uint8_t mods=0; uint8_t finalKey=0; bool finalResolved=false;
  for (int i=0;i<count;i++) {
    String p=parts[i]; p.trim(); String u=p; u.toUpperCase();
    if (u=="LCTRL" || u=="CTRL") { mods |= 0x01; continue; }
    if (u=="RCTRL") { mods |= 0x10; continue; }
    if (u=="LSHIFT" || u=="SHIFT") { mods |= 0x02; continue; }
    if (u=="RSHIFT") { mods |= 0x20; continue; }
    if (u=="LALT" || u=="ALT") { mods |= 0x04; continue; }
    if (u=="RALT" || u=="ALTGR") { mods |= 0x40; continue; }
    if (u=="LGUI" || u=="WIN") { mods |= 0x08; continue; }
    if (u=="RGUI") { mods |= 0x80; continue; }
    uint8_t sm=0, sk=0;
    if (resolveSpecialRaw(p, sk)) { finalKey=sk; finalResolved=true; }
    else if (resolveDEChar(p, sm, sk)) { mods |= sm; finalKey=sk; finalResolved=true; }
  }
  if (!finalResolved) return;

  uint8_t report[8]={mods,0,finalKey,0,0,0,0,0};
  Keyboard.sendReport((KeyReport*)report);
  delay(3);
  memset(report,0,8);
  Keyboard.sendReport((KeyReport*)report);
}

void executeSequence() {
  for (int i=0; i<g_stepCount&&g_running; i++) {
    server.handleClient(); // Stop-Befehl empfangen
    if (!g_running) break; // sofort abbrechen
    if (g_steps[i].type=="text") {
      // Per-Schritt Geschwindigkeit
      // -1 = global, -2 = Blitz (0ms), >= 0 = expliziter ms-Wert.
      // Blitz veraendert niemals den Burst-Modus: ohne expliziten Burst bleibt es 1 Zeichen.
      int savedDelay = g_typeDelay;
      if (g_steps[i].stepSpeed == -2) {
        g_typeDelay = 0;
      } else if (g_steps[i].stepSpeed >= 0) {
        g_typeDelay = g_steps[i].stepSpeed;
      }
      typeText(g_steps[i].value);
      g_typeDelay = savedDelay;
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
.key-grid{display:flex;flex-wrap:wrap;gap:.38rem;}
.key{padding:.58rem .78rem;background:var(--bg);border:1px solid var(--border);border-radius:8px;font-family:'JetBrains Mono',monospace;font-size:.74rem;font-weight:600;color:var(--text);cursor:pointer;user-select:none;touch-action:manipulation;-webkit-appearance:none;appearance:none;line-height:1.15;transition:background .08s,border-color .08s,color .08s,transform .05s;}
.key:hover{border-color:#3b3b58;background:#171722;}
.key:active{background:var(--accent);border-color:var(--accent);color:#fff;transform:scale(.96);}
.key.mod{color:var(--accent2);border-color:#343052;}
.key-section-title{font-size:.72rem;font-weight:700;letter-spacing:.06em;text-transform:uppercase;color:var(--text);margin-bottom:.55rem;}
.virtual-kbd{margin-top:.9rem;touch-action:manipulation;user-select:none;display:flex;flex-direction:column;gap:.7rem;}
.vk-zone{background:var(--bg);border:1px solid var(--border);border-radius:16px;padding:.65rem;display:flex;flex-direction:column;gap:.42rem;overflow:hidden;}
.vk-zone-head{display:flex;align-items:center;justify-content:space-between;gap:.5rem;margin-bottom:.05rem;}
.vk-zone-title{font-size:.66rem;font-weight:700;color:var(--muted);letter-spacing:.08em;text-transform:uppercase;}
.vk-row{display:grid;gap:.34rem;width:100%;}
.vk-row.r10{grid-template-columns:repeat(10,minmax(0,1fr));}.vk-row.r9{grid-template-columns:repeat(9,minmax(0,1fr));}.vk-row.r8{grid-template-columns:repeat(8,minmax(0,1fr));}.vk-row.letters-bottom{grid-template-columns:1.2fr repeat(7,minmax(0,1fr)) 1.2fr;}
.vk-row.actions{grid-template-columns:1.25fr 1fr 1fr 4fr 1.35fr;}.vk-row.umlaut{grid-template-columns:repeat(4,minmax(0,1fr));}
.vk-row.pc4{grid-template-columns:repeat(4,minmax(0,1fr));}.vk-row.pc6{grid-template-columns:repeat(6,minmax(0,1fr));}
.vk-key{min-width:0;height:56px;padding:0 .1rem;background:linear-gradient(180deg,#1b1b25,#111118);border:1px solid #2a2a3b;border-radius:12px;color:var(--text);font-family:Inter,sans-serif;font-size:.93rem;font-weight:700;box-shadow:0 2px 0 rgba(0,0,0,.28),inset 0 1px 0 rgba(255,255,255,.035);cursor:pointer;touch-action:manipulation;display:flex;align-items:center;justify-content:center;transition:transform .05s,border-color .08s,background .08s;}
.vk-key:active,.vk-key.active{background:var(--accent);border-color:var(--accent);color:#fff;transform:translateY(1px);box-shadow:none;}
.vk-key.mod{color:var(--accent2);font-size:.75rem}.vk-key.action{background:#181823}.vk-key.space{font-size:.75rem;letter-spacing:.02em}.vk-key.danger{color:#fca5a5}.vk-key.secondary{height:48px;font-size:.74rem;font-weight:600;}
.vk-toolbar{display:grid;grid-template-columns:1fr 1fr;gap:.45rem;}
.vk-switch{height:42px;border:1px solid var(--border);border-radius:10px;background:var(--bg);color:var(--muted);font-weight:700;font-size:.72rem;cursor:pointer;touch-action:manipulation;}.vk-switch.active{color:#fff;background:#242438;border-color:#3a3a58;}
.vk-pane{display:none;flex-direction:column;gap:.42rem}.vk-pane.active{display:flex;}
.vk-mod-status{display:none;align-items:center;gap:.45rem;padding:.3rem .6rem;border:1px solid var(--border);border-radius:999px;color:var(--accent2);font-size:.68rem;}
.vk-mod-status .pulse{width:7px;height:7px;border-radius:50%;background:var(--green);animation:blink .8s infinite;}
@media(max-width:600px){.card.keyboard-card{padding:.85rem}.virtual-kbd{margin-left:-.1rem;margin-right:-.1rem}.vk-zone{padding:.52rem;border-radius:14px}.vk-row{gap:.28rem}.vk-key{height:54px;border-radius:11px;font-size:.9rem}.vk-key.secondary{height:46px;font-size:.7rem}}
@media(max-width:600px){.tmpl-item{padding:.75rem}.tmpl-item>div:first-child{flex-wrap:wrap;align-items:flex-start!important}.tmpl-info{flex:1 1 calc(100% - 2.2rem)!important;min-width:0!important}.tmpl-name{font-size:.9rem;white-space:normal;overflow:visible;text-overflow:clip;word-break:break-word;line-height:1.3}.tmpl-meta{margin-top:.25rem}.tmpl-actions{width:100%;display:grid;grid-template-columns:1.25fr 1.25fr .75fr .75fr;gap:.35rem;margin-top:.15rem}.tmpl-btn{width:100%;padding:.48rem .35rem;min-height:38px}.tmpl-icon{margin-top:.1rem}}
@media(max-width:390px){.card.keyboard-card{padding:.7rem}.vk-zone{padding:.45rem}.vk-row{gap:.22rem}.vk-key{height:51px;font-size:.84rem;border-radius:10px}.vk-key.mod{font-size:.66rem}.vk-key.secondary{height:43px;font-size:.64rem}.vk-row.actions{grid-template-columns:1.2fr .9fr .9fr 3.4fr 1.25fr}}

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
    <div class="tab" onclick="switchTab('ducky')">&#129414; DuckyScript 3</div>
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
      <div style="display:flex;align-items:center;gap:.55rem;flex-wrap:wrap;margin-top:.65rem;padding-top:.65rem;border-top:1px solid var(--border);">
        <input id="ino-file" type="file" accept=".ino,.cpp,.h,text/plain" style="display:none" onchange="importSequenceFile(this.files&&this.files[0])">
        <button type="button" class="btn btn-ghost btn-sm" onclick="document.getElementById('ino-file').click()">&#128194; .INO importieren</button>
        <span id="ino-import-info" style="font-size:.7rem;color:var(--muted);line-height:1.35;flex:1;min-width:180px;">Arduino-.INO importieren. DuckyScript 3 hat jetzt einen eigenen Menuepunkt.</span>
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
              <div class="stepper-val" id="burst-val">1 Zeichen</div>
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
          <div id="clip-speed-block" style="display:none;margin-top:.7rem;">
            <div style="display:flex;align-items:center;justify-content:space-between;gap:.6rem;">
              <div>
                <div style="font-size:.7rem;color:var(--text);font-weight:600;">Eigene Copy+Paste-Geschwindigkeit</div>
                <div id="clip-speed-info" style="font-size:.64rem;color:var(--muted);margin-top:.2rem;">20 ms / Zeichen</div>
              </div>
              <div style="display:flex;align-items:center;gap:.35rem;flex-shrink:0;">
                <button type="button" class="stepper-btn" onclick="changeClipSpeed(-5)">&#8722;</button>
                <input type="number" id="clip-custom-speed-val" value="20" min="0" max="500"
                  style="width:78px;text-align:center;background:var(--bg);border:1px solid var(--border);border-radius:7px;color:var(--accent2);font-family:monospace;font-size:.85rem;padding:.38rem;outline:none;"
                  oninput="setClipCustomSpeed(parseInt(this.value)||0)" onfocus="this.select()">
                <button type="button" class="stepper-btn" onclick="changeClipSpeed(5)">&#43;</button>
              </div>
            </div>
            <div style="font-size:.62rem;color:var(--muted);margin-top:.35rem;text-align:center;">0 = maximal schnell &middot; gilt nur für Copy+Paste</div>
          </div>
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
      <div class="s-right">Läufe: <span id="s-count">0</span></div>
    </div>
  </div>

  <div class="panel" id="tab-ducky">
    <div class="card">
      <div style="display:flex;align-items:center;justify-content:space-between;gap:.7rem;flex-wrap:wrap;">
        <div><lbl>DuckyScript Import</lbl><div style="font-size:.7rem;color:var(--muted);">.txt laden und direkt in normale Controller-Schritte umwandeln.</div></div>
        <div style="display:flex;gap:.45rem;flex-wrap:wrap;">
          <input id="ducky-file" type="file" accept=".txt,text/plain" style="display:none" onchange="loadDuckyFile(this.files&&this.files[0])">
          <button class="btn btn-accent btn-sm" onclick="document.getElementById('ducky-file').click()">&#128194; .TXT laden</button>
          <button class="btn btn-ghost btn-sm" onclick="clearDuckyEditor()">Leeren</button>
        </div>
      </div>

      <textarea id="ducky-source" spellcheck="false" placeholder="DuckyScript hier einfuegen oder .txt laden..." oninput="scheduleDuckyAnalyze()" style="width:100%;min-height:290px;margin-top:.75rem;background:var(--bg);border:1px solid var(--border);border-radius:10px;color:var(--text);font-family:'JetBrains Mono',monospace;font-size:.76rem;line-height:1.45;padding:.8rem;outline:none;resize:vertical;"></textarea>

      <div style="display:flex;gap:.5rem;align-items:center;flex-wrap:wrap;margin-top:.65rem;">
        <select id="ducky-target-os" onchange="analyzeDuckyEditor()" title="Zielsystem fuer statisch auswertbare OS-Bedingungen" style="background:var(--bg);border:1px solid var(--border);border-radius:9px;color:var(--text);font-size:.75rem;padding:.58rem .65rem;outline:none;">
          <option value="WINDOWS">Ziel: Windows</option><option value="MACOS">Ziel: macOS</option><option value="LINUX">Ziel: Linux</option><option value="CHROMEOS">Ziel: ChromeOS</option><option value="ANDROID">Ziel: Android</option><option value="IOS">Ziel: iOS</option><option value="OTHER">Ziel: Sonstige</option>
        </select>
        <select id="ducky-compat-mode" onchange="analyzeDuckyEditor()" title="Automatisch passt nicht darstellbare Hardwarefunktionen an oder entfernt sie; Strikt meldet sie als Fehler; Roh behaelt einen Teilimport mit Warnungen." style="background:var(--bg);border:1px solid var(--border);border-radius:9px;color:var(--text);font-size:.75rem;padding:.58rem .65rem;outline:none;">
          <option value="auto" selected>Kompatibilitaet: Automatisch</option><option value="strict">Kompatibilitaet: Strikt</option><option value="raw">Kompatibilitaet: Roh/Teilimport</option>
        </select>
        <label title="Ersatzpause fuer WAIT_FOR_BUTTON_PRESS im Automatikmodus" style="display:flex;align-items:center;gap:.35rem;font-size:.7rem;color:var(--muted);">Button-Wartezeit <input id="ducky-button-wait" onchange="analyzeDuckyEditor()" type="number" min="0" max="60000" step="100" value="1000" style="width:82px;background:var(--bg);border:1px solid var(--border);border-radius:9px;color:var(--text);font-size:.75rem;padding:.55rem .6rem;outline:none;"> ms</label>
      </div>

      <div id="ducky-direct-tools" style="display:none;margin-top:.7rem;padding:.7rem;border:1px solid var(--border);border-radius:9px;background:var(--bg2);">
        <div style="display:flex;align-items:center;justify-content:space-between;gap:.6rem;flex-wrap:wrap;">
          <div><b id="ducky-direct-title" style="font-size:.78rem;color:var(--text);">Direkte Schritte</b><div id="ducky-direct-note" style="font-size:.68rem;color:var(--muted);margin-top:.2rem;"></div></div>
          <div style="display:flex;gap:.45rem;flex-wrap:wrap;">
            <button class="btn btn-add btn-sm" onclick="applyDuckyEditor(false)">&#9654; Sequenz ersetzen</button>
            <button class="btn btn-ghost btn-sm" onclick="applyDuckyEditor(true)">&#43; Anhaengen</button>
          </div>
        </div>
      </div>

      <div id="ducky-function-tools" style="display:none;margin-top:.7rem;padding:.7rem;border:1px solid var(--border);border-radius:9px;background:var(--bg2);">
        <div style="font-size:.78rem;color:var(--text);font-weight:700;margin-bottom:.25rem;">Funktionen als Sequenzbaustein</div>
        <div id="ducky-function-note" style="font-size:.68rem;color:var(--muted);margin-bottom:.55rem;">Funktion auswaehlen, optionale Argumente eintragen und direkt in normale Schritte expandieren.</div>
        <div style="display:flex;gap:.45rem;align-items:center;flex-wrap:wrap;">
          <select id="ducky-function-select" style="min-width:220px;background:var(--bg);border:1px solid var(--border);border-radius:9px;color:var(--text);font-size:.75rem;padding:.55rem .6rem;outline:none;"></select>
          <input id="ducky-function-args" type="text" placeholder="Keine Argumente erforderlich" style="flex:1;min-width:190px;background:var(--bg);border:1px solid var(--border);border-radius:9px;color:var(--text);font-size:.75rem;padding:.55rem .6rem;outline:none;">
          <button class="btn btn-add btn-sm" onclick="importSelectedDuckyFunction(false)">&#9654; Funktion als Sequenz</button>
          <button class="btn btn-ghost btn-sm" onclick="importSelectedDuckyFunction(true)">&#43; Funktion anhaengen</button>
        </div>
      </div>

      <div id="ducky-report" style="margin-top:.7rem;padding:.7rem;border:1px solid var(--border);border-radius:9px;background:var(--bg2);font-size:.72rem;line-height:1.5;color:var(--muted);white-space:pre-wrap;">Noch keine Datei geladen.</div>
    </div>
    <div class="card">
      <lbl>Import-Verhalten</lbl>
      <div style="font-size:.72rem;color:var(--muted);line-height:1.55;">Direkt ausfuehrbare Befehle werden sofort als normale Text-, Tasten- und Pause-Schritte angeboten. Enthaltene <b>FUNCTION</b>-Definitionen erscheinen als auswaehlbare Bausteine und koennen mit aufgeloesten DEFINEs/Variablen in die Sequenz expandiert werden. Reine Extension-/Library-Dateien bleiben beim Laden passiv, bis du eine Funktion bewusst in die Sequenz uebernimmst. Parserfehler, Anpassungen und uebersprungene Hardwarebefehle werden kompakt im Importstatus angezeigt.</div>
    </div>
  </div>

  <div class="panel" id="tab-quick">
    <div class="card keyboard-card">
      <div style="display:flex;align-items:center;justify-content:space-between;gap:.5rem;">
        <div>
          <div class="key-section-title" style="margin:0;">Virtuelle QWERTZ-Tastatur</div>
          <div style="font-size:.68rem;color:var(--muted);margin-top:.2rem;">Große Touch-Tasten · Schreiben und PC-Funktionen getrennt</div>
        </div>
        <div id="vk-mod-status" class="vk-mod-status"><span class="pulse"></span><span id="vk-mod-text"></span></div>
      </div>
      <div class="virtual-kbd">
        <div class="vk-toolbar">
          <button id="vk-tab-write" class="vk-switch active" onclick="vkPane('write')">ABC · Schreiben</button>
          <button id="vk-tab-pc" class="vk-switch" onclick="vkPane('pc')">PC · Navigation</button>
        </div>

        <div id="vk-pane-write" class="vk-pane active">
          <div class="vk-zone">
            <div class="vk-zone-head"><span class="vk-zone-title">Zahlen</span></div>
            <div class="vk-row r10">
              <button class="vk-key secondary" onclick="vkKey('1')">1</button><button class="vk-key secondary" onclick="vkKey('2')">2</button><button class="vk-key secondary" onclick="vkKey('3')">3</button><button class="vk-key secondary" onclick="vkKey('4')">4</button><button class="vk-key secondary" onclick="vkKey('5')">5</button><button class="vk-key secondary" onclick="vkKey('6')">6</button><button class="vk-key secondary" onclick="vkKey('7')">7</button><button class="vk-key secondary" onclick="vkKey('8')">8</button><button class="vk-key secondary" onclick="vkKey('9')">9</button><button class="vk-key secondary" onclick="vkKey('0')">0</button>
            </div>
          </div>
          <div class="vk-zone">
            <div class="vk-zone-head"><span class="vk-zone-title">QWERTZ</span></div>
            <div class="vk-row r10"><button class="vk-key" onclick="vkKey('q')">Q</button><button class="vk-key" onclick="vkKey('w')">W</button><button class="vk-key" onclick="vkKey('e')">E</button><button class="vk-key" onclick="vkKey('r')">R</button><button class="vk-key" onclick="vkKey('t')">T</button><button class="vk-key" onclick="vkKey('z')">Z</button><button class="vk-key" onclick="vkKey('u')">U</button><button class="vk-key" onclick="vkKey('i')">I</button><button class="vk-key" onclick="vkKey('o')">O</button><button class="vk-key" onclick="vkKey('p')">P</button></div>
            <div class="vk-row r9"><button class="vk-key" onclick="vkKey('a')">A</button><button class="vk-key" onclick="vkKey('s')">S</button><button class="vk-key" onclick="vkKey('d')">D</button><button class="vk-key" onclick="vkKey('f')">F</button><button class="vk-key" onclick="vkKey('g')">G</button><button class="vk-key" onclick="vkKey('h')">H</button><button class="vk-key" onclick="vkKey('j')">J</button><button class="vk-key" onclick="vkKey('k')">K</button><button class="vk-key" onclick="vkKey('l')">L</button></div>
            <div class="vk-row letters-bottom"><button class="vk-key mod" id="vk-shift" onclick="vkToggle('shift')">⇧</button><button class="vk-key" onclick="vkKey('y')">Y</button><button class="vk-key" onclick="vkKey('x')">X</button><button class="vk-key" onclick="vkKey('c')">C</button><button class="vk-key" onclick="vkKey('v')">V</button><button class="vk-key" onclick="vkKey('b')">B</button><button class="vk-key" onclick="vkKey('n')">N</button><button class="vk-key" onclick="vkKey('m')">M</button><button class="vk-key action" onclick="vkKey('BACKSPACE')">⌫</button></div>
            <div class="vk-row r8"><button class="vk-key" onclick="vkKey('ä')">Ä</button><button class="vk-key" onclick="vkKey('ö')">Ö</button><button class="vk-key" onclick="vkKey('ü')">Ü</button><button class="vk-key" onclick="vkKey('ß')">ß</button><button class="vk-key" onclick="vkKey(',')">,</button><button class="vk-key" onclick="vkKey('.')">.</button><button class="vk-key" onclick="vkKey('-')">−</button><button class="vk-key" onclick="vkKey('?')">?</button></div>
          </div>
          <div class="vk-zone">
            <div class="vk-zone-head"><span class="vk-zone-title">Steuerung</span></div>
            <div class="vk-row actions"><button class="vk-key mod" id="vk-ctrl" onclick="vkToggle('ctrl')">Ctrl</button><button class="vk-key mod" id="vk-alt" onclick="vkToggle('alt')">Alt</button><button class="vk-key mod" id="vk-altgr" onclick="vkToggle('altgr')">AltGr</button><button class="vk-key space action" onclick="vkKey(' ')">Leertaste</button><button class="vk-key action" onclick="vkKey('ENTER')">Enter</button></div>
          </div>
        </div>

        <div id="vk-pane-pc" class="vk-pane">
          <div class="vk-zone">
            <div class="vk-zone-head"><span class="vk-zone-title">Navigation</span></div>
            <div class="vk-row pc4"><button class="vk-key" onclick="vkKey('HOME')">Pos1</button><button class="vk-key" onclick="vkKey('UP')">↑</button><button class="vk-key" onclick="vkKey('END')">Ende</button><button class="vk-key" onclick="vkKey('PAGEUP')">Bild ↑</button></div>
            <div class="vk-row pc4"><button class="vk-key" onclick="vkKey('LEFT')">←</button><button class="vk-key" onclick="vkKey('DOWN')">↓</button><button class="vk-key" onclick="vkKey('RIGHT')">→</button><button class="vk-key" onclick="vkKey('PAGEDOWN')">Bild ↓</button></div>
          </div>
          <div class="vk-zone">
            <div class="vk-zone-head"><span class="vk-zone-title">PC-Tasten</span></div>
            <div class="vk-row pc4"><button class="vk-key" onclick="vkKey('ESC')">Esc</button><button class="vk-key" onclick="vkKey('TAB')">Tab</button><button class="vk-key" onclick="vkKey('INSERT')">Einfg</button><button class="vk-key danger" onclick="vkKey('DELETE')">Entf</button></div>
            <div class="vk-row pc6"><button class="vk-key secondary" onclick="vkKey('F1')">F1</button><button class="vk-key secondary" onclick="vkKey('F2')">F2</button><button class="vk-key secondary" onclick="vkKey('F3')">F3</button><button class="vk-key secondary" onclick="vkKey('F4')">F4</button><button class="vk-key secondary" onclick="vkKey('F5')">F5</button><button class="vk-key secondary" onclick="vkKey('F6')">F6</button></div>
            <div class="vk-row pc6"><button class="vk-key secondary" onclick="vkKey('F7')">F7</button><button class="vk-key secondary" onclick="vkKey('F8')">F8</button><button class="vk-key secondary" onclick="vkKey('F9')">F9</button><button class="vk-key secondary" onclick="vkKey('F10')">F10</button><button class="vk-key secondary" onclick="vkKey('F11')">F11</button><button class="vk-key secondary" onclick="vkKey('F12')">F12</button></div>
          </div>
          <div class="vk-zone">
            <div class="vk-zone-head"><span class="vk-zone-title">Sonderzeichen</span></div>
            <div class="vk-row pc6"><button class="vk-key secondary" onclick="vkKey('+')">+</button><button class="vk-key secondary" onclick="vkKey('=')">=</button><button class="vk-key secondary" onclick="vkKey('*')">*</button><button class="vk-key secondary" onclick="vkKey('/')">/</button><button class="vk-key secondary" onclick="vkKey('?')">?</button><button class="vk-key secondary" onclick="vkKey('@')">@</button></div>
            <div class="vk-row pc6"><button class="vk-key secondary" onclick="vkKey('#')">#</button><button class="vk-key secondary" onclick="vkKey('\\')">\\</button><button class="vk-key secondary" onclick="vkKey('[')">[</button><button class="vk-key secondary" onclick="vkKey(']')">]</button><button class="vk-key secondary" onclick="vkKey('{')">{</button><button class="vk-key secondary" onclick="vkKey('}')">}</button></div>
          </div>
        </div>
      </div>
    </div>

    <div class="card">
      <div class="key-section-title">Sondertasten</div>
      <div class="key-grid">
        <button type="button" class="key mod" onclick="quickKey('LCTRL')">L Ctrl</button><button type="button" class="key mod" onclick="quickKey('LSHIFT')">L Shift</button>
        <button type="button" class="key mod" onclick="quickKey('LALT')">L Alt</button><button type="button" class="key mod" onclick="quickKey('LGUI')">Win</button>
        <button type="button" class="key mod" onclick="quickKey('RCTRL')">R Ctrl</button><button type="button" class="key mod" onclick="quickKey('RSHIFT')">R Shift</button>
        <button type="button" class="key mod" onclick="quickKey('RALT')">AltGr</button><button type="button" class="key mod" onclick="quickKey('RGUI')">Win R</button>
      </div>
      <div class="key-section-title">Funktionstasten</div>
      <div class="key-grid">
        <button type="button" class="key" onclick="quickKey('ESC')">ESC</button><button type="button" class="key" onclick="quickKey('F1')">F1</button><button type="button" class="key" onclick="quickKey('F2')">F2</button><button type="button" class="key" onclick="quickKey('F3')">F3</button><button type="button" class="key" onclick="quickKey('F4')">F4</button><button type="button" class="key" onclick="quickKey('F5')">F5</button><button type="button" class="key" onclick="quickKey('F6')">F6</button><button type="button" class="key" onclick="quickKey('F7')">F7</button><button type="button" class="key" onclick="quickKey('F8')">F8</button><button type="button" class="key" onclick="quickKey('F9')">F9</button><button type="button" class="key" onclick="quickKey('F10')">F10</button><button type="button" class="key" onclick="quickKey('F11')">F11</button><button type="button" class="key" onclick="quickKey('F12')">F12</button>
      </div>
      <div class="key-section-title">Navigation &amp; System</div>
      <div class="key-grid">
        <button type="button" class="key" onclick="quickKey('TAB')">Tab</button><button type="button" class="key" onclick="quickKey('CAPS')">Caps</button><button type="button" class="key" onclick="quickKey('ENTER')">Enter</button><button type="button" class="key" onclick="quickKey('BACKSPACE')">⌫</button>
        <button type="button" class="key" onclick="quickKey('DELETE')">Del</button><button type="button" class="key" onclick="quickKey('INSERT')">Ins</button><button type="button" class="key" onclick="quickKey('HOME')">Home</button><button type="button" class="key" onclick="quickKey('END')">End</button>
        <button type="button" class="key" onclick="quickKey('PAGEUP')">PgUp</button><button type="button" class="key" onclick="quickKey('PAGEDOWN')">PgDn</button><button type="button" class="key" onclick="quickKey('UP')">↑</button><button type="button" class="key" onclick="quickKey('DOWN')">↓</button><button type="button" class="key" onclick="quickKey('LEFT')">←</button><button type="button" class="key" onclick="quickKey('RIGHT')">→</button>
        <button type="button" class="key" onclick="quickKey('SPACE')">Space</button><button type="button" class="key" onclick="quickKey('PRINT')">PrtSc</button><button type="button" class="key" onclick="quickKey('NUMLOCK')">NumLk</button>
      </div>
      <div class="key-section-title">Häufige Kombinationen</div>
      <div class="key-grid">
        <button type="button" class="key" onclick="quickKey('LCTRL+c')">Ctrl+C</button><button type="button" class="key" onclick="quickKey('LCTRL+v')">Ctrl+V</button><button type="button" class="key" onclick="quickKey('LCTRL+x')">Ctrl+X</button><button type="button" class="key" onclick="quickKey('LCTRL+z')">Ctrl+Z</button>
        <button type="button" class="key" onclick="quickKey('LCTRL+a')">Ctrl+A</button><button type="button" class="key" onclick="quickKey('LCTRL+s')">Ctrl+S</button><button type="button" class="key" onclick="quickKey('LALT+F4')">Alt+F4</button><button type="button" class="key" onclick="quickKey('LCTRL+LALT+DELETE')">Ctrl+Alt+Del</button>
        <button type="button" class="key" onclick="quickKey('LGUI+d')">Win+D</button><button type="button" class="key" onclick="quickKey('LGUI+l')">Win+L</button><button type="button" class="key" onclick="quickKey('LGUI+r')">Win+R</button><button type="button" class="key" onclick="quickKey('LALT+TAB')">Alt+Tab</button><button type="button" class="key" onclick="quickKey('LCTRL+LSHIFT+ESC')">Task-Mgr</button>
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
let steps=[],repeatMode='once',running=false,pollTimer=null,loopInterval=1000,speedIndex=2,burstSize=1,clipMode=false,customSpeedMs=0,clipCustomSpeedMs=20;

//  Persistenz: alles in localStorage speichern 
function saveState(){
  try {
    localStorage.setItem('esp32kb', JSON.stringify({
      steps, repeatMode, loopInterval, speedIndex, burstSize, burstEnabled, clipMode, customSpeedMs, clipCustomSpeedMs
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
      // Alte Versionen hatten 3 als Default. Ohne aktivierten Burst auf 1 migrieren.
      burstSize = (s.burstEnabled === true) ? s.burstSize : 1;
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
    if (s.clipCustomSpeedMs !== undefined) { { const parsed=parseInt(s.clipCustomSpeedMs); clipCustomSpeedMs = Math.max(0, Math.min(500, Number.isFinite(parsed)?parsed:20)); } const ccsv=document.getElementById('clip-custom-speed-val'); if(ccsv) ccsv.value=clipCustomSpeedMs; updateClipSpeedInfo(); }
    if (s.customSpeedMs !== undefined) { customSpeedMs = s.customSpeedMs; const csv=document.getElementById('custom-speed-val'); if(csv) csv.value=customSpeedMs; }
  } catch(e){}
}
function switchTab(n){document.querySelectorAll('.tab').forEach(t=>t.classList.remove('active'));document.querySelectorAll('.panel').forEach(p=>p.classList.remove('active'));['seq','quick','ducky','tmpl','auto','wifi'].forEach((t,i)=>{if(t===n){document.querySelectorAll('.tab')[i].classList.add('active');document.getElementById('tab-'+t).classList.add('active');}});if(n==='wifi')loadWifiInfo();if(n==='tmpl')loadTemplates();if(n==='auto'){loadTemplates();setTimeout(loadAutostart,300);}}
function onTypeChange(){const t=document.getElementById('step-type').value;document.getElementById('step-text').style.display=t==='text'?'':'none';document.getElementById('step-speed').style.display=t==='text'?'':'none';document.getElementById('step-key').style.display=t==='key'?'':'none';document.getElementById('step-custom').style.display='none';document.getElementById('step-delay').style.display=t==='delay'?'':'none';if(t==='key')document.getElementById('step-key').onchange=function(){document.getElementById('step-custom').style.display=this.value==='__custom__'?'':'none';};}
function prettyKeyLabel(value){
  if(!value) return '';
  const names={LCTRL:'Ctrl',RCTRL:'Ctrl (R)',LSHIFT:'Shift',RSHIFT:'Shift (R)',LALT:'Alt',LGUI:'Win',RGUI:'Win (R)',RALT:'AltGr',ESC:'Esc',ENTER:'Enter',TAB:'Tab',BACKSPACE:'Backspace',DELETE:'Entf',INSERT:'Einfg',PAGEUP:'Bild↑',PAGEDOWN:'Bild↓',UP:'↑',DOWN:'↓',LEFT:'←',RIGHT:'→',HOME:'Pos1',END:'Ende',CAPS:'Caps Lock',SPACE:'Leertaste',PRINT:'Druck',NUMLOCK:'Num'};
  if(value==='+') return '+';
  return value.split('+').filter(Boolean).map(function(part){const p=part.trim();if(names[p]) return names[p];if(p.length===1) return p.toUpperCase();return p;}).join(' + ') + (value.endsWith('++')?' + +':'');
}
function normalizeKeyValue(value){
  const aliases={'CTRL':'LCTRL','CTRL (R)':'RCTRL','SHIFT':'LSHIFT','SHIFT (R)':'RSHIFT','ALT':'LALT','WIN':'LGUI','WIN (R)':'RGUI','ALTGR':'RALT','ESC':'ESC','ENTER':'ENTER','TAB':'TAB','BACKSPACE':'BACKSPACE','ENTF':'DELETE','EINFG':'INSERT','BILD↑':'PAGEUP','BILD↓':'PAGEDOWN','↑':'UP','↓':'DOWN','←':'LEFT','→':'RIGHT','POS1':'HOME','ENDE':'END','CAPS LOCK':'CAPS','LEERTASTE':'SPACE','DRUCK':'PRINT','NUM':'NUMLOCK'};
  if(value.trim()==='+') return '+';
  const plusTail=value.trim().endsWith(' + +') || value.trim().endsWith('++');
  const base=value.replace(/\s*\+\s*\+\s*$/,'');
  const normalized=base.split('+').map(function(part){
    const p=part.trim(), u=p.toUpperCase();
    if(aliases[u]) return aliases[u];
    if(p.length===1) return p.toLowerCase();
    return p;
  }).filter(Boolean).join('+');
  return normalized + (plusTail ? '++' : '');
}

// -- .INO Import -------------------------------------------------------------
// Liest typische Arduino-Keyboard-Skripte clientseitig und wandelt sie in
// Sequenz-Schritte um. Es wird KEIN Code aus der Datei ausgefuehrt.
const INO_KEY_MAP={
  KEY_LEFT_CTRL:'LCTRL',KEY_RIGHT_CTRL:'RCTRL',KEY_LEFT_SHIFT:'LSHIFT',KEY_RIGHT_SHIFT:'RSHIFT',
  KEY_LEFT_ALT:'LALT',KEY_RIGHT_ALT:'RALT',KEY_LEFT_GUI:'LGUI',KEY_RIGHT_GUI:'RGUI',
  KEY_ESC:'ESC',KEY_ESCAPE:'ESC',KEY_RETURN:'ENTER',KEY_ENTER:'ENTER',KEY_TAB:'TAB',
  KEY_BACKSPACE:'BACKSPACE',KEY_DELETE:'DELETE',KEY_INSERT:'INSERT',KEY_HOME:'HOME',KEY_END:'END',
  KEY_PAGE_UP:'PAGEUP',KEY_PAGE_DOWN:'PAGEDOWN',KEY_UP_ARROW:'UP',KEY_DOWN_ARROW:'DOWN',
  KEY_LEFT_ARROW:'LEFT',KEY_RIGHT_ARROW:'RIGHT',KEY_CAPS_LOCK:'CAPS',KEY_PRINT_SCREEN:'PRINT',
  KEY_NUM_LOCK:'NUMLOCK',KEY_F1:'F1',KEY_F2:'F2',KEY_F3:'F3',KEY_F4:'F4',KEY_F5:'F5',KEY_F6:'F6',
  KEY_F7:'F7',KEY_F8:'F8',KEY_F9:'F9',KEY_F10:'F10',KEY_F11:'F11',KEY_F12:'F12'
};

function inoStripComments(src){
  let out='',i=0,quote='',esc=false;
  while(i<src.length){
    const c=src[i],n=src[i+1]||'';
    if(quote){out+=c;if(esc)esc=false;else if(c==='\\')esc=true;else if(c===quote)quote='';i++;continue;}
    if(c==='"'||c==="'"){quote=c;out+=c;i++;continue;}
    if(c==='/'&&n==='/'){while(i<src.length&&src[i]!=='\n')i++;continue;}
    if(c==='/'&&n==='*'){i+=2;while(i<src.length-1&&!(src[i]==='*'&&src[i+1]==='/'))i++;i+=2;continue;}
    out+=c;i++;
  }
  return out;
}
function inoMatchClose(s,start,open,close){
  let d=0,q='',esc=false;
  for(let i=start;i<s.length;i++){
    const c=s[i];
    if(q){if(esc)esc=false;else if(c==='\\')esc=true;else if(c===q)q='';continue;}
    if(c==='"'||c==="'"){q=c;continue;}
    if(c===open)d++; else if(c===close){d--;if(d===0)return i;}
  }
  return -1;
}
function inoSplitTop(s,sep){
  const a=[];let st=0,p=0,b=0,q='',esc=false;
  for(let i=0;i<s.length;i++){
    const c=s[i];
    if(q){if(esc)esc=false;else if(c==='\\')esc=true;else if(c===q)q='';continue;}
    if(c==='"'||c==="'"){q=c;continue;}
    if(c==='(')p++; else if(c===')')p--; else if(c==='{')b++; else if(c==='}')b--;
    else if(c===sep&&p===0&&b===0){a.push(s.slice(st,i).trim());st=i+1;}
  }
  a.push(s.slice(st).trim());return a.filter(x=>x.length);
}
function inoDecodeString(tok){
  tok=tok.trim();if(tok.length<2)return tok;
  const q=tok[0];if((q!=='"'&&q!=="'")||tok[tok.length-1]!==q)return tok;
  let o='';
  for(let i=1;i<tok.length-1;i++){
    let c=tok[i];
    if(c==='\\'&&i+1<tok.length-1){
      const n=tok[++i];
      if(n==='n')o+='\n';else if(n==='r')o+='\r';else if(n==='t')o+='\t';
      else if(n==='\\')o+='\\';else if(n==='"')o+='"';else if(n==="'")o+="'";else o+=n;
    }else o+=c;
  }
  return o;
}
function inoEvalExpr(expr,env){
  expr=(expr||'').trim();
  while(expr.startsWith('(')&&inoMatchClose(expr,0,'(',')')===expr.length-1)expr=expr.slice(1,-1).trim();
  const parts=inoSplitTop(expr,'+');
  if(parts.length>1)return parts.map(p=>inoEvalExpr(p,env)).join('');
  if((expr[0]==='"'&&expr.endsWith('"'))||(expr[0]==="'"&&expr.endsWith("'")))return inoDecodeString(expr);
  if(Object.prototype.hasOwnProperty.call(env,expr))return String(env[expr]);
  if(/^[-+]?\d+$/.test(expr))return String(parseInt(expr,10));
  if(expr==='String()')return '';
  return expr;
}
function inoExtractFunctions(src){
  const funcs={};
  const re=/\b(?:void|String|int|bool|char|long|unsigned\s+long|size_t)\s+([A-Za-z_]\w*)\s*\(([^)]*)\)\s*\{/g;
  let m;
  while((m=re.exec(src))){
    const open=re.lastIndex-1,close=inoMatchClose(src,open,'{','}');if(close<0)break;
    const params=m[2].split(',').map(x=>x.trim()).filter(Boolean).map(x=>{const z=x.replace(/\s*=.*$/,'').trim().split(/\s+/);return z[z.length-1].replace(/[&*]/g,'');});
    funcs[m[1]]={params:params,body:src.slice(open+1,close)};re.lastIndex=close+1;
  }
  return funcs;
}
function inoAddText(ctx,text){
  if(text===undefined||text===null||text==='')return;
  const val=String(text);
  const last=ctx.out[ctx.out.length-1];
  if(last&&last.type==='text'&&last.speed===-1){last.value+=val;last.label='"'+(last.value.length>32?last.value.slice(0,32)+'...':last.value)+'"';}
  else ctx.out.push({type:'text',value:val,speed:-1,label:'"'+(val.length>32?val.slice(0,32)+'...':val)+'"'});
}
function inoAddKey(ctx,key){
  if(!key)return;
  const v=INO_KEY_MAP[key]||key.replace(/^KEY_/,'');
  ctx.out.push({type:'key',value:v,label:prettyKeyLabel(v)});
}
function inoAddDelay(ctx,v){
  const n=Math.max(0,parseInt(v,10)||0);ctx.out.push({type:'delay',value:String(n),label:n+'ms warten'});
}
function inoFlushPressed(ctx){
  if(!ctx.pressed.length)return;
  const v=ctx.pressed.map(k=>INO_KEY_MAP[k]||k.replace(/^KEY_/,'')).join('+');ctx.pressed=[];
  if(v)ctx.out.push({type:'key',value:v,label:prettyKeyLabel(v)});
}
function inoCallArgs(stmt,name){
  const p=stmt.indexOf('(',stmt.indexOf(name)+name.length);if(p<0)return null;
  const e=inoMatchClose(stmt,p,'(',')');if(e<0)return null;
  return inoSplitTop(stmt.slice(p+1,e),',');
}
function inoExecStatement(stmt,env,ctx,depth){
  stmt=stmt.trim().replace(/;\s*$/,'').trim();if(!stmt)return;
  let m=stmt.match(/^(?:const\s+)?(?:String|char\s*\*|int|long|unsigned\s+long|bool)\s+([A-Za-z_]\w*)\s*=\s*([\s\S]+)$/);
  if(m){env[m[1]]=inoEvalExpr(m[2],env);return;}
  m=stmt.match(/^([A-Za-z_]\w*)\s*=\s*([\s\S]+)$/);
  if(m&&Object.prototype.hasOwnProperty.call(env,m[1])){env[m[1]]=inoEvalExpr(m[2],env);return;}
  let a;
  if((a=inoCallArgs(stmt,'delay'))&&/^delay\s*\(/.test(stmt)){inoFlushPressed(ctx);inoAddDelay(ctx,inoEvalExpr(a[0],env));return;}
  if((a=inoCallArgs(stmt,'typeKey'))&&/^typeKey\s*\(/.test(stmt)){inoFlushPressed(ctx);inoAddKey(ctx,inoEvalExpr(a[0],env));return;}
  if((a=inoCallArgs(stmt,'Keyboard.press'))&&/^Keyboard\.press\s*\(/.test(stmt)){ctx.pressed.push(inoEvalExpr(a[0],env));return;}
  if(/^Keyboard\.releaseAll\s*\(/.test(stmt)){inoFlushPressed(ctx);return;}
  if((a=inoCallArgs(stmt,'Keyboard.release'))&&/^Keyboard\.release\s*\(/.test(stmt)){inoFlushPressed(ctx);return;}
  if((a=inoCallArgs(stmt,'Keyboard.println'))&&/^Keyboard\.println\s*\(/.test(stmt)){inoFlushPressed(ctx);inoAddText(ctx,inoEvalExpr(a[0]||'""',env));inoAddKey(ctx,'KEY_RETURN');return;}
  if((a=inoCallArgs(stmt,'Keyboard.print'))&&/^Keyboard\.print\s*\(/.test(stmt)){inoFlushPressed(ctx);inoAddText(ctx,inoEvalExpr(a[0]||'""',env));return;}
  if((a=inoCallArgs(stmt,'Keyboard.write'))&&/^Keyboard\.write\s*\(/.test(stmt)){inoFlushPressed(ctx);const x=inoEvalExpr(a[0],env);if(INO_KEY_MAP[x]||/^KEY_/.test(x))inoAddKey(ctx,x);else inoAddText(ctx,x);return;}
  if((a=inoCallArgs(stmt,'esp_print'))&&/^esp_print\s*\(/.test(stmt)){inoFlushPressed(ctx);inoAddText(ctx,inoEvalExpr(a[0]||'""',env));return;}
  if(/^Keyboard\.(begin|end)\s*\(/.test(stmt))return;
  m=stmt.match(/^([A-Za-z_]\w*)\s*\(([\s\S]*)\)$/);
  if(m&&ctx.funcs[m[1]]&&depth<12){
    inoFlushPressed(ctx);const f=ctx.funcs[m[1]],vals=inoSplitTop(m[2],',').map(x=>inoEvalExpr(x,env)),sub=Object.assign({},env);
    f.params.forEach((p,i)=>sub[p]=vals[i]!==undefined?vals[i]:'');inoExecBody(f.body,sub,ctx,depth+1);return;
  }
  if(m&&!['setup','loop'].includes(m[1]))ctx.unsupported.add(m[1]+'()');
}
function inoExecBody(body,env,ctx,depth){
  let i=0;
  while(i<body.length){
    while(i<body.length&&/\s/.test(body[i]))i++;if(i>=body.length)break;
    if(body.slice(i,i+3)==='for'&&/\s*\(/.test(body.slice(i+3))){
      const p=body.indexOf('(',i+3),pe=inoMatchClose(body,p,'(',')');if(pe<0)break;
      let j=pe+1;while(j<body.length&&/\s/.test(body[j]))j++;
      let loopBody='',after=j;
      if(body[j]==='{'){const be=inoMatchClose(body,j,'{','}');if(be<0)break;loopBody=body.slice(j+1,be);after=be+1;}
      else{let se=body.indexOf(';',j);if(se<0)se=body.length-1;loopBody=body.slice(j,se+1);after=se+1;}
      const h=body.slice(p+1,pe),parts=inoSplitTop(h,';');let count=0,varName='i',start=0,end=0,inclusive=false;
      if(parts.length===3){const im=parts[0].match(/(?:int\s+)?([A-Za-z_]\w*)\s*=\s*(-?\d+)/),cm=parts[1].match(/([A-Za-z_]\w*)\s*(<=|<)\s*(-?\d+)/);if(im&&cm&&im[1]===cm[1]){varName=im[1];start=parseInt(im[2]);end=parseInt(cm[3]);inclusive=cm[2]==='<=';count=Math.max(0,(end-start)+(inclusive?1:0));}}
      if(count>0&&count<=50){for(let n=0;n<count;n++){const sub=Object.assign({},env);sub[varName]=String(start+n);inoExecBody(loopBody,sub,ctx,depth+1);}}
      else ctx.unsupported.add('for-Schleife');i=after;continue;
    }
    if(body[i]==='{'){const e=inoMatchClose(body,i,'{','}');if(e<0)break;inoExecBody(body.slice(i+1,e),Object.assign({},env),ctx,depth+1);i=e+1;continue;}
    let p=0,b=0,q='',esc=false,j=i;
    for(;j<body.length;j++){
      const c=body[j];if(q){if(esc)esc=false;else if(c==='\\')esc=true;else if(c===q)q='';continue;}if(c==='"'||c==="'"){q=c;continue;}
      if(c==='(')p++;else if(c===')')p--;else if(c==='{')b++;else if(c==='}')b--;
      if(c===';'&&p===0&&b===0){j++;break;}
    }
    inoExecStatement(body.slice(i,j),env,ctx,depth);i=j;
  }
  if(depth===0)inoFlushPressed(ctx);
}
function parseInoToSteps(source){
  const clean=inoStripComments(source),funcs=inoExtractFunctions(clean),ctx={out:[],pressed:[],funcs:funcs,unsupported:new Set()};
  if(!funcs.setup)throw new Error('Keine setup()-Funktion gefunden.');
  inoExecBody(funcs.setup.body,{},ctx,0);inoFlushPressed(ctx);
  return {steps:ctx.out,warnings:Array.from(ctx.unsupported)};
}
// -- DuckyScript Import -------------------------------------------------------
// DuckyScript 3.x Importer: wandelt statisch auswertbare Payload-Logik in die
// vorhandenen Text/Taste/Pause-Schritte um. Im Automatikmodus werden nicht
// darstellbare Hardwarebefehle sicher angenaehert oder mit Zeilennummer entfernt.
const DUCKY_KEY_MAP={
  CTRL:'LCTRL',CONTROL:'LCTRL',LCTRL:'LCTRL',RCTRL:'RCTRL',SHIFT:'LSHIFT',LSHIFT:'LSHIFT',RSHIFT:'RSHIFT',
  ALT:'LALT',LALT:'LALT',RALT:'RALT',ALTGR:'RALT',GUI:'LGUI',WINDOWS:'LGUI',WIN:'LGUI',COMMAND:'LGUI',CMD:'LGUI',LGUI:'LGUI',RGUI:'RGUI',
  ESC:'ESC',ESCAPE:'ESC',ENTER:'ENTER',RETURN:'ENTER',TAB:'TAB',BACKSPACE:'BACKSPACE',BKSP:'BACKSPACE',DELETE:'DELETE',DEL:'DELETE',
  INSERT:'INSERT',HOME:'HOME',END:'END',PAGEUP:'PAGEUP',PAGE_UP:'PAGEUP',PAGEDOWN:'PAGEDOWN',PAGE_DOWN:'PAGEDOWN',
  UP:'UP',UPARROW:'UP',DOWN:'DOWN',DOWNARROW:'DOWN',LEFT:'LEFT',LEFTARROW:'LEFT',RIGHT:'RIGHT',RIGHTARROW:'RIGHT',
  CAPSLOCK:'CAPS',CAPS_LOCK:'CAPS',SPACE:'SPACE',SPACEBAR:'SPACE',PRINTSCREEN:'PRINT',PRINT_SCREEN:'PRINT',PRNTSCRN:'PRINT',NUMLOCK:'NUMLOCK',NUM_LOCK:'NUMLOCK',
  MENU:'MENU',APP:'MENU',PAUSE:'PAUSE',BREAK:'PAUSE'
};
for(let i=1;i<=12;i++)DUCKY_KEY_MAP['F'+i]='F'+i;
function duckyWarn(ctx,msg,line){const full=(line?('Zeile '+line+': '):'')+msg;if(!ctx.warningSet.has(full)){ctx.warningSet.add(full);ctx.warnings.push(full);}}
function duckyTrace(ctx,line,kind,detail){if(!ctx.trace)return;if(ctx.trace.length>=1000)return;ctx.trace.push({line:line||0,kind:String(kind||'INFO'),detail:String(detail||'')});}
function duckyError(ctx,msg,line){const full=(line?('Zeile '+line+': '):'')+msg;if(!ctx.errorSet.has(full)){ctx.errorSet.add(full);ctx.errors.push(full);}}
function duckyAdapt(ctx,msg,line){const full=(line?('Zeile '+line+': '):'')+msg;if(!ctx.adaptedSet.has(full)){ctx.adaptedSet.add(full);ctx.adapted.push(full);}}
function duckySkip(ctx,msg,line){const full=(line?('Zeile '+line+': '):'')+msg;if(!ctx.skippedSet.has(full)){ctx.skippedSet.add(full);ctx.skipped.push(full);}}
function duckyUnsupported(ctx,msg,line){
  if(ctx.mode==='strict')duckyError(ctx,msg,line);
  else duckySkip(ctx,msg,line);
}
function duckyCondState(cond,ctx,line){
  const raw=String(cond||'').trim().replace(/\s*THEN\s*$/i,''),r=duckyEvalExpr(raw,ctx,line);
  if(!r.complete||r.unknown)return {known:false,value:false,raw};
  return {known:true,value:duckyTruthy(r.value),raw};
}
function duckyLookup(name,ctx){
  if(Object.prototype.hasOwnProperty.call(ctx.vars,name))return ctx.vars[name];
  if(Object.prototype.hasOwnProperty.call(ctx.defines,name))return ctx.defines[name];
  const u=String(name).toUpperCase();if(u==='TRUE')return true;if(u==='FALSE')return false;if(u==='NULL')return 0;
  if(['WINDOWS','MACOS','LINUX','IOS','CHROMEOS','ANDROID','OTHER'].includes(u))return u;
  return undefined;
}
function duckyTokenize(expr){
  const a=[];let i=0,s=String(expr||'');
  while(i<s.length){let c=s[i];if(/\s/.test(c)){i++;continue;}
    const two=s.slice(i,i+2);if(['==','!=','>=','<=','&&','||','<<','>>'].includes(two)){a.push({t:'op',v:two});i+=2;continue;}
    if('()+-*/%<>&|^!~'.includes(c)){a.push({t:'op',v:c});i++;continue;}
    if(c==='"'||c==="'"){let q=c,j=i+1,v='';for(;j<s.length;j++){c=s[j];if(c==='\\'&&j+1<s.length){const n=s[++j];v+=n==='n'?'\n':n==='r'?'\r':n==='t'?'\t':n;}else if(c===q)break;else v+=c;}a.push({t:'str',v});i=Math.min(s.length,j+1);continue;}
    let m=s.slice(i).match(/^0x[0-9a-f]+/i);if(m){a.push({t:'num',v:parseInt(m[0],16)});i+=m[0].length;continue;}
    m=s.slice(i).match(/^\d+(?:\.\d+)?/);if(m){a.push({t:'num',v:Number(m[0])});i+=m[0].length;continue;}
    m=s.slice(i).match(/^[#$]?[A-Za-z_][\w$#]*/);if(m){let v=m[0],u=v.toUpperCase();if(u==='AND'||u==='OR'||u==='NOT')a.push({t:'op',v:u});else a.push({t:'id',v});i+=v.length;continue;}
    a.push({t:'str',v:c});i++;
  }return a;
}
function duckyEvalExpr(expr,ctx,line){
  const tok=duckyTokenize(expr);let p=0,unknown=false;
  const peek=()=>tok[p],take=()=>tok[p++];
  function primary(){const x=take();if(!x)return 0;if(x.t==='num'||x.t==='str')return x.v;if(x.t==='id'){const v=duckyLookup(x.v,ctx);if(v===undefined){unknown=true;return 0;}return v;}if(x.v==='('){const v=lor();if(peek()&&peek().v===')')take();return v;}return 0;}
  function unary(){const x=peek();if(x&&x.t==='op'&&['!','NOT','-','+','~'].includes(x.v)){take();const v=unary();if(x.v==='!'||x.v==='NOT')return !duckyTruthy(v);if(x.v==='-')return -Number(v||0);if(x.v==='+')return Number(v||0);return ~Number(v||0);}return primary();}
  function mul(){let v=unary();while(peek()&&['*','/','%'].includes(peek().v)){const o=take().v,b=Number(unary()||0),a=Number(v||0);v=o==='*'?a*b:o==='/'?(b===0?0:Math.trunc(a/b)):(b===0?0:a%b);}return v;}
  function add(){let v=mul();while(peek()&&['+','-'].includes(peek().v)){const o=take().v,b=mul();if(o==='+'&&(typeof v==='string'||typeof b==='string'))v=String(v)+String(b);else v=o==='+'?Number(v||0)+Number(b||0):Number(v||0)-Number(b||0);}return v;}
  function shift(){let v=add();while(peek()&&['<<','>>'].includes(peek().v)){const o=take().v,b=Number(add()||0);v=o==='<<'?(Number(v||0)<<b):(Number(v||0)>>b);}return v;}
  function rel(){let v=shift();while(peek()&&['<','>','<=','>='].includes(peek().v)){const o=take().v,b=shift(),an=Number(v),bn=Number(b),num=Number.isFinite(an)&&Number.isFinite(bn),a=num?an:String(v).toUpperCase(),bb=num?bn:String(b).toUpperCase();v=o==='<'?a<bb:o==='>'?a>bb:o==='<='?a<=bb:a>=bb;}return v;}
  function eq(){let v=rel();while(peek()&&['==','!='].includes(peek().v)){const o=take().v,b=rel(),an=Number(v),bn=Number(b),num=Number.isFinite(an)&&Number.isFinite(bn),same=num?an===bn:String(v).toUpperCase()===String(b).toUpperCase();v=o==='=='?same:!same;}return v;}
  function band(){let v=eq();while(peek()&&peek().v==='&'){take();v=Number(v||0)&Number(eq()||0);}return v;}
  function bxor(){let v=band();while(peek()&&peek().v==='^'){take();v=Number(v||0)^Number(band()||0);}return v;}
  function bor(){let v=bxor();while(peek()&&peek().v==='|'){take();v=Number(v||0)|Number(bxor()||0);}return v;}
  function land(){let v=bor();while(peek()&&['&&','AND'].includes(peek().v)){take();const b=bor();v=duckyTruthy(v)&&duckyTruthy(b);}return v;}
  function lor(){let v=land();while(peek()&&['||','OR'].includes(peek().v)){take();const b=land();v=duckyTruthy(v)||duckyTruthy(b);}return v;}
  const val=lor(),complete=p>=tok.length;if(unknown&&complete)duckyWarn(ctx,'Ausdruck enthaelt eine zur Importzeit unbekannte Host-Variable: '+String(expr).trim(),line);return {value:val,unknown,complete};
}
function duckyTruthy(v){return !(v===undefined||v===null||v===false||v===0||String(v).toUpperCase()==='FALSE'||String(v)==='');}
function duckyScalar(v,ctx,line){
  const raw=String(v===undefined?'':v).trim();
  if(!raw)return '';
  // Quoted values are explicit strings. Keeping this fast path also prevents
  // URLs/file names/content inside quotes from being mistaken for expressions.
  if((raw[0]==='"'&&raw[raw.length-1]==='"')||(raw[0]==="'"&&raw[raw.length-1]==="'")){
    const r=duckyEvalExpr(raw,ctx,line);return r.complete?r.value:raw.slice(1,-1);
  }
  // A single identifier is a variable only when it is actually known. DuckyScript
  // DEFINE commonly uses unquoted literal words, e.g. DEFINE #OUTPUT default.
  // Those must stay literals instead of producing a fake unknown-host-variable warning.
  if(/^[A-Za-z_][\w.-]*$/.test(raw)){
    const known=duckyLookup(raw,ctx);return known===undefined?duckyExpandText(raw,ctx):known;
  }
  // Explicit Ducky variables / defines should still be evaluated as such.
  if(/^[#$][A-Za-z_][\w$#]*$/.test(raw)){
    const known=duckyLookup(raw,ctx);if(known!==undefined)return known;
    duckyWarn(ctx,'Unbekannte Variable/Konstante: '+raw,line);return raw;
  }
  // Plain values such as URLs, paths or filenames are literals unless they contain
  // clear expression syntax. This is important for DEFINE #URL https://example.com/.
  const looksExpr=/^(?:0x[0-9a-f]+|\d+(?:\.\d+)?)$/i.test(raw)||/[()<>!=&|+*%~]/.test(raw)||/\s(?:AND|OR|NOT)\s/i.test(' '+raw+' ');
  if(!looksExpr)return duckyExpandText(raw,ctx);
  const r=duckyEvalExpr(raw,ctx,line);return r.complete&&!r.unknown?r.value:duckyExpandText(raw,ctx);
}
function duckyEvalCond(cond,ctx,line){const raw=String(cond||'').trim().replace(/\s*THEN\s*$/i,''),r=duckyEvalExpr(raw,ctx,line);if(!r.complete){duckyWarn(ctx,'Bedingung konnte nicht vollstaendig ausgewertet werden: '+raw,line);return false;}return duckyTruthy(r.value);}
function duckyExpandText(text,ctx){
  return String(text===undefined?'':text).replace(/[$#][A-Za-z_][\w$#]*/g,m=>{const v=duckyLookup(m,ctx);return v===undefined?m:String(v);});
}
function duckyPreprocess(lines,ctx){
  const out=lines.slice();
  function blank(a,b){for(let k=a;k<=b&&k<out.length;k++)out[k]='';}
  for(let i=0;i<out.length;i++){
    const t=out[i].trim();let m=t.match(/^DEFINE\s+(\S+)\s+(.+)$/i);
    if(m){ctx.defines[m[1]]=duckyScalar(m[2],ctx,i+1);out[i]='';continue;}
    m=t.match(/^IF_DEFINED_(TRUE|FALSE)\s+(\S+)/i);
    if(m){let depth=1,j=i+1,elseAt=-1;for(;j<out.length;j++){const x=out[j].trim();if(/^IF_DEFINED_(?:TRUE|FALSE)\b/i.test(x))depth++;else if(/^END_IF_DEFINED\b/i.test(x)){depth--;if(depth===0)break;}else if(depth===1&&/^ELSE_DEFINED\b/i.test(x))elseAt=j;}
      const yes=duckyTruthy(duckyLookup(m[2],ctx))===(m[1].toUpperCase()==='TRUE');out[i]='';if(j>=out.length){duckyError(ctx,'Syntaxfehler: IF_DEFINED ohne END_IF_DEFINED',i+1);duckyTrace(ctx,i+1,'SYNTAX_ERROR','IF_DEFINED ohne END_IF_DEFINED');ctx.abort=true;continue;}
      if(elseAt>=0){if(yes)blank(elseAt,j);else{blank(i,elseAt);out[j]='';}}else if(!yes)blank(i,j);else{out[j]='';}
      i=j;continue;
    }
    if(/^ELSE_DEFINED\b|^END_IF_DEFINED\b/i.test(t))out[i]='';
  }return out;
}
function duckyExtractFunctions(lines,ctx){
  const funcs={},main=lines.slice();
  for(let i=0;i<lines.length;i++){
    const t=lines[i].trim(),m=t.match(/^FUNCTION\s+([A-Za-z_][\w$#]*)\s*\(([^)]*)\)/i);if(!m)continue;
    let depth=1,j=i+1;for(;j<lines.length;j++){const x=lines[j].trim();if(/^FUNCTION\b/i.test(x))depth++;else if(/^END_FUNCTION\b/i.test(x)){depth--;if(depth===0)break;}}
    if(j>=lines.length){duckyError(ctx,'Syntaxfehler: FUNCTION '+m[1]+' ohne END_FUNCTION',i+1);duckyTrace(ctx,i+1,'SYNTAX_ERROR','FUNCTION '+m[1]+' ohne END_FUNCTION');ctx.abort=true;for(let k=i;k<main.length;k++)main[k]='';break;}
    const params=m[2].split(',').map(x=>x.trim()).filter(Boolean);funcs[m[1].toUpperCase()]={name:m[1],params,lines:lines.slice(i+1,j),baseLine:i+2};duckyTrace(ctx,i+1,'FUNCTION','Definition '+m[1]+'('+params.join(', ')+')');for(let k=i;k<=Math.min(j,main.length-1);k++)main[k]='';i=j;
  }return {funcs,main};
}
function duckyKey(tok){tok=String(tok||'').trim();if(!tok)return '';const u=tok.toUpperCase().replace(/-/g,'_');if(DUCKY_KEY_MAP[u])return DUCKY_KEY_MAP[u];if(tok.length===1)return /[A-Z]/.test(tok)?tok.toLowerCase():tok;return '';}
const DUCKY_RAW_HID_MAP={
  0x28:'ENTER',0x29:'ESC',0x2A:'BACKSPACE',0x2B:'TAB',0x2C:'SPACE',0x2D:'-',0x2E:'=',0x2F:'[',0x30:']',0x31:'\\',0x33:';',0x34:"'",0x35:'`',0x36:',',0x37:'.',0x38:'/',
  0x39:'CAPS',0x46:'PRINT',0x47:'SCROLLLOCK',0x48:'PAUSE',0x49:'INSERT',0x4A:'HOME',0x4B:'PAGEUP',0x4C:'DELETE',0x4D:'END',0x4E:'PAGEDOWN',0x4F:'RIGHT',0x50:'LEFT',0x51:'DOWN',0x52:'UP',0x53:'NUMLOCK',0x65:'MENU'
};
for(let i=0;i<26;i++)DUCKY_RAW_HID_MAP[0x04+i]=String.fromCharCode(97+i);
for(let i=0;i<9;i++)DUCKY_RAW_HID_MAP[0x1E+i]=String(i+1);DUCKY_RAW_HID_MAP[0x27]='0';
for(let i=0;i<12;i++)DUCKY_RAW_HID_MAP[0x3A+i]='F'+(i+1);
function duckyRawHidKey(raw){const n=typeof raw==='number'?raw:Number(String(raw).trim());return Number.isInteger(n)?(DUCKY_RAW_HID_MAP[n]||''):'';}
function duckyAddDefaultDelay(ctx){if(ctx.defaultDelay>0)inoAddDelay(ctx,ctx.defaultDelay);}
function duckyAddCombo(ctx,tokens,line){
  let keys=(ctx.held||[]).slice();for(const tok of tokens){const expanded=String(duckyExpandText(tok,ctx));const k=duckyKey(expanded);if(k)keys.push(k);else if(tok)duckyWarn(ctx,'Unbekannte Taste '+tok,line);}
  keys=[...new Set(keys)];if(!keys.length)return false;const v=keys.join('+');ctx.out.push({type:'key',value:v,label:prettyKeyLabel(v)});duckyAddDefaultDelay(ctx);return true;
}
function duckyFindIfEnd(lines,start){let depth=0,branches=[],branchStart=start+1,cond=lines[start].trim().replace(/^IF\s*/i,'').replace(/\s*THEN\s*$/i,'');let curCond=cond;
  for(let i=start+1;i<lines.length;i++){const t=lines[i].trim();if(/^IF\b/i.test(t)){depth++;continue;}if(/^END_IF\b/i.test(t)){if(depth>0){depth--;continue;}branches.push({cond:curCond,start:branchStart,end:i,line:start+1});return {branches,end:i};}if(depth===0&&/^ELSE\s+IF\b/i.test(t)){branches.push({cond:curCond,start:branchStart,end:i,line:start+1});curCond=t.replace(/^ELSE\s+IF\s*/i,'').replace(/\s*THEN\s*$/i,'');branchStart=i+1;}else if(depth===0&&/^ELSE\b/i.test(t)){branches.push({cond:curCond,start:branchStart,end:i,line:start+1});curCond=null;branchStart=i+1;}}
  return null;
}
function duckyFindWhileEnd(lines,start){let depth=0;for(let i=start+1;i<lines.length;i++){const t=lines[i].trim();if(/^WHILE\b/i.test(t))depth++;else if(/^END_WHILE\b/i.test(t)){if(depth===0)return i;depth--;}}return -1;}
function duckyRepeatAction(ctx,a,n,line){
  if(n>5000){duckyWarn(ctx,'REPEAT '+n+' aus Sicherheitsgruenden auf 5000 begrenzt',line);n=5000;}
  for(let r=0;r<n;r++){if(a.kind==='delay')inoAddDelay(ctx,a.arg);else if(a.kind==='string'){inoAddText(ctx,a.arg);duckyAddDefaultDelay(ctx);}else if(a.kind==='stringln'){inoAddText(ctx,a.arg);inoAddKey(ctx,'ENTER');duckyAddDefaultDelay(ctx);}else if(a.kind==='combo')duckyAddCombo(ctx,a.arg,line);if(ctx.out.length>10000){duckyWarn(ctx,'Import bei 10000 Schritten begrenzt',line);ctx.abort=true;break;}}
}
function duckyExecLines(lines,ctx,depth,baseLine){
  if(depth>24){duckyWarn(ctx,'Maximale Funktions-/Blocktiefe erreicht',baseLine);return;}let prevAction=null;baseLine=baseLine||1;
  for(let i=0;i<lines.length&&!ctx.abort&&!ctx.stopped;i++){
    let raw=lines[i],t=raw.trim(),line=baseLine+i;if(!t)continue;
    if(/^REM(?:\s|$)/i.test(t)||/^REM_BLOCK\b/i.test(t)||/^END_REM\b/i.test(t)||/^DEFINE\b/i.test(t)||/^EXTENSION\b/i.test(t)||/^END_EXTENSION\b/i.test(t))continue;
    if(/^IF\b/i.test(t)){const block=duckyFindIfEnd(lines,i);if(!block){duckyError(ctx,'Syntaxfehler: IF ohne END_IF',line);duckyTrace(ctx,line,'SYNTAX_ERROR','IF ohne END_IF');ctx.abort=true;continue;}let chosen=null,unknown=false;for(const b of block.branches){if(b.cond===null){if(!unknown)chosen=b;break;}const cs=duckyCondState(b.cond,ctx,line);if(!cs.known){unknown=true;break;}if(cs.value){chosen=b;break;}}if(unknown){if(ctx.mode==='strict')duckyError(ctx,'Host-/Hardware-abhaengige IF-Bedingung kann beim Import nicht sicher ausgewertet werden; Block nicht importiert',line);else duckySkip(ctx,'IF-Block entfernt, weil die Bedingung erst auf dem Zielsystem/der Rubber-Ducky-Hardware bestimmbar ist',line);}else if(chosen){duckyTrace(ctx,line,'IF','Bedingung ausgewertet; Branch ab Zeile '+(baseLine+chosen.start));duckyExecLines(lines.slice(chosen.start,chosen.end),ctx,depth+1,baseLine+chosen.start);}else duckyTrace(ctx,line,'IF','Kein Branch ausgewaehlt');i=block.end;prevAction=null;continue;}
    if(/^WHILE\b/i.test(t)){const end=duckyFindWhileEnd(lines,i);if(end<0){duckyError(ctx,'Syntaxfehler: WHILE ohne END_WHILE',line);duckyTrace(ctx,line,'SYNTAX_ERROR','WHILE ohne END_WHILE');ctx.abort=true;continue;}const cond=t.replace(/^WHILE\s*/i,''),first=duckyCondState(cond,ctx,line);if(!first.known){if(ctx.mode==='strict')duckyError(ctx,'Host-/Hardware-abhaengige WHILE-Bedingung kann beim Import nicht sicher ausgewertet werden',line);else duckySkip(ctx,'WHILE-Block entfernt, weil seine Bedingung erst zur Laufzeit auf der Originalhardware bestimmbar ist',line);i=end;prevAction=null;continue;}let loops=0;while(duckyCondState(cond,ctx,line).value&&!ctx.abort&&!ctx.stopped){duckyExecLines(lines.slice(i+1,end),ctx,depth+1,baseLine+i+1);loops++;if(loops>=10000){duckyWarn(ctx,'WHILE nach 10000 Durchlaeufen abgebrochen',line);break;}}duckyTrace(ctx,line,'WHILE',loops+' Durchlauf/Durchlaeufe');i=end;prevAction=null;continue;}
    if(/^ELSE\b|^END_IF\b|^END_WHILE\b/i.test(t))continue;
    let m=t.match(/^(?:VAR|LET)\s+([\w$#]+)\s*=\s*(.*)$/i)||t.match(/^([\w$#]+)\s*=\s*(.*)$/);
    if(m){if(m[1]==='$_OS'){ctx.vars['$_OS']=ctx.targetOS;}else ctx.vars[m[1]]=duckyScalar(m[2],ctx,line);duckyTrace(ctx,line,'VAR',m[1]+' = '+String(ctx.vars[m[1]]));continue;}
    m=t.match(/^DEFAULT_?DELAY\s+(.+)$/i);if(m){ctx.defaultDelay=Math.max(0,Number(duckyScalar(m[1],ctx,line))||0);duckyTrace(ctx,line,'DEFAULT_DELAY',ctx.defaultDelay+' ms');continue;}
    m=t.match(/^DELAY\s+(.+)$/i);if(m){const d=Math.max(0,Number(duckyScalar(m[1],ctx,line))||0);inoAddDelay(ctx,d);duckyTrace(ctx,line,'DELAY',d+' ms');prevAction={kind:'delay',arg:d};continue;}
    if(/^STRINGLN(?:_BLOCK)?\s*$/i.test(t)){let j=i+1,buf=[];while(j<lines.length&&!/^END_STRINGLN\b/i.test(lines[j].trim()))buf.push(duckyExpandText(lines[j++],ctx));if(j<lines.length){const text=buf.join('\n');inoAddText(ctx,text);inoAddKey(ctx,'ENTER');duckyAddDefaultDelay(ctx);duckyTrace(ctx,line,'STRINGLN_BLOCK',text);i=j;prevAction={kind:'stringln',arg:text};continue;}duckyError(ctx,'Syntaxfehler: STRINGLN_BLOCK ohne END_STRINGLN',line);duckyTrace(ctx,line,'SYNTAX_ERROR','STRINGLN_BLOCK ohne END_STRINGLN');ctx.abort=true;continue;}
    if(/^STRING_BLOCK\s*$/i.test(t)){let j=i+1,buf=[];while(j<lines.length&&!/^END_STRING\b/i.test(lines[j].trim()))buf.push(duckyExpandText(lines[j++],ctx));if(j<lines.length){const text=buf.join('\n');inoAddText(ctx,text);duckyAddDefaultDelay(ctx);duckyTrace(ctx,line,'STRING_BLOCK',text);i=j;prevAction={kind:'string',arg:text};continue;}duckyError(ctx,'Syntaxfehler: STRING_BLOCK ohne END_STRING',line);duckyTrace(ctx,line,'SYNTAX_ERROR','STRING_BLOCK ohne END_STRING');ctx.abort=true;continue;}
    m=t.match(/^STRINGLN\s+(.*)$/i);if(m){const text=duckyExpandText(m[1],ctx);inoAddText(ctx,text);inoAddKey(ctx,'ENTER');duckyAddDefaultDelay(ctx);duckyTrace(ctx,line,'STRINGLN',text);prevAction={kind:'stringln',arg:text};continue;}
    m=t.match(/^STRING(?:\s+(.*))?$/i);if(m){const text=duckyExpandText(m[1]||'',ctx);inoAddText(ctx,text);duckyAddDefaultDelay(ctx);duckyTrace(ctx,line,'STRING',text);prevAction={kind:'string',arg:text};continue;}
    m=t.match(/^REPEAT\s+(.+)$/i);if(m){if(!prevAction){duckyWarn(ctx,'REPEAT ohne vorherige Aktion',line);continue;}const n=Math.max(0,Math.trunc(Number(duckyScalar(m[1],ctx,line))||0));duckyRepeatAction(ctx,prevAction,n,line);duckyTrace(ctx,line,'REPEAT',n+' Wiederholung(en)');continue;}
    m=t.match(/^RAW_HID\s+(.+)$/i);if(m){const r=duckyEvalExpr(m[1],ctx,line),num=Number(r.value),k=r.complete?duckyRawHidKey(num):'';if(k){duckyAddCombo(ctx,[k],line);prevAction={kind:'combo',arg:[k]};continue;}duckyUnsupported(ctx,'RAW_HID '+m[1]+' kann nicht als bekannte Keyboard-HID-Taste abgebildet werden und wurde nicht uebernommen',line);continue;}
    m=t.match(/^HOLD\s+(.+)$/i);if(m){const k=duckyKey(duckyExpandText(m[1],ctx));if(k&&!ctx.held.includes(k)){ctx.held.push(k);duckyWarn(ctx,'HOLD/RELEASE wird im Schritte-Modell als gehaltene Kombination angenaehert',line);}else if(!k)duckyWarn(ctx,'Unbekannte HOLD-Taste '+m[1],line);continue;}
    m=t.match(/^RELEASE(?:\s+(.*))?$/i);if(m){if(!m[1]||/^ALL$/i.test(m[1]))ctx.held=[];else{const k=duckyKey(duckyExpandText(m[1],ctx));ctx.held=ctx.held.filter(x=>x!==k);}continue;}
    m=t.match(/^([A-Za-z_][\w$#]*)\s*\((.*)\)\s*$/);if(m&&ctx.funcs[m[1].toUpperCase()]){const f=ctx.funcs[m[1].toUpperCase()],args=m[2].trim()?m[2].split(',').map(x=>duckyScalar(x,ctx,line)):[],old={};duckyTrace(ctx,line,'CALL','Funktion '+f.name+'()');f.params.forEach((p,n)=>{old[p]={had:Object.prototype.hasOwnProperty.call(ctx.vars,p),v:ctx.vars[p]};ctx.vars[p]=args[n]===undefined?'':args[n];});duckyExecLines(f.lines,ctx,depth+1,f.baseLine);f.params.forEach(p=>{if(!old[p].had)delete ctx.vars[p];else ctx.vars[p]=old[p].v;});prevAction=null;continue;}
    if(/^STOP_PAYLOAD\b/i.test(t)){ctx.stopped=true;duckyWarn(ctx,'STOP_PAYLOAD: nachfolgende Befehle dieses Importpfads wurden nicht importiert',line);continue;}
    if(/^WAIT_FOR_BUTTON_PRESS\b/i.test(t)){if(ctx.mode==='strict'){duckyError(ctx,'WAIT_FOR_BUTTON_PRESS benoetigt den Hardware-Button des Rubber Ducky',line);}else if(ctx.mode==='raw'){duckySkip(ctx,'WAIT_FOR_BUTTON_PRESS im Rohmodus ohne Ersatz uebersprungen',line);}else{const d=Math.max(0,Math.min(60000,Number(ctx.buttonWait)||0));if(d>0){inoAddDelay(ctx,d);prevAction={kind:'delay',arg:d};duckyAdapt(ctx,'WAIT_FOR_BUTTON_PRESS durch feste Pause von '+d+' ms ersetzt',line);}else duckySkip(ctx,'WAIT_FOR_BUTTON_PRESS entfernt (Fallback-Wartezeit = 0 ms)',line);}continue;}
    if(/^WAIT_FOR_(?:CAPS_ON|CAPS_OFF|NUM_ON|NUM_OFF|SCROLL_ON|SCROLL_OFF)\b/i.test(t)){duckyUnsupported(ctx,t.split(/\s+/)[0]+' entfernt: der ESP32-Sequenzimport kann den Keyboard-Lock-Zustand des Hosts nicht verlaesslich abfragen',line);continue;}
    if(/^ATTACKMODE\b/i.test(t)){if(ctx.mode==='strict')duckyError(ctx,'ATTACKMODE ist Rubber-Ducky-Hardwarekonfiguration und nicht als Sequenz darstellbar',line);else if(ctx.mode==='auto')duckyAdapt(ctx,'ATTACKMODE entfernt; der ESP32 arbeitet bereits als konfigurierte HID-Tastatur',line);else duckySkip(ctx,'ATTACKMODE im Rohmodus uebersprungen',line);continue;}
    if(/^LED(?:_[RGB]|_OFF)?\b/i.test(t)){duckyUnsupported(ctx,t.split(/\s+/)[0]+' entfernt: Rubber-Ducky-LED ist nicht Teil der importierten Tastatursequenz',line);continue;}
    if(/^(?:SAVE_HOST_KEY|SAVE_HOST_KEYBOARD_LOCK_STATE|RESTORE_HOST_KEYBOARD_LOCK_STATE|INJECT_MOD|JITTER|EXFIL|HIDE_PAYLOAD|STORAGE)\b/i.test(t)){duckyUnsupported(ctx,t.split(/\s+/)[0]+' entfernt: benoetigt Rubber-Ducky-spezifischen Laufzeit-/Hardwarezustand',line);continue;}
    let rm=t.match(/^RANDOM_DELAY(?:\s+(.+))?$/i);if(rm){if(ctx.mode==='strict'){duckyError(ctx,'RANDOM_DELAY kann im statischen Sequenzmodell nicht originalgetreu wiedergegeben werden',line);}else if(ctx.mode==='raw'){duckySkip(ctx,'RANDOM_DELAY im Rohmodus ohne statischen Ersatz uebersprungen',line);}else{const max=Math.max(0,Number(duckyScalar(rm[1]||'1000',ctx,line))||0),d=Math.round(max/2);inoAddDelay(ctx,d);prevAction={kind:'delay',arg:d};duckyAdapt(ctx,'RANDOM_DELAY deterministisch auf Mittelwert '+d+' ms umgesetzt',line);}continue;}
    rm=t.match(/^RANDOM_(?:CHAR|LETTER|NUMBER)\b/i);if(rm){duckyUnsupported(ctx,t.split(/\s+/)[0]+' entfernt: zufaellige Laufzeit-Eingabe kann beim statischen Import das Verhalten veraendern',line);continue;}
    let toks=t==='+'?['+']:t.replace(/\s*\+\s*/g,' ').replace(/([A-Za-z_]+)-(?=[A-Za-z_])/g,'$1 ').split(/\s+/).filter(Boolean);
    if(duckyAddCombo(ctx,toks,line)){prevAction={kind:'combo',arg:toks};continue;}
    duckyUnsupported(ctx,'Nicht unterstuetzt und uebersprungen: '+(t.length>60?t.slice(0,60)+'...':t),line);
    if(ctx.out.length>10000){duckyWarn(ctx,'Import bei 10000 Schritten begrenzt',line);ctx.abort=true;}
  }
}
function duckyScanExtensions(lines){
  const out=[];
  for(let i=0;i<lines.length;i++){
    const m=String(lines[i]||'').trim().match(/^EXTENSION\s+([A-Za-z_][\w$#-]*)/i);
    if(m&&!out.some(x=>x.name.toUpperCase()===m[1].toUpperCase()))out.push({name:m[1],line:i+1});
  }
  return out;
}
function duckyStaticRiskScan(source,steps){
  const src=String(source||'');
  const findings=[]; const seen=new Set();
  function add(level,kind,detail){const k=level+'|'+kind+'|'+detail;if(!seen.has(k)){seen.add(k);findings.push({level,kind,detail});}}
  if(/\bpowershell(?:\.exe)?\b/i.test(src))add('MEDIUM','SHELL','PowerShell-Aufruf oder -Bezug erkannt');
  if(/\b(?:cmd(?:\.exe)?|wscript|cscript|mshta)\b/i.test(src))add('MEDIUM','SHELL','Windows-Shell/Script-Host-Bezug erkannt');
  if(/webhook/i.test(src))add('HIGH','NETWORK','Webhook-Referenz erkannt');
  if(/https?:\/\//i.test(src))add('MEDIUM','NETWORK','Externe URL erkannt');
  if(/\b(?:Invoke-WebRequest|Invoke-RestMethod|Start-BitsTransfer|curl(?:\.exe)?|wget)\b/i.test(src))add('HIGH','NETWORK','Download-/Netzwerkkommando erkannt');
  if(/\b(?:certutil|FromBase64String|EncodedCommand)\b/i.test(src))add('HIGH','ENCODING','Dekodier-/Encoded-Command-Muster erkannt');
  if(/\b(?:Clear-History|Remove-Item)\b/i.test(src))add('MEDIUM','CLEANUP','Verlaufs-/Dateibereinigung erkannt');
  if(/\bGet-Process\b/i.test(src))add('HIGH','DATA','Prozessauflistung erkannt');
  if(/\b(?:camera|webcam|ffmpeg|dshow)\b/i.test(src))add('HIGH','DEVICE','Kamera-/Videoaufnahme-Bezug erkannt');
  const longText=(steps||[]).filter(x=>x.type==='text'&&String(x.value||'').length>=512);
  if(longText.length)add('MEDIUM','LONG_STRING',longText.length+' sehr lange STRING-Aktion(en) ab 512 Zeichen erkannt');
  const b64=src.match(/[A-Za-z0-9+\/]{600,}={0,2}/g);
  if(b64&&b64.length)add('MEDIUM','ENCODING',b64.length+' Base64-artiger Datenblock/Datenbloecke erkannt (nicht dekodiert)');
  return findings;
}
function duckyActionSummary(steps){
  const c={total:0,text:0,key:0,delay:0,other:0,textChars:0,delayMs:0};
  (steps||[]).forEach(x=>{c.total++;if(x.type==='text'){c.text++;c.textChars+=String(x.value||'').length;}else if(x.type==='key')c.key++;else if(x.type==='delay'){c.delay++;c.delayMs+=Math.max(0,Number(x.value)||0);}else c.other++;});
  return c;
}
function parseDuckyToSteps(source,targetOS,mode,buttonWait){
  let lines=String(source||'').replace(/^\uFEFF/,'').replace(/\r\n?/g,'\n').split('\n');
  const sourceLines=lines.length;
  const extensions=duckyScanExtensions(lines);
  const ctx={out:[],warnings:[],warningSet:new Set(),errors:[],errorSet:new Set(),adapted:[],adaptedSet:new Set(),skipped:[],skippedSet:new Set(),trace:[],defines:{},vars:{},targetOS:(targetOS||'WINDOWS').toUpperCase(),mode:['auto','strict','raw'].includes(mode)?mode:'auto',buttonWait:Math.max(0,Math.min(60000,Number(buttonWait)||0)),defaultDelay:0,held:[],funcs:{},abort:false,stopped:false};ctx.vars['$_OS']=ctx.targetOS;
  let inRem=false;for(let i=0;i<lines.length;i++){const t=lines[i].trim();if(/^REM_BLOCK\b/i.test(t)){inRem=true;lines[i]='';continue;}if(/^END_REM\b/i.test(t)){inRem=false;lines[i]='';continue;}if(inRem)lines[i]='';}
  lines=duckyPreprocess(lines,ctx);const ext=duckyExtractFunctions(lines,ctx);ctx.funcs=ext.funcs;duckyExecLines(ext.main,ctx,0,1);
  const fnNames=Object.keys(ctx.funcs);
  const functionDetails=fnNames.map(n=>({name:ctx.funcs[n].name,params:(ctx.funcs[n].params||[]).slice(),line:Math.max(1,(ctx.funcs[n].baseLine||2)-1)}));
  const libraryOnly=!ctx.out.length&&!ctx.stopped&&fnNames.length>0;
  const libraryInfo=libraryOnly?('Bibliothek/Extension erkannt: '+fnNames.length+' Funktion(en) definiert, aber keine Funktion wurde auf Top-Level aufgerufen. Erwartungsgemaess wurden 0 HID-/Delay-Aktionen erzeugt.') : '';
  if(libraryOnly)duckyTrace(ctx,extensions.length?extensions[0].line:1,'LIBRARY','Definitionen geladen; keine Top-Level-Ausfuehrung');
  const actionSummary=duckyActionSummary(ctx.out);
  const riskFindings=duckyStaticRiskScan(source,ctx.out);
  return {steps:ctx.out,warnings:ctx.warnings,errors:ctx.errors,adapted:ctx.adapted,skipped:ctx.skipped,trace:ctx.trace,functions:fnNames.map(n=>ctx.funcs[n].name),functionDetails,extensions:extensions.map(x=>x.name),libraryOnly,libraryInfo,riskFindings,actionSummary,report:{sourceLines:sourceLines,steps:ctx.out.length,traceEntries:ctx.trace.length,warnings:ctx.warnings.length,errors:ctx.errors.length,adapted:ctx.adapted.length,skipped:ctx.skipped.length,functions:fnNames.length,extensions:extensions.length,mode:ctx.mode,stopped:ctx.stopped,aborted:ctx.abort,libraryOnly:libraryOnly}};
}

function detectImportType(name,source){
  if(/\.(ino|cpp|h)$/i.test(name))return 'ino';
  if(/\b(?:Keyboard\.|setup\s*\(|void\s+setup\b)/.test(source))return 'ino';
  if(/\b(?:STRING|DELAY|REM|EXTENSION|FUNCTION|DEFAULT_?DELAY|GUI|CTRL|ALT)\b/i.test(source))return 'ducky';
  return 'ducky';
}
function importSequenceFile(file){
  const input=document.getElementById('ino-file'),info=document.getElementById('ino-import-info');if(!file)return;
  const reader=new FileReader();reader.onload=function(){
    try{
      const source=String(reader.result||''),parsed=parseInoToSteps(source);
      if(!parsed.steps.length)throw new Error('Keine unterstuetzten Keyboard-Befehle gefunden.');
      let replace=true;if(steps.length)replace=confirm('Es sind bereits '+steps.length+' Schritte vorhanden.\n\nOK = vorhandene Schritte ERSETZEN\nAbbrechen = Import ANHAENGEN');
      steps=replace?parsed.steps:steps.concat(parsed.steps);renderSteps();saveState();
      const warn=parsed.warnings&&parsed.warnings.length?' | '+parsed.warnings.length+' Hinweis(e): '+parsed.warnings.slice(0,5).join(' · ')+(parsed.warnings.length>5?' · ...':''):'';
      if(info){info.textContent=file.name+': '+parsed.steps.length+' Schritte aus .INO importiert.'+warn;info.title=(parsed.warnings||[]).join('\n');info.style.color=parsed.warnings&&parsed.warnings.length?'#fbbf24':'var(--green)';}
      showToast(parsed.steps.length+' Schritte aus .INO geladen!');
    }catch(e){if(info){info.textContent='Import fehlgeschlagen: '+e.message;info.style.color='var(--red)';}showToast('Import fehlgeschlagen');}
    if(input)input.value='';
  };reader.onerror=function(){showToast('Datei konnte nicht gelesen werden.');if(input)input.value='';};reader.readAsText(file);
}

let duckyLastParsed=null;
let duckyLoadedName='';
let duckyAnalyzeTimer=0;
function clearDuckyEditor(){
  const src=document.getElementById('ducky-source'),rep=document.getElementById('ducky-report');if(src)src.value='';
  duckyLoadedName='';duckyLastParsed=null;updateDuckyImportTools(null);if(rep){rep.textContent='Noch keine Datei geladen.';rep.style.color='var(--muted)';}
}
function scheduleDuckyAnalyze(){clearTimeout(duckyAnalyzeTimer);duckyAnalyzeTimer=setTimeout(analyzeDuckyEditor,250);}
function loadDuckyFile(file){
  const input=document.getElementById('ducky-file');if(!file)return;
  const reader=new FileReader();reader.onload=function(){const src=document.getElementById('ducky-source');if(src)src.value=String(reader.result||'');duckyLoadedName=file.name||'';duckyLastParsed=null;analyzeDuckyEditor();if(input)input.value='';};
  reader.onerror=function(){showToast('DuckyScript-Datei konnte nicht gelesen werden.');if(input)input.value='';};reader.readAsText(file);
}
function formatDuckyImportReport(parsed){
  const r=parsed.report||{},w=parsed.warnings||[],e=parsed.errors||[],a=parsed.adapted||[],s=parsed.skipped||[];
  const name=duckyLoadedName||'Editor';
  let out=name+'\n';
  if(r.libraryOnly)out+='Library/Extension erkannt: '+(r.functions||0)+' Funktion(en), keine direkten Top-Level-Schritte.\nWaehle unten eine Funktion und uebernimm sie als Sequenz.';
  else out+=(r.steps||0)+' direkte Schritt(e) bereit'+((r.functions||0)?' · '+r.functions+' Funktion(en) zusaetzlich verfuegbar':'')+'.';
  if(e.length)out+='\n\nFEHLER ('+e.length+'):\n- '+e.slice(0,8).join('\n- ')+(e.length>8?'\n- ...':'');
  if(a.length)out+='\n\nANGEPASST ('+a.length+'):\n- '+a.slice(0,6).join('\n- ')+(a.length>6?'\n- ...':'');
  if(s.length)out+='\n\nUEBERSPRUNGEN ('+s.length+'):\n- '+s.slice(0,6).join('\n- ')+(s.length>6?'\n- ...':'');
  if(w.length)out+='\n\nHINWEISE ('+w.length+'):\n- '+w.slice(0,6).join('\n- ')+(w.length>6?'\n- ...':'');
  if(!e.length&&!a.length&&!s.length&&!w.length)out+='\nParserstatus: OK.';
  return out;
}
function updateDuckyImportTools(parsed){
  const direct=document.getElementById('ducky-direct-tools'),title=document.getElementById('ducky-direct-title'),note=document.getElementById('ducky-direct-note');
  const wrap=document.getElementById('ducky-function-tools'),sel=document.getElementById('ducky-function-select'),args=document.getElementById('ducky-function-args'),fnNote=document.getElementById('ducky-function-note');
  const n=parsed&&parsed.steps?parsed.steps.length:0;
  if(direct)direct.style.display=n?'block':'none';
  if(title&&n)title.textContent=n+' direkte Schritt(e) bereit';
  if(note&&n)note.textContent='Diese Aktionen stammen aus dem Top-Level der Datei und koennen direkt uebernommen werden.';
  const f=(parsed&&parsed.functionDetails)||[];
  const previousSelection=sel?sel.value:'';
  if(wrap)wrap.style.display=f.length?'block':'none';
  if(sel){
    sel.innerHTML='';
    f.forEach(x=>{const o=document.createElement('option');o.value=x.name;o.textContent=x.name+'('+(x.params||[]).join(', ')+')';sel.appendChild(o);});
    if(previousSelection&&f.some(x=>x.name===previousSelection))sel.value=previousSelection;
  }
  const updateArgs=()=>{const x=f.find(y=>sel&&y.name===sel.value)||f[0];if(args&&x){args.value='';args.placeholder=(x.params||[]).length?('Argumente: '+x.params.join(', ')):'Keine Argumente erforderlich';}if(fnNote&&x)fnNote.textContent=(x.params||[]).length?('Erwartete Parameter: '+x.params.join(', ')+'. Die Funktion wird beim Import mit diesen Argumenten expandiert.'):'Keine Parameter erforderlich. Die Funktion wird direkt in normale Sequenzschritte expandiert.';};
  if(sel){sel.onchange=updateArgs;if(f.length)updateArgs();}
}
function analyzeDuckyEditor(){
  const src=document.getElementById('ducky-source'),target=document.getElementById('ducky-target-os'),mode=document.getElementById('ducky-compat-mode'),bw=document.getElementById('ducky-button-wait'),box=document.getElementById('ducky-report');
  if(!src||!target||!mode||!bw||!box)return null;
  if(!src.value.trim()){duckyLastParsed=null;updateDuckyImportTools(null);box.textContent='Noch keine Datei geladen.';box.style.color='var(--muted)';return null;}
  try{
    duckyLastParsed=parseDuckyToSteps(src.value,target.value,mode.value,bw.value);
    box.textContent=formatDuckyImportReport(duckyLastParsed);
    box.style.color=duckyLastParsed.errors.length?'var(--red)':((duckyLastParsed.warnings.length||duckyLastParsed.skipped.length||duckyLastParsed.adapted.length)?'#fbbf24':'var(--green)');
    updateDuckyImportTools(duckyLastParsed);return duckyLastParsed;
  }catch(e){duckyLastParsed=null;updateDuckyImportTools(null);box.textContent='Parserfehler: '+e.message;box.style.color='var(--red)';return null;}
}
function confirmDuckyPartial(parsed,label){
  if(!parsed||!parsed.errors||!parsed.errors.length)return true;
  const preview=parsed.errors.slice(0,6).join('\n')+(parsed.errors.length>6?'\n... und '+(parsed.errors.length-6)+' weitere.':'');
  return confirm(label+' enthaelt '+parsed.errors.length+' Parserfehler.\n\n'+preview+'\n\nDen darstellbaren Teil trotzdem uebernehmen?');
}
function applyImportedDuckySteps(newSteps,append,label,parsed){
  if(!newSteps||!newSteps.length){showToast('Keine Sequenzschritte erzeugt.');return false;}
  const total=(append?steps.length:0)+newSteps.length;if(total>128){showToast('Import zu gross: '+total+' Schritte, Controller-Limit 128.');return false;}
  if(parsed&&!confirmDuckyPartial(parsed,label||'Import'))return false;
  steps=append?steps.concat(newSteps):newSteps.slice();renderSteps();saveState();switchTab('seq');
  const a=parsed&&parsed.adapted?parsed.adapted.length:0,s=parsed&&parsed.skipped?parsed.skipped.length:0;
  showToast(newSteps.length+' Schritt(e) '+(append?'angehaengt':'geladen')+(a||s?' · '+a+' angepasst · '+s+' uebersprungen':'')+'.');return true;
}
function applyDuckyEditor(append){
  const parsed=analyzeDuckyEditor();if(!parsed)return;
  if(!parsed.steps.length){if(parsed.functions&&parsed.functions.length)showToast('Keine direkten Schritte. Waehle unten eine Funktion.');else showToast('Keine ausfuehrbaren DuckyScript-Schritte gefunden.');return;}
  applyImportedDuckySteps(parsed.steps,append,'DuckyScript-Import',parsed);
}
function duckyStepsPrefix(base,full){
  if(!base||!full||full.length<base.length)return false;
  for(let i=0;i<base.length;i++){const a=base[i],b=full[i];if(!a||!b||a.type!==b.type||String(a.value)!==String(b.value)||String(a.speed===undefined?'':a.speed)!==String(b.speed===undefined?'':b.speed))return false;}
  return true;
}
function importSelectedDuckyFunction(append){
  // Auswahl VOR der erneuten Analyse sichern. analyzeDuckyEditor() baut die
  // Funktions-Selectbox neu auf und wuerde sie sonst auf den ersten Eintrag
  // zuruecksetzen (z.B. immer Invoke_WebRequest).
  const sel=document.getElementById('ducky-function-select'),argEl=document.getElementById('ducky-function-args');
  const selectedName=sel?sel.value:'';
  const selectedArgs=String(argEl?argEl.value:'').trim();
  const base=analyzeDuckyEditor();
  if(!base||!selectedName)return;
  const fn=(base.functionDetails||[]).find(x=>x.name===selectedName);if(!fn){showToast('Funktion nicht gefunden.');return;}
  // Sichtbare Auswahl nach dem Rebuild wiederherstellen.
  if(sel)sel.value=selectedName;
  if(argEl)argEl.value=selectedArgs;
  const args=selectedArgs;
  if((fn.params||[]).length&&!args){showToast('Argumente erforderlich: '+fn.params.join(', '));if(argEl)argEl.focus();return;}
  const src=document.getElementById('ducky-source').value+'\n\n'+fn.name+'('+args+')';
  const target=document.getElementById('ducky-target-os').value,mode=document.getElementById('ducky-compat-mode').value,buttonWait=document.getElementById('ducky-button-wait').value;
  try{
    const p=parseDuckyToSteps(src,target,mode,buttonWait);
    let fnSteps=p.steps;
    if(duckyStepsPrefix(base.steps,p.steps))fnSteps=p.steps.slice(base.steps.length);
    if(!fnSteps.length){showToast('Die Funktion erzeugt keine darstellbaren Sequenzschritte.');return;}
    applyImportedDuckySteps(fnSteps,append,'Funktion '+fn.name+'()',p);
  }catch(e){showToast('Funktionsimport fehlgeschlagen: '+e.message);}
}

function addStep(){const t=document.getElementById('step-type').value;let step={type:t},v;if(t==='text'){v=document.getElementById('step-text').value;if(!v){showToast('Bitte Text!');return;}const spd=parseInt(document.getElementById('step-speed').value);step.value=v;step.speed=spd;step.label='"'+(v.length>32?v.slice(0,32)+'...':v)+'"';document.getElementById('step-text').value='';}else if(t==='key'){const sel=document.getElementById('step-key').value;if(sel==='__custom__'){
    v=document.getElementById('step-custom').value.trim();
    if(!v){showToast('Taste waehlen!');return;}
    step.value=v; step.label=prettyKeyLabel(v);
  } else {
    v=sel;
    // Zeige den lesbaren Text aus der Option statt des internen Werts
    const keyEl=document.getElementById('step-key');
    const opt=keyEl?keyEl.options[keyEl.selectedIndex]:null;
    step.value=v;
    step.label=prettyKeyLabel(v);
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
  document.getElementById('edit-val').value=s.type==='key' ? prettyKeyLabel(s.value) : s.value;
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
  const val=document.getElementById('edit-val').value.trim();
  if(!val){showToast('Bitte Wert eingeben!');return;}
  const storedVal=steps[editIdx].type==='key' ? normalizeKeyValue(val) : val;
  steps[editIdx].value=storedVal;
  steps[editIdx].label=steps[editIdx].type==='text'?'"'+(val.length>32?val.slice(0,32)+'...':val)+'"':steps[editIdx].type==='delay'?val+'ms warten':prettyKeyLabel(storedVal);
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
function updateClipSpeedInfo(){
  const v=Math.max(0,Math.min(500,parseInt(clipCustomSpeedMs)||0));
  const el=document.getElementById('clip-speed-info');
  if(el) el.textContent=(v===0?'Maximum':v+' ms / Zeichen')+' · nur Copy+Paste';
}
function setClipCustomSpeed(v){
  clipCustomSpeedMs=Math.max(0,Math.min(500,parseInt(v)||0));
  const el=document.getElementById('clip-custom-speed-val');
  if(el) el.value=clipCustomSpeedMs;
  updateClipSpeedInfo();
  saveState();
}
function changeClipSpeed(d){ setClipCustomSpeed(clipCustomSpeedMs+d); }
function setClipMode(on){
  clipMode=on;
  document.getElementById('clip-off').classList.toggle('active',!on);
  document.getElementById('clip-on').classList.toggle('active',on);
  const desc=document.getElementById('clip-desc');
  const block=document.getElementById('clip-speed-block');
  if(desc){
    if(on){
      desc.innerHTML='<b>&#128203; Copy+Paste:</b> Der Text wird schnell über die Windows-Zwischenablage eingefügt. <b>0 ms</b> = maximal schnell; der Standardwert ist bewusst etwas langsamer für sichere Eingabe. <b>Nur Windows.</b>';
      if(block) block.style.display='';
      updateClipSpeedInfo();
    } else {
      desc.innerHTML='<b>&#9000; Normal tippen:</b> Jedes Zeichen wird einzeln als Tastendruck gesendet. Die normale Tippgeschwindigkeit gilt.';
      if(block) block.style.display='none';
    }
  }
  saveState();
}

let burstOpen = false;
function updateBurstPreview(){
  const p = document.getElementById('burst-preview');
  if(p) p.textContent = burstOpen ? burstSize+' Zeichen' : (burstSize!==1?burstSize+' Zeichen':'');
}
function runSeq(){if(!steps.length){showToast('Keine Schritte!');return;}const payload={steps:steps.map(s=>({type:s.type,value:s.value,speed:s.speed!==undefined?s.speed:-1})),speed:speedIndex,customMs:customSpeedMs,clipCustomMs:clipCustomSpeedMs,burst:burstEnabled?burstSize:1,clipMode:clipMode,repeat:repeatMode==='loop',interval:loopInterval};fetch('/run',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(payload)}).then(r=>{if(r.ok){running=true;setUIRunning(true);startPoll();}});}
function stopSeq(){fetch('/stop',{method:'POST'}).then(()=>{running=false;stopPoll();setUIRunning(false);setStatus('stopped','Gestoppt');});}
function setUIRunning(on){document.getElementById('btn-start').disabled=on;document.getElementById('btn-stop').disabled=!on;if(on)setStatus('running','Sende…');}
function setStatus(state,text){document.getElementById('sdot').className='sdot '+state;document.getElementById('s-text').textContent=text;}
function startPoll(){pollTimer=setInterval(()=>{fetch('/status').then(r=>r.json()).then(d=>{document.getElementById('s-count').textContent=d.count;if(!d.running&&running){running=false;stopPoll();setUIRunning(false);setStatus('done','OK Fertig');}}).catch(()=>{});},700);}
function stopPoll(){clearInterval(pollTimer);pollTimer=null;}
let vkMods={shift:false,ctrl:false,alt:false,altgr:false};
function vkPane(name){
  const write=name==='write';
  const wp=document.getElementById('vk-pane-write'), pp=document.getElementById('vk-pane-pc');
  const wt=document.getElementById('vk-tab-write'), pt=document.getElementById('vk-tab-pc');
  if(wp) wp.classList.toggle('active',write); if(pp) pp.classList.toggle('active',!write);
  if(wt) wt.classList.toggle('active',write); if(pt) pt.classList.toggle('active',!write);
}
function vkToggle(mod){
  vkMods[mod]=!vkMods[mod];
  const ids={shift:'vk-shift',ctrl:'vk-ctrl',alt:'vk-alt',altgr:'vk-altgr'};
  const el=document.getElementById(ids[mod]);
  if(el) el.classList.toggle('active',vkMods[mod]);
  const active=Object.keys(vkMods).filter(k=>vkMods[k]);
  const badge=document.getElementById('vk-mod-status');
  const label=document.getElementById('vk-mod-text');
  if(badge) badge.style.display=active.length?'inline-flex':'none';
  if(label) label.textContent=active.map(k=>({shift:'Shift',ctrl:'Ctrl',alt:'Alt',altgr:'AltGr'})[k]).join(' + ');
}
function vkKey(key){
  const combo=[];
  if(vkMods.ctrl) combo.push('LCTRL');
  if(vkMods.alt) combo.push('LALT');
  if(vkMods.altgr) combo.push('RALT');
  if(vkMods.shift) combo.push('LSHIFT');
  combo.push(key);
  quickKey(combo.join('+'));
  if(vkMods.shift) vkToggle('shift');
}
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
  // Immer zuerst lokal sichern. So bleibt selbst bei einem WLAN-/Flash-Fehler
  // eine Browser-Kopie erhalten.
  try { localStorage.setItem('esp32kb_templates', json); } catch(e){}

  // Promise zurueckgeben, damit die UI erst nach echter ESP32-Bestaetigung
  // "gespeichert" meldet.
  return fetch('/templates/save', {
    method: 'POST',
    headers: {'Content-Type': 'application/json'},
    body: json
  }).then(async function(r){
    const msg = await r.text().catch(function(){ return ''; });
    if(!r.ok) throw new Error(msg || ('HTTP '+r.status));
    return true;
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
  renderTemplates();
  saveTemplates().then(function(){
    newTmpl(); // Erst nach bestaetigtem Flash-Speichern zuruecksetzen
    showToast('Vorlage gespeichert!');
  }).catch(function(err){
    console.error('Vorlage speichern fehlgeschlagen:', err);
    showToast('Speichern fehlgeschlagen: '+(err && err.message ? err.message : 'ESP32 nicht erreichbar'));
  });
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
let autoCfg = { enabled:false, tmplId:null, delay:5000, repeat:1, speed:20, burst:1 };
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
function clearAll(){if(!confirm('Alle Schritte und Einstellungen loeschen?'))return;steps=[];repeatMode='once';loopInterval=1000;speedIndex=2;burstSize=1;clipMode=false;customSpeedMs=0;localStorage.removeItem('esp32kb');renderSteps();setRepeat('once');setSpeed(2);setClipMode(false);document.getElementById('interval-val').textContent='1.0 s';document.getElementById('burst-val').textContent='1 Zeichen';const csv2=document.getElementById('custom-speed-val'); if(csv2) csv2.value=0; burstOpen=false; const bb=document.getElementById('burst-block'); if(bb) bb.style.display='none'; updateBurstPreview();showToast('Alles geloescht!');}
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

// -- Vorlagen-Dateispeicher (LittleFS) -------------------------
// Preferences/NVS ist fuer einzelne Strings nur begrenzt geeignet. Lange
// Vorlagen werden deshalb als JSON-Datei gespeichert.
static const char* TEMPLATE_FILE = "/templates.json";
static const char* TEMPLATE_TMP  = "/templates.tmp";
static const char* TEMPLATE_BAK  = "/templates.bak";
static const size_t TEMPLATE_MAX_BYTES = 256 * 1024; // Schutz vor RAM-/Flash-Ueberlauf
bool templatesFsReady = false;

String readTemplatesStorage(){
  if(templatesFsReady && LittleFS.exists(TEMPLATE_FILE)){
    File f = LittleFS.open(TEMPLATE_FILE, "r");
    if(f){
      String data = f.readString();
      f.close();
      data.trim();
      if(data.length() >= 2) return data;
    }
  }

  // Rueckwaertskompatibilitaet fuer Vorlagen aus <= 1.1.6.
  prefs.begin("templates", true);
  String legacy = prefs.getString("data", "[]");
  prefs.end();
  return legacy.length() ? legacy : "[]";
}

bool writeTemplatesStorage(const String &data, String &err){
  if(!templatesFsReady){
    err = "Dateisystem nicht verfuegbar";
    return false;
  }
  if(data.length() > TEMPLATE_MAX_BYTES){
    err = "Vorlagen sind groesser als 256 KB";
    return false;
  }

  LittleFS.remove(TEMPLATE_TMP);
  File f = LittleFS.open(TEMPLATE_TMP, "w");
  if(!f){
    err = "Temp-Datei konnte nicht geoeffnet werden";
    return false;
  }
  size_t written = f.print(data);
  f.flush();
  f.close();
  if(written != data.length()){
    LittleFS.remove(TEMPLATE_TMP);
    err = "Flash voll oder Schreibfehler";
    return false;
  }

  // Atomarer Wechsel mit Backup: alte Datei erst nach erfolgreichem Schreiben
  // ersetzen, bei Rename-Fehler moeglichst wiederherstellen.
  LittleFS.remove(TEMPLATE_BAK);
  bool hadOld = LittleFS.exists(TEMPLATE_FILE);
  if(hadOld && !LittleFS.rename(TEMPLATE_FILE, TEMPLATE_BAK)){
    LittleFS.remove(TEMPLATE_TMP);
    err = "Alte Vorlagendatei konnte nicht gesichert werden";
    return false;
  }
  if(!LittleFS.rename(TEMPLATE_TMP, TEMPLATE_FILE)){
    if(hadOld && LittleFS.exists(TEMPLATE_BAK)) LittleFS.rename(TEMPLATE_BAK, TEMPLATE_FILE);
    LittleFS.remove(TEMPLATE_TMP);
    err = "Neue Vorlagendatei konnte nicht aktiviert werden";
    return false;
  }
  LittleFS.remove(TEMPLATE_BAK);
  return true;
}

void migrateLegacyTemplates(){
  if(!templatesFsReady || LittleFS.exists(TEMPLATE_FILE)) return;
  prefs.begin("templates", true);
  String legacy = prefs.getString("data", "[]");
  prefs.end();
  if(legacy.length() <= 2) return;

  String err;
  if(writeTemplatesStorage(legacy, err)){
    Serial.printf("Vorlagen aus NVS nach LittleFS migriert: %u bytes\n", (unsigned)legacy.length());
  } else {
    Serial.println("Vorlagen-Migration fehlgeschlagen: " + err);
  }
}

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
  String data = readTemplatesStorage();
  server.send(200, "application/json", data);
}

void handleSaveTemplates(){
  if(server.method() != HTTP_POST){ server.send(405,"text/plain","Method Not Allowed"); return; }
  String body = server.arg("plain");
  if(body.length() == 0){ server.send(400,"text/plain","Empty"); return; }
  if(body.length() > TEMPLATE_MAX_BYTES){ server.send(413,"text/plain","Vorlagen groesser als 256 KB"); return; }

  // Minimaler Plausibilitaetscheck: Die API erwartet ein JSON-Array.
  String trimmed = body;
  trimmed.trim();
  if(!trimmed.startsWith("[") || !trimmed.endsWith("]")){
    server.send(400,"text/plain","Ungueltige Vorlagendaten");
    return;
  }

  String err;
  if(!writeTemplatesStorage(body, err)){
    Serial.println("Vorlagen speichern FEHLER: " + err);
    server.send(507,"text/plain",err);
    return;
  }
  Serial.printf("Vorlagen gespeichert: %u bytes (LittleFS)\n", (unsigned)body.length());
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

  // Clipboard-Modus + eigene Geschwindigkeit
  g_clipMode = body.indexOf("\"clipMode\":true") >= 0;
  g_clipCustomDelay = -1;
  int cdi = body.indexOf("\"clipCustomMs\":");
  if(cdi>=0){
    cdi += 15;
    int cde=body.indexOf(",",cdi); if(cde<0)cde=body.indexOf("}",cdi);
    if(cde>cdi) g_clipCustomDelay=constrain(body.substring(cdi,cde).toInt(),0,500);
  }
  Serial.printf("Run: delay=%d burst=%d clip=%d clipDelay=%d\n", g_typeDelay, g_burstSize, g_clipMode, g_clipCustomDelay);

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

  // Vorlagen-Dateisystem initialisieren. Bei einem fabrikneuen/inkompatiblen
  // Dateisystem darf LittleFS einmalig formatieren.
  templatesFsReady = LittleFS.begin(false);
  if(!templatesFsReady){
    Serial.println("LittleFS Mount fehlgeschlagen - versuche Formatierung...");
    templatesFsReady = LittleFS.begin(true);
  }
  if(templatesFsReady){
    Serial.printf("LittleFS bereit: %u / %u bytes belegt\n",
                  (unsigned)LittleFS.usedBytes(), (unsigned)LittleFS.totalBytes());
    migrateLegacyTemplates();
  } else {
    Serial.println("WARNUNG: LittleFS nicht verfuegbar; lange Vorlagen koennen nicht gespeichert werden.");
  }

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

// Autostart: Vorlage aus LittleFS laden und ausfuehren
void loadAndRunAutoTemplate(){
  String data = readTemplatesStorage();
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
