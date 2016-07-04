snakem

* About
  Multi player telnet snake game!!!

* Compile
  run make to compile

  for the help menu:
    $ ./snakem -h

  to open a telnet server on 0.0.0.0:4444 run
    $ ./snakem -p 4444

  to play:
  	$ telnet 127.0.0.1 4444

TODO:
  usernames fail on port 23 because telnet automatics sends IAC stuff, so add
	  acceptance for those characters
  faster collision detection with taxi cab distance
  snakes hit each other
  handle ctrl+c to clean up sockets nicely...
  add a high score log
  add "A Challenger Approaches" nag
  add High Score screen
  add mods: 
    - invisibo
    - techno snake
    - invincible
    - speed
  add mod nag
  add annoying banner to README

Warranty:
  There is none, this may not work or may cause your system to get hacked.
  That said, I want the code to be good so if you find problems in it please
  let me know and I will fix them.

