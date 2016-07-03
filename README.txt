snakem

* About
  Multi player telent snake game!!!

* Compile
  run make to compile

  for the help menu:
    $ ./snakem -h

  to open a telnet server on 0.0.0.0:4444 run
    $ ./snakem -p 4444

  to play:
  	$ telnet 127.0.0.1 4444

TODO:
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
