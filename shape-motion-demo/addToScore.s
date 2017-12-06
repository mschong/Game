	.text
	.global addToScore
	.global resetScore

addToScore:
	mov &score, r12
	add #1, r12
	ret

resetScore:
	mov #48, r12
	ret
	
