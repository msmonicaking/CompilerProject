; =============================================================================
; io.X68 - Input/Output functions for the SubC language
;
; Functions treat D0, D1, A0 and A1 as scratch registers.  All others should
; be saved and restored.
; =============================================================================

        ORG     $100

; =============================================================================
; strlen - count number of characters in the string that starts at address
; (A1).  String is terminated with a trailing 0.  Return result in D0
; =============================================================================
strlen: CLR.L   D0
        MOVEA.L A1, A2
.loop:  TST.B   (A2)+
        BEQ     .done
        ADD.L   #1, D0
        BRA     .loop
.done   RTS

; =============================================================================
; int sayn(int n) - display 'n' as a decimal integer.  Returns 0.
;
; Note: for a call such as "i = sayn(42);", the SubC compiler will generate the
; 68000 code:
*                   MOVE.L  #42, -(A7)
;                   BSR     SAYN
;                   MOVE.L  D0, (@i,A6)
;
; where @i is the offset, from A6, for variable 'i', in the caller function.
; On entry to 'sayn', the stack will look like:
;
;                   |----------------|
;                   | return-address |<= SP=A7
;                   |       42       |
;                   |----------------|
;                   |                |
; =============================================================================

sayn:   MOVE.L  D1, -(A7)           ; save D1
        MOVE.L  #3, D0              ; Task #3
        MOVE.L  (8,A7), D1          ; parameter 'n' (eg: 42)
        TRAP    #15                 ; syscall

        MOVE.L  (A7)+, D1           ; restore D1
        MOVE.L  #0, D0              ; return value
        RTS

; =============================================================================
; int says(char* s) - display 's' as a string.  Returns 0.
; =============================================================================

says:   MOVEA.L (4,A7), A1          ; parameter 's' (eg: 'Hello')
        BSR     strlen              ; D0 = # chars in 's'
        MOVE.L  D0, D1

        MOVE.L  #1, D0              ; Task #1
        TRAP    #15                 ; syscall

        MOVE.L  #0, D0              ; return value
        RTS

; =============================================================================
; int sayl() - display a newline.  Returns 0.
; =============================================================================

crlf:   DC.B    13, 10              ; carriage-return, line-feed
sayl:   MOVE.L  #1, D0              ; Task #1
        MOVE.L  #2, D1              ; write 2 chars
        LEA.L   crlf, A1            ; chars to write
        TRAP    #15                 ; syscall
        MOVE.L  #0, D0              ; return value
        RTS
*~Font name~Courier New~
*~Font size~10~
*~Tab type~1~
*~Tab size~4~
