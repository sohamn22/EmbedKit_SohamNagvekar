# EmbedKit_SohamNagvekar

1)The parser waits for the 0xAA Start of Frame (SOF) byte.
2)After receiving SOF, it reads the command and payload length.
3)It then receives the payload bytes according to the specified length.
4)While receiving the frame, it calculates the XOR checksum of the command, length, and payload.
5)After the payload, the received checksum is compared with the calculated checksum.
6)If the checksum matches, the frame is accepted; otherwise, the parser resets.
7)The parser also monitors the time between bytes and resets if the configured timeout is exceeded.
8)After completing or resetting a frame, it returns to the SOF state and waits for the next frame.

Build Instructions-

1)Compile using - gcc -Wall -std=c99 uart_parser.c -o uart_parser

2)Run using command - ./uart_parser

Output-

<img width="442" height="474" alt="image" src="https://github.com/user-attachments/assets/b0b30833-8d24-4b69-84cf-8ab6b395e0ac" />
