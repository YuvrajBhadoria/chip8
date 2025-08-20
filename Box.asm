define boxx V0 ;defined X
define boxy V1 ;defined Y
define boxv V2


LD boxx ,0 ;loaded '00' in X
LD boxy ,11 ; loaded '0b' in Y
LD boxv ,1  ;variable register
LD I ,sprite
LD V3 ,1


start: ;jumping after one dabba making


CLS ; clear screen



;drawing now
DRW boxx, boxy,8

CALL frothy

;shiting pixels
ADD boxx, 1
SE boxx ,60
JP start

start1:
CLS



DRW boxx ,boxy ,8

CALL frothy

SUB boxx ,boxv
SE boxx ,0
JP start1
JP start

frothy:
	LD DT ,V3
	loop:
	LD V4, DT
	SE V4, 0
	JP loop
RET



sprite:
db  %00000000,
    %00000000,
    %11110000,
    %11110000,
    %11110000,
    %11110000,
    %00000000,
    %00000000,
