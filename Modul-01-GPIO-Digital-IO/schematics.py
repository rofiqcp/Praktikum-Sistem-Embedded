"""
GPIO Schematic Generator — 10 Praktikum (STM32F103 Blue Pill)
Semua percobaan menyertakan LCD I2C 16x2 (SDA=PB7, SCL=PB6)

Judul Percobaan:
  P01 — LED Parade        : Output Push-Pull & Pola Cahaya Digital
  P02 — Shadow & Ghost    : Active-LOW, Open-Drain, dan Logika Terbalik
  P03 — Sentinel Gate     : Tombol Pull-UP Eksternal (220Ω ke 3.3V)
  P04 — Ground Guardian   : Tombol Pull-DOWN Eksternal (220Ω ke GND)
  P05 — Phantom Touch     : Pull-UP Internal & Tombol Tanpa Resistor
  P06 — Force Field       : Pull-DOWN Internal & Logika Active-HIGH
  P07 — Clean Contact     : Debounce State Machine & Penghitung Akurat
  P08 — Speed Racer       : Kecepatan GPIO & Pola LED Multi-Kecepatan
  P09 — Twist & Count     : Rotary Encoder Kuadratur & Counter LCD
  P10 — Matrix Commander  : Pemindaian Keypad 4×4 & Tampilan LCD

Libraries: matplotlib
Output: schematic.png per folder di BASE
"""

import os
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches

BASE = r"D:\Praktikum-Sistem-Embedded\Modul-01-GPIO-Digital-IO\praktikum\STM32"

# ─────────────────────────────────────────────────────────────────
#  Warna & konstanta
# ─────────────────────────────────────────────────────────────────
RED    = '#CC0000'
GREEN  = '#006600'
BLUE   = '#0000BB'
BLACK  = '#111111'
GRAY   = '#888888'
ORANGE = '#CC6600'
PURPLE = '#660099'
TEAL   = '#007777'
LW     = 1.4

# ─────────────────────────────────────────────────────────────────
#  Primitif gambar bersama
# ─────────────────────────────────────────────────────────────────
def setup(w=16, h=10, title=''):
    fig, ax = plt.subplots(figsize=(w, h))
    ax.set_xlim(0, w); ax.set_ylim(0, h)
    ax.set_aspect('equal'); ax.axis('off')
    fig.patch.set_facecolor('white')
    if title:
        ax.set_title(title, fontsize=10, fontweight='bold', color=BLUE, pad=8)
    return fig, ax

def wire(ax, x1, y1, x2, y2, color=GREEN, lw=LW):
    ax.plot([x1, x2], [y1, y2], color=color, linewidth=lw, solid_capstyle='round')

def dot(ax, x, y, color=GREEN, r=0.07):
    ax.add_patch(plt.Circle((x, y), r, color=color, zorder=5))

def gnd(ax, x, y, color=BLACK):
    wire(ax, x, y, x, y - 0.3, color)
    for i, hw in enumerate([0.28, 0.18, 0.08]):
        yy = y - 0.3 - i * 0.13
        wire(ax, x - hw, yy, x + hw, yy, color)
    ax.text(x, y - 0.3 - 3 * 0.13 - 0.10, 'GND', ha='center', va='top',
            fontsize=6.5, color=color)

def vcc(ax, x, y, label='3V3', color=RED):
    wire(ax, x, y, x, y + 0.3, color)
    ax.plot([x - 0.22, x + 0.22], [y + 0.3, y + 0.3], color=color, linewidth=2)
    ax.text(x, y + 0.40, label, ha='center', va='bottom', fontsize=7, color=color)

def resistor(ax, x, y, horiz=True, label='220Ω', color=BLACK):
    if horiz:
        rw, rh = 0.55, 0.20
        ax.add_patch(mpatches.FancyBboxPatch(
            (x - rw / 2, y - rh / 2), rw, rh, boxstyle="square,pad=0",
            linewidth=1.2, edgecolor=color, facecolor='#FFFCE0'))
        ax.text(x, y + rh / 2 + 0.07, label, ha='center', va='bottom',
                fontsize=6.5, color=color)
    else:
        rw, rh = 0.20, 0.55
        ax.add_patch(mpatches.FancyBboxPatch(
            (x - rw / 2, y - rh / 2), rw, rh, boxstyle="square,pad=0",
            linewidth=1.2, edgecolor=color, facecolor='#FFFCE0'))
        ax.text(x + rw / 2 + 0.09, y, label, ha='left', va='center',
                fontsize=6.5, color=color)

def led_sym(ax, x, y, label='LED', color='red'):
    """LED: anode kiri, cathode kanan (horizontal)."""
    tri = mpatches.Polygon(
        [(x, y + 0.18), (x, y - 0.18), (x + 0.36, y)], closed=True,
        facecolor=color, edgecolor=BLACK, linewidth=1, alpha=0.85)
    ax.add_patch(tri)
    wire(ax, x + 0.36, y + 0.20, x + 0.36, y - 0.20, BLACK)
    # sinar cahaya
    for dy in [0.09, -0.04]:
        ax.annotate('', xy=(x + 0.56, y + dy + 0.16),
                    xytext=(x + 0.45, y + dy + 0.06),
                    arrowprops=dict(arrowstyle='->', color=ORANGE, lw=0.8))
    ax.text(x + 0.18, y - 0.30, label, ha='center', va='top',
            fontsize=6.5, color=BLACK)
    return x + 0.36   # ujung cathode

def button_sym(ax, x, y, label='BTN'):
    """SPST push button momentary."""
    wire(ax, x - 0.5, y, x - 0.15, y)
    wire(ax, x + 0.15, y, x + 0.5, y)
    dot(ax, x - 0.15, y, GREEN)
    dot(ax, x + 0.15, y, GREEN)
    ax.plot([x - 0.15, x + 0.15], [y + 0.28, y + 0.28], color=GREEN, linewidth=LW)
    wire(ax, x - 0.15, y, x - 0.15, y + 0.28)
    ax.text(x, y - 0.18, label, ha='center', va='top', fontsize=7, color=RED)

def stm32_chip(ax, x, y, w=2.8, h=6.0, label='STM32F103C8T6',
               left_pins=None, right_pins=None, stub=1.1, fs=7):
    """IC box dengan pin list kiri/kanan. Kembalikan dict net→(x,y) ujung stub."""
    ax.add_patch(mpatches.FancyBboxPatch(
        (x, y), w, h, boxstyle="square,pad=0",
        linewidth=2, edgecolor=RED, facecolor='#FFF8F8'))
    ax.text(x + w / 2, y - 0.25, label, ha='center', va='top',
            fontsize=7.5, color=BLUE, style='italic', fontweight='bold')
    ax.text(x + w / 2, y + h + 0.12, 'U1', ha='center', va='bottom',
            fontsize=8, color=BLUE)

    coords = {}

    def draw_pins(pins, side):
        n = len(pins)
        if not n:
            return
        step = h / (n + 1)
        for i, (pname, net) in enumerate(pins):
            py = y + h - (i + 1) * step
            if side == 'left':
                ex = x - stub
                wire(ax, ex, py, x, py)
                ax.text(x - 0.10, py, pname, ha='right', va='center',
                        fontsize=fs, color=BLACK)
            else:
                ex = x + w + stub
                wire(ax, x + w, py, ex, py)
                ax.text(x + w + 0.10, py, pname, ha='left', va='center',
                        fontsize=fs, color=BLACK)
            if net:
                coords[net] = (ex, py)

    if left_pins:  draw_pins(left_pins, 'left')
    if right_pins: draw_pins(right_pins, 'right')
    return coords

def lcd_i2c(ax, x, y, bw=2.8, bh=1.6, scl_net_y=None, sda_net_y=None,
            scl_x_end=None, sda_x_end=None, label='LCD I2C 16×2'):
    """Gambar blok LCD I2C dengan pin VCC/GND/SCL/SDA di sisi kiri."""
    ax.add_patch(mpatches.FancyBboxPatch(
        (x, y), bw, bh, boxstyle="round,pad=0.05",
        linewidth=1.8, edgecolor=TEAL, facecolor='#E0F4FF'))
    # layar LCD biru
    ax.add_patch(mpatches.FancyBboxPatch(
        (x + 0.15, y + 0.35), bw - 0.30, bh - 0.55, boxstyle="square,pad=0",
        linewidth=1, edgecolor=BLUE, facecolor='#003366'))
    ax.text(x + bw / 2, y + bh - 0.20, label, ha='center', va='center',
            fontsize=7, fontweight='bold', color=TEAL)
    ax.text(x + bw / 2, y + 0.62, 'Line 1: ████████████████', ha='center',
            va='center', fontsize=5.5, color='#00FF88', family='monospace')
    ax.text(x + bw / 2, y + 0.47, 'Line 2: ████████████████', ha='center',
            va='center', fontsize=5.5, color='#00FF88', family='monospace')

    # pin stubs kiri (VCC GND SCL SDA)
    pins = [('VCC', RED), ('GND', BLACK), ('SCL', TEAL), ('SDA', PURPLE)]
    pin_coords = {}
    step = bh / (len(pins) + 1)
    for i, (pname, col) in enumerate(pins):
        py = y + bh - (i + 1) * step
        ex = x - 0.9
        wire(ax, ex, py, x, py, col)
        ax.text(x - 0.95, py, pname, ha='right', va='center',
                fontsize=6.5, color=col)
        pin_coords[pname] = (ex, py)

    ax.text(x + bw / 2, y - 0.22, 'I²C addr: 0x27 (PCF8574)',
            ha='center', va='top', fontsize=6, color=GRAY)
    return pin_coords

def encoder_sym(ax, x, y, bw=1.8, bh=2.0):
    """Blok rotary encoder 5-pin."""
    ax.add_patch(mpatches.FancyBboxPatch(
        (x, y), bw, bh, boxstyle="round,pad=0.08",
        linewidth=1.8, edgecolor=ORANGE, facecolor='#FFF3E0'))
    # shaft bulat
    ax.add_patch(plt.Circle((x + bw / 2, y + bh / 2), 0.30,
                             facecolor='#CCCCCC', edgecolor=BLACK, linewidth=1.5))
    ax.add_patch(plt.Circle((x + bw / 2, y + bh / 2), 0.08,
                             facecolor=BLACK, edgecolor=BLACK))
    ax.text(x + bw / 2, y + bh + 0.12, 'ENCODER\n5-pin', ha='center',
            va='bottom', fontsize=6.5, color=ORANGE, fontweight='bold')
    pins = ['GND', '+', 'SW', 'DT', 'CLK']
    for i, p in enumerate(pins):
        px = x + bw * (i + 0.5) / 5
        wire(ax, px, y, px, y - 0.4, ORANGE)
        ax.text(px, y - 0.45, p, ha='center', va='top', fontsize=6, color=ORANGE)
    return {
        'GND': (x + bw * 0.5 / 5, y - 0.4),
        '+':   (x + bw * 1.5 / 5, y - 0.4),
        'SW':  (x + bw * 2.5 / 5, y - 0.4),
        'DT':  (x + bw * 3.5 / 5, y - 0.4),
        'CLK': (x + bw * 4.5 / 5, y - 0.4),
    }

def keypad_4x4_block(ax, x, y, bw=3.0, bh=3.0):
    """Blok keypad 4×4 dengan label tombol."""
    ax.add_patch(mpatches.FancyBboxPatch(
        (x, y), bw, bh, boxstyle="square,pad=0.05",
        linewidth=2, edgecolor=BLACK, facecolor='#E8E8FF'))
    keys = [['1', '2', '3', 'A'], ['4', '5', '6', 'B'],
            ['7', '8', '9', 'C'], ['*', '0', '#', 'D']]
    cw = bw / 4; ch = bh / 4
    for r in range(4):
        for c in range(4):
            kx = x + c * cw + cw / 2
            ky = y + bh - r * ch - ch / 2
            ax.add_patch(mpatches.FancyBboxPatch(
                (kx - cw * 0.38, ky - ch * 0.38), cw * 0.76, ch * 0.76,
                boxstyle="round,pad=0.02", linewidth=1,
                edgecolor=GRAY, facecolor='white'))
            ax.text(kx, ky, keys[r][c], ha='center', va='center',
                    fontsize=8, fontweight='bold', color=BLACK)
    ax.text(x + bw / 2, y + bh + 0.15, '4×4 Matrix Keypad (8-pin)',
            ha='center', va='bottom', fontsize=7.5, color=BLACK, fontweight='bold')

    # 8 pin stubs: 4 bawah (row), 4 atas (col) — sesuai skematik kabel
    row_coords = {}
    col_coords = {}
    for i in range(4):
        px = x + bw * (i + 0.5) / 4
        wire(ax, px, y, px, y - 0.5, RED)
        ax.text(px, y - 0.55, f'R{i+1}', ha='center', va='top', fontsize=6, color=RED)
        row_coords[f'R{i+1}'] = (px, y - 0.5)

        cpx = x + bw * (i + 0.5) / 4
        wire(ax, cpx, y + bh, cpx, y + bh + 0.5, BLUE)
        ax.text(cpx, y + bh + 0.52, f'C{i+1}', ha='center', va='bottom',
                fontsize=6, color=BLUE)
        col_coords[f'C{i+1}'] = (cpx, y + bh + 0.5)

    return row_coords, col_coords

def annotation(ax, x, y, text, w=5.5):
    ax.text(x, y, text, fontsize=7.5, va='top', color='#333333',
            bbox=dict(boxstyle='round,pad=0.4', facecolor='#FFFFF0',
                      edgecolor=GRAY, alpha=0.9),
            wrap=True)

def save(fig, folder, fname):
    os.makedirs(folder, exist_ok=True)
    path = os.path.join(folder, fname)
    fig.savefig(path, dpi=180, bbox_inches='tight', facecolor='white')
    plt.close(fig)
    print(f"  ✓  {path}")
    return path

# ─────────────────────────────────────────────────────────────────
#  Helper: gambar LCD + koneksi I2C ke titik koordinat chip
# ─────────────────────────────────────────────────────────────────
def draw_lcd_connected(ax, lcd_x, lcd_y, scl_src, sda_src, vcc_src=None, gnd_src=None):
    """
    Gambar LCD I2C dan tarik kabel dari koordinat sumber (ujung stub chip).
    scl_src, sda_src: (x,y) ujung stub pin chip.
    """
    lpins = lcd_i2c(ax, lcd_x, lcd_y)
    lscl = lpins['SCL']
    lsda = lpins['SDA']
    lvcc = lpins['VCC']
    lgnd = lpins['GND']

    # SCL
    ax.annotate('', xy=lscl, xytext=scl_src,
                arrowprops=dict(arrowstyle='-', color=TEAL, lw=1.3,
                                connectionstyle='arc3,rad=0'))
    ax.plot([scl_src[0], scl_src[0]], [scl_src[1], lscl[1]], color=TEAL, lw=1.1, ls='--')
    ax.plot([scl_src[0], lscl[0]], [lscl[1], lscl[1]], color=TEAL, lw=1.1, ls='--')

    # SDA
    ax.plot([sda_src[0], sda_src[0]], [sda_src[1], lsda[1]], color=PURPLE, lw=1.1, ls='--')
    ax.plot([sda_src[0], lsda[0]], [lsda[1], lsda[1]], color=PURPLE, lw=1.1, ls='--')

    # VCC dan GND LCD
    vcc(ax, lvcc[0] - 0.4, lvcc[1], '3V3', RED)
    gnd(ax, lgnd[0] - 0.4, lgnd[1], BLACK)


# ═══════════════════════════════════════════════════════════════════
#  P01 — LED Parade: Output Push-Pull & Pola Cahaya Digital
# ═══════════════════════════════════════════════════════════════════
def proj01():
    fig, ax = setup(17, 11,
        'P01 — LED Parade: GPIO Output Push-Pull & Pola Cahaya Digital')

    coords = stm32_chip(ax, 3.5, 2.0, h=7.0,
        left_pins=[('3V3', 'VCC'), ('GND', 'GND'),
                   ('PB6 (SCL)', 'SCL'), ('PB7 (SDA)', 'SDA')],
        right_pins=[('PA0', 'PA0'), ('PA1', 'PA1'), ('PA2', 'PA2'), ('PA3', 'PA3'),
                    ('PA4', 'PA4'), ('PA5', 'PA5'), ('PA6', 'PA6'), ('PA7', 'PA7'),
                    ('PC13', 'PC13')])

    # 8 LED PA0-PA7
    colors = ['red', '#FF5500', '#FF9900', 'yellow', 'green', 'cyan', 'blue', '#AA00FF']
    labels = ['LED1', 'LED2', 'LED3', 'LED4', 'LED5', 'LED6', 'LED7', 'LED8']
    for i in range(8):
        x, y = coords[f'PA{i}']
        wire(ax, x, y, x + 0.6, y)
        resistor(ax, x + 0.9, y, horiz=True, label='220Ω')
        wire(ax, x + 1.18, y, x + 1.65, y)
        led_sym(ax, x + 1.65, y, label=labels[i], color=colors[i])
        wire(ax, x + 2.05, y, x + 2.4, y)
        gnd(ax, x + 2.4, y)

    # PC13 (Active-LOW builtin)
    xp, yp = coords['PC13']
    wire(ax, xp, yp, xp + 0.6, yp)
    resistor(ax, xp + 0.9, yp, horiz=True, label='220Ω')
    wire(ax, xp + 1.18, yp, xp + 1.65, yp)
    led_sym(ax, xp + 1.65, yp, label='LED\nPC13\n(builtin)', color='red')
    wire(ax, xp + 2.05, yp, xp + 2.4, yp)
    gnd(ax, xp + 2.4, yp)

    vcc(ax, *coords['VCC'])
    gnd(ax, *coords['GND'])

    # LCD I2C
    draw_lcd_connected(ax, lcd_x=0.3, lcd_y=7.5,
                       scl_src=coords['SCL'], sda_src=coords['SDA'])

    annotation(ax, 0.3, 11.0 - 0.3,
        'PA0–PA7 → 220Ω → LED+ → LED- → GND  (Active-HIGH)\n'
        'PC13 → 220Ω → LED → GND  (Active-LOW, built-in Blue Pill)\n'
        '4 pola: ALL ON, ALL OFF, Running Light, Binary Counter\n'
        'LCD baris 1: nama pola | baris 2: delay saat ini')

    save(fig, os.path.join(BASE, 'STM32_P01_LED_Output_High'), 'schematic.png')


# ═══════════════════════════════════════════════════════════════════
#  P02 — Shadow & Ghost: Active-LOW, Open-Drain & Logika Terbalik
# ═══════════════════════════════════════════════════════════════════
def proj02():
    fig, ax = setup(17, 10,
        'P02 — Shadow & Ghost: Active-LOW, Open-Drain & Logika Terbalik')

    coords = stm32_chip(ax, 4.0, 1.5, h=7.0,
        left_pins=[('3V3', 'VCC'), ('GND', 'GND'),
                   ('PB6 (SCL)', 'SCL'), ('PB7 (SDA)', 'SDA')],
        right_pins=[('PA0 (PP)', 'PA0'), ('PA1 (OD)', 'PA1'),
                    ('PA2 (PP)', 'PA2'), ('PA3 (OD)', 'PA3'),
                    ('PC13 (AL)', 'PC13')])

    # PA0 Push-Pull
    x0, y0 = coords['PA0']
    wire(ax, x0, y0, x0 + 0.6, y0)
    resistor(ax, x0 + 0.9, y0, horiz=True, label='220Ω')
    wire(ax, x0 + 1.18, y0, x0 + 1.65, y0)
    led_sym(ax, x0 + 1.65, y0, label='LED-PP\n(terang)', color='green')
    wire(ax, x0 + 2.05, y0, x0 + 2.4, y0)
    gnd(ax, x0 + 2.4, y0)
    ax.text(x0 + 0.9, y0 + 0.55, 'OUTPUT_PP', ha='center', fontsize=6.5, color=GREEN,
            style='italic')

    # PA1 Open-Drain + internal pull-up 40kΩ
    x1, y1 = coords['PA1']
    wire(ax, x1, y1, x1 + 0.5, y1)
    # pull-up internal
    resistor(ax, x1 + 0.5, y1 + 0.6, horiz=False, label='40kΩ\n(internal)')
    wire(ax, x1 + 0.5, y1 + 0.88, x1 + 0.5, y1 + 1.15)
    vcc(ax, x1 + 0.5, y1 + 1.15, '3V3 (internal)')
    wire(ax, x1 + 0.5, y1 + 0.32, x1 + 0.5, y1)
    wire(ax, x1 + 0.5, y1, x1 + 1.0, y1)
    resistor(ax, x1 + 1.3, y1, horiz=True, label='220Ω')
    wire(ax, x1 + 1.58, y1, x1 + 2.05, y1)
    led_sym(ax, x1 + 2.05, y1, label='LED-OD\n(redup)', color='#FF6600')
    wire(ax, x1 + 2.45, y1, x1 + 2.8, y1)
    gnd(ax, x1 + 2.8, y1)
    ax.text(x1 + 1.3, y1 + 0.55, 'OUTPUT_OD+PULLUP', ha='center',
            fontsize=6.5, color=ORANGE, style='italic')

    # PA2 Push-Pull demo ke-2
    x2, y2 = coords['PA2']
    wire(ax, x2, y2, x2 + 0.6, y2)
    resistor(ax, x2 + 0.9, y2, horiz=True, label='220Ω')
    wire(ax, x2 + 1.18, y2, x2 + 1.65, y2)
    led_sym(ax, x2 + 1.65, y2, label='LED-PP2\n(blink)', color='cyan')
    wire(ax, x2 + 2.05, y2, x2 + 2.4, y2)
    gnd(ax, x2 + 2.4, y2)

    # PA3 Open-Drain tanpa pull-up → Hi-Z saat HIGH
    x3, y3 = coords['PA3']
    wire(ax, x3, y3, x3 + 0.5, y3)
    resistor(ax, x3 + 0.8, y3, horiz=True, label='220Ω')
    wire(ax, x3 + 1.08, y3, x3 + 1.55, y3)
    led_sym(ax, x3 + 1.55, y3, label='LED-OD2\n(mati saat Hi-Z)', color='#AA00FF')
    wire(ax, x3 + 1.95, y3, x3 + 2.3, y3)
    gnd(ax, x3 + 2.3, y3)
    ax.text(x3 + 0.8, y3 + 0.55, 'OD tanpa pull-up → Hi-Z=LED MATI',
            ha='center', fontsize=6, color=PURPLE, style='italic')

    # PC13 Active-LOW
    xc, yc = coords['PC13']
    wire(ax, xc, yc, xc + 0.6, yc)
    resistor(ax, xc + 0.9, yc, horiz=True, label='220Ω')
    wire(ax, xc + 1.18, yc, xc + 1.65, yc)
    led_sym(ax, xc + 1.65, yc, label='LED Active-LOW\n(ON saat output=0)', color='red')
    wire(ax, xc + 2.05, yc, xc + 2.4, yc)
    gnd(ax, xc + 2.4, yc)

    vcc(ax, *coords['VCC'])
    gnd(ax, *coords['GND'])

    draw_lcd_connected(ax, lcd_x=0.3, lcd_y=7.0,
                       scl_src=coords['SCL'], sda_src=coords['SDA'])

    annotation(ax, 0.3, 9.7,
        'PA0 OUTPUT_PP: maju & mundur → LED terang penuh\n'
        'PA1 OUTPUT_OD+PULLUP: Hi-Z saat HIGH → arus kecil (~0.03mA) → LED redup\n'
        'PA3 OUTPUT_OD tanpa pull-up: Hi-Z → LED MATI sepenuhnya\n'
        'PC13 Active-LOW: output LOW → LED ON (logika terbalik!)\n'
        'LCD: menampilkan mode aktif & logika HIGH/LOW tiap fase')

    save(fig, os.path.join(BASE, 'STM32_P02_LED_Output_Low'), 'schematic.png')


# ═══════════════════════════════════════════════════════════════════
#  P03 — Sentinel Gate: Tombol Pull-UP Eksternal 220Ω
# ═══════════════════════════════════════════════════════════════════
def proj03():
    fig, ax = setup(16, 10,
        'P03 — Sentinel Gate: Tombol Pull-UP Eksternal (220Ω ke 3.3V)')

    coords = stm32_chip(ax, 4.5, 1.8, h=6.5,
        left_pins=[('PB0 (INPUT)', 'PB0'), ('3V3', 'VCC'), ('GND', 'GND'),
                   ('PB6 (SCL)', 'SCL'), ('PB7 (SDA)', 'SDA')],
        right_pins=[('PA0', 'PA0'), ('PA1', 'PA1'), ('PA2', 'PA2'), ('PA3', 'PA3')])

    # Rangkaian tombol P03: 3.3V─[220Ω]─PB0─[BTN]─GND (Active-LOW)
    xb, yb = coords['PB0']
    # kabel ke node T
    tx = xb - 0.8
    wire(ax, xb, yb, tx, yb)
    dot(ax, tx, yb)
    # pull-up dari 3.3V via 220Ω ke node T
    resistor(ax, tx, yb + 0.6, horiz=False, label='220Ω\n(pull-up)')
    wire(ax, tx, yb + 0.88, tx, yb + 1.2)
    vcc(ax, tx, yb + 1.2)
    # tombol dari node T ke GND
    wire(ax, tx, yb, tx - 0.6, yb)
    button_sym(ax, tx - 0.9, yb, 'BTN1\n(Active-LOW)')
    wire(ax, tx - 1.4, yb, tx - 1.4, yb - 0.5)
    gnd(ax, tx - 1.4, yb - 0.5)

    ax.text(tx, yb + 1.8,
        '★ PB0 = INPUT NOPULL\n  (tanpa pull internal)',
        ha='center', va='bottom', fontsize=7, color=BLUE,
        bbox=dict(boxstyle='round', facecolor='#EEF', edgecolor=BLUE, alpha=0.7))

    # 4 LED output
    led_colors = ['red', '#FF9900', 'green', 'blue']
    for i in range(4):
        x, y = coords[f'PA{i}']
        wire(ax, x, y, x + 0.6, y)
        resistor(ax, x + 0.9, y, horiz=True, label='220Ω')
        wire(ax, x + 1.18, y, x + 1.65, y)
        led_sym(ax, x + 1.65, y, label=f'LED{i+1}', color=led_colors[i])
        wire(ax, x + 2.05, y, x + 2.4, y)
        gnd(ax, x + 2.4, y)

    vcc(ax, *coords['VCC'])
    gnd(ax, *coords['GND'])

    draw_lcd_connected(ax, lcd_x=0.3, lcd_y=6.5,
                       scl_src=coords['SCL'], sda_src=coords['SDA'])

    annotation(ax, 0.3, 9.7,
        'Wiring: 3.3V ─[220Ω]─ NODE ─ PB0     NODE ─[BTN]─ GND\n'
        'Saat BTN LEPAS: PB0 = HIGH (ditarik ke 3.3V via 220Ω)\n'
        'Saat BTN TEKAN: PB0 = LOW  (NodeGND via BTN)\n'
        'Deteksi falling edge (1→0) → toggle LED → tampil di LCD\n'
        'GPIO mode: INPUT + NOPULL (external resistor sepenuhnya menentukan level)')

    save(fig, os.path.join(BASE, 'STM32_P03_Button_PullUp_Ext'), 'schematic.png')


# ═══════════════════════════════════════════════════════════════════
#  P04 — Ground Guardian: Tombol Pull-DOWN Eksternal 220Ω
# ═══════════════════════════════════════════════════════════════════
def proj04():
    fig, ax = setup(16, 10,
        'P04 — Ground Guardian: Tombol Pull-DOWN Eksternal (220Ω ke GND)')

    coords = stm32_chip(ax, 4.5, 1.8, h=6.5,
        left_pins=[('PB1 (INPUT)', 'PB1'), ('3V3', 'VCC'), ('GND', 'GND'),
                   ('PB6 (SCL)', 'SCL'), ('PB7 (SDA)', 'SDA')],
        right_pins=[('PA0', 'PA0'), ('PA1', 'PA1'), ('PA2', 'PA2'), ('PA3', 'PA3')])

    # Wiring P04: VCC─[BTN]─PB1─[220Ω]─GND (Active-HIGH)
    xb, yb = coords['PB1']
    tx = xb - 0.8
    wire(ax, xb, yb, tx, yb)
    dot(ax, tx, yb)
    # pull-down 220Ω ke GND
    resistor(ax, tx, yb - 0.65, horiz=False, label='220Ω\n(pull-dn)')
    wire(ax, tx, yb - 0.93, tx, yb - 1.2)
    gnd(ax, tx, yb - 1.2)
    # tombol dari 3.3V ke node
    wire(ax, tx, yb, tx - 0.5, yb)
    button_sym(ax, tx - 0.85, yb, 'BTN2\n(Active-HIGH)')
    wire(ax, tx - 1.35, yb, tx - 1.35, yb + 0.6)
    vcc(ax, tx - 1.35, yb + 0.6)

    ax.text(tx, yb - 1.9,
        '★ PB1 = INPUT NOPULL\n  (tanpa pull internal)',
        ha='center', va='top', fontsize=7, color=BLUE,
        bbox=dict(boxstyle='round', facecolor='#EEF', edgecolor=BLUE, alpha=0.7))

    led_colors = ['red', '#FF9900', 'green', 'blue']
    for i in range(4):
        x, y = coords[f'PA{i}']
        wire(ax, x, y, x + 0.6, y)
        resistor(ax, x + 0.9, y, horiz=True, label='220Ω')
        wire(ax, x + 1.18, y, x + 1.65, y)
        led_sym(ax, x + 1.65, y, label=f'LED{i+1}', color=led_colors[i])
        wire(ax, x + 2.05, y, x + 2.4, y)
        gnd(ax, x + 2.4, y)

    vcc(ax, *coords['VCC'])
    gnd(ax, *coords['GND'])

    draw_lcd_connected(ax, lcd_x=0.3, lcd_y=6.5,
                       scl_src=coords['SCL'], sda_src=coords['SDA'])

    annotation(ax, 0.3, 9.7,
        'Wiring: 3.3V ─[BTN]─ NODE ─ PB1     NODE ─[220Ω]─ GND\n'
        'Saat BTN LEPAS: PB1 = LOW  (ditarik ke GND via 220Ω)\n'
        'Saat BTN TEKAN: PB1 = HIGH (3.3V masuk langsung)\n'
        'Deteksi rising edge (0→1) → toggle LED → tampil di LCD\n'
        'JANGAN hubungkan 5V ke PB1 — STM32 max 3.3V on GPIO (kecuali pin 5V-tolerant)')

    save(fig, os.path.join(BASE, 'STM32_P04_Button_PullDown_Ext'), 'schematic.png')


# ═══════════════════════════════════════════════════════════════════
#  P05 — Phantom Touch: Pull-UP Internal & Tombol Tanpa Resistor
# ═══════════════════════════════════════════════════════════════════
def proj05():
    fig, ax = setup(16, 10,
        'P05 — Phantom Touch: Pull-UP Internal ~40kΩ (Tanpa Resistor Eksternal)')

    coords = stm32_chip(ax, 4.2, 1.8, h=7.0,
        left_pins=[('PB0 (PULLUP)', 'PB0'), ('PB1 (PULLUP)', 'PB1'),
                   ('3V3', 'VCC'), ('GND', 'GND'),
                   ('PB6 (SCL)', 'SCL'), ('PB7 (SDA)', 'SDA')],
        right_pins=[('PA0', 'PA0'), ('PA1', 'PA1'), ('PA2', 'PA2'), ('PA3', 'PA3')])

    # internal pull-up annotation pada chip
    ax.text(4.2 + 2.8 / 2, 1.8 + 7.0 / 2, 'PULLUP\n~40kΩ\n(internal)', ha='center',
            va='center', fontsize=7, color=TEAL, style='italic',
            bbox=dict(boxstyle='round', facecolor='#E0FFE0', edgecolor=TEAL, alpha=0.6))

    # BTN1 langsung ke GND (tanpa resistor)
    xb0, yb0 = coords['PB0']
    wire(ax, xb0, yb0, xb0 - 0.6, yb0)
    button_sym(ax, xb0 - 0.9, yb0, 'BTN1')
    wire(ax, xb0 - 1.4, yb0, xb0 - 1.4, yb0 - 0.5)
    gnd(ax, xb0 - 1.4, yb0 - 0.5)
    ax.text(xb0 - 0.9, yb0 + 0.6, 'Langsung ke GND\n(NO resistor!)',
            ha='center', fontsize=6.5, color=RED,
            bbox=dict(boxstyle='round', facecolor='#FFE0E0', edgecolor=RED, alpha=0.7))

    # BTN2 langsung ke GND
    xb1, yb1 = coords['PB1']
    wire(ax, xb1, yb1, xb1 - 0.6, yb1)
    button_sym(ax, xb1 - 0.9, yb1, 'BTN2')
    wire(ax, xb1 - 1.4, yb1, xb1 - 1.4, yb1 - 0.5)
    gnd(ax, xb1 - 1.4, yb1 - 0.5)

    led_colors = ['red', '#FF9900', 'green', 'blue']
    for i in range(4):
        x, y = coords[f'PA{i}']
        wire(ax, x, y, x + 0.6, y)
        resistor(ax, x + 0.9, y, horiz=True, label='220Ω')
        wire(ax, x + 1.18, y, x + 1.65, y)
        led_sym(ax, x + 1.65, y, label=f'LED{i+1}', color=led_colors[i])
        wire(ax, x + 2.05, y, x + 2.4, y)
        gnd(ax, x + 2.4, y)

    vcc(ax, *coords['VCC'])
    gnd(ax, *coords['GND'])

    draw_lcd_connected(ax, lcd_x=0.3, lcd_y=7.2,
                       scl_src=coords['SCL'], sda_src=coords['SDA'])

    annotation(ax, 0.3, 9.7,
        'PB0 & PB1: GPIO_MODE_INPUT + GPIO_PULLUP (~40kΩ ke VCC secara internal)\n'
        'Wiring BTN: satu kaki ke PB0, kaki lain langsung ke GND — TANPA resistor\n'
        'Saat lepas → HIGH (ditarik internal); Saat tekan → LOW (GND masuk)\n'
        'Hemat komponen vs P03, namun pull-up lebih lemah (40kΩ vs 220Ω)\n'
        'LCD: tampilkan status BTN1/BTN2, jumlah press, toggle state LED')

    save(fig, os.path.join(BASE, 'STM32_P05_Button_PullUp_Internal'), 'schematic.png')


# ═══════════════════════════════════════════════════════════════════
#  P06 — Force Field: Pull-DOWN Internal & Logika Active-HIGH
# ═══════════════════════════════════════════════════════════════════
def proj06():
    fig, ax = setup(16, 10,
        'P06 — Force Field: Pull-DOWN Internal ~40kΩ (Active-HIGH)')

    coords = stm32_chip(ax, 4.2, 1.8, h=7.0,
        left_pins=[('PB1 (PULLDOWN)', 'PB1'), ('PB3 (PULLDOWN)', 'PB3'),
                   ('3V3', 'VCC'), ('GND', 'GND'),
                   ('PB6 (SCL)', 'SCL'), ('PB7 (SDA)', 'SDA')],
        right_pins=[('PA0', 'PA0'), ('PA1', 'PA1'), ('PA2', 'PA2'), ('PA3', 'PA3')])

    ax.text(4.2 + 2.8 / 2, 1.8 + 7.0 / 2, 'PULLDOWN\n~40kΩ\n(internal)', ha='center',
            va='center', fontsize=7, color=ORANGE, style='italic',
            bbox=dict(boxstyle='round', facecolor='#FFF3E0', edgecolor=ORANGE, alpha=0.6))

    for pin_name, btn_label in [('PB1', 'BTN1'), ('PB3', 'BTN3')]:
        xb, yb = coords[pin_name]
        wire(ax, xb, yb, xb - 0.6, yb)
        button_sym(ax, xb - 0.9, yb, btn_label)
        wire(ax, xb - 1.4, yb, xb - 1.4, yb + 0.6)
        vcc(ax, xb - 1.4, yb + 0.6)
        ax.text(xb - 0.9, yb - 0.55, '3.3V saat tekan\n(Active-HIGH)',
                ha='center', fontsize=6.5, color=GREEN,
                bbox=dict(boxstyle='round', facecolor='#E0FFE0', edgecolor=GREEN, alpha=0.7))

    led_colors = ['red', '#FF9900', 'green', 'blue']
    for i in range(4):
        x, y = coords[f'PA{i}']
        wire(ax, x, y, x + 0.6, y)
        resistor(ax, x + 0.9, y, horiz=True, label='220Ω')
        wire(ax, x + 1.18, y, x + 1.65, y)
        led_sym(ax, x + 1.65, y, label=f'LED{i+1}', color=led_colors[i])
        wire(ax, x + 2.05, y, x + 2.4, y)
        gnd(ax, x + 2.4, y)

    vcc(ax, *coords['VCC'])
    gnd(ax, *coords['GND'])

    draw_lcd_connected(ax, lcd_x=0.3, lcd_y=7.2,
                       scl_src=coords['SCL'], sda_src=coords['SDA'])

    annotation(ax, 0.3, 9.7,
        'PB1 & PB3: GPIO_MODE_INPUT + GPIO_PULLDOWN (~40kΩ ke GND secara internal)\n'
        'SKIP PB2 = BOOT1 pada Blue Pill (jangan gunakan!)\n'
        'Wiring BTN: satu kaki ke PBx, kaki lain ke 3.3V — TANPA resistor\n'
        'Saat lepas → LOW; Saat tekan → HIGH (3.3V) → rising edge → toggle LED\n'
        'LCD: tampilkan status BTN, jumlah press, perbandingan dengan P05 (pull-up)')

    save(fig, os.path.join(BASE, 'STM32_P06_Button_PullDown_Internal'), 'schematic.png')


# ═══════════════════════════════════════════════════════════════════
#  P07 — Clean Contact: Debounce State Machine & Penghitung Akurat
# ═══════════════════════════════════════════════════════════════════
def proj07():
    fig, ax = setup(17, 11,
        'P07 — Clean Contact: Debounce State Machine & Penghitung Akurat')

    coords = stm32_chip(ax, 4.5, 1.5, h=8.0,
        left_pins=[('PB0 (PULLUP)', 'PB0'), ('PB1 (PULLUP)', 'PB1'),
                   ('PB3 (PULLUP)', 'PB3'), ('PB4 (PULLUP)', 'PB4'),
                   ('3V3', 'VCC'), ('GND', 'GND'),
                   ('PB6 (SCL)', 'SCL'), ('PB7 (SDA)', 'SDA')],
        right_pins=[('PA0', 'PA0'), ('PA1', 'PA1'),
                    ('PA2', 'PA2'), ('PA3', 'PA3')])

    btn_labels = ['BTN1', 'BTN2', 'BTN3', 'BTN4']
    btn_pins   = ['PB0', 'PB1', 'PB3', 'PB4']
    for j, (pname, blabel) in enumerate(zip(btn_pins, btn_labels)):
        xb, yb = coords[pname]
        wire(ax, xb, yb, xb - 0.6, yb)
        button_sym(ax, xb - 0.9, yb, blabel)
        wire(ax, xb - 1.4, yb, xb - 1.4, yb - 0.5)
        gnd(ax, xb - 1.4, yb - 0.5)

    led_colors = ['red', '#FF9900', 'green', 'blue']
    for i in range(4):
        x, y = coords[f'PA{i}']
        wire(ax, x, y, x + 0.6, y)
        resistor(ax, x + 0.9, y, horiz=True, label='220Ω')
        wire(ax, x + 1.18, y, x + 1.65, y)
        led_sym(ax, x + 1.65, y, label=f'LED{i+1}', color=led_colors[i])
        wire(ax, x + 2.05, y, x + 2.4, y)
        gnd(ax, x + 2.4, y)

    # State machine diagram
    sm_x, sm_y = 0.3, 4.5
    states = [('RELEASED', 0.5), ('DEBOUNCING', 2.5), ('PRESSED', 4.5)]
    for sname, sx in states:
        ax.add_patch(mpatches.FancyBboxPatch(
            (sm_x + sx, sm_y), 1.6, 0.55, boxstyle="round,pad=0.06",
            linewidth=1.2, edgecolor=BLUE, facecolor='#EEF'))
        ax.text(sm_x + sx + 0.8, sm_y + 0.275, sname, ha='center', va='center',
                fontsize=6.5, color=BLUE, fontweight='bold')
    for sx_from, sx_to in [(0.5 + 1.6, 2.5), (2.5 + 1.6, 4.5)]:
        ax.annotate('', xy=(sm_x + sx_to, sm_y + 0.275),
                    xytext=(sm_x + sx_from, sm_y + 0.275),
                    arrowprops=dict(arrowstyle='->', color=ORANGE, lw=1.2))
    ax.text(sm_x + 3.0, sm_y + 0.7, '50ms ok', ha='center', fontsize=6, color=ORANGE)
    ax.text(sm_x + 1.6, sm_y + 0.7, 'tekan terdeteksi', ha='center', fontsize=6, color=ORANGE)
    ax.text(sm_x + 2.0, sm_y - 0.2, 'State Machine Debounce (DEBOUNCE_MS=50)',
            fontsize=7, color=BLUE)

    vcc(ax, *coords['VCC'])
    gnd(ax, *coords['GND'])

    draw_lcd_connected(ax, lcd_x=0.3, lcd_y=8.0,
                       scl_src=coords['SCL'], sda_src=coords['SDA'])

    annotation(ax, 0.3, 10.7,
        '4 tombol (PB0,PB1,PB3,PB4) masing-masing independen state machine\n'
        'RELEASED → tekan terdeteksi → DEBOUNCING (50ms) → PRESSED\n'
        'HAL_GetTick() digunakan untuk timing — bukan delay blocking\n'
        'Setiap BTN toggle LEDnya masing-masing + increment counter\n'
        'LCD baris 1: C1:xx C2:xx  |  baris 2: C3:xx C4:xx  (press count)')

    save(fig, os.path.join(BASE, 'STM32_P07_Button_Debounce'), 'schematic.png')


# ═══════════════════════════════════════════════════════════════════
#  P08 — Speed Racer: Kecepatan GPIO & Pola LED Multi-Kecepatan
# ═══════════════════════════════════════════════════════════════════
def proj08():
    fig, ax = setup(17, 11,
        'P08 — Speed Racer: Kecepatan GPIO Slew Rate & Pola LED Multi-Speed')

    coords = stm32_chip(ax, 4.0, 1.5, h=8.5,
        left_pins=[('PB0 (PULLUP)', 'PB0'), ('PB1 (PULLUP)', 'PB1'),
                   ('PB3 (PULLUP)', 'PB3'), ('PB4 (PULLUP)', 'PB4'),
                   ('3V3', 'VCC'), ('GND', 'GND'),
                   ('PB6 (SCL)', 'SCL'), ('PB7 (SDA)', 'SDA')],
        right_pins=[('PA0', 'PA0'), ('PA1', 'PA1'), ('PA2', 'PA2'), ('PA3', 'PA3'),
                    ('PA4', 'PA4'), ('PA5', 'PA5'), ('PA6', 'PA6'), ('PA7', 'PA7')])

    btn_labels = ['BTN1\n(SLOW 2MHz)', 'BTN2\n(MED 10MHz)',
                  'BTN3\n(FAST 50MHz)', 'BTN4\n(BinCount)']
    for j, (pname, blabel) in enumerate(zip(['PB0','PB1','PB3','PB4'], btn_labels)):
        xb, yb = coords[pname]
        wire(ax, xb, yb, xb - 0.6, yb)
        button_sym(ax, xb - 0.9, yb, blabel)
        wire(ax, xb - 1.4, yb, xb - 1.4, yb - 0.5)
        gnd(ax, xb - 1.4, yb - 0.5)

    colors_8 = ['red','#FF5500','#FF9900','yellow','green','cyan','blue','#AA00FF']
    for i in range(8):
        x, y = coords[f'PA{i}']
        wire(ax, x, y, x + 0.55, y)
        resistor(ax, x + 0.85, y, horiz=True, label='220Ω')
        wire(ax, x + 1.13, y, x + 1.58, y)
        led_sym(ax, x + 1.58, y, label=f'L{i}', color=colors_8[i])
        wire(ax, x + 1.98, y, x + 2.3, y)
        gnd(ax, x + 2.3, y)

    # speed table
    tbl_x, tbl_y = 0.3, 4.5
    hdrs = ['GPIO Speed', 'Slew Rate', 'Pola', 'Tombol']
    rows = [['FREQ_LOW', '2 MHz', 'Running Light', 'BTN1'],
            ['FREQ_MEDIUM', '10 MHz', 'Knight Rider', 'BTN2'],
            ['FREQ_HIGH', '50 MHz', 'Alternating', 'BTN3'],
            ['FREQ_MEDIUM', '10 MHz', 'Binary Counter', 'BTN4']]
    col_w = [1.4, 1.1, 1.5, 1.0]
    for ci, h in enumerate(hdrs):
        ax.text(tbl_x + sum(col_w[:ci]) + col_w[ci]/2, tbl_y + 0.7, h,
                ha='center', va='center', fontsize=6.5, fontweight='bold', color=BLUE)
    for ri, row in enumerate(rows):
        for ci, cell in enumerate(row):
            ax.text(tbl_x + sum(col_w[:ci]) + col_w[ci]/2, tbl_y + 0.3 - ri*0.3, cell,
                    ha='center', va='center', fontsize=6)
    ax.add_patch(mpatches.FancyBboxPatch(
        (tbl_x, tbl_y - rows.__len__()*0.3 + 0.1), sum(col_w), 0.85,
        boxstyle="square,pad=0", linewidth=1, edgecolor=GRAY, facecolor='#FAFAF0'))

    vcc(ax, *coords['VCC'])
    gnd(ax, *coords['GND'])

    draw_lcd_connected(ax, lcd_x=0.3, lcd_y=8.8,
                       scl_src=coords['SCL'], sda_src=coords['SDA'])

    annotation(ax, 0.3, 10.8,
        'BTN1 → SLOW (2MHz slew): Running Light [PA0→PA7 bergiliran]\n'
        'BTN2 → MED  (10MHz):    Knight Rider  [ping-pong PA0↔PA7]\n'
        'BTN3 → FAST (50MHz):    Alternating   [PA0,2,4,6 vs PA1,3,5,7]\n'
        'BTN4 → MED  (10MHz):    Binary Counter [ODR++]\n'
        'GPIO Speed adalah SLEW RATE, BUKAN frekuensi toggle — ubah via HAL_GPIO_Init runtime\n'
        'LCD: tampilkan speed aktif, nama pola, nilai ODR saat Binary Counter')

    save(fig, os.path.join(BASE, 'STM32_P08_LED_Patterns'), 'schematic.png')


# ═══════════════════════════════════════════════════════════════════
#  P09 — Twist & Count: Rotary Encoder Kuadratur & Counter LCD
# ═══════════════════════════════════════════════════════════════════
def proj09():
    fig, ax = setup(17, 11,
        'P09 — Twist & Count: Rotary Encoder Kuadratur & Counter LCD')

    coords = stm32_chip(ax, 4.5, 1.5, h=8.5,
        left_pins=[('3V3', 'VCC'), ('GND', 'GND'),
                   ('PB6 (SCL)', 'SCL'), ('PB7 (SDA)', 'SDA')],
        right_pins=[('PB12 (CLK)', 'CLK'), ('PB13 (DT)', 'DT'), ('PB14 (SW)', 'SW'),
                    ('PA0', 'PA0'), ('PA1', 'PA1'), ('PA2', 'PA2'), ('PA3', 'PA3'),
                    ('PA4', 'PA4'), ('PA5', 'PA5'), ('PA6', 'PA6'), ('PA7', 'PA7')])

    # Encoder 5-pin
    enc_coords = encoder_sym(ax, 8.2, 6.5, bw=2.2, bh=2.2)

    # Koneksi CLK, DT, SW dari chip ke encoder
    for sig, pin, col in [('CLK', 'CLK', ORANGE), ('DT', 'DT', GREEN), ('SW', 'SW', RED)]:
        xc, yc = coords[pin]
        ex, ey = enc_coords[sig]
        # horizontal ke kanan lalu vertikal ke encoder
        mid_x = (xc + ex) / 2 + 0.3
        wire(ax, xc, yc, mid_x, yc, col)
        wire(ax, mid_x, yc, mid_x, ey, col)
        wire(ax, mid_x, ey, ex, ey, col)
        ax.text(xc + 0.3, yc + 0.12, sig, fontsize=6.5, color=col)

    # GND dan VCC encoder
    eg, yg = enc_coords['GND']
    ev, yvp = enc_coords['+']
    gnd(ax, eg, yg)
    vcc(ax, ev, yvp, '3V3')

    # timing diagram sederhana
    td_x, td_y = 0.3, 5.5
    ax.text(td_x, td_y + 0.9, 'Timing Kuadratur:', fontsize=7, color=BLUE, fontweight='bold')
    for i, (sig, off, col) in enumerate([('CLK', 0, ORANGE), ('DT', 0.5, GREEN)]):
        ty = td_y + 0.6 - i * 0.45
        ax.text(td_x, ty, sig, fontsize=6.5, color=col, va='center')
        # dummy waveform CW
        xs = [td_x+0.4, td_x+0.4, td_x+0.8, td_x+0.8, td_x+1.2, td_x+1.2, td_x+1.6,
              td_x+1.6+off*0.1]
        ys_hi = [ty-0.1, ty+0.08, ty+0.08, ty-0.1, ty-0.1, ty+0.08, ty+0.08, ty-0.1]
        if i == 1:
            # DT versetzt
            xs = [td_x+0.4, td_x+0.6, td_x+0.6, td_x+1.0, td_x+1.0,
                  td_x+1.4, td_x+1.4, td_x+1.8]
            ys_hi = [ty+0.08,ty+0.08,ty-0.1,ty-0.1,ty+0.08,ty+0.08,ty-0.1,ty-0.1]
        ax.plot(xs, ys_hi, color=col, lw=1.2)
    ax.text(td_x + 0.9, td_y - 0.05, 'CW: CLK↓ & DT=HIGH → +1\nCCW: CLK↓ & DT=LOW → -1',
            fontsize=6.5, color=GRAY)

    # 8 LED
    colors_8 = ['red','#FF5500','#FF9900','yellow','green','cyan','blue','#AA00FF']
    for i in range(8):
        x, y = coords[f'PA{i}']
        wire(ax, x, y, x + 0.55, y)
        resistor(ax, x + 0.85, y, horiz=True, label='220Ω')
        wire(ax, x + 1.13, y, x + 1.58, y)
        led_sym(ax, x + 1.58, y, label=f'b{i}', color=colors_8[i])
        wire(ax, x + 1.98, y, x + 2.28, y)
        gnd(ax, x + 2.28, y)

    vcc(ax, *coords['VCC'])
    gnd(ax, *coords['GND'])

    draw_lcd_connected(ax, lcd_x=0.3, lcd_y=8.5,
                       scl_src=coords['SCL'], sda_src=coords['SDA'])

    annotation(ax, 0.3, 10.8,
        'Encoder: CLK=PB12, DT=PB13, SW=PB14 — semua INPUT PULLUP\n'
        'Algoritma: CLK falling edge → baca DT: HIGH→CW(+1), LOW→CCW(-1)\n'
        'Putar CW: count naik 0–255 | Putar CCW: count turun | Tekan SW: reset ke 128\n'
        'LED PA0-PA7: tampilkan count dalam biner (8-bit) via GPIOA→BSRR atomik\n'
        'LCD baris 1: Count=xxx [CW/CCW] | baris 2: BAR ========    ')

    save(fig, os.path.join(BASE, 'STM32_P09_Encoder_5Pin'), 'schematic.png')


# ═══════════════════════════════════════════════════════════════════
#  P10 — Matrix Commander: Pemindaian Keypad 4×4 & Tampilan LCD
# ═══════════════════════════════════════════════════════════════════
def proj10():
    fig, ax = setup(18, 12,
        'P10 — Matrix Commander: Pemindaian Keypad 4×4 & Tampilan LCD')

    coords = stm32_chip(ax, 4.0, 1.5, h=9.5,
        left_pins=[('3V3', 'VCC'), ('GND', 'GND'),
                   ('PB6 (SCL)', 'SCL'), ('PB7 (SDA)', 'SDA')],
        right_pins=[('PB8 (ROW1)', 'R1'), ('PB9 (ROW2)', 'R2'),
                    ('PB10 (ROW3)', 'R3'), ('PB11 (ROW4)', 'R4'),
                    ('PB12 (COL1)', 'C1'), ('PB13 (COL2)', 'C2'),
                    ('PB14 (COL3)', 'C3'), ('PB15 (COL4)', 'C4'),
                    ('PA0', 'PA0'), ('PA1', 'PA1'), ('PA2', 'PA3'),
                    ('PA4', 'PA4'), ('PA5', 'PA5'), ('PA6', 'PA6'), ('PA7', 'PA7')])

    # Keypad block
    row_c, col_c = keypad_4x4_block(ax, 9.5, 4.5, bw=3.2, bh=3.2)

    # Koneksi Row (PB8-PB11 → Row1-Row4 keypad)
    row_colors = [RED, '#FF5500', '#FF9900', 'green']
    for i, (rpin, rkey, col) in enumerate(zip(['R1','R2','R3','R4'],
                                               ['R1','R2','R3','R4'], row_colors)):
        if rpin not in coords: continue
        xc, yc = coords[rpin]
        kx, ky = row_c[rkey]
        # route ke kanan lalu ke bawah ke keypad
        wire(ax, xc, yc, kx, yc, col)
        wire(ax, kx, yc, kx, ky, col)

    # Koneksi Col (PB12-PB15 → Col1-Col4 keypad, INPUT PULLUP)
    col_colors = [BLUE, TEAL, PURPLE, ORANGE]
    for i, (cpin, ckey, col) in enumerate(zip(['C1','C2','C3','C4'],
                                               ['C1','C2','C3','C4'], col_colors)):
        if cpin not in coords: continue
        xc, yc = coords[cpin]
        kx, ky = col_c[ckey]
        wire(ax, xc, yc, kx, yc, col)
        wire(ax, kx, yc, kx, ky, col)

    # Label Row/Col
    ax.text(7.5, 7.0, 'ROW: OUTPUT_PP\n(set LOW satu per satu)', ha='center',
            fontsize=6.5, color=RED,
            bbox=dict(boxstyle='round', facecolor='#FFE0E0', edgecolor=RED, alpha=0.7))
    ax.text(7.5, 5.0, 'COL: INPUT PULLUP\n(baca; LOW = tertekan)', ha='center',
            fontsize=6.5, color=BLUE,
            bbox=dict(boxstyle='round', facecolor='#E0E0FF', edgecolor=BLUE, alpha=0.7))

    # LED PA0-PA7 (tampilkan kode key)
    colors_8 = ['red','#FF5500','#FF9900','yellow','green','cyan','blue','#AA00FF']
    for i in range(8):
        pin = f'PA{i}' if i != 2 else 'PA0'   # PA2 dan PA3 share slot liat coord
        pkey = ['PA0','PA1','PA2','PA3','PA4','PA5','PA6','PA7'][i]
        if pkey not in coords:
            continue
        x, y = coords[pkey]
        wire(ax, x, y, x + 0.5, y)
        resistor(ax, x + 0.8, y, horiz=True, label='220Ω')
        wire(ax, x + 1.08, y, x + 1.53, y)
        led_sym(ax, x + 1.53, y, label=f'b{i}', color=colors_8[i])
        wire(ax, x + 1.93, y, x + 2.2, y)
        gnd(ax, x + 2.2, y)

    vcc(ax, *coords['VCC'])
    gnd(ax, *coords['GND'])

    draw_lcd_connected(ax, lcd_x=0.3, lcd_y=9.5,
                       scl_src=coords['SCL'], sda_src=coords['SDA'])

    annotation(ax, 0.3, 11.7,
        'ROW PB8-PB11: OUTPUT_PP — set semua HIGH, lalu LOW per baris\n'
        'COL PB12-PB15: INPUT_PULLUP — jika LOW saat ROW=LOW → tombol tertekan\n'
        'Scanning: for row in 0..3: set_row_LOW → read cols → if col=LOW → key found\n'
        'KEYMAP[4][4]: row×col → karakter / angka (1-16)\n'
        'LED PA0-PA7: tampilkan kode biner key (0-15) via BSRR\n'
        'LCD baris 1: KEY PRESSED: [karakter] | baris 2: Total press: xxx')

    save(fig, os.path.join(BASE, 'STM32_P10_Keypad_8Pin'), 'schematic.png')


# ═══════════════════════════════════════════════════════════════════
#  Run all
# ═══════════════════════════════════════════════════════════════════
if __name__ == '__main__':
    print('=' * 65)
    print('  GPIO Schematic Generator — 10 Praktikum')
    print('  STM32F103 Blue Pill + LCD I2C 16x2')
    print('=' * 65)

    tasks = [
        ('P01 LED_Parade',         proj01),
        ('P02 Shadow_Ghost',       proj02),
        ('P03 Sentinel_Gate',      proj03),
        ('P04 Ground_Guardian',    proj04),
        ('P05 Phantom_Touch',      proj05),
        ('P06 Force_Field',        proj06),
        ('P07 Clean_Contact',      proj07),
        ('P08 Speed_Racer',        proj08),
        ('P09 Twist_Count',        proj09),
        ('P10 Matrix_Commander',   proj10),
    ]

    ok = 0
    for name, fn in tasks:
        print(f'\n── {name}')
        try:
            fn()
            ok += 1
        except Exception as e:
            print(f'  ✗  ERROR: {e}')
            import traceback; traceback.print_exc()

    print(f'\n{"=" * 65}')
    print(f'  Selesai: {ok}/{len(tasks)} schematic dibuat')
    print(f'  Tersimpan di: .../praktikum/STM32/STM32_Pxx_.../schematic.png')
    print('=' * 65)
