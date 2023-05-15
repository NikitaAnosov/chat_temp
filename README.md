#  IPC

## Part A: Chat
Chat cmd tool that can send messages over the network, to the same tool ,listening on the<br />
other side, and get the response, so there will be 2 sides communication, simultaniosly.<br />

How to run: <br />

  ```
   make
  ```
     ./stnc -s PORT 
  ```
   ./stnc -c IP PORT
  ```
  ```
   make clean
  ```
  * NOTE: We used poll() fuction (also in part b) <br />

## Part B: Performance Test
Network performance test utility that use chat channel for system communication, like transferring states, times , etc.

How to run: <br />
  ```
   make
  ```
     ./stnc -s PORT -p
  ```
   ./stnc -c IP PORT -p <type> <param>
  ```
  ```
   make clean
  ```
  
   * NOTE: Can add flag -q to quiet mode, in which only testing results will be printed. <br />

  type, param: <br />
 
  ```
ipv4 tcp
ipv4 udp
ipv6 tcp
ipv6 udp
uds dgram
uds stream
mmap filename
pipe filename
  ```

