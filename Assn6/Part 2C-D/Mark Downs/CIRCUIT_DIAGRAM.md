# Circuit Diagram - ESP8266 Integrated System

## Complete Wiring Diagram (ASCII Art)

```
                                   ESP8266 NodeMCU
                          ┌─────────────────────────────────┐
                          │                                 │
        [Switch 1]        │  GPIO0  (D3) ●                  │
           │              │               │ 10kΩ            │
           ├──────────────┼───────────────┤                 │
           │              │               │                 │
          GND             │              VCC                │
           │              │                                 │
           │              │                                 │
        [Switch 2]        │  GPIO16 (D0) ●                  │
           │              │               │ 10kΩ            │
           ├──────────────┼───────────────┤                 │
           │              │               │                 │
          GND             │              VCC                │
           │              │                                 │
           │              │                                 │
                          │  GPIO14 (D5) ●──────────┐       │
                          │                         │       │
        [DHT11]           │                    DHT11 Data   │
          VCC ────────────┼─────────────────── VCC          │
          GND ────────────┼─────────────────── GND          │
          Data ───────────┼─────────────────────┘           │
                          │                                 │
                          │                                 │
        [LED1]            │  GPIO12 (D6) ●                  │
          Anode ──220Ω────┼────────┐                        │
          Cathode ────────┼───────GND                       │
                          │                                 │
        [LED2]            │  GPIO13 (D7) ●                  │
          Anode ──220Ω────┼────────┐                        │
          Cathode ────────┼───────GND                       │
                          │                                 │
                          │                                 │
        [RGB LED]         │  GPIO15 (D8) ●                  │
          Red ────330Ω────┼────────┐                        │
                          │        │                        │
                          │  GPIO4  (D2) ●                  │
          Green ───330Ω───┼────────┐                        │
                          │        │                        │
                          │  GPIO5  (D1) ●                  │
          Blue ────330Ω───┼────────┐                        │
                          │        │                        │
          Cathode ────────┼───────GND                       │
                          │                                 │
                          │         ●  VIN (5V input)       │
                          │         ●  3V3 (3.3V output)    │
                          │         ●  GND (Ground)         │
                          │         ●  RST (Reset)          │
                          │                                 │
                          └─────────────────────────────────┘

Power Supply: USB Micro (5V) or Battery Pack (5V USB output)
```

## Breadboard Layout Diagram

```
                        Breadboard Top View
         
     +5V Rail ═══════════════════════════════════════ (Red)
      │
      │  10kΩ    10kΩ
      ├──/\/\────●──[SW1]──●──GND
      │          │          │
      ├──/\/\────●──[SW2]──●──GND
      │
      │                             ESP8266 NodeMCU
      │                        ┌─────────────────────┐
      │                        │ D0  D1  D2  D3  D4  │
      │                        │ ●   ●   ●   ●   ●   │
      │                        │ │   │   │   │   │   │
      │                        │ │   │   │   │   │   │
DHT11 ├────VCC────────────────▶│ │   │   │   │   │   │
Sensor│                        │ │   │   │   │   │   │
      │    Data──────────────▶│ │   │   │   │   D5  │
      │                        │ │   │   │   │   ●   │
      │                        │ │   │   │   │   │   │
      │                    RGB─┼─┤   │   │   │   │   │
      │                   Blue ├─┼───┘   │   │   │   │
      │                   Green├─┼───────┘   │   │   │
      │                   Red  ├─┼───────────┘   │   │
      │                        │ SW2             SW1 │
      │                        │ │                │  │
      │                        │ D6  D7  D8  GND VIN│
      │                        │ ●   ●   ●   ●   ●  │
      │                        └─┼───┼───┼───┼───┼──┘
      │                          │   │   │   │   │
      │                     LED1─┘   │   │   │   ▲ 5V Input
      │                         LED2─┘   │   │
      │                             RGB Red   │
      │                                      GND
      │
     GND Rail ═══════════════════════════════════════ (Blue)
                                ║
                            Common Ground
```

## Component Pinout Diagrams

### DHT11 Sensor (3-pin module)
```
  ┌─────────┐
  │  DHT11  │
  │  [===]  │
  └──┬─┬─┬──┘
     │ │ │
     1 2 3
     │ │ │
     │ │ └── GND
     │ └──── DATA (Signal)
     └────── VCC (3.3V or 5V)
```

### Standard LED (5mm)
```
        Anode (+)
           ║
           ║  ← Longer leg
         ──╨──
        │░░░░│
        │░░░░│  ← LED body
        │░░░░│
         ──┬──
           ║
           ║  ← Shorter leg, flat edge
        Cathode (-)
```

### RGB LED (Common Cathode)
```
    Looking at flat edge of LED:
    
       1    2    3    4
       │    │    │    │
       R   GND   G    B
       │    │    │    │
      Red Common Green Blue
           (Longest leg)
    
    Pin 1: Red (left)
    Pin 2: Common Cathode (2nd, longest)
    Pin 3: Green (3rd)
    Pin 4: Blue (rightmost)
```

### Push Button Switch
```
    Top View:          Side View:
    
    ┌───────┐         Not Pressed:  Pressed:
    │ ●   ● │          ●    ●       ●────●
    │       │          │    │       │    │
    │ ●   ● │          ●    ●       ●────●
    └───────┘
    
    Internal connections:
    • Diagonal pins always connected
    • Pressing connects top to bottom
```

## Power Distribution

```
    USB/Battery (5V)
         │
         ▼
    ┌─────────┐
    │ ESP8266 │
    │  VIN    │
    └────┬────┘
         │
         ├─────▶ Internal 3.3V Regulator
         │         │
         │         ├─────▶ 3V3 Pin (Output)
         │         │         │
         │         │         ├─────▶ DHT11 VCC
         │         │         └─────▶ Pull-up resistors
         │         │
         │         └─────▶ ESP8266 Core (3.3V)
         │
         └─────▶ Ground (GND)
                   │
                   ├─────▶ All component grounds
                   ├─────▶ LED cathodes
                   ├─────▶ RGB cathode
                   └─────▶ Switch grounds

Total Current Draw:
• ESP8266: 80-300mA (varies with WiFi activity)
• DHT11: 0.5-2.5mA
• LED1/LED2: 5-10mA each (with 220Ω resistors)
• RGB LED: 15-60mA total (depends on color mix)
• Total: ~100-380mA
```

## GPIO Usage Summary

```
GPIO  │ Label │ Function          │ Direction │ Notes
──────┼───────┼───────────────────┼───────────┼──────────────────────
0     │ D3    │ Switch 1 Input    │ INPUT     │ Boot mode, pull-up
16    │ D0    │ Switch 2 Input    │ INPUT     │ No interrupts, weak pull-up
14    │ D5    │ DHT11 Data        │ BIDIR     │ OneWire protocol
12    │ D6    │ LED1 Output       │ OUTPUT    │ Visual feedback
13    │ D7    │ LED2 Output       │ OUTPUT    │ Visual feedback
15    │ D8    │ RGB Red (PWM)     │ OUTPUT    │ Boot mode, pull-down
4     │ D2    │ RGB Green (PWM)   │ OUTPUT    │ I2C SDA
5     │ D1    │ RGB Blue (PWM)    │ OUTPUT    │ I2C SCL
```

## Resistor Color Codes

```
10kΩ (Brown-Black-Orange-Gold):
  │    │      │       │
  1    0   × 1kΩ   ±5%
  
220Ω (Red-Red-Brown-Gold):
  │   │     │      │
  2   2  × 10Ω  ±5%
  
330Ω (Orange-Orange-Brown-Gold):
  │     │       │       │
  3     3    × 10Ω   ±5%
```

## Connection Verification Checklist

Use a multimeter to verify:

**Power Rails:**
- [ ] VCC rail = 3.3V
- [ ] GND rail = 0V (common ground)

**Switch 1 (GPIO0):**
- [ ] When not pressed: GPIO0 = 3.3V
- [ ] When pressed: GPIO0 = 0V

**Switch 2 (GPIO16):**
- [ ] When not pressed: GPIO16 = 3.3V
- [ ] When pressed: GPIO16 = 0V

**DHT11:**
- [ ] VCC pin = 3.3V (or 5V if using 5V module)
- [ ] GND pin = 0V
- [ ] DATA pin = 3.3V when idle

**LEDs (when GPIO HIGH):**
- [ ] LED1 anode = 3.3V, cathode = 0V
- [ ] LED2 anode = 3.3V, cathode = 0V
- [ ] RGB pins = 0-3.3V (PWM signal)

**Resistors:**
- [ ] All resistors have correct values
- [ ] No short circuits across resistors
- [ ] Clean solder joints (if soldered)

## Common Wiring Mistakes

❌ **Mistake 1**: LED backwards (won't light, may damage LED)
✅ **Fix**: Long leg to GPIO (through resistor), short leg to GND

❌ **Mistake 2**: RGB LED wrong pin order
✅ **Fix**: Longest leg (2nd from left) to GND, verify pinout

❌ **Mistake 3**: Missing pull-up resistors on switches
✅ **Fix**: Connect 10kΩ between GPIO and VCC for each switch

❌ **Mistake 4**: GPIO15 floating at boot (won't boot)
✅ **Fix**: Add 10kΩ pull-down or ensure RGB red circuit pulls low

❌ **Mistake 5**: DHT11 powered by 5V with 3.3V GPIO
✅ **Fix**: Use 3.3V for DHT11 or add level shifter

❌ **Mistake 6**: All grounds not connected
✅ **Fix**: Connect all GND points to common breadboard ground rail

❌ **Mistake 7**: No current limiting resistors for LEDs
✅ **Fix**: Always use 220-330Ω resistors in series with LEDs

❌ **Mistake 8**: Breadboard connection loose
✅ **Fix**: Push wires firmly, check for oxidation, use new breadboard

## Safety Warnings

⚠️ **Never:**
- Short circuit VCC to GND
- Apply voltage >3.6V to GPIO pins
- Draw >12mA per GPIO pin
- Exceed 200mA total GPIO current
- Use damaged components
- Work on live circuits

⚠️ **Always:**
- Use current limiting resistors with LEDs
- Check polarity before powering on
- Verify connections before applying power
- Use a regulated 5V power supply
- Disconnect power when modifying wiring
- Keep liquids away from circuit

## Troubleshooting with Multimeter

**No power:**
1. Check USB cable and connector
2. Measure VIN pin (should be ~5V)
3. Measure 3V3 pin (should be 3.3V)
4. Check continuity of ground rail

**LED won't light:**
1. Check LED polarity with diode test mode
2. Measure voltage across LED (should be ~2V when on)
3. Check resistor value and placement
4. Verify GPIO output (3.3V when HIGH)

**Switch not working:**
1. Check continuity when pressed (should be 0Ω)
2. Measure pull-up resistor (should be ~10kΩ)
3. Check GPIO voltage when not pressed (3.3V)
4. Verify GPIO goes to 0V when pressed

**Sensor not responding:**
1. Check VCC = 3.3V and GND = 0V
2. Verify DATA line connection
3. Check for continuity in wires
4. Measure current draw (<3mA when active)

```

---

**Document Version**: 1.0
**Last Updated**: November 3, 2025
**Compatible with**: ESP8266 NodeMCU v1.0
