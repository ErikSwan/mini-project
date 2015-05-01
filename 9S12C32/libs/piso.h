/*
 * A few simple driver routines for a parallel-in, serial-out shift register like
 * the 74HC165. Data is shifted in MSB-first.
 *
 * These functions expect the following indentifiers to be defined:
 *
 *     PISO_CLK         The address of the CLK pin.
 *     PISO_CLK_INH     The address of the CLK_INH pin.
 *     PISO_SH_LD       The address of the SH/LD' pin.
 *     PISO_QH          The address of the QH pin.
 *
 */

/* Setup the interface */
void piso_init(void);

/* Enables the shift register. */
void piso_enable(void);

/* Disables the shift register */
void piso_disable(void);

/* Reads the inputs on the parallel bus by loading the register and then shifting
everything out. */
unsigned char piso_read(void);

/*******************************************************************************
 **** IMPLEMENTATION - DO NOT USE ANY OTHER FUNCTIONS BELOW THIS POINT! ********
 *******************************************************************************/

/*
 * PRIVATE FUNCTIONS
 */

/* Clock the shift register */
 #pragma INLINE
void piso_clock(void) {
    PISO_CLK = 1;
    PISO_CLK = 0;
}

/* Load the register */
#pragma INLINE
void piso_load(void) {
    PISO_SH_LD = 0; // load mode
}

/* Shift out the data */
#pragma INLINE
unsigned char piso_shift() {
    unsigned char result, i;

    PISO_SH_LD = 1; // shift mode

    // Read the first bit
    result = PISO_QH;

    // Shift out the remaining 7 bits
    for(i = 0; i < 7; i++) {
        result <<= 1;
        piso_clock();
        result |= PISO_QH;
    }

    // Result now contains:
    // 0b[H][G][F][E][D][C][B][A]

    return result;
}

/*
 * PUBLIC FUNCTIONS
 */

/* Get the device ready */
void piso_init(void) {
    PISO_CLK = 0;
    piso_enable();
}

/* Enables the shift register. */
#pragma INLINE
void piso_enable(void) {
    #ifdef PISO_CLK_INH
        PISO_CLK_INH = 0;
    #endif
}

/* Disables the shift register */
#pragma INLINE
void piso_disable(void) {
    #ifdef PISO_CLK_INH
        PISO_CLK_INH = 1;
    #endif
}

/* Reads the inputs on the parallel bus by loading the register and then shifting
everything out. */
unsigned char piso_read(void) {
    unsigned char result;
    piso_load();
    return piso_shift();
}
