snakem

* About
	Telnet Snake Game with annoying colors
	Eventually to be a telnet game for two players

* Compile
	run make to compile

	for the help menu:
		$ ./snakem -h

	to play locally set buffer size to 1 in your terminal
		$ ./stty -icanon

	to open a telnet server on 0.0.0.0:4444 run
		$ ./snakem -p 4444


TODO:
	handle ctrl+c to clean up sockets nicely...
	add usernames to high score 
