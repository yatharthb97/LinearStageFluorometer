# Linear-Stage Fluorometer





## Brief Description of the program

0. `setup()` : Setup runs only once: use to set `pinMode` etc.

1. `start()`: Procedure to start the scanning of samples.
2. `stop()`: Procedure to stop the scanning of samples.
3. `calibrate()` : Aligns sample 1 to the origin.
4. `next_sample()`: Scans and moves to the next sample. [returns false if no sample was detected.]
5. `spiral_calibrate()`: Aligns sample 1 to the origin by doing a spiral scan.







## **Note:**

1. Messages on `Serial` that start with `#` are not processed as data and are only printed on screen.



IR 1:

1. `LOW` on detection — white and `HIGH` when seeing black.

IR 2 (Photo-transistor):

1. `HIGH` on detection of metal.
2. 2nd LED switches off on metal detection.