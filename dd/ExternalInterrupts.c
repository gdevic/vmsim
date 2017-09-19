/******************************************************************************
*                                                                             *
*   Module:     ExternalInterrupts.c                                          *
*                                                                             *
*   Revision:   1.00                                                          *
*                                                                             *
*   Date:       3/27/2000                                                     *
*                                                                             *
*   Author:     Goran Devic                                                   *
*                                                                             *
*******************************************************************************

    Module Description:

        This module contains the code for executing external interrupts
        (32-255)

*******************************************************************************
*                                                                             *
*   Changes:                                                                  *
*                                                                             *
*   DATE     DESCRIPTION OF CHANGES                               AUTHOR      *
* --------   ---------------------------------------------------  ----------- *
* 3/27/2000  Original                                             Goran Devic *
* --------   ---------------------------------------------------  ----------- *
*******************************************************************************
*   Include Files                                                             *
******************************************************************************/
#include <strmini.h>                // Include driver level C functions

#include <ntddk.h>                  // Include ddk function header file

#include "Vm.h"                     // Include root header file

/******************************************************************************
*                                                                             *
*   Global Variables                                                          *
*                                                                             *
******************************************************************************/

/******************************************************************************
*                                                                             *
*   Local Defines, Variables and Macros                                       *
*                                                                             *
******************************************************************************/

typedef (*pInterruptFunction)();


#define INTERRUPT(n) __declspec (naked) Interrupt##n() \
{                                   \
    _asm   int     0x##n            \
    _asm   ret                      \
}

/******************************************************************************
*                                                                             *
*   Define interrupt handler functions                                        *
*                                                                             *
******************************************************************************/

INTERRUPT(20);
INTERRUPT(21);
INTERRUPT(22);
INTERRUPT(23);
INTERRUPT(24);
INTERRUPT(25);
INTERRUPT(26);
INTERRUPT(27);
INTERRUPT(28);
INTERRUPT(29);
INTERRUPT(2A);
INTERRUPT(2B);
INTERRUPT(2C);
INTERRUPT(2D);
INTERRUPT(2E);
INTERRUPT(2F);

INTERRUPT(30);
INTERRUPT(31);
INTERRUPT(32);
INTERRUPT(33);
INTERRUPT(34);
INTERRUPT(35);
INTERRUPT(36);
INTERRUPT(37);
INTERRUPT(38);
INTERRUPT(39);
INTERRUPT(3A);
INTERRUPT(3B);
INTERRUPT(3C);
INTERRUPT(3D);
INTERRUPT(3E);
INTERRUPT(3F);

INTERRUPT(40);
INTERRUPT(41);
INTERRUPT(42);
INTERRUPT(43);
INTERRUPT(44);
INTERRUPT(45);
INTERRUPT(46);
INTERRUPT(47);
INTERRUPT(48);
INTERRUPT(49);
INTERRUPT(4A);
INTERRUPT(4B);
INTERRUPT(4C);
INTERRUPT(4D);
INTERRUPT(4E);
INTERRUPT(4F);

INTERRUPT(50);
INTERRUPT(51);
INTERRUPT(52);
INTERRUPT(53);
INTERRUPT(54);
INTERRUPT(55);
INTERRUPT(56);
INTERRUPT(57);
INTERRUPT(58);
INTERRUPT(59);
INTERRUPT(5A);
INTERRUPT(5B);
INTERRUPT(5C);
INTERRUPT(5D);
INTERRUPT(5E);
INTERRUPT(5F);

INTERRUPT(60);
INTERRUPT(61);
INTERRUPT(62);
INTERRUPT(63);
INTERRUPT(64);
INTERRUPT(65);
INTERRUPT(66);
INTERRUPT(67);
INTERRUPT(68);
INTERRUPT(69);
INTERRUPT(6A);
INTERRUPT(6B);
INTERRUPT(6C);
INTERRUPT(6D);
INTERRUPT(6E);
INTERRUPT(6F);

INTERRUPT(70);
INTERRUPT(71);
INTERRUPT(72);
INTERRUPT(73);
INTERRUPT(74);
INTERRUPT(75);
INTERRUPT(76);
INTERRUPT(77);
INTERRUPT(78);
INTERRUPT(79);
INTERRUPT(7A);
INTERRUPT(7B);
INTERRUPT(7C);
INTERRUPT(7D);
INTERRUPT(7E);
INTERRUPT(7F);

INTERRUPT(80);
INTERRUPT(81);
INTERRUPT(82);
INTERRUPT(83);
INTERRUPT(84);
INTERRUPT(85);
INTERRUPT(86);
INTERRUPT(87);
INTERRUPT(88);
INTERRUPT(89);
INTERRUPT(8A);
INTERRUPT(8B);
INTERRUPT(8C);
INTERRUPT(8D);
INTERRUPT(8E);
INTERRUPT(8F);

INTERRUPT(90);
INTERRUPT(91);
INTERRUPT(92);
INTERRUPT(93);
INTERRUPT(94);
INTERRUPT(95);
INTERRUPT(96);
INTERRUPT(97);
INTERRUPT(98);
INTERRUPT(99);
INTERRUPT(9A);
INTERRUPT(9B);
INTERRUPT(9C);
INTERRUPT(9D);
INTERRUPT(9E);
INTERRUPT(9F);

INTERRUPT(A0);
INTERRUPT(A1);
INTERRUPT(A2);
INTERRUPT(A3);
INTERRUPT(A4);
INTERRUPT(A5);
INTERRUPT(A6);
INTERRUPT(A7);
INTERRUPT(A8);
INTERRUPT(A9);
INTERRUPT(AA);
INTERRUPT(AB);
INTERRUPT(AC);
INTERRUPT(AD);
INTERRUPT(AE);
INTERRUPT(AF);

INTERRUPT(B0);
INTERRUPT(B1);
INTERRUPT(B2);
INTERRUPT(B3);
INTERRUPT(B4);
INTERRUPT(B5);
INTERRUPT(B6);
INTERRUPT(B7);
INTERRUPT(B8);
INTERRUPT(B9);
INTERRUPT(BA);
INTERRUPT(BB);
INTERRUPT(BC);
INTERRUPT(BD);
INTERRUPT(BE);
INTERRUPT(BF);

INTERRUPT(C0);
INTERRUPT(C1);
INTERRUPT(C2);
INTERRUPT(C3);
INTERRUPT(C4);
INTERRUPT(C5);
INTERRUPT(C6);
INTERRUPT(C7);
INTERRUPT(C8);
INTERRUPT(C9);
INTERRUPT(CA);
INTERRUPT(CB);
INTERRUPT(CC);
INTERRUPT(CD);
INTERRUPT(CE);
INTERRUPT(CF);

INTERRUPT(D0);
INTERRUPT(D1);
INTERRUPT(D2);
INTERRUPT(D3);
INTERRUPT(D4);
INTERRUPT(D5);
INTERRUPT(D6);
INTERRUPT(D7);
INTERRUPT(D8);
INTERRUPT(D9);
INTERRUPT(DA);
INTERRUPT(DB);
INTERRUPT(DC);
INTERRUPT(DD);
INTERRUPT(DE);
INTERRUPT(DF);

INTERRUPT(E0);
INTERRUPT(E1);
INTERRUPT(E2);
INTERRUPT(E3);
INTERRUPT(E4);
INTERRUPT(E5);
INTERRUPT(E6);
INTERRUPT(E7);
INTERRUPT(E8);
INTERRUPT(E9);
INTERRUPT(EA);
INTERRUPT(EB);
INTERRUPT(EC);
INTERRUPT(ED);
INTERRUPT(EE);
INTERRUPT(EF);

INTERRUPT(F0);
INTERRUPT(F1);
INTERRUPT(F2);
INTERRUPT(F3);
INTERRUPT(F4);
INTERRUPT(F5);
INTERRUPT(F6);
INTERRUPT(F7);
INTERRUPT(F8);
INTERRUPT(F9);
INTERRUPT(FA);
INTERRUPT(FB);
INTERRUPT(FC);
INTERRUPT(FD);
INTERRUPT(FE);
INTERRUPT(FF);

// 旼컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴커
// �  Define interrupt handlers for interrupts 32 - 255                       �
// 읕컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴켸

static pInterruptFunction InterruptHandlerTable [256-32] =
{
    Interrupt20, Interrupt21, Interrupt22, Interrupt23, Interrupt24, Interrupt25, Interrupt26, Interrupt27,
    Interrupt28, Interrupt29, Interrupt2A, Interrupt2B, Interrupt2C, Interrupt2D, Interrupt2E, Interrupt2F,
    Interrupt30, Interrupt31, Interrupt32, Interrupt33, Interrupt34, Interrupt35, Interrupt36, Interrupt37,
    Interrupt38, Interrupt39, Interrupt3A, Interrupt3B, Interrupt3C, Interrupt3D, Interrupt3E, Interrupt3F,
    Interrupt40, Interrupt41, Interrupt42, Interrupt43, Interrupt44, Interrupt45, Interrupt46, Interrupt47,
    Interrupt48, Interrupt49, Interrupt4A, Interrupt4B, Interrupt4C, Interrupt4D, Interrupt4E, Interrupt4F,
    Interrupt50, Interrupt51, Interrupt52, Interrupt53, Interrupt54, Interrupt55, Interrupt56, Interrupt57,
    Interrupt58, Interrupt59, Interrupt5A, Interrupt5B, Interrupt5C, Interrupt5D, Interrupt5E, Interrupt5F,
    Interrupt60, Interrupt61, Interrupt62, Interrupt63, Interrupt64, Interrupt65, Interrupt66, Interrupt67,
    Interrupt68, Interrupt69, Interrupt6A, Interrupt6B, Interrupt6C, Interrupt6D, Interrupt6E, Interrupt6F,
    Interrupt70, Interrupt71, Interrupt72, Interrupt73, Interrupt74, Interrupt75, Interrupt76, Interrupt77,
    Interrupt78, Interrupt79, Interrupt7A, Interrupt7B, Interrupt7C, Interrupt7D, Interrupt7E, Interrupt7F,
    Interrupt80, Interrupt81, Interrupt82, Interrupt83, Interrupt84, Interrupt85, Interrupt86, Interrupt87,
    Interrupt88, Interrupt89, Interrupt8A, Interrupt8B, Interrupt8C, Interrupt8D, Interrupt8E, Interrupt8F,

    Interrupt90, Interrupt91, Interrupt92, Interrupt93, Interrupt94, Interrupt95, Interrupt96, Interrupt97,
    Interrupt98, Interrupt99, Interrupt9A, Interrupt9B, Interrupt9C, Interrupt9D, Interrupt9E, Interrupt9F,
    InterruptA0, InterruptA1, InterruptA2, InterruptA3, InterruptA4, InterruptA5, InterruptA6, InterruptA7,
    InterruptA8, InterruptA9, InterruptAA, InterruptAB, InterruptAC, InterruptAD, InterruptAE, InterruptAF,
    InterruptB0, InterruptB1, InterruptB2, InterruptB3, InterruptB4, InterruptB5, InterruptB6, InterruptB7,
    InterruptB8, InterruptB9, InterruptBA, InterruptBB, InterruptBC, InterruptBD, InterruptBE, InterruptBF,
    InterruptC0, InterruptC1, InterruptC2, InterruptC3, InterruptC4, InterruptC5, InterruptC6, InterruptC7,
    InterruptC8, InterruptC9, InterruptCA, InterruptCB, InterruptCC, InterruptCD, InterruptCE, InterruptCF,
    InterruptD0, InterruptD1, InterruptD2, InterruptD3, InterruptD4, InterruptD5, InterruptD6, InterruptD7,
    InterruptD8, InterruptD9, InterruptDA, InterruptDB, InterruptDC, InterruptDD, InterruptDE, InterruptDF,
    InterruptE0, InterruptE1, InterruptE2, InterruptE3, InterruptE4, InterruptE5, InterruptE6, InterruptE7,
    InterruptE8, InterruptE9, InterruptEA, InterruptEB, InterruptEC, InterruptED, InterruptEE, InterruptEF,
    InterruptF0, InterruptF1, InterruptF2, InterruptF3, InterruptF4, InterruptF5, InterruptF6, InterruptF7,
    InterruptF8, InterruptF9, InterruptFA, InterruptFB, InterruptFC, InterruptFD, InterruptFE, InterruptFF
};


/******************************************************************************
*                                                                             *
*   Functions                                                                 *
*                                                                             *
******************************************************************************/

/******************************************************************************
*                                                                             *
*   void ExecuteINTn(int nInt)                                                *
*                                                                             *
*******************************************************************************
*
*   Execute external interrupt
*
*   Where:
*       nInt is the interrupt number (32-255)
*
******************************************************************************/
void ExecuteINTn(int nInt)
{
    (*InterruptHandlerTable[nInt-32])();
}
