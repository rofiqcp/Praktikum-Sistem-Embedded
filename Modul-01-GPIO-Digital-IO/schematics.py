"""
STM32 Schematic Generator — 12 Projects
Libraries: matplotlib, Pillow (schemdraw for elements)

Generates one PNG schematic per project in each project's folder.
"""

import os
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
import matplotlib.lines as mlines

BASE = r"D:\PROGRAM\PCB\STM32"

# ─────────────────────────────────────────────────────────────────
#  Low-level drawing primitives (shared)
# ─────────────────────────────────────────────────────────────────
RED   = '#CC0000'
GREEN = '#006600'
BLUE  = '#0000BB'
BLACK = '#111111'
GRAY  = '#888888'
ORANGE= '#CC6600'
LW    = 1.4

def setup(w=14, h=9, title=''):
    fig, ax = plt.subplots(figsize=(w, h))
    ax.set_xlim(0, w); ax.set_ylim(0, h)
    ax.set_aspect('equal'); ax.axis('off')
    fig.patch.set_facecolor('white')
    if title:
        ax.set_title(title, fontsize=11, fontweight='bold', color=BLUE, pad=6)
    return fig, ax

def wire(ax, x1, y1, x2, y2, color=GREEN, lw=LW):
    ax.plot([x1,x2],[y1,y2], color=color, linewidth=lw, solid_capstyle='round')

def dot(ax, x, y, color=GREEN, r=0.07):
    ax.add_patch(plt.Circle((x,y), r, color=color, zorder=5))

def gnd(ax, x, y, color=GREEN):
    wire(ax, x, y, x, y-0.3, color)
    for i,hw in enumerate([0.28,0.18,0.08]):
        yy = y - 0.3 - i*0.13
        wire(ax, x-hw, yy, x+hw, yy, color)
    ax.text(x, y-0.3-3*0.13-0.08, 'GND', ha='center', va='top',
            fontsize=7, color=color)

def vcc(ax, x, y, label='VCC', color=RED):
    wire(ax, x, y, x, y+0.3, color)
    ax.plot([x-0.2, x+0.2],[y+0.3,y+0.3], color=color, linewidth=2)
    ax.text(x, y+0.38, label, ha='center', va='bottom', fontsize=7, color=color)

def resistor(ax, x, y, horiz=True, label='220Ω', color=BLACK):
    """Draw a small rectangle resistor."""
    if horiz:
        rw, rh = 0.55, 0.20
        ax.add_patch(mpatches.FancyBboxPatch(
            (x-rw/2, y-rh/2), rw, rh, boxstyle="square,pad=0",
            linewidth=1.2, edgecolor=color, facecolor='#FFFCE0'))
        ax.text(x, y+rh/2+0.06, label, ha='center', va='bottom', fontsize=6.5, color=color)
    else:
        rw, rh = 0.20, 0.55
        ax.add_patch(mpatches.FancyBboxPatch(
            (x-rw/2, y-rh/2), rw, rh, boxstyle="square,pad=0",
            linewidth=1.2, edgecolor=color, facecolor='#FFFCE0'))
        ax.text(x+rw/2+0.08, y, label, ha='left', va='center', fontsize=6.5, color=color)

def led_sym(ax, x, y, label='LED', color='red', horiz=True):
    """Simple LED triangle symbol."""
    if horiz:
        # anode left, cathode right
        tri = mpatches.Polygon(
            [(x,y+0.18),(x,y-0.18),(x+0.36,y)], closed=True,
            facecolor=color, edgecolor=BLACK, linewidth=1, alpha=0.8)
        ax.add_patch(tri)
        wire(ax, x+0.36, y+0.18, x+0.36, y-0.18, BLACK)  # cathode bar
        # light rays
        for dy in [0.08, -0.08]:
            ax.annotate('', xy=(x+0.55+0.15, y+dy+0.18),
                        xytext=(x+0.45, y+dy+0.08),
                        arrowprops=dict(arrowstyle='->', color=ORANGE, lw=0.8))
        ax.text(x+0.18, y-0.28, label, ha='center', va='top', fontsize=6.5, color=color)
    else:
        tri = mpatches.Polygon(
            [(x-0.18,y),(x+0.18,y),(x,y-0.36)], closed=True,
            facecolor=color, edgecolor=BLACK, linewidth=1, alpha=0.8)
        ax.add_patch(tri)
        wire(ax, x-0.18, y-0.36, x+0.18, y-0.36, BLACK)
        ax.text(x+0.28, y-0.18, label, ha='left', va='center', fontsize=6.5, color=color)

def button_sym(ax, x, y, label='BTN'):
    """SPST momentary push button."""
    # left terminal
    wire(ax, x-0.5, y, x-0.15, y)
    # right terminal
    wire(ax, x+0.15, y, x+0.5, y)
    # contact dots
    dot(ax, x-0.15, y)
    dot(ax, x+0.15, y)
    # movable contact (tilted line)
    ax.plot([x-0.15, x+0.15],[y+0.28, y+0.28], color=GREEN, linewidth=LW)
    wire(ax, x-0.15, y, x-0.15, y+0.28)
    ax.text(x, y-0.15, label, ha='center', va='top', fontsize=7, color=RED)

def nc_button_sym(ax, x, y, label='E-STOP NC'):
    """Normally Closed push button."""
    wire(ax, x-0.5, y, x-0.15, y)
    wire(ax, x+0.15, y, x+0.5, y)
    dot(ax, x-0.15, y); dot(ax, x+0.15, y)
    # NC = contact line drawn (closed)
    wire(ax, x-0.15, y+0.25, x+0.15, y+0.25)
    wire(ax, x-0.15, y, x-0.15, y+0.25)
    # slash for NC
    ax.plot([x-0.05, x+0.18],[y+0.18, y+0.36], color=RED, linewidth=1)
    ax.text(x, y-0.15, label, ha='center', va='top', fontsize=7, color=RED)

def buzzer_sym(ax, x, y, label='BUZZER'):
    """Simple buzzer symbol."""
    arc = mpatches.Arc((x,y), 0.5, 0.5, angle=0, theta1=0, theta2=180,
                       color=ORANGE, linewidth=1.5)
    ax.add_patch(arc)
    wire(ax, x-0.25, y, x-0.25, y-0.3)
    wire(ax, x+0.25, y, x+0.25, y-0.3)
    wire(ax, x-0.25, y-0.3, x+0.25, y-0.3)
    ax.text(x, y-0.42, label, ha='center', va='top', fontsize=7, color=ORANGE)

def dip_switch(ax, x, y, n=4, labels=None):
    """DIP switch package."""
    bw = 1.2; bh = 0.5 * n + 0.2
    ax.add_patch(mpatches.FancyBboxPatch(
        (x, y-bh/2), bw, bh, boxstyle="square,pad=0",
        linewidth=1.5, edgecolor=BLACK, facecolor='#ADD8E6'))
    ax.text(x+bw/2, y+bh/2+0.1, 'DIP SW', ha='center', va='bottom',
            fontsize=7, color=BLACK)
    for i in range(n):
        yy = y + (n/2 - 0.5 - i) * 0.5
        lbl = labels[i] if labels else f'SW{i+1}'
        ax.text(x+bw/2, yy, lbl, ha='center', va='center', fontsize=6.5, color=BLACK)
        # left pin
        wire(ax, x-0.5, yy, x, yy)
        dot(ax, x-0.5, yy)
        # right pin
        wire(ax, x+bw, yy, x+bw+0.5, yy)
        dot(ax, x+bw+0.5, yy)
    return bw, bh

def keypad_4x4(ax, x, y, bw=3.2, bh=3.2):
    """4x4 membrane keypad block."""
    ax.add_patch(mpatches.FancyBboxPatch(
        (x, y), bw, bh, boxstyle="square,pad=0.05",
        linewidth=2, edgecolor=BLACK, facecolor='#E0E0FF'))
    keys = [['1','2','3','A'],['4','5','6','B'],
            ['7','8','9','C'],['*','0','#','D']]
    cw = bw/4; ch = bh/4
    for r in range(4):
        for c in range(4):
            kx = x + c*cw + cw/2
            ky = y + bh - r*ch - ch/2
            ax.add_patch(mpatches.FancyBboxPatch(
                (kx-cw*0.38, ky-ch*0.38), cw*0.76, ch*0.76,
                boxstyle="round,pad=0.02",
                linewidth=1, edgecolor=GRAY, facecolor='white'))
            ax.text(kx, ky, keys[r][c], ha='center', va='center',
                    fontsize=8, fontweight='bold', color=BLACK)
    ax.text(x+bw/2, y+bh+0.15, '4×4 Matrix Keypad', ha='center', va='bottom',
            fontsize=8, color=BLACK)

def stm32_chip(ax, x, y, w=2.8, h=5.5, label='STM32F411CEU6',
               left_pins=None, right_pins=None, stub=1.2, fs=7):
    """
    Draw an STM32 IC with arbitrary left/right pin lists.
    left_pins / right_pins: list of (pin_label, net_label) tuples top→bottom.
    Returns dict: net_label → (wire_end_x, y)
    """
    ax.add_patch(mpatches.FancyBboxPatch(
        (x, y), w, h, boxstyle="square,pad=0",
        linewidth=2, edgecolor=RED, facecolor='white'))
    ax.text(x+w/2, y-0.22, label, ha='center', va='top',
            fontsize=7.5, color=BLUE, style='italic', fontweight='bold')
    ax.text(x+w/2, y+h+0.1, 'U1', ha='center', va='bottom', fontsize=8, color=BLUE)

    coords = {}

    def draw_pins(pins, side):
        n = len(pins)
        if n == 0: return
        step = h / (n + 1)
        for i, (pname, net) in enumerate(pins):
            py = y + h - (i+1)*step
            if side == 'left':
                ex = x - stub
                wire(ax, ex, py, x, py)
                ax.text(x-0.08, py, pname, ha='right', va='center',
                        fontsize=fs, color=BLACK)
            else:
                ex = x + w + stub
                wire(ax, x+w, py, ex, py)
                ax.text(x+w+0.08, py, pname, ha='left', va='center',
                        fontsize=fs, color=BLACK)
            if net:
                coords[net] = (ex, py)

    if left_pins:  draw_pins(left_pins,  'left')
    if right_pins: draw_pins(right_pins, 'right')
    return coords

def save(fig, folder, fname):
    os.makedirs(folder, exist_ok=True)
    path = os.path.join(folder, fname)
    fig.savefig(path, dpi=200, bbox_inches='tight', facecolor='white')
    plt.close(fig)
    print(f"  ✓  {path}")
    return path


# ═══════════════════════════════════════════════════════════════════
#  Project 01 — LED Blink  (PC13)
# ═══════════════════════════════════════════════════════════════════
def proj01():
    fig, ax = setup(10, 7, 'STM32_01 — LED Blink  (PC13, Active-Low)')
    coords = stm32_chip(ax, 3, 1.5, h=4,
        left_pins =[('3V3','VCC'),('GND','GND1')],
        right_pins=[('PC13','PC13')])

    # PC13 → resistor → LED → GND
    x, y = coords['PC13']
    wire(ax, x, y, x+0.8, y)
    resistor(ax, x+1.1, y, horiz=True, label='220Ω')
    wire(ax, x+1.38, y, x+1.9, y)
    # LED anode
    led_sym(ax, x+1.9, y, label='LED\nPC13', color='red')
    wire(ax, x+2.28, y, x+2.7, y)
    gnd(ax, x+2.7, y)

    # VCC / GND on left
    xv, yv = coords['VCC']
    vcc(ax, xv, yv, '3V3')
    xg, yg = coords['GND1']
    gnd(ax, xg, yg)

    ax.text(1.0, 6.5,
        'PC13 → 220Ω → LED → GND\nActive-Low: output LOW = LED ON',
        fontsize=8, va='top', color=GRAY,
        bbox=dict(boxstyle='round', facecolor='#FFFFF0', edgecolor=GRAY, alpha=0.8))
    save(fig, os.path.join(BASE,'STM32_01_LED_Blink'), 'schematic.png')


# ═══════════════════════════════════════════════════════════════════
#  Project 02 — Multi LED Running (PA0–PA3)
# ═══════════════════════════════════════════════════════════════════
def proj02():
    fig, ax = setup(13, 9, 'STM32_02 — Multi LED Running  (PA0–PA3)')
    coords = stm32_chip(ax, 3, 2, h=5,
        left_pins=[('3V3','VCC'),('GND','GNDC')],
        right_pins=[('PA0','PA0'),('PA1','PA1'),
                    ('PA2','PA2'),('PA3','PA3')])

    colors = ['red','#FF9900','green','blue']
    for i, pin in enumerate(['PA0','PA1','PA2','PA3']):
        x, y = coords[pin]
        wire(ax, x, y, x+0.7, y)
        resistor(ax, x+1.0, y, horiz=True, label='220Ω')
        wire(ax, x+1.28, y, x+1.8, y)
        led_sym(ax, x+1.8, y, label=f'LED{i+1}', color=colors[i])
        wire(ax, x+2.2, y, x+2.6, y)
        gnd(ax, x+2.6, y)

    vcc(ax, *coords['VCC'], '3V3')
    gnd(ax, *coords['GNDC'])

    ax.text(0.3, 8.5,
        'Running-light: LEDs lit one at a time in sequence\n'
        'Each: PA_n → 220Ω → LED → GND',
        fontsize=8, va='top', color=GRAY,
        bbox=dict(boxstyle='round', facecolor='#FFFFF0', edgecolor=GRAY, alpha=0.8))
    save(fig, os.path.join(BASE,'STM32_02_Multi_LED_Running'), 'schematic.png')


# ═══════════════════════════════════════════════════════════════════
#  Project 03 — LED Binary Counter (PA0–PA3)
# ═══════════════════════════════════════════════════════════════════
def proj03():
    fig, ax = setup(13, 9, 'STM32_03 — LED Binary Counter  (PA0–PA3, 0–15)')
    coords = stm32_chip(ax, 3, 2, h=5,
        left_pins=[('3V3','VCC'),('GND','GNDC')],
        right_pins=[('PA0 (bit0)','PA0'),('PA1 (bit1)','PA1'),
                    ('PA2 (bit2)','PA2'),('PA3 (bit3)','PA3')])

    bits  = ['bit0\n(LSB)','bit1','bit2','bit3\n(MSB)']
    colors= ['red','#FF9900','green','blue']
    for i, pin in enumerate(['PA0','PA1','PA2','PA3']):
        x, y = coords[pin]
        wire(ax, x, y, x+0.7, y)
        resistor(ax, x+1.0, y, horiz=True, label='220Ω')
        wire(ax, x+1.28, y, x+1.8, y)
        led_sym(ax, x+1.8, y, label=bits[i], color=colors[i])
        wire(ax, x+2.2, y, x+2.6, y)
        gnd(ax, x+2.6, y)

    vcc(ax, *coords['VCC'], '3V3')
    gnd(ax, *coords['GNDC'])

    ax.text(0.3, 8.5,
        'Binary counter 0–15: 4 LEDs show binary value\n'
        'PA0=LSB(1) PA1(2) PA2(4) PA3=MSB(8)',
        fontsize=8, va='top', color=GRAY,
        bbox=dict(boxstyle='round', facecolor='#FFFFF0', edgecolor=GRAY, alpha=0.8))
    save(fig, os.path.join(BASE,'STM32_03_LED_Binary_Counter'), 'schematic.png')


# ═══════════════════════════════════════════════════════════════════
#  Project 04 — Button Debounce (PB0 + PC13 LED)
# ═══════════════════════════════════════════════════════════════════
def proj04():
    fig, ax = setup(13, 8, 'STM32_04 — Button Debounce  (PB0 input, PC13 LED)')
    coords = stm32_chip(ax, 4, 1.5, h=5,
        left_pins=[('PB0','PB0'),('3V3','VCC'),('GND','GNDC')],
        right_pins=[('PC13','PC13')])

    # Button circuit: PB0 active-low pull-up
    xb, yb = coords['PB0']
    wire(ax, xb-0.8, yb, xb, yb)
    button_sym(ax, xb-1.1, yb, 'BTN1')
    wire(ax, xb-1.6, yb, xb-1.6, yb-0.6)
    gnd(ax, xb-1.6, yb-0.6)
    # pull-up to 3V3
    wire(ax, xb-1.1, yb, xb-1.1, yb+0.8)
    resistor(ax, xb-1.1, yb+1.1, horiz=False, label='10kΩ')
    wire(ax, xb-1.1, yb+1.4, xb-1.1, yb+1.7)
    vcc(ax, xb-1.1, yb+1.7, '3V3')

    # LED on PC13 (active-low)
    x, y = coords['PC13']
    wire(ax, x, y, x+0.8, y)
    resistor(ax, x+1.1, y, horiz=True, label='220Ω')
    wire(ax, x+1.38, y, x+1.9, y)
    led_sym(ax, x+1.9, y, label='LED\nPC13', color='red')
    wire(ax, x+2.28, y, x+2.7, y)
    gnd(ax, x+2.7, y)

    vcc(ax, *coords['VCC'], '3V3')
    gnd(ax, *coords['GNDC'])

    ax.text(0.2, 7.6,
        'Debounce state machine: IDLE → PRESS_DETECTED → CONFIRMED\n'
        'PB0: Active-low with 10kΩ pull-up | PC13: Active-low LED',
        fontsize=8, va='top', color=GRAY,
        bbox=dict(boxstyle='round', facecolor='#FFFFF0', edgecolor=GRAY, alpha=0.8))
    save(fig, os.path.join(BASE,'STM32_04_Button_Debounce'), 'schematic.png')


# ═══════════════════════════════════════════════════════════════════
#  Project 05 — Long/Short Press (PB0 + PA0 + PA1)
# ═══════════════════════════════════════════════════════════════════
def proj05():
    fig, ax = setup(14, 8, 'STM32_05 — Long/Short Press  (PB0, PA0 short, PA1 long)')
    coords = stm32_chip(ax, 4, 1.5, h=5.5,
        left_pins=[('PB0','PB0'),('3V3','VCC'),('GND','GNDC')],
        right_pins=[('PA0','PA0'),('PA1','PA1')])

    # Button
    xb, yb = coords['PB0']
    wire(ax, xb-0.8, yb, xb, yb)
    button_sym(ax, xb-1.1, yb, 'BTN')
    wire(ax, xb-1.6, yb, xb-1.6, yb-0.6)
    gnd(ax, xb-1.6, yb-0.6)
    wire(ax, xb-1.1, yb, xb-1.1, yb+0.8)
    resistor(ax, xb-1.1, yb+1.1, horiz=False, label='10kΩ')
    wire(ax, xb-1.1, yb+1.4, xb-1.1, yb+1.7)
    vcc(ax, xb-1.1, yb+1.7, '3V3')

    leds = [('PA0','LED_SHORT','<1s','orange'),
            ('PA1','LED_LONG', '>1s','blue')]
    for pin, lname, note, col in leds:
        x, y = coords[pin]
        wire(ax, x, y, x+0.7, y)
        resistor(ax, x+1.0, y, horiz=True, label='220Ω')
        wire(ax, x+1.28, y, x+1.8, y)
        led_sym(ax, x+1.8, y, label=f'{lname}\n{note}', color=col)
        wire(ax, x+2.2, y, x+2.6, y)
        gnd(ax, x+2.6, y)

    vcc(ax, *coords['VCC'], '3V3')
    gnd(ax, *coords['GNDC'])

    ax.text(0.2, 7.6,
        'Short press (<1 s): toggles LED_SHORT (PA0)\n'
        'Long press  (>1 s): toggles LED_LONG  (PA1)',
        fontsize=8, va='top', color=GRAY,
        bbox=dict(boxstyle='round', facecolor='#FFFFF0', edgecolor=GRAY, alpha=0.8))
    save(fig, os.path.join(BASE,'STM32_05_Long_Short_Press'), 'schematic.png')


# ═══════════════════════════════════════════════════════════════════
#  Project 06 — Toggle Latch (PB0 + PC13)
# ═══════════════════════════════════════════════════════════════════
def proj06():
    fig, ax = setup(13, 8, 'STM32_06 — Toggle Latch  (PB0 edge detect, PC13 LED)')
    coords = stm32_chip(ax, 4, 1.5, h=5,
        left_pins=[('PB0','PB0'),('3V3','VCC'),('GND','GNDC')],
        right_pins=[('PC13','PC13')])

    xb, yb = coords['PB0']
    wire(ax, xb-0.8, yb, xb, yb)
    button_sym(ax, xb-1.1, yb, 'BTN')
    wire(ax, xb-1.6, yb, xb-1.6, yb-0.6)
    gnd(ax, xb-1.6, yb-0.6)
    wire(ax, xb-1.1, yb, xb-1.1, yb+0.8)
    resistor(ax, xb-1.1, yb+1.1, horiz=False, label='10kΩ')
    wire(ax, xb-1.1, yb+1.4, xb-1.1, yb+1.7)
    vcc(ax, xb-1.1, yb+1.7, '3V3')

    x, y = coords['PC13']
    wire(ax, x, y, x+0.8, y)
    resistor(ax, x+1.1, y, horiz=True, label='220Ω')
    wire(ax, x+1.38, y, x+1.9, y)
    led_sym(ax, x+1.9, y, label='LED\nPC13', color='red')
    wire(ax, x+2.28, y, x+2.7, y)
    gnd(ax, x+2.7, y)

    vcc(ax, *coords['VCC'], '3V3')
    gnd(ax, *coords['GNDC'])

    ax.text(0.2, 7.6,
        'Edge detection: falling edge on PB0 toggles PC13 LED\n'
        'prev_state != curr_state → toggle (software debounce 50ms)',
        fontsize=8, va='top', color=GRAY,
        bbox=dict(boxstyle='round', facecolor='#FFFFF0', edgecolor=GRAY, alpha=0.8))
    save(fig, os.path.join(BASE,'STM32_06_Toggle_Latch'), 'schematic.png')


# ═══════════════════════════════════════════════════════════════════
#  Project 07 — GPIO Drive Strength (PA0 + oscilloscope)
# ═══════════════════════════════════════════════════════════════════
def proj07():
    fig, ax = setup(13, 7, 'STM32_07 — GPIO Drive Strength  (PA0, slew-rate demo)')
    coords = stm32_chip(ax, 4, 1.5, h=4,
        left_pins=[('3V3','VCC'),('GND','GNDC')],
        right_pins=[('PA0','PA0')])

    x, y = coords['PA0']
    wire(ax, x, y, x+0.7, y)
    dot(ax, x+0.7, y)
    # to resistor + LED
    wire(ax, x+0.7, y, x+0.7+0.3, y)
    resistor(ax, x+1.3, y, horiz=True, label='220Ω')
    wire(ax, x+1.58, y, x+2.1, y)
    led_sym(ax, x+2.1, y, label='LED\nPA0', color='red')
    wire(ax, x+2.5, y, x+2.9, y)
    gnd(ax, x+2.9, y)

    # Oscilloscope probe branch
    wire(ax, x+0.7, y, x+0.7, y+1.0)
    ax.add_patch(mpatches.FancyBboxPatch(
        (x+0.3, y+1.0), 1.0, 0.55, boxstyle="square,pad=0.05",
        linewidth=1.5, edgecolor=ORANGE, facecolor='#FFF3E0'))
    ax.text(x+0.8, y+1.28, '⚡ Probe\n(Oscilloscope)', ha='center', va='center',
            fontsize=7, color=ORANGE)
    wire(ax, x+0.8, y+1.0, x+0.8, y+0.7)
    wire(ax, x+0.7, y+0.7, x+0.9, y+0.7)

    vcc(ax, *coords['VCC'], '3V3')
    gnd(ax, *coords['GNDC'])

    ax.text(0.2, 6.6,
        'Drive strength changes: LOW(2MHz) → MEDIUM(25MHz) → HIGH(50MHz) → VHIGH(100MHz)\n'
        'Observe slew-rate on oscilloscope at PA0. LED toggles at each level.',
        fontsize=8, va='top', color=GRAY,
        bbox=dict(boxstyle='round', facecolor='#FFFFF0', edgecolor=GRAY, alpha=0.8))
    save(fig, os.path.join(BASE,'STM32_07_GPIO_Drive_Strength'), 'schematic.png')


# ═══════════════════════════════════════════════════════════════════
#  Project 08 — DIP Switch Reader (PB0, PB1, PB3, PB4)
# ═══════════════════════════════════════════════════════════════════
def proj08():
    fig, ax = setup(14, 9, 'STM32_08 — DIP Switch Reader  (PB0,PB1,PB3,PB4 input pull-up)')
    coords = stm32_chip(ax, 5, 2, h=5,
        left_pins=[('PB0 (bit0)','PB0'),('PB1 (bit1)','PB1'),
                   ('PB3 (bit2)','PB3'),('PB4 (bit3)','PB4'),
                   ('3V3','VCC'),('GND','GNDC')],
        right_pins=[])

    dip_w, dip_h = dip_switch(ax, 1.5, 5, n=4,
                               labels=['DIP1\nPB0','DIP2\nPB1','DIP3\nPB3','DIP4\nPB4'])
    dip_pins_y = [5 + (4/2-0.5-i)*0.5 for i in range(4)]
    stm_pins   = ['PB0','PB1','PB3','PB4']

    for i, pin in enumerate(stm_pins):
        xs, ys = coords[pin]
        xd_r   = 1.5 + dip_w + 0.5   # right side of DIP switch
        yd     = dip_pins_y[i]
        # wire from STM32 left stub to DIP right output
        wire(ax, xs, ys, xd_r, ys)
        if abs(ys - yd) > 0.05:
            dot(ax, xd_r, ys)
            wire(ax, xd_r, ys, xd_r, yd)
        # DIP left output to GND
        xd_l = 1.5 - 0.5
        wire(ax, xd_l, yd, xd_l-0.3, yd)
        gnd(ax, xd_l-0.3, yd)

    vcc(ax, *coords['VCC'], '3V3')
    gnd(ax, *coords['GNDC'])

    ax.text(0.2, 8.6,
        'SW ON = pin connected to GND (reads LOW) = logic 1\n'
        'SW OFF = internal pull-up HIGH = logic 0 | PB2 skipped (BOOT1)',
        fontsize=8, va='top', color=GRAY,
        bbox=dict(boxstyle='round', facecolor='#FFFFF0', edgecolor=GRAY, alpha=0.8))
    save(fig, os.path.join(BASE,'STM32_08_DIP_Switch_Reader'), 'schematic.png')


# ═══════════════════════════════════════════════════════════════════
#  Project 09 — GPIO Port Register (PA0–PA3, 4 LEDs)
# ═══════════════════════════════════════════════════════════════════
def proj09():
    fig, ax = setup(13, 9, 'STM32_09 — GPIO Port Register  (Direct BSRR/ODR, PA0–PA3)')
    coords = stm32_chip(ax, 3, 2, h=5,
        left_pins=[('3V3','VCC'),('GND','GNDC')],
        right_pins=[('PA0 (LED0)','PA0'),('PA1 (LED1)','PA1'),
                    ('PA2 (LED2)','PA2'),('PA3 (LED3)','PA3')])

    colors = ['red','#FF9900','green','blue']
    names  = ['LED0','LED1','LED2','LED3']
    for i, pin in enumerate(['PA0','PA1','PA2','PA3']):
        x, y = coords[pin]
        wire(ax, x, y, x+0.7, y)
        resistor(ax, x+1.0, y, horiz=True, label='220Ω')
        wire(ax, x+1.28, y, x+1.8, y)
        led_sym(ax, x+1.8, y, label=names[i], color=colors[i])
        wire(ax, x+2.2, y, x+2.6, y)
        gnd(ax, x+2.6, y)

    vcc(ax, *coords['VCC'], '3V3')
    gnd(ax, *coords['GNDC'])

    ax.text(0.2, 8.6,
        'Direct register: BSRR[15:0]=Set, BSRR[31:16]=Reset, ODR, IDR\n'
        'Benchmark: BSRR > HAL_WritePin > TogglePin > ODR (RMW)',
        fontsize=8, va='top', color=GRAY,
        bbox=dict(boxstyle='round', facecolor='#FFFFF0', edgecolor=GRAY, alpha=0.8))
    save(fig, os.path.join(BASE,'STM32_09_GPIO_Port_Register'), 'schematic.png')


# ═══════════════════════════════════════════════════════════════════
#  Project 10 — Matrix Keypad 4×4 (PA0–PA3 rows, PB0,1,3,4 cols)
# ═══════════════════════════════════════════════════════════════════
def proj10():
    fig, ax = setup(16, 10, 'STM32_10 — 4×4 Matrix Keypad  (PA0-3 Rows, PB0,1,3,4 Cols)')
    coords = stm32_chip(ax, 5, 2, h=6,
        left_pins=[('PA0 ROW0','ROW0'),('PA1 ROW1','ROW1'),
                   ('PA2 ROW2','ROW2'),('PA3 ROW3','ROW3'),
                   ('3V3','VCC'),('GND','GNDC')],
        right_pins=[('PB0 COL0','COL0'),('PB1 COL1','COL1'),
                    ('PB3 COL2','COL2'),('PB4 COL3','COL3')])

    # Keypad block
    keypad_4x4(ax, 8, 3, bw=3.6, bh=3.6)
    kx_l = 8; kx_r = 11.6
    ky_top = 6.6

    # Row wires: left side of STM → left side of keypad
    row_net = ['ROW0','ROW1','ROW2','ROW3']
    for i, net in enumerate(row_net):
        xs, ys = coords[net]
        ky = ky_top - i * 3.6/4 - 3.6/8
        wire(ax, xs, ys, kx_l-0.5, ys)
        wire(ax, kx_l-0.5, ys, kx_l-0.5, ky)
        wire(ax, kx_l-0.5, ky, kx_l, ky)
        ax.text(kx_l-0.7, ky, f'R{i}', ha='right', va='center', fontsize=6.5, color=GREEN)

    # Col wires: right side of keypad → right side of STM
    col_net = ['COL0','COL1','COL2','COL3']
    for i, net in enumerate(col_net):
        xs, ys = coords[net]
        kx = kx_l + i * 3.6/4 + 3.6/8
        wire(ax, kx_r, ky_top+0.2-i*0.1, xs, ys)   # rough routing
        ax.text(kx_r+0.1, ky_top-i*0.3, f'C{i}', ha='left', va='center',
                fontsize=6.5, color=GREEN)

    vcc(ax, *coords['VCC'], '3V3')
    gnd(ax, *coords['GNDC'])

    ax.text(0.2, 9.6,
        'Row-column scanning: drive each row LOW, read columns\n'
        'Cols use internal pull-up | Debounce 50ms | PB2 skipped (BOOT1)',
        fontsize=8, va='top', color=GRAY,
        bbox=dict(boxstyle='round', facecolor='#FFFFF0', edgecolor=GRAY, alpha=0.8))
    save(fig, os.path.join(BASE,'STM32_10_GPIO_Matrix_Keypad'), 'schematic.png')


# ═══════════════════════════════════════════════════════════════════
#  Project 11 — Emergency Stop (PB0 NC, PC13 LED, PA1 Buzzer)
# ═══════════════════════════════════════════════════════════════════
def proj11():
    fig, ax = setup(15, 9, 'STM32_11 — Emergency Stop  (PB0 NC, PC13 LED, PA1 Buzzer)')
    coords = stm32_chip(ax, 5, 1.5, h=6,
        left_pins=[('PB0 ESTOP','PB0'),('3V3','VCC'),('GND','GNDC')],
        right_pins=[('PC13 LED','PC13'),('PA1 BZR','PA1')])

    # NC Emergency button on PB0
    xb, yb = coords['PB0']
    wire(ax, xb-0.8, yb, xb, yb)
    nc_button_sym(ax, xb-1.1, yb, 'E-STOP\n(NC)')
    wire(ax, xb-1.6, yb, xb-1.6, yb-0.6)
    gnd(ax, xb-1.6, yb-0.6)
    # external pull-up
    wire(ax, xb-1.1, yb, xb-1.1, yb+1.0)
    resistor(ax, xb-1.1, yb+1.3, horiz=False, label='10kΩ')
    wire(ax, xb-1.1, yb+1.6, xb-1.1, yb+2.0)
    vcc(ax, xb-1.1, yb+2.0, '3V3')

    # Status LED PC13 (active-low)
    x, y = coords['PC13']
    wire(ax, x, y, x+0.7, y)
    resistor(ax, x+1.0, y, horiz=True, label='220Ω')
    wire(ax, x+1.28, y, x+1.8, y)
    led_sym(ax, x+1.8, y, label='STATUS\nLED', color='red')
    wire(ax, x+2.2, y, x+2.6, y)
    gnd(ax, x+2.6, y)

    # Buzzer PA1
    xz, yz = coords['PA1']
    wire(ax, xz, yz, xz+0.7, yz)
    # NPN transistor symbol (simple)
    ax.add_patch(plt.Circle((xz+0.9, yz), 0.25, fill=False,
                             edgecolor=BLACK, linewidth=1.2))
    ax.text(xz+0.9, yz, 'NPN', ha='center', va='center', fontsize=6, color=BLACK)
    wire(ax, xz+1.15, yz, xz+1.5, yz)
    buzzer_sym(ax, xz+1.8, yz)
    wire(ax, xz+1.8, yz-0.3, xz+1.8, yz-0.6)
    vcc(ax, xz+1.8, yz+0.28, '5V')
    wire(ax, xz+0.9, yz-0.25, xz+0.9, yz-0.6)
    gnd(ax, xz+0.9, yz-0.6)

    vcc(ax, *coords['VCC'], '3V3')
    gnd(ax, *coords['GNDC'])

    ax.text(0.2, 8.7,
        'NC button: OPEN contact or wire-break → PB0 LOW → EMERGENCY\n'
        'PC13 blinks fast + PA1 buzzer ON | Reset: PB0 HIGH for 2s',
        fontsize=8, va='top', color=GRAY,
        bbox=dict(boxstyle='round', facecolor='#FFFFF0', edgecolor=GRAY, alpha=0.8))
    save(fig, os.path.join(BASE,'STM32_11_Emergency_Stop'), 'schematic.png')


# ═══════════════════════════════════════════════════════════════════
#  Project 12 — 8-LED Test Pattern (PA0–PA7)
# ═══════════════════════════════════════════════════════════════════
def proj12():
    fig, ax = setup(16, 11, 'STM32_12 — 8-LED Test Pattern  (PA0–PA7, direct ODR)')
    coords = stm32_chip(ax, 3, 1.5, h=7.5,
        left_pins=[('3V3','VCC'),('GND','GNDC')],
        right_pins=[('PA0 (bit0)','PA0'),('PA1 (bit1)','PA1'),
                    ('PA2 (bit2)','PA2'),('PA3 (bit3)','PA3'),
                    ('PA4 (bit4)','PA4'),('PA5 (bit5)','PA5'),
                    ('PA6 (bit6)','PA6'),('PA7 (bit7)','PA7')])

    colors = ['red','#FF5500','#FF9900','yellow','green','cyan','blue','#AA00FF']
    for i in range(8):
        pin  = f'PA{i}'
        x, y = coords[pin]
        wire(ax, x, y, x+0.6, y)
        resistor(ax, x+0.9, y, horiz=True, label='220Ω')
        wire(ax, x+1.18, y, x+1.7, y)
        led_sym(ax, x+1.7, y, label=f'LED{i}', color=colors[i])
        wire(ax, x+2.1, y, x+2.5, y)
        gnd(ax, x+2.5, y)

    vcc(ax, *coords['VCC'], '3V3')
    gnd(ax, *coords['GNDC'])

    ax.text(0.2, 10.5,
        '8 LEDs display patterns via GPIOA→ODR (8-bit write)\n'
        'Patterns: all-on, all-off, walk-1, march, binary-cnt, strobe…',
        fontsize=8, va='top', color=GRAY,
        bbox=dict(boxstyle='round', facecolor='#FFFFF0', edgecolor=GRAY, alpha=0.8))
    save(fig, os.path.join(BASE,'STM32_12_LED_Test_Pattern'), 'schematic.png')


# ═══════════════════════════════════════════════════════════════════
#  Run all
# ═══════════════════════════════════════════════════════════════════
if __name__ == '__main__':
    print('=' * 65)
    print('  STM32 Schematic Generator — 12 Projects')
    print('  Libraries: matplotlib, Pillow')
    print('=' * 65)

    tasks = [
        ('01 LED_Blink',          proj01),
        ('02 Multi_LED_Running',  proj02),
        ('03 LED_Binary_Counter', proj03),
        ('04 Button_Debounce',    proj04),
        ('05 Long_Short_Press',   proj05),
        ('06 Toggle_Latch',       proj06),
        ('07 GPIO_Drive_Strength',proj07),
        ('08 DIP_Switch_Reader',  proj08),
        ('09 GPIO_Port_Register', proj09),
        ('10 GPIO_Matrix_Keypad', proj10),
        ('11 Emergency_Stop',     proj11),
        ('12 LED_Test_Pattern',   proj12),
    ]

    ok = 0
    for name, fn in tasks:
        print(f'\n── STM32_{name}')
        try:
            fn()
            ok += 1
        except Exception as e:
            print(f'  ✗  ERROR: {e}')
            import traceback; traceback.print_exc()

    print(f'\n{"=" * 65}')
    print(f'  Done: {ok}/{len(tasks)} schematics generated')
    print(f'  Each saved as  <project_folder>/schematic.png')
    print('=' * 65)
