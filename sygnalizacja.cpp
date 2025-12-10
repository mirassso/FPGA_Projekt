#include <systemc.h>
#include <iostream>

#define SW0 (1 << 0)
#define SW1 (1 << 1)
#define SW2 (1 << 2)
#define SW3 (1 << 3)
#define SW4 (1 << 4)

#define LED0 (1 << 0)
#define LED1 (1 << 1)
#define LED2 (1 << 2)
#define LED3 (1 << 3)
#define LED4 (1 << 4)

#define ZERO  0x3F
#define ONE   0x06
#define TWO   0x5B
#define THREE 0x4F
#define FOUR  0x66
#define HEX_N 0x37
#define HEX_O 0x3F

#define SEGA  0x01
#define SEGC  0x04
#define SEGD  0x08
#define SEGG  0x40

typedef enum {
    PH_RED,
    PH_RED_YELLOW,
    PH_GREEN,
    PH_YELLOW,
    PH_RED_GREEN,
    PH_NONO
} Phase;

Phase decode_desired_phase(const int r, const int y, const int g) {
    if (r && y && !g) return PH_RED_YELLOW;
    if (g && !r && !y) return PH_GREEN;
    if (y && !r && !g) return PH_YELLOW;
    if (r && !y && !g) return PH_RED;
    if (r && !y && g) return PH_RED_GREEN;
    return PH_NONO;
}

int is_legal_next(Phase from, Phase to) {
    return (from == PH_RED && to == PH_RED_YELLOW) ||
           (from == PH_RED_YELLOW && to == PH_GREEN) ||
           (from == PH_GREEN && to == PH_YELLOW) ||
           (from == PH_YELLOW && to == PH_RED) ||
           (from == PH_RED && to == PH_RED_GREEN) ||
           (from == PH_RED_GREEN && to == PH_RED);
}


SC_MODULE(TrafficController) {
    sc_in<bool> clk;
    sc_in<sc_uint<10>> sw_sliders;
    
    sc_out<sc_uint<10>> leds;
    sc_out<sc_uint<7>> hex[6];

    int powerOn;
    int emergencyBlink;
    Phase currentPhase;

    void logic_process() {
        int swstate = sw_sliders.read();
        int current_leds = 0;
        
        
        if (swstate & SW0) {
            powerOn = 1;
            current_leds |= LED0;
        } else {
            powerOn = 0;
            currentPhase = PH_RED;
            
            leds.write(0);
            for(int i=0; i<6; i++) hex[i].write(0);
            return;
        }

        if (swstate & SW1) {
            emergencyBlink = 1;
            currentPhase = PH_RED;
            leds.write(LED1);
            return;
        } else {
            emergencyBlink = 0;
        }

        int hex0_val = 0;
        int hex5_val = 0;

        if (currentPhase == PH_RED) {
            current_leds |= LED2;
            hex0_val = SEGA;
            hex5_val = ZERO;
        } else if (currentPhase == PH_RED_YELLOW) {
            current_leds |= (LED2 | LED3);
            hex0_val = SEGA | SEGG;
            hex5_val = ONE;
        } else if (currentPhase == PH_GREEN) {
            current_leds |= LED4;
            hex0_val = SEGD;
            hex5_val = TWO;
        } else if (currentPhase == PH_YELLOW) {
            current_leds |= LED3;
            hex0_val = SEGG;
            hex5_val = THREE;
        } else if (currentPhase == PH_RED_GREEN) {
            current_leds |= (LED2 | LED4);
            hex0_val = SEGA | SEGC;
            hex5_val = FOUR;
        }

        leds.write(current_leds);
        hex[0].write(hex0_val);
        hex[5].write(hex5_val);

        int sw2 = (swstate & SW2) ? 1 : 0;
        int sw3 = (swstate & SW3) ? 1 : 0;
        int sw4 = (swstate & SW4) ? 1 : 0;

        Phase desired_phase = decode_desired_phase(sw2, sw3, sw4);

        if (desired_phase == PH_NONO) {
            hex[3].write(HEX_N);
            hex[2].write(HEX_O);
        } else if (is_legal_next(currentPhase, desired_phase)) {
            currentPhase = desired_phase;
            hex[2].write(0);
            hex[3].write(0);
            
        }
    }

    SC_CTOR(TrafficController) {
        SC_METHOD(logic_process);
        sensitive << sw_sliders;
        
        powerOn = 0;
        emergencyBlink = 0;
        currentPhase = PH_GREEN;
    }
};

int sc_main(int argc, char* argv[]) {
    sc_signal<bool> clk;
    sc_signal<sc_uint<10>> sw_sliders;
    sc_signal<sc_uint<10>> leds;
    sc_signal<sc_uint<7>> hex[6];

    TrafficController dut("TrafficController");
    dut.clk(clk);
    dut.sw_sliders(sw_sliders);
    dut.leds(leds);
    for(int i=0; i<6; i++) dut.hex[i](hex[i]);

    sc_trace_file *tf = sc_create_vcd_trace_file("traffic_waves");
    sc_trace(tf, sw_sliders, "SW_SLIDERS");
    sc_trace(tf, leds, "LEDS");
    sc_trace(tf, hex[5], "HEX5_STATE");

    cout << "Start symulacji..." << endl;

    sw_sliders.write(0);
    sc_start(10, SC_NS);

    cout << "Włączanie zasilania (SW0)..." << endl;
    sw_sliders.write(SW0); 
    sc_start(10, SC_NS);

    cout << "Zmiana na RED_YELLOW (SW0 + SW2 + SW3)..." << endl;
    sw_sliders.write(SW0 | SW2 | SW3);
    sc_start(10, SC_NS);

    cout << "Zmiana na GREEN (SW0 + SW4)..." << endl;
    sw_sliders.write(SW0 | SW4);
    sc_start(10, SC_NS);

    cout << "Tryb awaryjny (SW1)..." << endl;
    sw_sliders.write(SW0 | SW1);
    sc_start(10, SC_NS);

    cout << "Koniec symulacji." << endl;
    sc_close_vcd_trace_file(tf);

    return 0;
}