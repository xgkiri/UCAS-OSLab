.global main
main:

// TODO: [p0-task1] Add your code here!
	li a0,0
	li s1,100
	li s2,1
	loop:
		add a0,a0,s2
		addi s2,s2,1
		bge s1,s2,loop
end:
	j end
