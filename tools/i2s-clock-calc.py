import math

# ----------------- USER SETTINGS -----------------

HSE = 8_000_000  # Hz, external crystal frequency
TARGET_I2S_FREQ = 48_000 # Hz
I2S_DATAFORMAT_16B = True
I2S_MCLKOUTPUT_ENABLE = True

# Frequency constraints (adjust to match your exact datasheet if needed)
PLL_IN_MIN =   950_000      # Hz, f_HSE / PLLM min
PLL_IN_MAX = 2_100_000      # Hz, f_HSE / PLLM max

VCO_MIN = 100_000_000       # Hz, VCO min
VCO_MAX = 432_000_000       # Hz, VCO max

# Optional: constrain I2S clock range (broad but non-insane)
I2S_MIN = 43_008_000        # Hz
I2S_MAX = 216_000_000       # Hz

# Search ranges from reference manual
PLLM_RANGE = range(2, 64)       # 2..63
PLLI2SN_RANGE = range(192, 433) # 192..432
PLLI2SR_RANGE = range(2, 8)     # 2..7

TOP_RESULTS = 15  # how many best configs to print

# ----------------- CALCULATION -----------------

def calc_pll_in(hse, pllm):
    return hse / pllm

def calc_vco(f_pll_in, plln):
    return f_pll_in * plln

def calc_i2s(f_vco, pllr):
    return f_vco / pllr


# Drivers\STM32F4xx_HAL_Driver\Src\stm32f4xx_hal_i2s.c
def calc_i2s_internal_clk(i2sclk):
    packetlength = 0
        
    if (I2S_DATAFORMAT_16B):
        # Packet length is 16 bits 
        packetlength = 16
    else:
        # Packet length is 32 bits
        packetlength = 32
     
    # in I2S, LSB, MSB format the packet length is multiplied by 2
    packetlength = packetlength * 2
    
    if I2S_MCLKOUTPUT_ENABLE:
        # MCLK output enabled
        if not I2S_DATAFORMAT_16B:
            tmp = (i2sclk / (packetlength * 4))  / TARGET_I2S_FREQ
        else:
            tmp = (i2sclk / (packetlength * 8))  / TARGET_I2S_FREQ
    else:
        # MCLK output disabled
        tmp = (i2sclk / packetlength)  / TARGET_I2S_FREQ

    
    tmp_int = math.round(tmp);
    err = abs(100 * (tmp - tmp_int) / tmp_int);
    return err


results = []

for pllm in PLLM_RANGE:
    f_pll_in = calc_pll_in(HSE, pllm)
    if not (PLL_IN_MIN <= f_pll_in <= PLL_IN_MAX):
        continue

    for plln in PLLI2SN_RANGE:
        f_vco = calc_vco(f_pll_in, plln)
        if not (VCO_MIN <= f_vco <= VCO_MAX):
            continue

        for pllr in PLLI2SR_RANGE:
            f_i2s = calc_i2s(f_vco, pllr)
            if not (I2S_MIN <= f_i2s <= I2S_MAX):
                continue


            error = calc_i2s_internal_clk(f_i2s)
            
           # error = abs(f_i2s - TARGET_I2S)

            results.append({
                "PLLM": pllm,
                "PLLI2SN": plln,
                "PLLI2SR": pllr,
                "I2S_Freq": f_i2s,
                "Error": error
            })

# Sort by smallest error
results.sort(key=lambda x: x["Error"])

if not results:
    print("No valid PLLI2S configuration found with current constraints.")
else:
    print(f"\nTop {min(TOP_RESULTS, len(results))} closest PLLI2S configurations:\n")
    for r in results[:TOP_RESULTS]:
        print(
            f"PLLM={r['PLLM']:2d}, PLLI2SN={r['PLLI2SN']:3d}, PLLI2SR={r['PLLI2SR']}: "
            f"I2S={r['I2S_Freq']/1e6:8.4f} MHz  "
            f"=> error={r['Error']:.8f} %"
        )
