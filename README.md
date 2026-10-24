# asm
I made this assembly program where you can move a cube around in 3d with lighting. It runs on the console and only on windows. Press WASD to move left-to-right and forwards-and-back, press Q and E to move up-and-down, and press arrow keys to rotate the camera.
<br/>
This project uses quaternions for rotation using a lookup table for sine values. It leverages the AVX SIMD instruction set for performance.
