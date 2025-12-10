#include <systemc.h>
#include <iostream>
#include <iomanip>
#include <cstdlib> // do system("clear") lub system("cls")

using namespace std;

// --- DEFINICJE STAŁYCH ---
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

// Segmenty (uproszczone mapowanie do wyświetlania)
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

typedef enum { PH_RED, PH_RED_YELLOW, PH_GREEN, PH_YELLOW, PH_RED_GREEN, PH_NONO } Phase;

// --- LOGIKA KONTROLERA (Ta sama co wcześniej - to jest Twój sprzęt) ---
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

        int hex0_val = 0, hex5_val = 0;
        if (currentPhase == PH_RED) {
            current_leds |= LED2;
            hex0_val = SEGA; hex5_val = ZERO;
        } else if (currentPhase == PH_RED_YELLOW) {
            current_leds |= (LED2 | LED3);
            hex0_val = SEGA | SEGG; hex5_val = ONE;
        } else if (currentPhase == PH_GREEN) {
            current_leds |= LED4;
            hex0_val = SEGD; hex5_val = TWO;
        } else if (currentPhase == PH_YELLOW) {
            current_leds |= LED3;
            hex0_val = SEGG; hex5_val = THREE;
        } else if (currentPhase == PH_RED_GREEN) {
            current_leds |= (LED2 | LED4);
            hex0_val = SEGA | SEGC; hex5_val = FOUR;
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
            hex[2].write(0); hex[3].write(0);
        }
    }

    SC_CTOR(TrafficController) {
        SC_METHOD(logic_process);
        sensitive << sw_sliders;
        powerOn = 0; emergencyBlink = 0; currentPhase = PH_GREEN;
    }
};

// --- FUNKCJE POMOCNICZE DO INTERFEJSU ---

// Dekoduje wartości 7-segmentowe na czytelny znak
char decode_hex_char(int val) {
    if (val == ZERO) return '0';
    if (val == ONE) return '1';
    if (val == TWO) return '2';
    if (val == THREE) return '3';
    if (val == FOUR) return '4';
    if (val == HEX_N) return 'N';
    if (val == HEX_O) return 'O';
    if (val == 0) return ' ';
    return '?'; // Inne stany
}

void print_interface(int switches, int leds_val, int h0, int h1, int h2, int h3, int h4, int h5) {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif

    cout << "==========================================" << endl;
    cout << "   SYMULATOR SYGNALIZACJI SWIETLNEJ" << endl;
    cout << "==========================================" << endl;

    // Wyświetlacze HEX
    cout << "\n[ HEX DISPLAYS ]" << endl;
    cout << "HEX5 HEX4 HEX3 HEX2 HEX1 HEX0" << endl;
    cout << "| " << decode_hex_char(h5) << " |  | " << decode_hex_char(h4) << " |  | " 
         << decode_hex_char(h3) << " |  | " << decode_hex_char(h2) << " |  | " 
         << decode_hex_char(h1) << " |  | " << decode_hex_char(h0) << " |" << endl;

    // Diody LED
    cout << "\n[ LEDS ]" << endl;
    cout << "L4(Grn)  L3(Yel)  L2(Red)  L1(Emg)  L0(Pwr)" << endl;
    cout << "   " << ((leds_val & LED4) ? "O" : ".") 
         << "        " << ((leds_val & LED3) ? "O" : ".") 
         << "        " << ((leds_val & LED2) ? "O" : ".") 
         << "        " << ((leds_val & LED1) ? "!" : ".") 
         << "        " << ((leds_val & LED0) ? "*" : ".") << endl;

    // Przełączniki (Switche)
    cout << "\n[ SWITCHES ] (Wpisz numer aby przelaczyc)" << endl;
    cout << "SW4(Grn) SW3(Yel) SW2(Red) SW1(Emg) SW0(Pwr)" << endl;
    cout << " [" << ((switches & SW4) ? "^" : "v") << "]      "
         << " [" << ((switches & SW3) ? "^" : "v") << "]      "
         << " [" << ((switches & SW2) ? "^" : "v") << "]      "
         << " [" << ((switches & SW1) ? "^" : "v") << "]      "
         << " [" << ((switches & SW0) ? "^" : "v") << "]" << endl;

    cout << "\nSterowanie:" << endl;
    cout << " 0 - Przelacz zasilanie (SW0)" << endl;
    cout << " 1 - Przelacz tryb awaryjny (SW1)" << endl;
    cout << " 2,3,4 - Sterowanie faza (R, Y, G)" << endl;
    cout << " q - Zakoncz" << endl;
    cout << "Twój wybór > ";
}

// --- GŁÓWNA PĘTLA SYMULACJI ---
int sc_main(int argc, char* argv[]) {
    // Sygnały
    sc_signal<sc_uint<10>> sw_sliders;
    sc_signal<sc_uint<10>> leds;
    sc_signal<sc_uint<7>> hex[6];

    // Podłączenie modułu
    TrafficController dut("TrafficController");
    dut.sw_sliders(sw_sliders);
    dut.leds(leds);
    for(int i=0; i<6; i++) dut.hex[i](hex[i]);

    int current_switches = 0;
    char input;

    // Pętla nieskończona symulatora
    while (true) {
        // 1. Zasilamy sygnały aktualnym stanem przełączników
        sw_sliders.write(current_switches);

        // 2. Wykonujemy "krok" symulacji (pozwalamy logice zareagować)
        sc_start(10, SC_NS);

        // 3. Pobieramy wartości wyjściowe do wyświetlenia
        int l_val = leds.read().to_int();
        int h0 = hex[0].read().to_int();
        int h1 = hex[1].read().to_int();
        int h2 = hex[2].read().to_int();
        int h3 = hex[3].read().to_int();
        int h4 = hex[4].read().to_int();
        int h5 = hex[5].read().to_int();

        // 4. Rysujemy interfejs
        print_interface(current_switches, l_val, h0, h1, h2, h3, h4, h5);

        // 5. Czekamy na wejście użytkownika
        cin >> input;

        // 6. Obsługa wejścia
        if (input == 'q') break;

        // Zamiana znaku '0'-'4' na maskę bitową
        if (input >= '0' && input <= '4') {
            int bit = input - '0';
            current_switches ^= (1 << bit); // XOR - przełącza bit (toggle)
        }
    }

    return 0;
}
