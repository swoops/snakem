snakem

* About
  Snakem is a feature poor telnet honeypot.  It's only real feature is a
  multi-player snake game.

* Compile
  run make to compile

  to open a telnet server:
    $ ./snakem [<config file>]

  to play:
  	$ telnet 127.0.0.1 4444

   see the default.conf file for more information

TODO:
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
  check int overflow on player speed changes

Warranty:
  There is no warranty.  You should be excited if it compiles.  At worst
  running this program might be a security risk (please do not run as root).
  That said, I want the code to be good so if you find problems, security holes
  or just a concern please let me know. If you find a security vulnerability I
  will likely reward you with a custom t-shirt!  Yeah it's cheap! but it is
  more then you will get then trying to hack the people running this program!
  (I you and I are the only ones who knows this exists)

