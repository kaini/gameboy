.INCLUDE "cgb_hardware.i"

.MEMORYMAP
SLOTSIZE $4000
DEFAULTSLOT 0
SLOT 0 $0000
SLOT 1 $4000
.ENDME

.ROMBANKMAP
BANKSTOTAL 2
BANKSIZE $4000
BANKS 2
.ENDRO

.BANK 0 SLOT 0

.CARTRIDGETYPE $00
.COMPUTECHECKSUM
.COMPUTEGBCOMPLEMENTCHECK
.LICENSEECODENEW "  "
.ORG $100
	nop
	jp entry
.DB $CE $ED $66 $66 $CC $0D $00 $0B $03 $73 $00 $83 $00 $0C $00 $0D
.DB $00 $08 $11 $1F $88 $89 $00 $0E $DC $CC $6E $E6 $DD $DD $D9 $99
.DB $BB $BB $67 $63 $6E $0E $EC $CC $DD $DC $99 $9F $BB $B9 $33 $3E
.BYTE "IRAM"
.ORG $13F
.BYTE "TEST"
.ORG $143
.BYTE $C0

; Data
.ORG $150
entry: jp main
result_ok: .DB "ok" 0
result_bank0_error: .DB "bank 0 error" 0

; Code
main:
	ld sp,$ffff
	call clear_output

	; fill bank 0 with $f0
	ld hl,$c000
	ld b,$f0
	call fill_ram

	; fill bank n=1-7 with $f0+n
	ld hl,$d000
	ld c,7
-	ld a,c
	ld (SVBK),a
	or $f0
	ld b,a
	call fill_ram
	dec c
	jr nz,-

	; check bank 0
	ld hl,$c000
	ld b,$f0
	ld de,result_bank0_error
	call check_ram

	; everything fine
	ld hl,result_ok
	jp test_output

; hl = start address
; b = pattern
; de = error address
check_ram:
	push af
	push bc
	push hl

	ld a,h
	add $10
	ld c,a  ; c = end hl address (high)

-	ldi a,(hl)
	cp b
	jr nz,check_ram_fail
	ld a,c
	cp h
	jr nz,-

	pop hl
	pop bc
	pop af
	ret
check_ram_fail:
	ld h,d
	ld l,e
	jp test_output

; hl = start address
; b = pattern
fill_ram:
	push af
	push bc
	push hl

	ld a,h
	add $10
	ld c,a  ; c = end hl address (high)

-	ld a,h
	cp c
	jr z,+

	ld (hl),b

	inc hl
	jr -
+
	pop hl
	pop bc
	pop af
	ret

; Clears the result memory
clear_output:
	push hl
	ld hl,$ff80
	ld (hl),0
	pop hl
	ret

; Outputs the test result and hangs
; hl = result address
test_output:
	ld de,$ff80

-	ldi a,(hl)
	ld (de),a
	inc de
	cp $00
	jr nz,-

-	halt
	jr -
